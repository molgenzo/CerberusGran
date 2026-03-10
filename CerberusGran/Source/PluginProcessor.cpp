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

    // Cache global param pointers
    masterGainParam  = apvts.getRawParameterValue ("masterGain");
    masterPitchParam = apvts.getRawParameterValue ("masterPitch");
    dryWetParam      = apvts.getRawParameterValue ("dryWet");
    sourceModeParam  = apvts.getRawParameterValue ("sourceMode");

    // Cache per-head param pointers
    for (int h = 0; h < kNumHeads; ++h)
    {
        auto id = [h] (const juce::String& name) { return "head" + juce::String (h) + "_" + name; };
        auto& hp = headParams[h];

        hp.enable       = apvts.getRawParameterValue (id ("enable"));
        hp.position     = apvts.getRawParameterValue (id ("position"));
        hp.posScatter   = apvts.getRawParameterValue (id ("posScatter"));
        hp.density      = apvts.getRawParameterValue (id ("density"));
        hp.duration     = apvts.getRawParameterValue (id ("duration"));
        hp.pitch        = apvts.getRawParameterValue (id ("pitch"));
        hp.pitchScatter = apvts.getRawParameterValue (id ("pitchScatter"));
        hp.window       = apvts.getRawParameterValue (id ("window"));
        hp.gain         = apvts.getRawParameterValue (id ("gain"));
        hp.pan          = apvts.getRawParameterValue (id ("pan"));
        hp.filterBypass = apvts.getRawParameterValue (id ("filterBypass"));
        hp.filterType   = apvts.getRawParameterValue (id ("filterType"));
        hp.filterCutoff = apvts.getRawParameterValue (id ("filterCutoff"));
        hp.filterRes    = apvts.getRawParameterValue (id ("filterRes"));
        hp.satBypass    = apvts.getRawParameterValue (id ("satBypass"));
        hp.satDrive     = apvts.getRawParameterValue (id ("satDrive"));
        hp.crushBypass  = apvts.getRawParameterValue (id ("crushBypass"));
        hp.crushBits    = apvts.getRawParameterValue (id ("crushBits"));
        hp.crushRate    = apvts.getRawParameterValue (id ("crushRate"));
        hp.delayBypass  = apvts.getRawParameterValue (id ("delayBypass"));
        hp.delayTime    = apvts.getRawParameterValue (id ("delayTime"));
        hp.delayFeedback= apvts.getRawParameterValue (id ("delayFeedback"));
        hp.delayMix     = apvts.getRawParameterValue (id ("delayMix"));
        hp.lfoRate      = apvts.getRawParameterValue (id ("lfoRate"));
        hp.lfoDepth     = apvts.getRawParameterValue (id ("lfoDepth"));
        hp.lfoShape     = apvts.getRawParameterValue (id ("lfoShape"));
        hp.lfoTarget    = apvts.getRawParameterValue (id ("lfoTarget"));
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

float CerberusGranAudioProcessor::computeLFO (int headIndex, float rate, int shape, int numSamples)
{
    auto& lfo = lfoStates[headIndex];
    double phaseInc = rate / currentSampleRate * numSamples;
    lfo.phase += phaseInc;
    if (lfo.phase >= 1.0) lfo.phase -= 1.0;

    float p = static_cast<float> (lfo.phase);
    float value = 0.0f;

    switch (shape)
    {
        case 0: // Sine
            value = std::sin (p * juce::MathConstants<float>::twoPi);
            break;
        case 1: // Triangle
            value = (p < 0.5f) ? (4.0f * p - 1.0f) : (3.0f - 4.0f * p);
            break;
        case 2: // Saw
            value = 2.0f * p - 1.0f;
            break;
        case 3: // Square
            value = (p < 0.5f) ? 1.0f : -1.0f;
            break;
        case 4: // Sample and Hold
            if (phaseInc > 0 && lfo.phase < phaseInc * 1.5)
                lfo.shValue = juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f;
            value = lfo.shValue;
            break;
    }

    return value;
}

void CerberusGranAudioProcessor::updateParametersFromAPVTS()
{
    grainEngine.setMasterGain (masterGainParam->load());

    int sm = static_cast<int> (sourceModeParam->load());
    sourceMode.store (sm, std::memory_order_relaxed);

    for (int h = 0; h < kNumHeads; ++h)
    {
        auto& hp = headParams[h];
        auto& head = grainEngine.getHead (h);

        head.setEnabled (hp.enable->load() >= 0.5f);
        head.setPosition (hp.position->load());
        head.setPositionScatter (hp.posScatter->load());
        head.setDensity (hp.density->load());
        head.setGrainDuration (hp.duration->load());
        head.setPitch (hp.pitch->load() * masterPitchParam->load());
        head.setPitchScatter (hp.pitchScatter->load());
        head.setWindowType (static_cast<int> (hp.window->load()));
        head.setGain (hp.gain->load());
        head.setPan (hp.pan->load());

        head.setFilterBypassed (hp.filterBypass->load() >= 0.5f);
        head.setFilterType (static_cast<int> (hp.filterType->load()));
        head.setFilterCutoff (hp.filterCutoff->load());
        head.setFilterResonance (hp.filterRes->load());

        head.setSaturatorBypassed (hp.satBypass->load() >= 0.5f);
        head.setSaturatorDrive (hp.satDrive->load());

        head.setBitcrusherBypassed (hp.crushBypass->load() >= 0.5f);
        head.setBitcrusherBits (hp.crushBits->load());
        head.setBitcrusherRate (hp.crushRate->load());

        head.setDelayBypassed (hp.delayBypass->load() >= 0.5f);
        head.setDelayTime (hp.delayTime->load());
        head.setDelayFeedback (hp.delayFeedback->load());
        head.setDelayMix (hp.delayMix->load());

        // LFO modulation
        float lfoDepth = hp.lfoDepth->load();
        int lfoTarget  = static_cast<int> (hp.lfoTarget->load());

        if (lfoDepth > 0.0f && lfoTarget > 0)
        {
            float lfoRate  = hp.lfoRate->load();
            int lfoShape   = static_cast<int> (hp.lfoShape->load());
            float lfoVal   = computeLFO (h, lfoRate, lfoShape, getBlockSize());
            float mod      = lfoVal * lfoDepth;

            switch (lfoTarget)
            {
                case 1: // Position
                    head.setPosition (juce::jlimit (0.0f, 1.0f, hp.position->load() + mod * 0.5f));
                    break;
                case 2: // Pitch
                    head.setPitch (juce::jmax (0.1f, hp.pitch->load() + mod));
                    break;
                case 3: // Filter Cutoff
                {
                    float base = hp.filterCutoff->load();
                    float modulated = base * std::pow (2.0f, mod * 3.0f);
                    head.setFilterCutoff (juce::jlimit (20.0f, 20000.0f, modulated));
                    break;
                }
            }
        }
    }
}

void CerberusGranAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                               juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    ringBuffer.write (buffer, buffer.getNumSamples());

    updateParametersFromAPVTS();

    bool liveMode = (getSourceMode() == AudioSourceMode::Live);
    const auto* sb = getSampleBuffer();

    float dryWet = dryWetParam->load();

    int numSamples = buffer.getNumSamples();
    int numCh = buffer.getNumChannels();

    if (dryWet < 1.0f)
    {
        for (int ch = 0; ch < juce::jmin (2, numCh); ++ch)
            dryBuffer.copyFrom (ch, 0, buffer, ch, 0, numSamples);

        buffer.clear();
        grainEngine.process (buffer, numSamples, ringBuffer, sb, liveMode);

        for (int ch = 0; ch < numCh; ++ch)
        {
            float* wet = buffer.getWritePointer (ch);
            const float* dry = dryBuffer.getReadPointer (juce::jmin (ch, 1));
            for (int i = 0; i < numSamples; ++i)
                wet[i] = dry[i] * (1.0f - dryWet) + wet[i] * dryWet;
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
    if (reader == nullptr)
        return;

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
