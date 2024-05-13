#pragma once
#include "./Components.h"
#include "../Utils.h"
#include "../processors/Ladder.h"

struct LadderControls final : public Component
{
    explicit LadderControls (AudioProcessorEditor& editor,
                                const LadderProcessor::Parameters& state)
        : toggle    (editor, state.enabled),
            mode      (editor, state.mode),
            freq      (editor, state.cutoff),
            resonance (editor, state.resonance),
            drive     (editor, state.drive)
    {
        addAllAndMakeVisible (*this, toggle, mode, freq, resonance, drive);
    }

    void resized() override
    {
        performLayout (getLocalBounds(), toggle, mode, freq, resonance, drive);
    }

    AttachedToggle toggle;
    AttachedCombo mode;
    AttachedSlider freq, resonance, drive;
};