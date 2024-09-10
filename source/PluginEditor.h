#pragma once
// #include "neural/NeuralUI.h"
#include "PluginProcessor.h"
#include "cmaj_JUCEPlugin.h"
#include "melatonin_inspector/melatonin_inspector.h"

//==============================================================================
class AudioPluginAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    AudioPluginAudioProcessor& processorRef;
    std::unique_ptr<juce::Component> cmajorEditor; 
    // std::unique_ptr<NeuralComponent> neuralComponent;
    melatonin::Inspector inspector { *this };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
