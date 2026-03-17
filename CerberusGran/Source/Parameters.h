#pragma once
#include <JuceHeader.h>

static constexpr int kNumHeads = 5;

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Global
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "masterGain", "Gain",
        juce::NormalisableRange<float> (0.0f, 2.0f, 0.01f), 1.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "mix", "Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 100.0f));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        "freeze", "Freeze", false));

    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        "sourceMode", "Source",
        juce::StringArray { "Live", "File" }, 0));

    // Per-head parameters
    for (int h = 0; h < kNumHeads; ++h)
    {
        auto id = [h] (const juce::String& name) -> juce::String
        { return "head" + juce::String (h) + "_" + name; };

        auto nm = [h] (const juce::String& name) -> juce::String
        { return "H" + juce::String (h) + " " + name; };

        float defaultPos = static_cast<float> (h) / static_cast<float> (juce::jmax (1, kNumHeads - 1));

        params.push_back (std::make_unique<juce::AudioParameterBool> (
            id ("enable"), nm ("Enable"), h == 0));

        // Scan
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            id ("position"), nm ("Position"),
            juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), defaultPos * 100.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            id ("spread"), nm ("Spread"),
            juce::NormalisableRange<float> (0.0f, 50.0f, 0.1f), 0.0f));

        // Placement
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            id ("rate"), nm ("Rate"),
            juce::NormalisableRange<float> (5.0f, 500.0f, 1.0f, 0.4f), 100.0f));

        // Lifetime
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            id ("length"), nm ("Length"),
            juce::NormalisableRange<float> (5.0f, 1000.0f, 1.0f, 0.4f), 200.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            id ("pitch"), nm ("Pitch"),
            juce::NormalisableRange<float> (-24.0f, 24.0f, 0.01f), 0.0f));

        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            id ("shape"), nm ("Shape"),
            juce::StringArray { "Hann", "Gaussian", "Tukey", "Triangle" }, 0));

        params.push_back (std::make_unique<juce::AudioParameterBool> (
            id ("reverse"), nm ("Reverse"), false));

        // Output
        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            id ("gain"), nm ("Gain"),
            juce::NormalisableRange<float> (-24.0f, 6.0f, 0.1f), 0.0f));

        // Filter
        params.push_back (std::make_unique<juce::AudioParameterChoice> (
            id ("filterType"), nm ("Filter Type"),
            juce::StringArray { "Off", "LPF", "HPF", "BPF" }, 0));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            id ("filterCutoff"), nm ("Filter Cutoff"),
            juce::NormalisableRange<float> (20.0f, 20000.0f, 1.0f, 0.3f), 1000.0f));

        params.push_back (std::make_unique<juce::AudioParameterFloat> (
            id ("filterQ"), nm ("Filter Q"),
            juce::NormalisableRange<float> (0.1f, 10.0f, 0.01f, 0.5f), 0.707f));
    }

    return { params.begin(), params.end() };
}