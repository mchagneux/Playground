#pragma once

#include <JuceHeader.h>

#include <anira/anira.h>
#include "../utils/Parameters.h"
#include "../utils/Components.h"

#include "../../3rd_party/anira/extras/desktop/models/cnn/CNNConfig.h"
#include "../../3rd_party/anira/extras/desktop/models/cnn/CNNPrePostProcessor.h"
#include "../../3rd_party/anira/extras/desktop/models/cnn/advanced-configs/CNNNoneProcessor.h" // This one is only needed for the round trip test, when selecting the None backend
#include "../../3rd_party/anira/extras/desktop/models/hybrid-nn/HybridNNConfig.h"
#include "../../3rd_party/anira/extras/desktop/models/hybrid-nn/HybridNNPrePostProcessor.h"
#include "../../3rd_party/anira/extras/desktop/models/hybrid-nn/advanced-configs/HybridNNNoneProcessor.h" // Only needed for round trip test
#include "../../3rd_party/anira/extras/desktop/models/stateful-rnn/StatefulRNNConfig.h"
#include "../../3rd_party/anira/extras/desktop/models/stateful-rnn/StatefulRNNPrePostProcessor.h"

//==============================================================================
class NeuralProcessor  : public juce::AudioProcessor, private juce::AudioProcessorParameter::Listener

{
public:

    NeuralProcessor(const NeuralParameters& p) 
        : juce::AudioProcessor (getBusesProperties()), 
          parameters(p),
    #if MODEL_TO_USE == 1 || MODEL_TO_USE == 2
        //The noneProcessor is not needed for inference, but for the round trip test to output audio when selecting the NONE backend. It must be customized when default prePostProcessor is replaced by a custom one.
        noneProcessor(inferenceConfig),
        inferenceHandler(prePostProcessor, inferenceConfig, noneProcessor),
    #elif MODEL_TO_USE == 3
        inferenceHandler(prePostProcessor, inferenceConfig),
    #endif
        dryWetMixer(32768) // 32768 samples of max latency compensation for the dry-wet mixer
    {
        parameters.neuralDryWet.addListener(this); 
        parameters.neuralBackend.addListener(this); 
    }
    ~NeuralProcessor() override 
    {
        parameters.neuralDryWet.removeListener(this); 
        parameters.neuralBackend.removeListener(this);     
    }

    static BusesProperties getBusesProperties() 
    {
        return BusesProperties()
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true);
    }

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        juce::dsp::ProcessSpec monoSpec {sampleRate,
                                    static_cast<juce::uint32>(samplesPerBlock),
                                    static_cast<juce::uint32>(1)};

        anira::HostAudioConfig monoConfig {
            1,
            (size_t) samplesPerBlock,
            sampleRate
        };

        dryWetMixer.prepare(monoSpec);

        monoBuffer.setSize(1, samplesPerBlock);
        inferenceHandler.prepare(monoConfig);

        auto newLatency = inferenceHandler.getLatency();
        setLatencySamples(newLatency);

        dryWetMixer.setWetLatency(newLatency);

        parameterValueChanged(parameters.neuralDryWet.getParameterIndex(), parameters.neuralDryWet.get());
        parameterValueChanged(parameters.neuralBackend.getParameterIndex(), parameters.neuralBackend.getIndex());

    }

    void releaseResources() override {}

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override 
    {
        #if JucePlugin_IsMidiEffect
            juce::ignoreUnused (layouts);
            return true;
        #else
            // This is the place where you check if the layout is supported.
            // In this template code we only support mono or stereo.
            // Some plugin hosts, such as certain GarageBand versions, will only
            // load plugins that support stereo bus layouts.
            if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
            && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
                return false;

            // This checks if the input layout matches the output layout
        #if ! JucePlugin_IsSynth
            if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
                return false;
        #endif

            return true;
        #endif
    }

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override
    {
        juce::ignoreUnused (midiMessages);

        juce::ScopedNoDenormals noDenormals;
        auto totalNumInputChannels  = getTotalNumInputChannels();
        auto totalNumOutputChannels = getTotalNumOutputChannels();

        stereoToMono(monoBuffer, buffer);
        dryWetMixer.pushDrySamples(monoBuffer);

        auto inferenceBuffer = const_cast<float **>(monoBuffer.getArrayOfWritePointers());
        inferenceHandler.process(inferenceBuffer, (size_t) buffer.getNumSamples());

        dryWetMixer.mixWetSamples(monoBuffer);
        monoToStereo(buffer, monoBuffer);
    }

    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override { return nullptr;}
    bool hasEditor() const override { return false ; }

    //==============================================================================
    const juce::String getName() const override { return "Neural Processor" ; }

    bool acceptsMidi() const override { return false ;}
    bool producesMidi() const override {return false ;}
    bool isMidiEffect() const override {return false ;}
    double getTailLengthSeconds() const override {return 0;}

    //==============================================================================
    int getNumPrograms() override {return 1;}
    int getCurrentProgram() override {return 0;}
    void setCurrentProgram (int index) override { juce::ignoreUnused(index); }
    const juce::String getProgramName (int index) override { juce::ignoreUnused(index) ; }
    void changeProgramName (int index, const juce::String& newName) override {  juce::ignoreUnused(index) ; juce::ignoreUnused(newName) ; }

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override {juce::ignoreUnused(destData) ; }
    void setStateInformation (const void* data, int sizeInBytes) override {juce::ignoreUnused(data) ; juce::ignoreUnused(sizeInBytes) ; }

    anira::InferenceManager &getInferenceManager()  { return inferenceHandler.getInferenceManager(); }

private:

    void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override {}

    void parameterValueChanged (int parameterIndex, float newValue) override
    {
        if (parameterIndex == parameters.neuralBackend.getParameterIndex()) 
        {
            dryWetMixer.setWetMixProportion(newValue);
        } 
        else if (parameterIndex == parameters.neuralDryWet.getParameterIndex()) 
        
        {
            const auto paramInt = static_cast<int>(newValue);
            auto paramString = parameters.backendTypes.getReference(paramInt);
            if (paramString == "TFLITE") inferenceHandler.setInferenceBackend(anira::TFLITE);
            if (paramString == "ONNXRUNTIME") inferenceHandler.setInferenceBackend(anira::ONNX);
            if (paramString == "LIBTORCH") inferenceHandler.setInferenceBackend(anira::LIBTORCH);
            if (paramString == "NONE") inferenceHandler.setInferenceBackend(anira::NONE);
        }
    }

    void stereoToMono(juce::AudioBuffer<float> &targetMonoBlock, juce::AudioBuffer<float> &sourceBlock)
    {
        if (sourceBlock.getNumChannels() == 1) {
            targetMonoBlock.makeCopyOf(sourceBlock);
        } else {
            auto nSamples = sourceBlock.getNumSamples();

            auto monoWrite = targetMonoBlock.getWritePointer(0);
            auto lRead = sourceBlock.getReadPointer(0);
            auto rRead = sourceBlock.getReadPointer(1);

            juce::FloatVectorOperations::copy(monoWrite, lRead, nSamples);
            juce::FloatVectorOperations::add(monoWrite, rRead, nSamples);
            juce::FloatVectorOperations::multiply(monoWrite, 0.5f, nSamples);
        }
    }

    void monoToStereo(juce::AudioBuffer<float> &targetStereoBlock, juce::AudioBuffer<float> &sourceBlock)
    {
        if (targetStereoBlock.getNumChannels() == 1) 
        {
            targetStereoBlock.makeCopyOf(sourceBlock);
        } 
        else 
        {
            auto nSamples = sourceBlock.getNumSamples();

            auto lWrite = targetStereoBlock.getWritePointer(0);
            auto rWrite = targetStereoBlock.getWritePointer(1);
            auto monoRead = sourceBlock.getReadPointer(0);

            juce::FloatVectorOperations::copy(lWrite, monoRead, nSamples);
            juce::FloatVectorOperations::copy(rWrite, monoRead, nSamples);
        }
    }


private:
    const NeuralParameters& parameters; 
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NeuralProcessor)
};


struct NeuralControls final : public juce::Component
{
    explicit NeuralControls(juce::AudioProcessorEditor& editorIn, const NeuralParameters& parameters)
    : backendType(editorIn, parameters.neuralBackend),
      dryWet(editorIn, parameters.neuralDryWet)
      
    {
        addAllAndMakeVisible(*this, backendType, dryWet);
    }

    void resized()
    {
        performLayout(getLocalBounds(), backendType, dryWet);
    }
    AttachedCombo backendType; 
    AttachedSlider dryWet; 
};
