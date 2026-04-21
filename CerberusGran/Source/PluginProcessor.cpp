#include "PluginProcessor.h"
#include "PluginEditor.h"

static constexpr double kRingBufferSeconds = 5.0;

CerberusGranAudioProcessor::CerberusGranAudioProcessor()
    : juce::AudioProcessor (BusesProperties()
                                .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                                .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    formatManager.registerBasicFormats();

    masterGainParam = apvts.getRawParameterValue ("masterGain");
    mixParam        = apvts.getRawParameterValue ("mix");
    freezeParam     = apvts.getRawParameterValue ("freeze");
    sourceModeParam = apvts.getRawParameterValue ("sourceMode");

    for (int h = 0; h < kNumHeads; ++h)
    {
        auto id = [h] (const juce::String& name) { return "head" + juce::String (h) + "_" + name; };
        auto& hp = headParams[h];

        hp.enable   = apvts.getRawParameterValue (id ("enable"));
        hp.position = apvts.getRawParameterValue (id ("position"));
        hp.spread   = apvts.getRawParameterValue (id ("spread"));
        hp.rateMode     = apvts.getRawParameterValue (id ("rateMode"));
        hp.rate         = apvts.getRawParameterValue (id ("rate"));
        hp.rateSyncDiv  = apvts.getRawParameterValue (id ("rateSyncDiv"));
        hp.rateSyncType = apvts.getRawParameterValue (id ("rateSyncType"));
        hp.sizeLink  = apvts.getRawParameterValue (id ("sizeLink"));
        hp.length    = apvts.getRawParameterValue (id ("length"));
        hp.sizeRatio = apvts.getRawParameterValue (id ("sizeRatio"));
        hp.pitch    = apvts.getRawParameterValue (id ("pitch"));
        hp.shape    = apvts.getRawParameterValue (id ("shape"));
        hp.reverse  = apvts.getRawParameterValue (id ("reverse"));
        hp.gain     = apvts.getRawParameterValue (id ("gain"));

        // Filter (extended)
        hp.filterOn       = apvts.getRawParameterValue (id ("filterOn"));
        hp.filterType     = apvts.getRawParameterValue (id ("filterType"));
        hp.filterCutoff   = apvts.getRawParameterValue (id ("filterCutoff"));
        hp.filterRes      = apvts.getRawParameterValue (id ("filterRes"));
        hp.filterPan      = apvts.getRawParameterValue (id ("filterPan"));
        hp.filterDrive    = apvts.getRawParameterValue (id ("filterDrive"));
        hp.filterCombFreq = apvts.getRawParameterValue (id ("filterCombFreq"));
        hp.filterMix      = apvts.getRawParameterValue (id ("filterMix"));

        hp.crushOn       = apvts.getRawParameterValue (id ("crushOn"));
        hp.crushBits     = apvts.getRawParameterValue (id ("crushBits"));
        hp.crushRate     = apvts.getRawParameterValue (id ("crushRate"));

        hp.delayOn       = apvts.getRawParameterValue (id ("delayOn"));
        hp.delayTimeMode = apvts.getRawParameterValue (id ("delayTimeMode"));
        hp.delayTime     = apvts.getRawParameterValue (id ("delayTime"));
        hp.delaySyncDiv  = apvts.getRawParameterValue (id ("delaySyncDiv"));
        hp.delaySyncType = apvts.getRawParameterValue (id ("delaySyncType"));
        hp.delayFeedback = apvts.getRawParameterValue (id ("delayFeedback"));
        hp.delayMix      = apvts.getRawParameterValue (id ("delayMix"));

        hp.reverbOn      = apvts.getRawParameterValue (id ("reverbOn"));
        hp.reverbSize    = apvts.getRawParameterValue (id ("reverbSize"));
        hp.reverbDamp    = apvts.getRawParameterValue (id ("reverbDamp"));
        hp.reverbMix     = apvts.getRawParameterValue (id ("reverbMix"));
    }
}

CerberusGranAudioProcessor::~CerberusGranAudioProcessor() = default;

bool CerberusGranAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void CerberusGranAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    int ringSize = static_cast<int> (sampleRate * kRingBufferSeconds);
    ringBuffer.prepare (2, ringSize);
    grainEngine.prepare (sampleRate, samplesPerBlock);
    dryBuffer.setSize (2, samplesPerBlock);
}

void CerberusGranAudioProcessor::updateParametersFromAPVTS()
{
    float gainDb = masterGainParam->load();
    grainEngine.setMasterGain (gainDb <= -59.9f ? 0.0f : juce::Decibels::decibelsToGain (gainDb));

    int sm = static_cast<int> (sourceModeParam->load());
    sourceMode.store (sm, std::memory_order_relaxed);
    freeze.store (freezeParam->load() >= 0.5f, std::memory_order_relaxed);

    float finestGridMs = 0.0f;
    bool anySync = false;

    for (int h = 0; h < kNumHeads; ++h)
    {
        auto& hp = headParams[h];
        auto& head = grainEngine.getHead (h);

        head.setEnabled (hp.enable->load() >= 0.5f);
        head.setPosition (hp.position->load());
        head.setSpread (hp.spread->load());

        // Rate: Time mode uses raw ms, Sync mode calculates from tempo
        float effectiveRateMs = hp.rate->load();
        {
            int rateMode = static_cast<int> (hp.rateMode->load());
            if (rateMode == 0) // Time
            {
                effectiveRateMs = hp.rate->load();
                head.setRate (effectiveRateMs);
            }
            else // Sync
            {
                double bpm = 120.0;
                if (auto* playHead = getPlayHead())
                    if (auto pos = playHead->getPosition())
                        if (auto b = pos->getBpm())
                            bpm = *b;

                int divIdx = static_cast<int> (hp.rateSyncDiv->load());
                double divValue = 1.0 / (1 << divIdx);
                double quarterMs = 60000.0 / bpm;
                double noteMs = divValue * 4.0 * quarterMs;

                int syncType = static_cast<int> (hp.rateSyncType->load());
                if (syncType == 1)      noteMs *= 2.0 / 3.0;
                else if (syncType == 2) noteMs *= 3.0 / 2.0;

                effectiveRateMs = static_cast<float> (juce::jmax (1.0, noteMs));
                head.setRate (effectiveRateMs);
                head.setSyncGrid (true, effectiveRateMs);
            }
        }

        if (static_cast<int> (hp.rateMode->load()) == 0)
        {
            head.setSyncGrid (false, 0.0f);
        }
        else if (hp.enable->load() >= 0.5f)
        {
            anySync = true;
            if (finestGridMs <= 0.0f || effectiveRateMs < finestGridMs)
                finestGridMs = effectiveRateMs;
        }

        bool linked = hp.sizeLink->load() >= 0.5f;
        if (linked)
        {
            static const float ratios[] = { 0.25f, 0.5f, 1.0f, 2.0f, 4.0f, 8.0f, 16.0f };
            int ratioIdx = juce::jlimit (0, 6, static_cast<int> (hp.sizeRatio->load()));
            float linkedLength = effectiveRateMs * ratios[ratioIdx];
            head.setLength (juce::jmax (1.0f, linkedLength));
        }
        else
        {
            head.setLength (hp.length->load());
        }
        head.setPitchSemitones (hp.pitch->load());
        head.setShape (hp.shape->load());
        head.setReversePct (hp.reverse->load());
        head.setGainDb (hp.gain->load());

        // ---- Filter (extended) ----
        head.setFilterEnabled  (hp.filterOn->load() >= 0.5f);
        head.setFilterType     (static_cast<int> (hp.filterType->load()));
        head.setFilterCutoff   (hp.filterCutoff->load());
        head.setFilterResonance(hp.filterRes->load());
        head.setFilterPan      (hp.filterPan->load());
        head.setFilterDrive    (hp.filterDrive->load());
        head.setFilterCombFreq (hp.filterCombFreq->load());
        head.setFilterMix      (hp.filterMix->load());

        head.setCrushEnabled (hp.crushOn->load() >= 0.5f);
        head.setCrushBits (hp.crushBits->load());
        head.setCrushRate (hp.crushRate->load());

        head.setDelayEnabled (hp.delayOn->load() >= 0.5f);
        {
            int dtMode = static_cast<int> (hp.delayTimeMode->load());
            if (dtMode == 0)
            {
                head.setDelayTime (hp.delayTime->load());
            }
            else
            {
                double bpm = 120.0;
                if (auto* playHead = getPlayHead())
                    if (auto pos = playHead->getPosition())
                        if (auto b = pos->getBpm())
                            bpm = *b;

                int divIdx = static_cast<int> (hp.delaySyncDiv->load());
                double divValue = 1.0 / (1 << divIdx);
                double quarterMs = 60000.0 / bpm;
                double noteMs = divValue * 4.0 * quarterMs;

                int syncType = static_cast<int> (hp.delaySyncType->load());
                if (syncType == 1)      noteMs *= 2.0 / 3.0;
                else if (syncType == 2) noteMs *= 3.0 / 2.0;

                head.setDelayTime (static_cast<float> (juce::jlimit (1.0, 2000.0, noteMs)));
            }
        }
        head.setDelayFeedback (hp.delayFeedback->load());
        head.setDelayMix (hp.delayMix->load());

        head.setReverbEnabled (hp.reverbOn->load() >= 0.5f);
        head.setReverbSize (hp.reverbSize->load());
        head.setReverbDamp (hp.reverbDamp->load());
        head.setReverbMix (hp.reverbMix->load());
    }

    anySyncActive.store (anySync, std::memory_order_relaxed);
    syncGridMs.store (finestGridMs, std::memory_order_relaxed);
}

void CerberusGranAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                               juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    if (!freeze.load (std::memory_order_relaxed))
        ringBuffer.write (buffer, buffer.getNumSamples());

    updateParametersFromAPVTS();

    bool liveMode = (getSourceMode() == AudioSourceMode::Live);
    const auto* sb = getSampleBuffer();

    float mixPct = mixParam->load();
    float wet = mixPct / 100.0f;
    int numSamples = buffer.getNumSamples();
    int numCh = buffer.getNumChannels();

    if (wet < 1.0f)
    {
        for (int ch = 0; ch < juce::jmin (2, numCh); ++ch)
            dryBuffer.copyFrom (ch, 0, buffer, ch, 0, numSamples);

        buffer.clear();
        grainEngine.process (buffer, numSamples, ringBuffer, sb, liveMode);

        for (int ch = 0; ch < numCh; ++ch)
        {
            float* w = buffer.getWritePointer (ch);
            const float* d = dryBuffer.getReadPointer (juce::jmin (ch, 1));
            for (int i = 0; i < numSamples; ++i)
                w[i] = d[i] * (1.0f - wet) + w[i] * wet;
        }
    }
    else
    {
        buffer.clear();
        grainEngine.process (buffer, numSamples, ringBuffer, sb, liveMode);
    }
}

void CerberusGranAudioProcessor::loadSampleFile (const juce::File& file)
{
    std::unique_ptr<juce::AudioFormatReader> reader (formatManager.createReaderFor (file));
    if (reader == nullptr) return;

    auto& target = useBufferA ? sampleBufferA : sampleBufferB;
    target.setSize (static_cast<int> (reader->numChannels),
                    static_cast<int> (reader->lengthInSamples));
    reader->read (&target, 0, static_cast<int> (reader->lengthInSamples), 0, true, true);

    sampleBuffer.store (&target, std::memory_order_release);
    useBufferA = !useBufferA;
}

void CerberusGranAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void CerberusGranAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* CerberusGranAudioProcessor::createEditor()
{
    return new CerberusGranAudioProcessorEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CerberusGranAudioProcessor();
}