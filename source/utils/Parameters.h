#pragma once 
#include "./Misc.h"


template <typename Param>
void add (juce::AudioProcessorParameterGroup& group, std::unique_ptr<Param> param)
{
    group.addChild (std::move (param));
}

template <typename Param>
void add (juce::AudioProcessorValueTreeState::ParameterLayout& group, std::unique_ptr<Param> param)
{
    group.add (std::move (param));
}

template <typename Param, typename Group, typename... Ts>
Param& addToLayout (Group& layout, Ts&&... ts)
{
    auto param = new Param (std::forward<Ts> (ts)...);
    auto& ref = *param;
    add (layout, juce::rawToUniquePtr (param));
    return ref;
}


namespace ID
{
   #define PARAMETER_ID(str) constexpr const char* str { #str };

    PARAMETER_ID (inputGain)
    PARAMETER_ID (outputGain)
    PARAMETER_ID (pan)
    PARAMETER_ID (distortionEnabled)
    PARAMETER_ID (distortionType)
    PARAMETER_ID (distortionOversampler)
    PARAMETER_ID (distortionLowpass)
    PARAMETER_ID (distortionHighpass)
    PARAMETER_ID (distortionInGain)
    PARAMETER_ID (distortionCompGain)
    PARAMETER_ID (distortionMix)
    PARAMETER_ID (neuralBackend)
    PARAMETER_ID (neuralDryWet)
    PARAMETER_ID (compressorEnabled)
    PARAMETER_ID (compressorThreshold)
    PARAMETER_ID (compressorRatio)
    PARAMETER_ID (compressorAttack)
    PARAMETER_ID (compressorRelease)
    #undef PARAMETER_ID

}


using Parameter = juce::AudioProcessorValueTreeState::Parameter; 

struct CompressorParameters
{
    template <typename T>
    explicit CompressorParameters (T& layout)
        : enabled (addToLayout<juce::AudioParameterBool> (layout,
                                                    juce::ParameterID { ID::compressorEnabled, 1 },
                                                    "Comp.",
                                                    false)),
            threshold (addToLayout<Parameter> (layout,
                                                juce::ParameterID { ID::compressorThreshold, 1 },
                                                "Threshold",
                                                juce::NormalisableRange<float> (-100.0f, 0.0f),
                                                0.0f,
                                                getDbAttributes())),
            ratio (addToLayout<Parameter> (layout,
                                            juce::ParameterID { ID::compressorRatio, 1 },
                                            "Ratio",
                                            juce::NormalisableRange<float> (1.0f, 100.0f, 0.0f, 0.25f),
                                            1.0f,
                                            getRatioAttributes())),
            attack (addToLayout<Parameter> (layout,
                                            juce::ParameterID { ID::compressorAttack, 1 },
                                            "Attack",
                                            juce::NormalisableRange<float> (0.01f, 1000.0f, 0.0f, 0.25f),
                                            1.0f,
                                            getMsAttributes())),
            release (addToLayout<Parameter> (layout,
                                            juce::ParameterID { ID::compressorRelease, 1 },
                                            "Release",
                                            juce::NormalisableRange<float> (10.0f, 10000.0f, 0.0f, 0.25f),
                                            100.0f,
                                            getMsAttributes())) {}

    juce::AudioParameterBool& enabled;
    Parameter& threshold;
    Parameter& ratio;
    Parameter& attack;
    Parameter& release;
};



struct DistortionParameters
{
    template <typename T>
    explicit DistortionParameters (T& layout) 
        : enabled (addToLayout<juce::AudioParameterBool> (layout,
                                                    juce::ParameterID { ID::distortionEnabled, 1 },
                                                    "Distortion",
                                                    true)),
            type (addToLayout<juce::AudioParameterChoice> (layout,
                                                    juce::ParameterID { ID::distortionType, 1 },
                                                    "Waveshaper",
                                                    juce::StringArray { "std::tanh", "Approx. tanh" },
                                                    0)),
            inGain (addToLayout<Parameter> (layout,
                                            juce::ParameterID { ID::distortionInGain, 1 },
                                            "Gain",
                                            juce::NormalisableRange<float> (-40.0f, 40.0f),
                                            0.0f,
                                            getDbAttributes())),
            lowpass (addToLayout<Parameter> (layout,
                                            juce::ParameterID { ID::distortionLowpass, 1 },
                                            "Post Low-pass",
                                            juce::NormalisableRange<float> (20.0f, 22000.0f, 0.0f, 0.25f),
                                            22000.0f,
                                            getHzAttributes())),
            highpass (addToLayout<Parameter> (layout,
                                            juce::ParameterID { ID::distortionHighpass, 1 },
                                            "Pre High-pass",
                                            juce::NormalisableRange<float> (20.0f, 22000.0f, 0.0f, 0.25f),
                                            20.0f,
                                            getHzAttributes())),
            compGain (addToLayout<Parameter> (layout,
                                            juce::ParameterID { ID::distortionCompGain, 1 },
                                            "Compensat.",
                                            juce::NormalisableRange<float> (-40.0f, 40.0f),
                                            0.0f,
                                            getDbAttributes())),
            mix (addToLayout<Parameter> (layout,
                                        juce::ParameterID { ID::distortionMix, 1 },
                                        "Mix",
                                        juce::NormalisableRange<float> (0.0f, 100.0f),
                                        100.0f,
                                        getPercentageAttributes())),
            oversampler (addToLayout<juce::AudioParameterChoice> (layout,
                                                            juce::ParameterID { ID::distortionOversampler, 1 },
                                                            "Oversampling",
                                                            juce::StringArray { "2X",
                                                                        "4X",
                                                                        "8X",
                                                                        "2X compensated",
                                                                        "4X compensated",
                                                                        "8X compensated" },
                                                            1)) {}

    juce::AudioParameterBool& enabled;
    juce::AudioParameterChoice& type;
    Parameter& inGain;
    Parameter& lowpass;
    Parameter& highpass;
    Parameter& compGain;
    Parameter& mix;
    juce::AudioParameterChoice& oversampler;
};


struct NeuralParameters 
{
    inline static juce::StringArray backendTypes {"TFLITE", "LIBTORCH", "ONNXRUNTIME", "NONE"};
    inline static juce::String defaultBackend { backendTypes[2] };


    template <typename T>
    explicit NeuralParameters(T& layout)
        :   neuralBackend(addToLayout<juce::AudioParameterChoice> (layout, 
                                                                ID::neuralBackend,
                                                                "Backend type",
                                                                backendTypes,
                                                                backendTypes.indexOf(defaultBackend))),
            neuralDryWet(addToLayout<Parameter> (layout, 
                                            ID::neuralDryWet,
                                            "Dry / Wet",
                                            juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
                                            1.0f)) {}



    juce::AudioParameterChoice& neuralBackend; 
    Parameter& neuralDryWet; 

};




struct PostProcessorParameters
{
    template <typename T>
    explicit PostProcessorParameters(T& layout)
        : distortion(addToLayout<juce::AudioProcessorParameterGroup> (layout, "distortion",    "Distortion",    "|")),
          compressor(addToLayout<juce::AudioProcessorParameterGroup> (layout, "compressor",    "Compressor",    "|")){ }
    
    DistortionParameters distortion; 
    CompressorParameters compressor; 

}; 


struct Parameters

{
    explicit Parameters(juce::AudioProcessorValueTreeState::ParameterLayout& layout)
        : postProcessor(addToLayout<juce::AudioProcessorParameterGroup>(layout, "postProcessor",    "Post Processor",    "|")),
          neural(addToLayout<juce::AudioProcessorParameterGroup>(layout, "neural",    "Neural Processor",    "|")) {}
    PostProcessorParameters postProcessor; 
    NeuralParameters neural; 

}; 

// struct State
// {
//     explicit State(juce::AudioProcessorValueTreeState::ParameterLayout& layout, juce::AudioProcessor& p)
//     : parameterRefs { layout }, 
//       apvts (p, nullptr, "Plugin", std::move(layout)){ }

//     Parameters parameterRefs;
//     juce::AudioProcessorValueTreeState apvts; 
// }; 