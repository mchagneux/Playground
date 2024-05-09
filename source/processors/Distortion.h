#pragma once

#include "../Utils.h"
#include "../Parameters.h"
#include <juce_dsp/juce_dsp.h>

using namespace juce;



struct DistortionProcessor
{
    struct Parameters
    {
        explicit Parameters (AudioProcessorParameterGroup& layout)
            : enabled (addToLayout<AudioParameterBool> (layout,
                                                        ParameterID { ID::distortionEnabled, 1 },
                                                        "Distortion",
                                                        true)),
                type (addToLayout<AudioParameterChoice> (layout,
                                                        ParameterID { ID::distortionType, 1 },
                                                        "Waveshaper",
                                                        StringArray { "std::tanh", "Approx. tanh" },
                                                        0)),
                inGain (addToLayout<Parameter> (layout,
                                                ParameterID { ID::distortionInGain, 1 },
                                                "Gain",
                                                NormalisableRange<float> (-40.0f, 40.0f),
                                                0.0f,
                                                getDbAttributes())),
                lowpass (addToLayout<Parameter> (layout,
                                                ParameterID { ID::distortionLowpass, 1 },
                                                "Post Low-pass",
                                                NormalisableRange<float> (20.0f, 22000.0f, 0.0f, 0.25f),
                                                22000.0f,
                                                getHzAttributes())),
                highpass (addToLayout<Parameter> (layout,
                                                ParameterID { ID::distortionHighpass, 1 },
                                                "Pre High-pass",
                                                NormalisableRange<float> (20.0f, 22000.0f, 0.0f, 0.25f),
                                                20.0f,
                                                getHzAttributes())),
                compGain (addToLayout<Parameter> (layout,
                                                ParameterID { ID::distortionCompGain, 1 },
                                                "Compensat.",
                                                NormalisableRange<float> (-40.0f, 40.0f),
                                                0.0f,
                                                getDbAttributes())),
                mix (addToLayout<Parameter> (layout,
                                            ParameterID { ID::distortionMix, 1 },
                                            "Mix",
                                            NormalisableRange<float> (0.0f, 100.0f),
                                            100.0f,
                                            getPercentageAttributes())),
                oversampler (addToLayout<AudioParameterChoice> (layout,
                                                                ParameterID { ID::distortionOversampler, 1 },
                                                                "Oversampling",
                                                                StringArray { "2X",
                                                                            "4X",
                                                                            "8X",
                                                                            "2X compensated",
                                                                            "4X compensated",
                                                                            "8X compensated" },
                                                                1)) {}

        AudioParameterBool& enabled;
        AudioParameterChoice& type;
        Parameter& inGain;
        Parameter& lowpass;
        Parameter& highpass;
        Parameter& compGain;
        Parameter& mix;
        AudioParameterChoice& oversampler;
    };

    DistortionProcessor()
    {
        forEach ([] (dsp::Gain<float>& gain) { gain.setRampDurationSeconds (0.05); },
                    distGain,
                    compGain);

        lowpass.setType  (dsp::FirstOrderTPTFilterType::lowpass);
        highpass.setType (dsp::FirstOrderTPTFilterType::highpass);
        mixer.setMixingRule (dsp::DryWetMixingRule::linear);
    }

    void prepare (const dsp::ProcessSpec& spec)
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

        dsp::ProcessContextReplacing<float> waveshaperContext (ovBlock);

        if (isPositiveAndBelow (currentIndexWaveshaper, waveShapers.size()))
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

    std::array<dsp::Oversampling<float>, 6> oversamplers
    { {
        { 2, 1, dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false },
        { 2, 2, dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false },
        { 2, 3, dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false },

        { 2, 1, dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true },
        { 2, 2, dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true },
        { 2, 3, dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true },
    } };

    static float clip (float in) { return juce::jlimit (-1.0f, 1.0f, in); }

    dsp::FirstOrderTPTFilter<float> lowpass, highpass;
    dsp::Gain<float> distGain, compGain;
    dsp::DryWetMixer<float> mixer { 10 };
    std::array<dsp::WaveShaper<float>, 2> waveShapers { { { std::tanh },
                                                            { dsp::FastMathApproximations::tanh } } };
    dsp::WaveShaper<float> clipping { clip };
    int currentIndexOversampling = 0;
    int currentIndexWaveshaper   = 0;
};