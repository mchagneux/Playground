#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p), cmajorEditor(p.getCmajorProcessor().createUI())
{
    juce::ignoreUnused (processorRef);
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    // cmajorJITComponent = std::make_unique<juce::Component>(static_cast<juce::Component *>(processorRef.getCmajorProcessor().createEditor()));
    neuralComponent = std::make_unique<NeuralProcessorComponent>(processorRef.getValueTreeState()); 
    addAndMakeVisible(*neuralComponent);
    addAndMakeVisible(*cmajorEditor);
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
    g.fillAll (juce::Colours::black); 

    // g.setColour (juce::Colours::white);
    // g.setFont (15.0f);
}

void AudioPluginAudioProcessorEditor::resized()
{

    auto area = getLocalBounds();
    cmajorEditor->setBounds(area.removeFromLeft((int) (getWidth() / 3)));
    neuralComponent->setBounds(area.removeFromLeft((int) (getWidth() / 3)));
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}
