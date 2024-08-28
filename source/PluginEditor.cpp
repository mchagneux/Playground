#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    juce::ignoreUnused (processorRef);
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    cmajorJITComponent = std::make_unique<CmajorJITComponent>(processorRef.getCmajorProcessor());
    addAndMakeVisible(*cmajorJITComponent);
    // setResizable(true, true);
    setSize (1280, 720);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{

}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    // g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
}

void AudioPluginAudioProcessorEditor::resized()
{

    auto area = getLocalBounds();
    cmajorJITComponent->setBounds(area.removeFromLeft((int)(getWidth() / 3)));

    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}
