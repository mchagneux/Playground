#pragma once

#include <JuceHeader.h>

#include <anira/anira.h>
#include "../neural_configs/RAVE.h"
#include "../utils/Parameters.h"
#include "../utils/Components.h"

//==============================================================================
class NeuralProcessor : private juce::AudioProcessorParameter::Listener

{
public:
    NeuralProcessor (const NeuralParameters& p)
        : parameters (p)
        , inferenceHandler (prePostProcessor, inferenceConfig)
        , dryWetMixer (32768) // 32768 samples of max latency compensation for the dry-wet mixer
    {
        parameters.neuralDryWet.addListener (this);
        // parameters.neuralBackend.addListener (this);
    }

    ~NeuralProcessor() override
    {
        parameters.neuralDryWet.removeListener (this);
        // parameters.neuralBackend.removeListener (this);
    }

    //==============================================================================
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        juce::dsp::ProcessSpec monoSpec { spec.sampleRate,
                                          static_cast<juce::uint32> (spec.maximumBlockSize),
                                          static_cast<juce::uint32> (1) };

        anira::HostAudioConfig monoConfig {
            1,
            (size_t) spec.maximumBlockSize,
            spec.sampleRate
        };

        dryWetMixer.prepare (monoSpec);

        monoBuffer.setSize (1, spec.maximumBlockSize);
        inferenceHandler.prepare (monoConfig);
        inferenceHandler.setInferenceBackend (anira::LIBTORCH);

        auto newLatency = inferenceHandler.getLatency();

        dryWetMixer.setWetLatency (newLatency);

        parameterValueChanged (parameters.neuralDryWet.getParameterIndex(), parameters.neuralDryWet.get());
        // parameterValueChanged (parameters.neuralBackend.getParameterIndex(), parameters.neuralBackend.getIndex());
    }

    void reset()
    {
    }

    void process (juce::dsp::ProcessContextReplacing<SampleType>& context)
    {
        auto numChannels = context.getInputBlock().getNumChannels();
        auto numSamples = context.getInputBlock().getNumSamples();

        float* dataToReferTo[numChannels];

        for (unsigned int idx = 0; idx < numChannels; ++idx)
        {
            dataToReferTo[idx] = context.getOutputBlock().getChannelPointer (idx);
        }

        auto buffer = juce::AudioBuffer<SampleType> (dataToReferTo, numChannels, numSamples);

        stereoToMono (monoBuffer, buffer);
        dryWetMixer.pushDrySamples (monoBuffer);

        auto inferenceBuffer = const_cast<float**> (monoBuffer.getArrayOfWritePointers());
        inferenceHandler.process (inferenceBuffer, (size_t) buffer.getNumSamples());

        dryWetMixer.mixWetSamples (monoBuffer);
        monoToStereo (buffer, monoBuffer);
    }

    anira::InferenceManager& getInferenceManager() { return inferenceHandler.getInferenceManager(); }

private:
    void parameterGestureChanged (int parameterIndex, bool gestureIsStarting) override {}

    void parameterValueChanged (int parameterIndex, float newValue) override
    {
        if (parameterIndex == parameters.neuralDryWet.getParameterIndex())
        {
            dryWetMixer.setWetMixProportion (newValue);
        }
    }

    void stereoToMono (juce::AudioBuffer<float>& targetMonoBlock, juce::AudioBuffer<float>& sourceBlock)
    {
        if (sourceBlock.getNumChannels() == 1)
        {
            targetMonoBlock.makeCopyOf (sourceBlock);
        }
        else
        {
            auto nSamples = sourceBlock.getNumSamples();

            auto monoWrite = targetMonoBlock.getWritePointer (0);
            auto lRead = sourceBlock.getReadPointer (0);
            auto rRead = sourceBlock.getReadPointer (1);

            juce::FloatVectorOperations::copy (monoWrite, lRead, nSamples);
            juce::FloatVectorOperations::add (monoWrite, rRead, nSamples);
            juce::FloatVectorOperations::multiply (monoWrite, 0.5f, nSamples);
        }
    }

    void monoToStereo (juce::AudioBuffer<float>& targetStereoBlock, juce::AudioBuffer<float>& sourceBlock)
    {
        if (targetStereoBlock.getNumChannels() == 1)
        {
            targetStereoBlock.makeCopyOf (sourceBlock);
        }
        else
        {
            auto nSamples = sourceBlock.getNumSamples();

            auto lWrite = targetStereoBlock.getWritePointer (0);
            auto rWrite = targetStereoBlock.getWritePointer (1);
            auto monoRead = sourceBlock.getReadPointer (0);

            juce::FloatVectorOperations::copy (lWrite, monoRead, nSamples);
            juce::FloatVectorOperations::copy (rWrite, monoRead, nSamples);
        }
    }

private:
    const NeuralParameters& parameters;
    juce::AudioBuffer<float> monoBuffer;

    anira::InferenceConfig inferenceConfig = RAVEConfig;
    anira::PrePostProcessor prePostProcessor;
    anira::InferenceHandler inferenceHandler;
    juce::dsp::DryWetMixer<float> dryWetMixer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NeuralProcessor)
};

struct NeuralControls final : public juce::Component
{
    explicit NeuralControls (juce::AudioProcessorEditor& editorIn, const NeuralParameters& parameters)
        : dryWet (editorIn, parameters.neuralDryWet)

    {
        addAllAndMakeVisible (*this, dryWet);
    }

    void resized()
    {
        performLayout (getLocalBounds(), dryWet);
    }

    // AttachedCombo backendType;
    AttachedSlider dryWet;
};
