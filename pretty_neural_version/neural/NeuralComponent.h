#pragma once

#include "./NeuralProcessor.h"
#include "./ui/BackendSelector.h"
#include "./ui/DryWetSlider.h"
#include "./NeuralParameters.h"

//==============================================================================
class NeuralProcessorComponent  : public juce::Component, private juce::AudioProcessorValueTreeState::Listener
{
public:
    explicit NeuralProcessorComponent (NeuralProcessor&);
    ~NeuralProcessorComponent() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void parameterChanged (const juce::String& parameterID, float newValue) override;

private:
    NeuralProcessor& processorRef;

    const int pluginEditorWidth = 520;
    const int pluginEditorHeight = 750;

    std::unique_ptr<juce::Drawable> background = juce::Drawable::createFromImageData (BinaryData::background_png, BinaryData::background_pngSize);
    std::unique_ptr<juce::Drawable> texture = juce::Drawable::createFromImageData (BinaryData::texture_overlay_png, BinaryData::texture_overlay_pngSize);

    std::unique_ptr<juce::Drawable> tfliteFont = juce::Drawable::createFromImageData (BinaryData::tflite_font_svg, BinaryData::tflite_font_svgSize);
    std::unique_ptr<juce::Drawable> onnxFont = juce::Drawable::createFromImageData (BinaryData::onnx_font_svg, BinaryData::onnx_font_svgSize);
    std::unique_ptr<juce::Drawable> libtorchFont = juce::Drawable::createFromImageData (BinaryData::libtorch_font_svg, BinaryData::libtorch_font_svgSize);
    juce::Rectangle<int> fontBounds;

    BackendSelector backendSelector;
    DryWetSlider dryWetSlider;

    juce::AudioProcessorValueTreeState& apvts;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NeuralProcessorComponent)
};
