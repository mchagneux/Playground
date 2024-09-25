#pragma once 
#include "../utils/Parameters.h"
#include "../utils/Components.h"


using CompressorProcessor = juce::dsp::Compressor<float>; 

struct CompressorControls final : public juce::Component
{
    explicit CompressorControls (juce::AudioProcessorEditor& editor,
                                    const CompressorParameters& state)
        : toggle    (editor, state.enabled),
            threshold (editor, state.threshold),
            ratio     (editor, state.ratio),
            attack    (editor, state.attack),
            release   (editor, state.release)
    {
        addAllAndMakeVisible (*this, toggle, threshold, ratio, attack, release);
    }

    void resized() override
    {
        performLayout (getLocalBounds(), toggle, threshold, ratio, attack, release);
    }

    AttachedToggle toggle;
    AttachedSlider threshold, ratio, attack, release;
};