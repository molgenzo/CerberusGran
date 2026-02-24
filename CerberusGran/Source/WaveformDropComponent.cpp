#include "WaveformDropComponent.h"

WaveformDropComponent::WaveformDropComponent()
{
    formatManager.registerBasicFormats();
    thumbnail.addChangeListener(this);
    
    browseButton.addListener(this);
    browseButton.setColour(juce::TextButton::buttonColourId, juce::Colour::fromRGB(60, 120, 216));
    browseButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    browseButton.setInterceptsMouseClicks(true, false);
    addAndMakeVisible(browseButton);
}

WaveformDropComponent::~WaveformDropComponent()
{
    thumbnail.removeChangeListener(this);
}

void WaveformDropComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour::fromRGB(15, 16, 19));

    auto bounds = getLocalBounds();

    g.setColour(juce::Colour::fromRGB(28, 31, 36));
    g.fillRoundedRectangle(bounds.toFloat(), 8.0f);

    g.setColour(juce::Colour::fromRGB(71, 78, 88));
    g.drawRoundedRectangle(bounds.toFloat(), 8.0f, 1.0f);

    // Reserve space for status text at bottom
    auto statusArea = bounds.removeFromBottom(35).reduced(12, 8);
    
    // Wave display area (with top space for button and bottom space for status)
    auto drawArea = bounds.reduced(12).withTrimmedTop(48);

    if (hasWaveform && thumbnail.getTotalLength() > 0.0)
    {
        g.setColour(juce::Colour::fromRGB(146, 218, 186));
        thumbnail.drawChannels(g, drawArea, 0.0, thumbnail.getTotalLength(), 1.0f);
    }
    else
    {
        g.setColour(juce::Colours::lightgrey);
        g.setFont(16.0f);
        g.drawFittedText("Click 'Browse' to load an audio file", drawArea, juce::Justification::centred, 1);
    }

    // Status text at bottom
    g.setColour(juce::Colours::lightgrey);
    g.setFont(14.0f);
    g.drawFittedText(statusText, statusArea, juce::Justification::centredLeft, 1);
}

void WaveformDropComponent::resized()
{
    auto bounds = getLocalBounds();
    browseButton.setBounds(bounds.removeFromTop(44).reduced(12, 8));
}

bool WaveformDropComponent::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (const auto& filePath : files)
    {
        if (isSupportedAudioFile(juce::File(filePath)))
            return true;
    }

    return false;
}

void WaveformDropComponent::filesDropped(const juce::StringArray& files, int, int)
{
    if (files.isEmpty())
        return;

    loadAudioFile(juce::File(files[0]));
}

void WaveformDropComponent::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &thumbnail)
        repaint();
}

void WaveformDropComponent::buttonClicked(juce::Button* button)
{
    if (button == &browseButton)
        openFileChooser();
}

void WaveformDropComponent::setFileDropHandler(std::function<bool(const juce::File&, juce::String&)> handler)
{
    fileDropHandler = std::move(handler);
}

void WaveformDropComponent::setLoadedNameProvider(std::function<juce::String()> provider)
{
    loadedNameProvider = std::move(provider);
}

bool WaveformDropComponent::isSupportedAudioFile(const juce::File& file)
{
    const auto ext = file.getFileExtension().toLowerCase();
    return ext == ".wav" || ext == ".aif" || ext == ".aiff" || ext == ".flac" || ext == ".mp3";
}

void WaveformDropComponent::openFileChooser()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Select an audio file",
        juce::File::getSpecialLocation(juce::File::userMusicDirectory),
        "*.wav;*.aif;*.aiff;*.flac;*.mp3");
    
    auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
    
    fileChooser->launchAsync(flags, [this](const juce::FileChooser& fc)
    {
        auto file = fc.getResult();
        if (file != juce::File{} && file.existsAsFile())
        {
            loadAudioFile(file);
        }
    });
}

void WaveformDropComponent::loadAudioFile(const juce::File& file)
{
    if (fileDropHandler == nullptr)
        return;
    
    juce::String errorMessage;

    if (fileDropHandler(file, errorMessage))
    {
        hasWaveform = thumbnail.setSource(new juce::FileInputSource(file));

        if (loadedNameProvider != nullptr)
            statusText = loadedNameProvider();
        else
            statusText = file.getFileName();
    }
    else
    {
        hasWaveform = false;
        statusText = "Load failed: " + errorMessage;
    }

    repaint();
}
