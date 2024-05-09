#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../processors/Distortion.h"
#include "./Components.h"

struct DistortionControls final : public Component
{
    explicit DistortionControls (AudioProcessorEditor& editor,
                                    const DistortionProcessor::Parameters& state)
        : toggle       (editor, state.enabled),
            lowpass      (editor, state.lowpass),
            highpass     (editor, state.highpass),
            mix          (editor, state.mix),
            gain         (editor, state.inGain),
            compv        (editor, state.compGain),
            type         (editor, state.type),
            oversampling (editor, state.oversampler)
    {
        addAllAndMakeVisible (*this, toggle, type, lowpass, highpass, mix, gain, compv, oversampling);
    }

    void resized() override
    {
        performLayout (getLocalBounds(), toggle, type, gain, highpass, lowpass, compv, mix, oversampling);
    }

    AttachedToggle toggle;
    AttachedSlider lowpass, highpass, mix, gain, compv;
    AttachedCombo type, oversampling;
};
