#pragma once 
#include <juce_gui_basics/juce_gui_basics.h>  



class LowerPanel   : public juce::Component
{
public:
    LowerPanel() 
    {

        // selector_1.addItemList(choices, 0);
        // selector_2.addItemList(choices, 0);
        // selector_3.addItemList(choices, 0);

        addAndMakeVisible(&selector_1);
        addAndMakeVisible(&selector_2);
        addAndMakeVisible(&selector_3);

    }

    // void paint (juce::Graphics&) override {}

    void resized() override
    {
        auto area = getLocalBounds();
        auto padding_height = 0.05 * getHeight();
        auto padding_width = 0.05 * getWidth() / 3 ;
        auto selector_1_bounds = area.removeFromLeft(getWidth() / 3).reduced(padding_width, padding_height);
        auto selector_2_bounds = area.removeFromLeft(getWidth()/3).reduced(padding_width, padding_height);
        auto selector_3_bounds = area.reduced(padding_width, padding_height);
        
        selector_1.setBounds(selector_1_bounds);
        selector_2.setBounds(selector_2_bounds);
        selector_3.setBounds(selector_3_bounds);

    
    }
private:
    //==============================================================================
    juce::ComboBox selector_1; 
    juce::ComboBox selector_2;
    juce::ComboBox selector_3;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LowerPanel)
};