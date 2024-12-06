#pragma once
#include "./Components.h"

struct GainMeter : public AudioReactiveComponent
{
    GainMeter() {}

    void paint (juce::Graphics& g) override
    {
        if (! drawnPrevious)
        {
            auto bounds = getLocalBounds().toFloat();

            auto rectangleToDraw = bounds.removeFromLeft (valueToDraw01 * (float) getWidth());

            g.setColour (juce::Colours::grey);

            g.drawRect (rectangleToDraw);
            g.fillRect (rectangleToDraw);
            drawnPrevious = true;
        };
    }

    void updateFromBuffer (const juce::AudioBuffer<float>& buffer) override
    {
        valueToDraw01 = buffer.getRMSLevel (0, 0, buffer.getNumSamples());
        drawnPrevious = false;
    }

private:
    float valueToDraw01 = 0.0f;
};