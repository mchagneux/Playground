#pragma once
#include <JuceHeader.h>
#include "./CircularBuffer.h"
#include "./Scope.h"
#include "./GainMeter.h"

struct TopPanel : public juce::Component
{
public:
    TopPanel()
    {
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

    Scope<float> scope;
    GainMeter gainMeter;

private:
};

struct BottomPanel : public juce::Component
{
public:
    BottomPanel() {}

    ~BottomPanel() override {}
};
