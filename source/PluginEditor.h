#pragma once
#include "PluginProcessor.h"
#include "Panels.h"
#include "melatonin_inspector/melatonin_inspector.h"
#include "./processors/PostProcessor.h"
#include "./processors/NeuralProcessor.h"

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
    TopPanel topPanelComponent; 
    BottomPanel bottomPanelComponent; 
    PostProcessorControls postProcessorControls; 
    NeuralControls neuralControls; 
    melatonin::Inspector inspector { *this };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
