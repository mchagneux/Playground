#pragma once
#include <JuceHeader.h>

template <typename Type>
struct Scope : public juce::Component
{
public:
    Scope()

    {
        numSamplesToDraw = samplesToDraw.getNumSamples();
    }

    void paint (juce::Graphics& g) override

    {
        if (! samplesDrawn)
        {
            auto bounds = getLocalBounds().toFloat();

            scopePath.clear();
            scopePath.preallocateSpace (8 + 3 * numSamplesToDraw);

            auto maxValue = samplesToDraw.getMagnitude (0, 0);

            if (juce::approximatelyEqual (maxValue, 0.0f))
                maxValue = 1.0f;
            // std::cout << juce::String (maxValue) << std::endl;

            auto sampleToDraw = samplesToDraw.getSample (0, 0) / maxValue;
            scopePath.startNewSubPath (0, sampleValueToY (sampleToDraw, bounds));

            for (int i = 1; i < numSamplesToDraw; ++i)
            {
                auto sampleToDraw = samplesToDraw.getSample (0, i) / maxValue;
                scopePath.lineTo (indexToX ((float) i / (float) (numSamplesToDraw - 1), bounds),
                                  sampleValueToY (sampleToDraw,
                                                  bounds));
            }
            g.setColour (juce::Colours::grey);
            g.strokePath (scopePath, juce::PathStrokeType (1.0));
            samplesDrawn = true;
        }
    }

    float sampleValueToY (float normalizedSampleValue, const juce::Rectangle<float>& bounds)

    {
        return juce::jmap (normalizedSampleValue, bounds.getBottom(), bounds.getY());
    }

    float indexToX (float normalizedIndex, const juce::Rectangle<float>& bounds)
    {
        return juce::jmap (normalizedIndex, bounds.getBottomLeft().getX(), bounds.getWidth());
    }

    juce::AudioBuffer<float> samplesToDraw { 1, 1000 };
    bool samplesDrawn = false;

private:
    int numSamplesToDraw;
    juce::Path scopePath;
};