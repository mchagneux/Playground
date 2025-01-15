#pragma once
#include <JuceHeader.h>
#include "../utils/Parameters.h"
#include "../utils/Misc.h"
#include "./Compressor.h"
#include "./EQ.h"

struct PostProcessor
{
public:
    PostProcessor (const PostProcessorParameters& p)
        : parameters (p)
        , eq (parameters.eq)
        , compressor (parameters.compressor)
    {
    }

    ~PostProcessor() {}

    //==============================================================================
    void prepare (juce::dsp::ProcessSpec& spec)
    {
        // Use this method as the place to do any pre-playback
        // initialisation that you need..
        // const auto channels = juce::jmax (getTotalNumInputChannels(), getTotalNumOutputChannels());

        if (spec.numChannels == 0)
            return;
        eq.prepare (spec);
        compressor.prepare (spec);
    }

    void reset()
    {
        eq.reset();
        compressor.reset();
    }

    void process (juce::dsp::ProcessContextReplacing<SampleType>& context)
    {
        eq.process (context);

        auto numChannels = context.getOutputBlock().getNumChannels();
        auto numSamples = context.getOutputBlock().getNumSamples();

        float* dataToReferTo[numChannels];

        for (unsigned int idx = 0; idx < numChannels; ++idx)
        {
            dataToReferTo[idx] = context.getOutputBlock().getChannelPointer (idx);
        }

        eq.postAnalyzer.addAudioData (juce::AudioBuffer<SampleType> (dataToReferTo, numChannels, numSamples), 0, numChannels);
        compressor.process (context);
    }

    const PostProcessorParameters& parameters;

    auto& getEQ()
    {
        return eq;
    }

    auto& getCompressor()
    {
        return compressor;
    }

private:
    EQ eq;
    Compressor compressor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PostProcessor)
};

struct PostProcessorControls final : public juce::Component
{
    explicit PostProcessorControls (juce::AudioProcessorEditor& editor, PostProcessor& pp)
        : eq (editor, pp.getEQ())
        , compressor (editor, pp.parameters.compressor)

    {
        addAllAndMakeVisible (*this, eq, compressor);
    }

    void resized() override
    {
        auto r = getLocalBounds();
        // distortion.setBounds(r.removeFromTop((int) (getHeight() / 2)));
        eq.setBounds (r.removeFromTop (getHeight() / 3));
        compressor.setBounds (r.removeFromTop ((int) (r.getHeight() / 3)));
    }

    EQControls eq;
    CompressorControls compressor;
};
