#pragma once

#include <JuceHeader.h>
#include <functional>

class WaveformDropComponent : public juce::Component,
                              public juce::FileDragAndDropTarget,
                              public juce::ChangeListener,
                              public juce::Button::Listener
{
public:
    WaveformDropComponent();
    ~WaveformDropComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;
    void buttonClicked(juce::Button* button) override;

    void setFileDropHandler(std::function<bool(const juce::File&, juce::String&)> handler);
    void setLoadedNameProvider(std::function<juce::String()> provider);

private:
    static bool isSupportedAudioFile(const juce::File& file);
    void openFileChooser();
    void loadAudioFile(const juce::File& file);

    juce::AudioFormatManager formatManager;
    juce::AudioThumbnailCache thumbnailCache { 10 };
    juce::AudioThumbnail thumbnail { 512, formatManager, thumbnailCache };

    std::function<bool(const juce::File&, juce::String&)> fileDropHandler;
    std::function<juce::String()> loadedNameProvider;

    juce::String statusText { "No file loaded" };
    bool hasWaveform { false };
    
    juce::TextButton browseButton { "Browse..." };
    std::unique_ptr<juce::FileChooser> fileChooser;
};
