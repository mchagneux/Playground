#pragma once
#include <JuceHeader.h>
#include "./CircularBuffer.h"
#include "./Scope.h"
#include "./GainMeter.h"

struct TopPanel : public juce::Component
    , private juce::Timer
{
public:
    TopPanel (CircularBuffer<float>& visualizersFIFOIn)
        : visualizersFIFO (visualizersFIFOIn)
    {
        startTimer (20);
        addAndMakeVisible (scope);
        addAndMakeVisible (gainMeter);
    }

    void resized() override
    {
        auto amountToRemove = (float) getWidth() / 5.0f;

        auto r = getLocalBounds();
        r.removeFromLeft (amountToRemove);
        r.removeFromRight (amountToRemove);
        scope.setBounds (r.removeFromLeft ((float) r.getWidth() / 2.0f));
        gainMeter.setBounds (r);
    }

private:
    void timerCallback() override
    {
        if (visualizersFIFO.copyAvailableData (visualizationBuffer))
        {
            if (scope.samplesDrawn)
            {
                scope.samplesToDraw.copyFrom (0, 0, visualizationBuffer.getReadPointer (0), scope.samplesToDraw.getNumSamples());
                scope.samplesDrawn = false;
                scope.repaint();
            }

            if (gainMeter.valueDrawn)
            {
                gainMeter.valueToDraw01 = visualizationBuffer.getRMSLevel (0, 0, visualizationBuffer.getNumSamples());
                gainMeter.valueDrawn = false;
                gainMeter.repaint();
            }
        }
    }

    Scope<float> scope;
    GainMeter gainMeter;
    juce::AudioBuffer<float> visualizationBuffer { 1, 1000 };
    CircularBuffer<float>& visualizersFIFO;
};

struct BottomPanel : public juce::Component
{
public:
    BottomPanel() {}

    ~BottomPanel() override {}
};
