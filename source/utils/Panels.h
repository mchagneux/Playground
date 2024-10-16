#pragma once
#include <JuceHeader.h>

struct TopPanel : public juce::Component
{
public:
    TopPanel() {}

    ~TopPanel() override {}

    void paint (juce::Graphics& g) override
    {
        // g.fillAll(juce::Colours::black);
        auto textBounds = getLocalBounds();
        textBounds.removeFromRight ((int) (getWidth() / 3));
        textBounds.removeFromLeft ((int) (getWidth() / 3));
        g.drawText ("Playground", textBounds, juce::Justification::centred);
    }

    void resized() override
    {
        // auto titleArea = getLocalBounds();
        // titleArea.removeFromLeft((int) (getWidth()  / 3 ));
        // titleArea.removeFromRight((int) (getWidth()  / 3 ));
        // titleComponent.setBounds(titleArea);
    }

private:
    // juce::TextEditor titleComponent { "Playground" };
};

struct BottomPanel : public juce::Component
{
public:
    BottomPanel() {}

    ~BottomPanel() override {}
};
