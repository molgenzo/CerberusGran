#pragma once
#include <JuceHeader.h>

static constexpr int kNumGrainHeads = 5;

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Global params
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("masterGain", 1), "Master Gain",
        juce::NormalisableRange<float> (0.0f, 2.0f, 0.01f), 1.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("masterPitch", 1), "Master Pitch",
        juce::NormalisableRange<float> (0.25f, 4.0f, 0.01f, 0.5f), 1.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("dryWet", 1), "Dry/Wet",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 1.0f));
    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID ("sourceMode", 1), "Source Mode",
        juce::StringArray { "Live", "File" }, 0));

    // Per-head params
    for (int h = 0; h < kNumGrainHeads; ++h)
    {
        auto id = [h] (const juce::String& name) { return "head" + juce::String (h) + "_" + name; };
        auto nm = [h] (const juce::String& name) { return "H" + juce::String (h) + " " + name; };

        layout.add (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID (id ("enable"), 1), nm ("Enable"), h == 0));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID (id ("position"), 1), nm ("Position"),
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), h * 0.25f));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID (id ("posScatter"), 1), nm ("Pos Scatter"),
            juce::NormalisableRange<float> (0.0f, 0.5f, 0.001f), 0.0f));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID (id ("density"), 1), nm ("Density"),
            juce::NormalisableRange<float> (0.5f, 100.0f, 0.1f, 0.4f), 10.0f));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID (id ("duration"), 1), nm ("Duration"),
            juce::NormalisableRange<float> (5.0f, 500.0f, 0.1f, 0.4f), 80.0f));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID (id ("pitch"), 1), nm ("Pitch"),
            juce::NormalisableRange<float> (0.1f, 4.0f, 0.01f, 0.5f), 1.0f));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID (id ("pitchScatter"), 1), nm ("Pitch Scatter"),
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID (id ("window"), 1), nm ("Window"),
            juce::StringArray { "Hann", "Gaussian", "Tukey", "Triangle" }, 0));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID (id ("gain"), 1), nm ("Gain"),
            juce::NormalisableRange<float> (0.0f, 2.0f, 0.01f), 0.8f));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID (id ("pan"), 1), nm ("Pan"),
            juce::NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.0f));

        // Filter
        layout.add (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID (id ("filterBypass"), 1), nm ("Filter Bypass"), true));
        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID (id ("filterType"), 1), nm ("Filter Type"),
            juce::StringArray { "Low Pass", "High Pass", "Band Pass" }, 0));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID (id ("filterCutoff"), 1), nm ("Filter Cutoff"),
            juce::NormalisableRange<float> (20.0f, 20000.0f, 1.0f, 0.3f), 1000.0f));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID (id ("filterRes"), 1), nm ("Filter Res"),
            juce::NormalisableRange<float> (0.1f, 10.0f, 0.01f, 0.5f), 0.707f));

        // Saturator
        layout.add (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID (id ("satBypass"), 1), nm ("Sat Bypass"), true));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID (id ("satDrive"), 1), nm ("Sat Drive"),
            juce::NormalisableRange<float> (1.0f, 20.0f, 0.1f, 0.5f), 1.0f));

        // Bitcrusher
        layout.add (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID (id ("crushBypass"), 1), nm ("Crush Bypass"), true));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID (id ("crushBits"), 1), nm ("Crush Bits"),
            juce::NormalisableRange<float> (1.0f, 16.0f, 0.1f), 16.0f));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID (id ("crushRate"), 1), nm ("Crush Rate"),
            juce::NormalisableRange<float> (1.0f, 50.0f, 0.1f), 1.0f));

        // Delay
        layout.add (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID (id ("delayBypass"), 1), nm ("Delay Bypass"), true));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID (id ("delayTime"), 1), nm ("Delay Time"),
            juce::NormalisableRange<float> (1.0f, 2000.0f, 0.1f, 0.4f), 250.0f));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID (id ("delayFeedback"), 1), nm ("Delay Feedback"),
            juce::NormalisableRange<float> (0.0f, 0.95f, 0.01f), 0.3f));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID (id ("delayMix"), 1), nm ("Delay Mix"),
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

        // LFO
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID (id ("lfoRate"), 1), nm ("LFO Rate"),
            juce::NormalisableRange<float> (0.01f, 20.0f, 0.01f, 0.4f), 1.0f));
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID (id ("lfoDepth"), 1), nm ("LFO Depth"),
            juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.0f));
        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID (id ("lfoShape"), 1), nm ("LFO Shape"),
            juce::StringArray { "Sine", "Triangle", "Saw", "Square", "S&H" }, 0));
        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID (id ("lfoTarget"), 1), nm ("LFO Target"),
            juce::StringArray { "Off", "Position", "Pitch", "Filter Cutoff" }, 0));
    }

    return layout;
}
