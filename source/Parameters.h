#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

using namespace juce;

using Parameter = AudioProcessorValueTreeState::Parameter;
using Attributes = AudioProcessorValueTreeStateParameterAttributes;


static String getPanningTextForValue (float value)
{
    if (approximatelyEqual (value, 0.5f))
        return "center";

    if (value < 0.5f)
        return String (roundToInt ((0.5f - value) * 200.0f)) + "%L";

    return String (roundToInt ((value - 0.5f) * 200.0f)) + "%R";
}

static float getPanningValueForText (String strText)
{
    if (strText.compareIgnoreCase ("center") == 0 || strText.compareIgnoreCase ("c") == 0)
        return 0.5f;

    strText = strText.trim();

    if (strText.indexOfIgnoreCase ("%L") != -1)
    {
        auto percentage = (float) strText.substring (0, strText.indexOf ("%")).getDoubleValue();
        return (100.0f - percentage) / 100.0f * 0.5f;
    }

    if (strText.indexOfIgnoreCase ("%R") != -1)
    {
        auto percentage = (float) strText.substring (0, strText.indexOf ("%")).getDoubleValue();
        return percentage / 100.0f * 0.5f + 0.5f;
    }

    return 0.5f;
}

template <typename Param>
static void add (AudioProcessorParameterGroup& group, std::unique_ptr<Param> param)
{
    group.addChild (std::move (param));
}

template <typename Param>
static void add (AudioProcessorValueTreeState::ParameterLayout& group, std::unique_ptr<Param> param)
{
    group.add (std::move (param));
}

template <typename Param, typename Group, typename... Ts>
static Param& addToLayout (Group& layout, Ts&&... ts)
{
    auto param = new Param (std::forward<Ts> (ts)...);
    auto& ref = *param;
    add (layout, rawToUniquePtr (param));
    return ref;
}

static String valueToTextFunction (float x, int) { return String (x, 2); }
static float textToValueFunction (const String& str) { return str.getFloatValue(); }

static auto getBasicAttributes()
{
    return Attributes().withStringFromValueFunction (valueToTextFunction)
                        .withValueFromStringFunction (textToValueFunction);
}

static auto getDbAttributes()           { return getBasicAttributes().withLabel ("dB"); }
static auto getMsAttributes()           { return getBasicAttributes().withLabel ("ms"); }
static auto getHzAttributes()           { return getBasicAttributes().withLabel ("Hz"); }
static auto getPercentageAttributes()   { return getBasicAttributes().withLabel ("%"); }
static auto getRatioAttributes()        { return getBasicAttributes().withLabel (":1"); }

static String valueToTextPanFunction (float x, int) { return getPanningTextForValue ((x + 100.0f) / 200.0f); }
static float textToValuePanFunction (const String& str) { return getPanningValueForText (str) * 200.0f - 100.0f; }


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
    PARAMETER_ID (convolutionCabEnabled)
    PARAMETER_ID (convolutionReverbEnabled)
    PARAMETER_ID (convolutionReverbMix)
    PARAMETER_ID (multiBandEnabled)
    PARAMETER_ID (multiBandFreq)
    PARAMETER_ID (multiBandLowVolume)
    PARAMETER_ID (multiBandHighVolume)
    PARAMETER_ID (compressorEnabled)
    PARAMETER_ID (compressorThreshold)
    PARAMETER_ID (compressorRatio)
    PARAMETER_ID (compressorAttack)
    PARAMETER_ID (compressorRelease)
    PARAMETER_ID (noiseGateEnabled)
    PARAMETER_ID (noiseGateThreshold)
    PARAMETER_ID (noiseGateRatio)
    PARAMETER_ID (noiseGateAttack)
    PARAMETER_ID (noiseGateRelease)
    PARAMETER_ID (limiterEnabled)
    PARAMETER_ID (limiterThreshold)
    PARAMETER_ID (limiterRelease)
    PARAMETER_ID (directDelayEnabled)
    PARAMETER_ID (directDelayType)
    PARAMETER_ID (directDelayValue)
    PARAMETER_ID (directDelaySmoothing)
    PARAMETER_ID (directDelayMix)
    PARAMETER_ID (delayEffectEnabled)
    PARAMETER_ID (delayEffectType)
    PARAMETER_ID (delayEffectValue)
    PARAMETER_ID (delayEffectSmoothing)
    PARAMETER_ID (delayEffectLowpass)
    PARAMETER_ID (delayEffectFeedback)
    PARAMETER_ID (delayEffectMix)
    PARAMETER_ID (phaserEnabled)
    PARAMETER_ID (phaserRate)
    PARAMETER_ID (phaserDepth)
    PARAMETER_ID (phaserCentreFrequency)
    PARAMETER_ID (phaserFeedback)
    PARAMETER_ID (phaserMix)
    PARAMETER_ID (chorusEnabled)
    PARAMETER_ID (chorusRate)
    PARAMETER_ID (chorusDepth)
    PARAMETER_ID (chorusCentreDelay)
    PARAMETER_ID (chorusFeedback)
    PARAMETER_ID (chorusMix)
    PARAMETER_ID (ladderEnabled)
    PARAMETER_ID (ladderCutoff)
    PARAMETER_ID (ladderResonance)
    PARAMETER_ID (ladderDrive)
    PARAMETER_ID (ladderMode)
    PARAMETER_ID (cmajorEnabled)


   #undef PARAMETER_ID
}
