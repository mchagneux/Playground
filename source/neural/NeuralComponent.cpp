#include "./NeuralComponent.h"

// TODO: Not fully implemented yet
#define WINDOW_RESIZEABLE false

//==============================================================================
NeuralProcessorComponent::NeuralProcessorComponent (juce::AudioProcessorValueTreeState& a)
    : parameters(a), backendSelector(a), dryWetSlider(a), apvts(a)
{
    // juce::ignoreUnused (processorRef);

    for (auto & parameterID : PluginParameters::getPluginParameterList()) {
        apvts.addParameterListener(parameterID, this);
    }

    // if (WINDOW_RESIZEABLE) {
    //     double ratio = static_cast<double>(pluginEditorWidth) / static_cast<double>(pluginEditorHeight);
    //     setResizeLimits(pluginEditorWidth / 2, pluginEditorHeight / 2, pluginEditorWidth, pluginEditorHeight);
    //     getConstrainer()->setFixedAspectRatio(ratio);
    // }
    // setSize(pluginEditorWidth, pluginEditorHeight);

    addAndMakeVisible(backendSelector);
    addAndMakeVisible(dryWetSlider);
}

NeuralProcessorComponent::~NeuralProcessorComponent()
{
    for (auto & parameterID : PluginParameters::getPluginParameterList()) {
        apvts.removeParameterListener(parameterID, this);
    }
}

//==============================================================================
void NeuralProcessorComponent::paint (juce::Graphics& g)
{
    auto currentBound = getLocalBounds();
    background->drawWithin(g, currentBound.toFloat(), juce::RectanglePlacement::centred, 1.0f);
    texture->drawWithin(g, currentBound.toFloat(), juce::RectanglePlacement::centred, .25f);

    auto currentBackend = backendSelector.getBackend();

    switch (currentBackend) {
        case anira::TFLITE:
            tfliteFont->drawWithin(g, fontBounds.toFloat(), juce::RectanglePlacement::doNotResize, 1.0f);
            break;
        case anira::LIBTORCH:
            libtorchFont->drawWithin(g, fontBounds.toFloat(), juce::RectanglePlacement::doNotResize, 1.0f);
            break;
        case anira::ONNX:
            onnxFont->drawWithin(g, fontBounds.toFloat(), juce::RectanglePlacement::doNotResize, 1.0f);
            break;
        default:
            break;
    }

}

void NeuralProcessorComponent::resized()
{
    auto scaledHeight = [&] (int factor) {
        return static_cast<int>((float) getHeight() / (float) pluginEditorHeight * factor);
    };

    fontBounds = getLocalBounds().removeFromTop(610).removeFromBottom(75);

    backendSelector.setBounds(getLocalBounds().removeFromTop(scaledHeight(500)));

    auto sliderComponentBound = getLocalBounds().removeFromTop(scaledHeight(710)).removeFromBottom(scaledHeight(60));
    dryWetSlider.setBounds(sliderComponentBound);

}

void NeuralProcessorComponent::parameterChanged(const juce::String &parameterID, float newValue) {
    if (parameterID == PluginParameters::BACKEND_TYPE_ID.getParamID()) {
        if (juce::MessageManager::getInstance()->isThisTheMessageThread()) {
            backendSelector.setBackend(static_cast<int>(newValue));
            repaint();
        } else {
            juce::MessageManager::callAsync( [this, newValue] {
                backendSelector.setBackend(static_cast<int>(newValue));
                repaint();
            } );
        }
    }
}
