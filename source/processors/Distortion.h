#pragma once

#include <JuceHeader.h>
#include "../utils/Misc.h"
#include "../utils/Components.h"
#include "../utils/Parameters.h"

using Parameter = juce::AudioProcessorValueTreeState::Parameter;

struct DistortionProcessor
{
    DistortionProcessor()
    {
        forEach ([] (juce::dsp::Gain<float>& gain)
                 {
                     gain.setRampDurationSeconds (0.05);
                 },
                 distGain,
                 compGain);

        lowpass.setType (juce::dsp::FirstOrderTPTFilterType::lowpass);
        highpass.setType (juce::dsp::FirstOrderTPTFilterType::highpass);
        mixer.setMixingRule (juce::dsp::DryWetMixingRule::linear);
    }

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        for (auto& oversampler : oversamplers)
            oversampler.initProcessing (spec.maximumBlockSize);

        prepareAll (spec, lowpass, highpass, distGain, compGain, mixer);
    }

    void reset()
    {
        for (auto& oversampler : oversamplers)
            oversampler.reset();

        resetAll (lowpass, highpass, distGain, compGain, mixer);
    }

    float getLatency() const
    {
        return oversamplers[size_t (currentIndexOversampling)].getLatencyInSamples();
    }

    template <typename Context>
    void process (Context& context)
    {
        if (context.isBypassed)
            return;

        const auto& inputBlock = context.getInputBlock();

        mixer.setWetLatency (getLatency());
        mixer.pushDrySamples (inputBlock);

        distGain.process (context);
        highpass.process (context);

        auto ovBlock = oversamplers[size_t (currentIndexOversampling)].processSamplesUp (inputBlock);

        juce::dsp::ProcessContextReplacing<float> waveshaperContext (ovBlock);

        if (juce::isPositiveAndBelow (currentIndexWaveshaper, waveShapers.size()))
        {
            waveShapers[size_t (currentIndexWaveshaper)].process (waveshaperContext);

            if (currentIndexWaveshaper == 1)
                clipping.process (waveshaperContext);

            waveshaperContext.getOutputBlock() *= 0.7f;
        }

        auto& outputBlock = context.getOutputBlock();
        oversamplers[size_t (currentIndexOversampling)].processSamplesDown (outputBlock);

        lowpass.process (context);
        compGain.process (context);
        mixer.mixWetSamples (outputBlock);
    }

    std::array<juce::dsp::Oversampling<float>, 6> oversamplers { {
        { 2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false },
        { 2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false },
        { 2, 3, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false },

        { 2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true },
        { 2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true },
        { 2, 3, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true },
    } };

    static float clip (float in) { return juce::jlimit (-1.0f, 1.0f, in); }

    juce::dsp::FirstOrderTPTFilter<float> lowpass, highpass;
    juce::dsp::Gain<float> distGain, compGain;
    juce::dsp::DryWetMixer<float> mixer { 10 };
    std::array<juce::dsp::WaveShaper<float>, 2> waveShapers { { { std::tanh },
                                                                { juce::dsp::FastMathApproximations::tanh } } };
    juce::dsp::WaveShaper<float> clipping { clip };
    int currentIndexOversampling = 0;
    int currentIndexWaveshaper = 0;
};

struct DistortionControls final : public juce::Component
{
    explicit DistortionControls (juce::AudioProcessorEditor& editor,
                                 const DistortionParameters& state)
        : toggle (editor, state.enabled)
        , lowpass (editor, state.lowpass)
        , highpass (editor, state.highpass)
        , mix (editor, state.mix)
        , gain (editor, state.inGain)
        , compv (editor, state.compGain)
        , type (editor, state.type)
        , oversampling (editor, state.oversampler)
    {
        addAllAndMakeVisible (*this, toggle, type, lowpass, highpass, mix, gain, compv, oversampling);
    }

    void resized() override
    {
        performLayout (getLocalBounds(), toggle, type, gain, highpass, lowpass, compv, mix, oversampling);
    }

    AttachedToggle toggle;
    AttachedSlider lowpass, highpass, mix, gain, compv;
    AttachedCombo type, oversampling;
};
