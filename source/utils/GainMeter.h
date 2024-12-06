#pragma once
#include <JuceHeader.h>

struct GainMeter : public juce::Component
{
    GainMeter() {}

    void paint (juce::Graphics& g) override
    {
        if (! valueDrawn)
        {
            auto bounds = getLocalBounds().toFloat();

            auto rectangleToDraw = bounds.removeFromLeft (valueToDraw01 * (float) getWidth());

            g.setColour (juce::Colours::grey);

            g.drawRect (rectangleToDraw);
            g.fillRect (rectangleToDraw);
            valueDrawn = true;
        };
    }

    bool valueDrawn = false;
    float valueToDraw01 = 0.0f;
};