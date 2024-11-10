#pragma once
#include <JuceHeader.h>
#include "../utils/Parameters.h"
#include "../utils/Misc.h"
#include "./Components.h"

enum class FilterType
{
    LowPass,
    BandPass,
    HighPass,
    Allpass,
    Notch,
    LowShelf,
    HighShelf,
    PeakFilter
};

inline static juce::dsp::IIR::Coefficients<SampleType> newBiquadCoeffsForParams (
    FilterType filterType,
    SampleType gain,
    SampleType cutoff,
    SampleType q,
    SampleType sampleRate)
{
    auto gainLinear = juce::Decibels::decibelsToGain (gain);
    juce::dsp::IIR::Coefficients<SampleType> newCoefficients;

    switch (filterType)
    {
        case FilterType::LowPass:
            newCoefficients = *juce::dsp::IIR::Coefficients<SampleType>::makeLowPass (sampleRate, cutoff, q);
            break;
        case FilterType::HighPass:
            newCoefficients = *juce::dsp::IIR::Coefficients<SampleType>::makeHighPass (sampleRate, cutoff, q);
            break;
        case FilterType::BandPass:
            newCoefficients = *juce::dsp::IIR::Coefficients<SampleType>::makeBandPass (sampleRate, cutoff, q);
            break;
        case FilterType::Allpass:
            newCoefficients = *juce::dsp::IIR::Coefficients<SampleType>::makeAllPass (sampleRate, cutoff, q);
            break;
        case FilterType::Notch:
            newCoefficients = *juce::dsp::IIR::Coefficients<SampleType>::makeNotch (sampleRate, cutoff, q);
            break;
        case FilterType::LowShelf:
            newCoefficients = *juce::dsp::IIR::Coefficients<SampleType>::makeLowShelf (sampleRate, cutoff, q, gainLinear);
            break;
        case FilterType::HighShelf:
            newCoefficients = *juce::dsp::IIR::Coefficients<SampleType>::makeHighShelf (sampleRate, cutoff, q, gainLinear);
            break;
        case FilterType::PeakFilter:
            newCoefficients = *juce::dsp::IIR::Coefficients<SampleType>::makePeakFilter (sampleRate, cutoff, q, gainLinear);
            break;
    }

    return newCoefficients;
}

class StereoIIRFilter : private juce::AudioProcessorParameter::Listener
    , public juce::ChangeBroadcaster
{
public:
    StereoIIRFilter() = default;

    ~StereoIIRFilter() override
    {
        if (parameters != nullptr)
        {
            parameters->cutoff.removeListener (this);
            parameters->Q.removeListener (this);
            parameters->gain.removeListener (this);
            parameters->type.removeListener (this);
        }
    }

    void connectToParameters (const FilterParameters& p)
    {
        p.cutoff.addListener (this);
        p.Q.addListener (this);
        p.gain.addListener (this);
        p.type.addListener (this);

        parameters = &p;
        parameterValueChanged (0, 0.0);
    }

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = (SampleType) spec.sampleRate;
        filter.prepare (spec);
    }

    void reset()
    {
        filter.reset();
    }

    void process (const juce::dsp::ProcessContextReplacing<SampleType>& context)
    {
        if (realTimeCoeffsRequireUpdate.load())
        {
            updateRealtimeCoeffs();
            realTimeCoeffsRequireUpdate.store (false);
        }

        filter.process (context);
    }

    SampleType getPhaseForFrequency (SampleType frequency) const noexcept
    {
        return (SampleType) nonRealtimeCoeffs.getPhaseForFrequency (frequency, sampleRate);
    }

    SampleType getMagnitudeForFrequency (SampleType frequency) const noexcept
    {
        return (SampleType) nonRealtimeCoeffs.getMagnitudeForFrequency (frequency, sampleRate);
    }

    const FilterParameters* parameters;

private:
    std::tuple<FilterType, SampleType, SampleType, SampleType> getNewParameterValues() const noexcept
    {
        return { static_cast<FilterType> (parameters->type.getIndex()),
                 static_cast<SampleType> (parameters->gain.get()),
                 static_cast<SampleType> (parameters->cutoff.get()),
                 static_cast<SampleType> (parameters->Q.get()) };
    }

    void updateNonRealTimeCoeffs()
    {
        auto [filterType, gain, cutoff, Q] = getNewParameterValues();
        nonRealtimeCoeffs = newBiquadCoeffsForParams (filterType, gain, cutoff, Q, sampleRate);
    }

    void updateRealtimeCoeffs()
    {
        auto [filterType, gain, cutoff, Q] = getNewParameterValues();
        *filter.state = newBiquadCoeffsForParams (filterType, gain, cutoff, Q, sampleRate);
    }

    void parameterValueChanged (int, float) override
    {
        realTimeCoeffsRequireUpdate.store (true);

        // const auto executeOrDeferToMessageThread = [] (auto&& fn) -> void
        // {
        //     if (juce::MessageManager::getInstance()->isThisTheMessageThread())
        //         return fn();

        //     juce::MessageManager::callAsync (std::forward<decltype (fn)> (fn));
        // };

        executeOnMessageThread ([this]
                                {
                                    updateNonRealTimeCoeffs();
                                    sendSynchronousChangeMessage();
                                });
    }

    void parameterGestureChanged (int, bool) override {}

    SampleType sampleRate = 44100.0;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<SampleType>, juce::dsp::IIR::Coefficients<SampleType>> filter;
    juce::dsp::IIR::Coefficients<SampleType> nonRealtimeCoeffs = *juce::dsp::IIR::Coefficients<SampleType>::makeLowPass (sampleRate, 100.0f);
    std::atomic<bool> realTimeCoeffsRequireUpdate;
};

class FilterHandle : public juce::Component
    , juce::AudioProcessorParameter::Listener
{
public:
    FilterHandle (juce::AudioProcessorEditor& editorIn, const FilterParameters& params)
        : Q (params.Q)
        , type (params.type)
        , editor (editorIn)
        , cutoffAttachment (params.cutoff, [&] (float newCutoff)
                            {
                                updateXFromNewCutoff (newCutoff);
                                repaint();
                            })
        , gainAttachment (params.gain, [&] (float newGain)
                          {
                              updateYFromNewGain (newGain);
                              repaint();
                          })
        , qAttachment (params.Q, [&] (float newQ)
                       {
                           updateYFromNewResonance (newQ);
                           repaint();
                       })
    {
        setSize (20, 20);
        type.addListener (this);

        updateYisQ();

        if (YisQ.load())
        {
            setCentreRelative (cutoffToRelativeX (params.cutoff.get()), QToNormalizedY (params.Q.get()));
        }
        else
        {
            setCentreRelative (cutoffToRelativeX (params.cutoff.get()), 1.0f - gainRange.convertTo0to1 (params.gain.get()));
        }
    }

    ~FilterHandle() override
    {
        type.removeListener (this);
    }

    float getCurrentRelativeX()
    {
        return (float) getBoundsInParent().getCentreX() / (float) getParentWidth();
    }

    float getCurrentRelativeY()
    {
        return (float) getBoundsInParent().getCentreY() / (float) getParentHeight();
    }

    void updateXFromNewCutoff (float newCutoff)
    {
        setCentreRelative (cutoffToRelativeX (newCutoff), getCurrentRelativeY());
    }

    void updateYFromNewResonance (float newQ)
    {
        if (YisQ.load())
            setCentreRelative (getCurrentRelativeX(), QToNormalizedY (newQ));
    }

    void updateYFromNewGain (float newGain)
    {
        setCentreRelative (getCurrentRelativeX(), 1.0f - gainRange.convertTo0to1 (newGain));
    }

    void paint (juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (2);
        g.setColour (juce::Colours::white);
        g.fillEllipse (bounds);
        g.setColour (juce::Colours::black);
        g.drawEllipse (bounds, 2);
    }

    float cutoffToRelativeX (float c)
    {
        return juce::mapFromLog10 (c, 20.0f, 20000.0f);
    }

    float normalizedYToNormalizedQ (float normY)
    {
        return 1.0f - normY;
    }

    float QToNormalizedY (float q)
    {
        return 1.0f - Q.convertTo0to1 (q);
    }

    bool isRightClick() const noexcept
    {
        return juce::ModifierKeys::currentModifiers.isRightButtonDown();
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (isRightClick())
        {
            struct FilterPropertyControls : public juce::Component
            {
            public:
                FilterPropertyControls (juce::AudioProcessorEditor& editorIn,
                                        juce::AudioParameterChoice& typeParam,
                                        Parameter& qParam)
                    : filterType (editorIn, typeParam)
                    , filterQ (editorIn, qParam)
                {
                    addAndMakeVisible (filterType);
                    if (static_cast<FilterType> (typeParam.getIndex()) == FilterType::PeakFilter)
                    {
                        isBell = true;
                        addAndMakeVisible (filterQ);
                        resized();
                    }
                }

                void resized() override
                {
                    if (isBell)
                    {
                        auto r = getLocalBounds();
                        filterType.setBounds (r.removeFromTop ((int) getHeight() / 2));
                        filterQ.setBounds (r);
                    }
                    filterType.setBounds (getLocalBounds());
                }

            private:
                bool isBell = false;
                AttachedCombo filterType;
                AttachedSlider filterQ;
            };

            auto filterControls = std::make_unique<FilterPropertyControls> (editor, type, Q);
            // auto r = getParentComponent()->getLocalBounds().toFloat();

            auto r = getParentComponent()->getLocalBounds().toFloat();
            auto filterControlsWidth = 0.4 * r.getWidth();
            auto filterControlsHeight = 0.4 * r.getHeight();
            filterControls->setSize ((int) filterControlsWidth, (int) filterControlsHeight);
            auto& calloutBox = juce::CallOutBox::launchAsynchronously (std::move (filterControls),
                                                                       getBoundsInParent(),
                                                                       getParentComponent());

            return;
        }

        dragger.startDraggingComponent (this, e);
        cutoffAttachment.beginGesture();

        if (YisQ.load())
            qAttachment.beginGesture();
        else
            gainAttachment.beginGesture();
    }

    float addToCurrentQNormalized (float normalizedAmtToAdd)
    {
        return Q.convertTo0to1 (Q.get()) + normalizedAmtToAdd;
    }

    void updateCutoffFromRelativeX (float relativeX)
    {
        auto correspondingCutoff = juce::mapToLog10 (relativeX, 20.0f, 20000.0f);
        cutoffAttachment.setValueAsPartOfGesture (correspondingCutoff);
    }

    void updateGainFromRelativeY (float relativeY)
    {
        auto correspondingGain = gainRange.convertFrom0to1 (1.0f - relativeY);
        gainAttachment.setValueAsPartOfGesture (correspondingGain);
    }

    void updateQFromRelativeY (float relativeY)
    {
        auto correspondingQ = Q.convertFrom0to1 (1.0f - relativeY);
        qAttachment.setValueAsPartOfGesture (correspondingQ);
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        dragger.dragComponent (this, e, nullptr);

        auto centre = getBoundsInParent().getCentre().toFloat();
        auto relativeX = centre.getX() / (float) getParentWidth();
        auto relativeY = centre.getY() / (float) getParentHeight();

        updateCutoffFromRelativeX (relativeX);

        if (YisQ.load())
            updateQFromRelativeY (relativeY);
        else
            updateGainFromRelativeY (relativeY);
    }

    void sendInitialUpdates()
    {
        cutoffAttachment.sendInitialUpdate();
        gainAttachment.sendInitialUpdate();
        qAttachment.sendInitialUpdate();
    }

    bool isCommandDown()
    {
        return juce::ModifierKeys::currentModifiers.isCommandDown();
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        cutoffAttachment.endGesture();
        if (YisQ.load())
            qAttachment.endGesture();
        else
            gainAttachment.endGesture();
    }

    void updateYisQ()
    {
        auto filterType = static_cast<FilterType> (type.getIndex());
        if (filterType == FilterType::LowPass || filterType == FilterType::HighPass || filterType == FilterType::BandPass || filterType == FilterType::Notch)
            YisQ.store (true);
        else
            YisQ.store (false);
    }

private:
    void parameterValueChanged (int, float) override
    {
        updateYisQ();
    }

    void parameterGestureChanged (int, bool) override
    {
    }

    std::atomic<bool> YisQ = false;

    juce::ComponentDragger dragger;

    Parameter& Q;
    juce::AudioParameterChoice& type;
    juce::AudioProcessorEditor& editor;

    juce::ParameterAttachment cutoffAttachment;
    juce::ParameterAttachment gainAttachment;
    juce::ParameterAttachment qAttachment;

    juce::NormalisableRange<float> gainRange = juce::NormalisableRange<float> (-10.0f, 10.0f, 0.0f);
    // FilterControls* listener = nullptr;
};

// struct FilterControls: public juce::Component
// {
//     FilterControls(juce::AudioProcessorEditor& editorIn, StereoIIRFilter& f)
//         : filterResponse(editorIn, f)
//     {
//         addAllAndMakeVisible(*this,
//                 filterResponse);
//     }

//     ~FilterControls() override
//     {
//         // FilterHandle.removeListener();
//     }

//     void resized() override
//     {
//         auto r = getLocalBounds();
//         // auto responseArea = r.removeFromTop((int) (0.6 * getHeight()));
//         filterResponse.setBounds(r);
//         // updateHandlePosition();
//     }

// private:
//     MagnitudeResponseComponent filterResponse;

// };