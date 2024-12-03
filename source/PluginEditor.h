#pragma once
#include "./processors/NeuralProcessor.h"
#include "./processors/PostProcessor.h"
#include "./utils/Panels.h"
#include "Plugin.h"

#include "melatonin_inspector/melatonin_inspector.h"

//==============================================================================
class PluginEditor final : public juce::AudioProcessorEditor
{
public:
    explicit PluginEditor (Plugin&);
    ~PluginEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    Plugin& processorRef;
    std::unique_ptr<juce::Component> cmajorEditor;
    TopPanel topPanelComponent;
    BottomPanel bottomPanelComponent;
    PostProcessorControls postProcessorControls;
    NeuralControls neuralControls;
    melatonin::Inspector inspector { *this };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
