#pragma once

#include "CmajorProcessor.h"
#include "PluginProcessor.h"
// #include "melatonin_inspector/melatonin_inspector.h"

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
    std::unique_ptr<CmajorLoaderUI> cmajorLoaderUI;
    std::unique_ptr<CmajorComponent> cmajorUI;
    // melatonin::Inspector inspector { *this };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
