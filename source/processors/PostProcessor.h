#pragma once 
#include <JuceHeader.h>
#include "./Distortion.h"
#include "./Compressor.h"

#include "../utils/Parameters.h"

struct PostProcessor : public juce::AudioProcessor
{

public: 

    PostProcessor (const PostProcessorParameters& p, BusesProperties b)
        : juce::AudioProcessor (b), 
          parameters(p){ }
    
    ~PostProcessor() override {}


    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) final
    {
        const auto channels = juce::jmax (getTotalNumInputChannels(), getTotalNumOutputChannels());

        if (channels == 0)
            return;

        chain.prepare ({ sampleRate, (juce::uint32) samplesPerBlock, (juce::uint32) channels });

        reset();
    }
    
    void reset() final
    {
        chain.reset();
        update();
    }

    void releaseResources() final {}

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) final
    {
        if (juce::jmax (getTotalNumInputChannels(), getTotalNumOutputChannels()) == 0)
            return;

        juce::ScopedNoDenormals noDenormals;

        if (requiresUpdate.load())
            update();
 
        const auto totalNumInputChannels  = getTotalNumInputChannels();
        const auto totalNumOutputChannels = getTotalNumOutputChannels();

        setLatencySamples (juce::dsp::isBypassed<distortionIndex> (chain) ? 0 : juce::roundToInt (juce::dsp::get<distortionIndex> (chain).getLatency()));

        const auto numChannels = juce::jmax (totalNumInputChannels, totalNumOutputChannels);

        auto inoutBlock = juce::dsp::AudioBlock<float> (buffer).getSubsetChannelBlock (0, (size_t) numChannels);
        chain.process (juce::dsp::ProcessContextReplacing<float> (inoutBlock));
    }

    void processBlock (juce::AudioBuffer<double>&, juce::MidiBuffer&) final {}

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }

    //==============================================================================
    const juce::String getName() const final { return "PostProcessor"; }

    bool acceptsMidi()  const final { return false; }
    bool producesMidi() const final { return false; }
    bool isMidiEffect() const final { return false; }

    double getTailLengthSeconds() const final { return 0.0; }

    //==============================================================================
    int getNumPrograms()    final { return 1; }
    int getCurrentProgram() final { return 0; }
    void setCurrentProgram (int) final {}
    const juce::String getProgramName (int) final { return "None"; }

    void changeProgramName (int, const juce::String&) final {}

    //==============================================================================
    bool isBusesLayoutSupported (const BusesLayout& layout) const final
    {
        return layout == BusesLayout { { juce::AudioChannelSet::stereo() },
                                       { juce::AudioChannelSet::stereo() } };
    }

    void getStateInformation (juce::MemoryBlock& destData) final
    {
        // copyXmlToBinary (*state.copyState().createXml(), destData);
    }

    void setStateInformation (const void* data, int sizeInBytes) final
    {
        // state.replaceState (juce::ValueTree::fromXml (*getXmlFromBinary (data, sizeInBytes)));
    }

private:

    void update()
    {
        {
            DistortionProcessor& distortion = juce::dsp::get<distortionIndex> (chain);

            if (distortion.currentIndexOversampling != parameters.distortion.oversampler.getIndex())
            {
                distortion.currentIndexOversampling = parameters.distortion.oversampler.getIndex();
                prepareToPlay (getSampleRate(), getBlockSize());
                return;
            }

            distortion.currentIndexWaveshaper = parameters.distortion.type.getIndex();
            distortion.lowpass .setCutoffFrequency (parameters.distortion.lowpass.get());
            distortion.highpass.setCutoffFrequency (parameters.distortion.highpass.get());
            distortion.distGain.setGainDecibels (parameters.distortion.inGain.get());
            distortion.compGain.setGainDecibels (parameters.distortion.compGain.get());
            distortion.mixer.setWetMixProportion (parameters.distortion.mix.get() / 100.0f);
            juce::dsp::setBypassed<distortionIndex> (chain, ! parameters.distortion.enabled);
        }

        {
            juce::dsp::Compressor<float>& compressor = juce::dsp::get<compressorIndex> (chain);
            compressor.setThreshold (parameters.compressor.threshold.get());
            compressor.setRatio     (parameters.compressor.ratio.get());
            compressor.setAttack    (parameters.compressor.attack.get());
            compressor.setRelease   (parameters.compressor.release.get());
            juce::dsp::setBypassed<compressorIndex> (chain, ! parameters.compressor.enabled);
        }


        requiresUpdate.store (false);
    }


    using Chain = juce::dsp::ProcessorChain<DistortionProcessor, CompressorProcessor>;
    Chain chain;

    enum ProcessorIndices
    {
        distortionIndex,
        compressorIndex
    };
    std::atomic<bool> requiresUpdate { true };

    const PostProcessorParameters& parameters; 

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PostProcessor)
};


struct PostProcessorControls final : public juce::Component
{

    explicit PostProcessorControls (juce::AudioProcessorEditor& editor,
                                    const PostProcessorParameters& params)
        : distortion(editor, params.distortion),
          compressor(editor, params.compressor)
          
    {
        addAllAndMakeVisible (*this, distortion, compressor);
    }

    void resized() override
    {
        auto r = getLocalBounds(); 
        distortion.setBounds(r.removeFromTop((int) (getHeight() / 2))); 
        compressor.setBounds(r); 
    }

    DistortionControls distortion; 
    CompressorControls compressor; 
};
