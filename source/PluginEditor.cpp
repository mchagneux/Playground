#include "PluginEditor.h"
#include "./utils/Panels.h"

//==============================================================================
PluginEditor::PluginEditor (Plugin& p)
    : juce::AudioProcessorEditor (&p)
    , processorRef (p)
    // , cmajorEditor (p.getCmajorProcessor().createUI())
    , postProcessorControls (*this, p.getPostProcessor())
    , neuralControls (*this, p.parameters.neural)
{
    juce::ignoreUnused (processorRef);

    // addAndMakeVisible (*cmajorEditor);
    addAndMakeVisible (topPanelComponent);
    addAndMakeVisible (bottomPanelComponent);
    addAndMakeVisible (postProcessorControls);
    addAndMakeVisible (neuralControls);
    setResizable (true, true);
    setSize (1600, 900);
}

PluginEditor::~PluginEditor() {}

//==============================================================================
void PluginEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a
    // solid colour)
    g.fillAll (juce::Colours::black);
}

void PluginEditor::resized()
{
    auto area = getLocalBounds();
    auto topPanelArea = area.removeFromTop ((int) (getHeight() / 8));
    auto bottomPanelArea = area.removeFromBottom ((int) (getHeight() / 8));
    topPanelComponent.setBounds (topPanelArea);
    bottomPanelComponent.setBounds (bottomPanelArea);

    // cmajorEditor->setBounds (area.removeFromLeft ((int) (getWidth() / 3)));
    postProcessorControls.setBounds (area.removeFromRight ((int) (getWidth() / 3)));
    neuralControls.setBounds (area);
}
