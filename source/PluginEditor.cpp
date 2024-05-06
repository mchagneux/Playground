#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor(
    AudioPluginAudioProcessor &p)
    : AudioProcessorEditor(&p), processorRef(p) {
  juce::ignoreUnused(processorRef);
  // Make sure that before the constructor has finished, you've set the
  // editor's size to whatever you need it to be.
  setSize(800, 300);
  addAndMakeVisible(&lowerPanel);

  // oscOnButton.setButtonText("Test Tone");
  // // oscOnButton.onClick = [this] {

  // // };
  // gainSlider.setSliderStyle(juce::Slider::LinearBarVertical);
  // gainSlider.setRange(0.0, 127.0, 1.0);
  // gainSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 90, 0);
  // gainSlider.setPopupDisplayEnabled(true, false, this);
  // gainSlider.setTextValueSuffix(" Gain");
  // gainSlider.setValue(1.0);
  // addAndMakeVisible(&oscOnButton);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor() {}

//==============================================================================
void AudioPluginAudioProcessorEditor::paint(juce::Graphics &g) {
  // (Our component is opaque, so we must completely fill the background with a
  // solid colour)
  g.fillAll(
      getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

  // g.setColour (juce::Colours::white);
  // g.setFont (15.0f);
  g.drawFittedText ("Hello World!", getLocalBounds(),
  juce::Justification::centred, 1);
}

void AudioPluginAudioProcessorEditor::resized() {
  // This is generally where you'll want to lay out the positions of any
  // subcomponents in your editor..
  auto area = getLocalBounds();
  auto lowerPanelBounds = area.removeFromBottom(0.1*getHeight());
  lowerPanel.setBounds(lowerPanelBounds);
  
  // auto gainSliderBounds = area.removeFromRight(0.1 * getWidth());
  // auto gainSliderBoundsReduced = gainSliderBounds.reduced(
  //     0.1 * gainSliderBounds.getWidth(), 0.1 * gainSliderBounds.getHeight());
  // gainSlider.setBounds(gainSliderBoundsReduced);

  // auto oscButtonArea = getLocalBounds().removeFromTop(0.1 * getHeight()).removeFromLeft(0.1*getWidth());
  // // auto oscOnButtonBounds = oscButtonArea.reduced(
  // //     0.05 * oscButtonArea.getWidth(), 0.1 * oscButtonArea.getHeight());
  // oscOnButton.setBounds(oscButtonArea);
}
