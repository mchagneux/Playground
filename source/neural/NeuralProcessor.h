#pragma once

#include <JuceHeader.h>


#include <anira/anira.h>

#include "../../3rd_party/anira/extras/desktop/models/cnn/CNNConfig.h"
#include "../../3rd_party/anira/extras/desktop/models/cnn/CNNPrePostProcessor.h"
#include "../../3rd_party/anira/extras/desktop/models/cnn/advanced-configs/CNNNoneProcessor.h" // This one is only needed for the round trip test, when selecting the None backend
#include "../../3rd_party/anira/extras/desktop/models/hybrid-nn/HybridNNConfig.h"
#include "../../3rd_party/anira/extras/desktop/models/hybrid-nn/HybridNNPrePostProcessor.h"
#include "../../3rd_party/anira/extras/desktop/models/hybrid-nn/advanced-configs/HybridNNNoneProcessor.h" // Only needed for round trip test
#include "../../3rd_party/anira/extras/desktop/models/stateful-rnn/StatefulRNNConfig.h"
#include "../../3rd_party/anira/extras/desktop/models/stateful-rnn/StatefulRNNPrePostProcessor.h"

//==============================================================================
class NeuralProcessor  : public juce::AudioProcessor, private juce::AudioProcessorValueTreeState::Listener
{
public:
    //==============================================================================
    NeuralProcessor(BusesProperties buses, juce::AudioProcessorValueTreeState& a);
    ~NeuralProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    anira::InferenceManager &getInferenceManager();
    juce::AudioProcessorValueTreeState& getValueTreeState() { return parameters; }

private:
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void stereoToMono(juce::AudioBuffer<float> &targetMonoBlock, juce::AudioBuffer<float> &sourceBlock);
    void monoToStereo(juce::AudioBuffer<float> &targetStereoBlock, juce::AudioBuffer<float> &sourceBlock);

private:
    juce::AudioProcessorValueTreeState& parameters;
    juce::AudioBuffer<float> monoBuffer;

#if MODEL_TO_USE == 1
    anira::InferenceConfig inferenceConfig = cnnConfig;
    CNNPrePostProcessor prePostProcessor;
    CNNNoneProcessor noneProcessor; // This one is only needed for the round trip test, when selecting the None backend
#elif MODEL_TO_USE == 2
    anira::InferenceConfig inferenceConfig = hybridNNConfig;
    HybridNNPrePostProcessor prePostProcessor;
    HybridNNNoneProcessor noneProcessor; // This one is only needed for the round trip test, when selecting the None backend
#elif MODEL_TO_USE == 3
    anira::InferenceConfig inferenceConfig = statefulRNNConfig;
    StatefulRNNPrePostProcessor prePostProcessor;
#endif
    anira::InferenceHandler inferenceHandler;

    juce::dsp::DryWetMixer<float> dryWetMixer;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NeuralProcessor)
};
