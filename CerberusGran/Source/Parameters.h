#pragma once
#include <JuceHeader.h>

static constexpr int kNumHeads = 5;

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Global
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "masterGain", "Gain",
        juce::NormalisableRange<float> (-60.0f, 3.0f, 0.1f), 0.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "mix", "Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 100.0f));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "sourceMode", "Source",
        juce::StringArray { "Live", "File" }, 0));

    // Per-head: core granular params only
    for (int h = 0; h < kNumHeads; ++h)
    {
        auto id = [h] (const juce::String& name) -> juce::String
        { return "head" + juce::String (h) + "_" + name; };

        auto nm = [h] (const juce::String& name) -> juce::String
        { return "H" + juce::String (h) + " " + name; };

        float defaultPos = static_cast<float> (h) / static_cast<float> (juce::jmax (1, kNumHeads - 1));

        params.push_back (std::make_unique<juce::AudioParameterBool> (
            id ("enable"), nm ("Enable"), h == 0));

        params.push_back (std::make_unique<juce::AudioParameterBool> (
            id ("freeze"), nm ("Freeze"), false));

        // Scan
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            id ("position"), nm ("Position"),
            juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), defaultPos * 100.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            id ("spread"), nm ("Spread"),
            juce::NormalisableRange<float> (0.0f, 50.0f, 0.1f), 0.0f));

        // Placement
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            id ("rateMode"), nm ("Rate Mode"),
            juce::StringArray { "Time", "Sync" }, 0));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            id ("rate"), nm ("Rate"),
            juce::NormalisableRange<float> (5.0f, 500.0f, 1.0f, 0.4f), 100.0f));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            id ("rateSyncDiv"), nm ("Sync Div"),
            juce::StringArray { "1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64", "1/128", "1/256" }, 3));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            id ("rateSyncType"), nm ("Sync Type"),
            juce::StringArray { "Normal", "Triplet", "Dotted" }, 0));

        // Lifetime
        params.push_back (std::make_unique<juce::AudioParameterBool> (
            id ("sizeLink"), nm ("Size Link"), false));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            id ("length"), nm ("Length"),
            juce::NormalisableRange<float> (5.0f, 1000.0f, 1.0f, 0.4f), 200.0f));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            id ("sizeRatio"), nm ("Size Ratio"),
            juce::StringArray { "1:4", "1:2", "1:1", "2:1", "4:1", "8:1", "16:1" }, 2));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            id ("pitch"), nm ("Pitch"),
            juce::NormalisableRange<float> (-24.0f, 24.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            id ("shape"), nm ("Grain Shape"),
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            id ("reverse"), nm ("Reverse"),
            juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f));

        // Output
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            id ("gain"), nm ("Gain"),
            juce::NormalisableRange<float> (-24.0f, 6.0f, 0.1f), 0.0f));

        // FX Chain: Filter
        params.push_back (std::make_unique<juce::AudioParameterBool> (
            id ("filterOn"), nm ("Filter On"), false));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            id ("filterType"), nm ("Filter Type"),
            juce::StringArray { "LP", "HP", "BP" }, 0));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            id ("filterCutoff"), nm ("Filter Cutoff"),
            juce::NormalisableRange<float> (20.0f, 20000.0f, 1.0f, 0.25f), 1000.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            id ("filterRes"), nm ("Filter Res"),
            juce::NormalisableRange<float> (0.1f, 10.0f, 0.01f, 0.5f), 0.707f));

        // FX Chain: Bitcrusher
        params.push_back (std::make_unique<juce::AudioParameterBool> (
            id ("crushOn"), nm ("Crush On"), false));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            id ("crushBits"), nm ("Crush Bits"),
            juce::NormalisableRange<float> (1.0f, 16.0f, 1.0f), 16.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            id ("crushRate"), nm ("Crush Rate"),
            juce::NormalisableRange<float> (1.0f, 50.0f, 1.0f), 1.0f));

        // FX Chain: Delay
        params.push_back (std::make_unique<juce::AudioParameterBool> (
            id ("delayOn"), nm ("Delay On"), false));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            id ("delayTimeMode"), nm ("Delay Time Mode"),
            juce::StringArray { "Time", "Sync" }, 0));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            id ("delayTime"), nm ("Delay Time"),
            juce::NormalisableRange<float> (1.0f, 2000.0f, 1.0f, 0.4f), 250.0f));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            id ("delaySyncDiv"), nm ("Delay Sync Div"),
            juce::StringArray { "1/1", "1/2", "1/4", "1/8", "1/16", "1/32" }, 3));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            id ("delaySyncType"), nm ("Delay Sync Type"),
            juce::StringArray { "Normal", "Triplet", "Dotted" }, 0));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            id ("delayFeedback"), nm ("Delay Fdbk"),
            juce::NormalisableRange<float> (0.0f, 0.95f, 0.01f), 0.3f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            id ("delayMix"), nm ("Delay Mix"),
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));
        
        params.push_back (std::make_unique<juce::AudioParameterBool> (
            id ("delayPingPong"), nm ("Delay Ping-Pong"), false));

        // FX Chain: Reverb
        params.push_back (std::make_unique<juce::AudioParameterBool> (
            id ("reverbOn"), nm ("Reverb On"), false));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            id ("reverbSize"), nm ("Reverb Size"),
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            id ("reverbDamp"), nm ("Reverb Damp"),
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            id ("reverbMix"), nm ("Reverb Mix"),
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.3f));
    }

    return { params.begin(), params.end() };
}
