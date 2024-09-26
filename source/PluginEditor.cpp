#include "./utils/Panels.h"
#include "PluginEditor.h"

//==============================================================================
MainProcessorEditor::MainProcessorEditor (MainProcessor& p)
    : juce::AudioProcessorEditor (&p), 
      processorRef (p), 
      cmajorEditor(p.getCmajorProcessor().createUI()),
      postProcessorControls(*this, p.parameters.postProcessor),
      neuralControls(*this, p.parameters.neural)
{
    juce::ignoreUnused (processorRef);
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    // cmajorJITComponent = std::make_unique<juce::Component>(static_cast<juce::Component *>(processorRef.getCmajorProcessor().createEditor()));


    addAndMakeVisible(*cmajorEditor);
    addAndMakeVisible(topPanelComponent);
    addAndMakeVisible(bottomPanelComponent);
    addAndMakeVisible(postProcessorControls); 
    addAndMakeVisible(neuralControls);
    setResizable(true, true);
    setSize (1920, 1080);
}

MainProcessorEditor::~MainProcessorEditor()
{

}

//==============================================================================
void MainProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (juce::Colours::black); 

    // g.setColour (juce::Colours::white);
    // g.setFont (15.0f);
}

void MainProcessorEditor::resized()
{

    auto area = getLocalBounds();
    auto topPanelArea = area.removeFromTop((int) (getHeight() / 8));
    auto bottomPanelArea = area.removeFromBottom((int)(getHeight() / 8));
    topPanelComponent.setBounds(topPanelArea); 
    bottomPanelComponent.setBounds(bottomPanelArea); 

    cmajorEditor->setBounds(area.removeFromLeft((int) (getWidth() / 3)));
    postProcessorControls.setBounds(area.removeFromRight((int) (getWidth() / 3))); 
    neuralControls.setBounds(area); 

    // neuralComponent.setBounds(area.removeFromLeft((int) (getWidth() / 3)));
// 

    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}
