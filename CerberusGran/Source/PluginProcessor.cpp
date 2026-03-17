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
        hp.rate     = apvts.getRawParameterValue (id ("rate"));
        hp.length   = apvts.getRawParameterValue (id ("length"));
        hp.pitch    = apvts.getRawParameterValue (id ("pitch"));
        hp.shape    = apvts.getRawParameterValue (id ("shape"));
        hp.reverse  = apvts.getRawParameterValue (id ("reverse"));
        hp.gain     = apvts.getRawParameterValue (id ("gain"));

        // Filter
        hp.filterType   = apvts.getRawParameterValue (id ("filterType"));
        hp.filterCutoff = apvts.getRawParameterValue (id ("filterCutoff"));
        hp.filterQ      = apvts.getRawParameterValue (id ("filterQ"));
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
    grainEngine.setMasterGain (masterGainParam->load());

    int sm = static_cast<int> (sourceModeParam->load());
    sourceMode.store (sm, std::memory_order_relaxed);
    freeze.store (freezeParam->load() >= 0.5f, std::memory_order_relaxed);

    for (int h = 0; h < kNumHeads; ++h)
    {
        auto& hp = headParams[h];
        auto& head = grainEngine.getHead (h);

        head.setEnabled (hp.enable->load() >= 0.5f);
        head.setPosition (hp.position->load());
        head.setSpread (hp.spread->load());
        head.setRate (hp.rate->load());
        head.setLength (hp.length->load());
        head.setPitchSemitones (hp.pitch->load());
        head.setShape (static_cast<int> (hp.shape->load()));
        head.setReverse (hp.reverse->load() >= 0.5f);
        head.setGainDb (hp.gain->load());

        // Filter
        head.setFilterParams (
            static_cast<int> (hp.filterType->load()),
            hp.filterCutoff->load(),
            hp.filterQ->load());
    }
}

void CerberusGranAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                               juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // Write to ring buffer unless frozen
    if (!freeze.load (std::memory_order_relaxed))
        ringBuffer.write (buffer, buffer.getNumSamples());

    updateParametersFromAPVTS();

    bool liveMode = (getSourceMode() == AudioSourceMode::Live);
    const auto* sb = getSampleBuffer();

    float mixPct = mixParam->load();          // 0-100
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

void CerberusGranAudioProcessor::clearSampleFile()
{
    sampleBuffer.store (nullptr, std::memory_order_release);
    sampleBufferA.setSize (0, 0);
    sampleBufferB.setSize (0, 0);
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
