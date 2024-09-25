#pragma once
#include <JuceHeader.h>


template <typename Func, typename... Items>
constexpr void forEach (Func&& func, Items&&... items)
{
    (func (std::forward<Items> (items)), ...);
}

template <typename... Processors>
void prepareAll (const juce::dsp::ProcessSpec& spec, Processors&... processors)
{
    forEach ([&] (auto& proc) { proc.prepare (spec); }, processors...);
}

template <typename... Processors>
void resetAll (Processors&... processors)
{
    forEach ([] (auto& proc) { proc.reset(); }, processors...);
}

// using Parameter = juce::AudioProcessorValueTreeState::Parameter;
using Attributes = juce::AudioProcessorValueTreeStateParameterAttributes;


static inline juce::String getPanningTextForValue (float value)
{
    if (juce::approximatelyEqual (value, 0.5f))
        return "center";

    if (value < 0.5f)
        return juce::String (juce::roundToInt ((0.5f - value) * 200.0f)) + "%L";

    return juce::String (juce::roundToInt ((value - 0.5f) * 200.0f)) + "%R";
}

static inline float getPanningValueForText (juce::String strText)
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


static inline juce::String valueToTextFunction (float x, int) { return juce::String (x, 2); }
static inline float textToValueFunction (const juce::String& str) { return str.getFloatValue(); }

static inline auto getBasicAttributes()
{
    return Attributes().withStringFromValueFunction (valueToTextFunction)
                        .withValueFromStringFunction (textToValueFunction);
}

static inline auto getDbAttributes()           { return getBasicAttributes().withLabel ("dB"); }
static inline auto getMsAttributes()           { return getBasicAttributes().withLabel ("ms"); }
static inline auto getHzAttributes()           { return getBasicAttributes().withLabel ("Hz"); }
static inline auto getPercentageAttributes()   { return getBasicAttributes().withLabel ("%"); }
static inline auto getRatioAttributes()        { return getBasicAttributes().withLabel (":1"); }

static inline juce::String valueToTextPanFunction (float x, int) { return getPanningTextForValue ((x + 100.0f) / 200.0f); }
static inline float textToValuePanFunction (const juce::String& str) { return getPanningValueForText (str) * 200.0f - 100.0f; }


