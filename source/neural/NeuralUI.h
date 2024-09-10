#include "./NeuralParameters.h"

class NeuralComponent: public juce::Component
{ 
public: 
    NeuralComponent(juce::AudioProcessorValueTreeState& a): parameters(a) 
    { 
        dryWetSlider.setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
        dryWetSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);



        backendSelector.addItemList(NeuralParameters::backendTypes, 0);

        sliderAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(parameters, 
                                                                                                NeuralParameters::DRY_WET_ID.getParamID(), 
                                                                                                dryWetSlider);

        backendSelectorAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(parameters, 
                                                                                                            NeuralParameters::BACKEND_TYPE_ID.getParamID(), 
                                                                                                            backendSelector);

        addAndMakeVisible(dryWetSlider);
        addAndMakeVisible(backendSelector);

    }; 

    ~NeuralComponent() override 
    {
        sliderAttachment.reset();

    }; 

    void resized() override 
    {
        auto area = getLocalBounds(); 


        auto sliderArea = area.removeFromTop((int)(getHeight() / 2)); 

        dryWetSlider.setBounds(sliderArea.reduced((int) ( sliderArea.getWidth() / 10 ), (int) ( sliderArea.getHeight() / 10))); 

        backendSelector.setBounds(area.reduced ((int) ( sliderArea.getWidth() / 4 ), (int) ( sliderArea.getHeight() / 4))); 

    };
    

private: 
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sliderAttachment; 
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> backendSelectorAttachment; 
    juce::AudioProcessorValueTreeState& parameters; 
    juce::Slider dryWetSlider; 
    juce::ComboBox backendSelector; 

};