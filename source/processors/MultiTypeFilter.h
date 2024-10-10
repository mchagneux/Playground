#pragma once
#include <JuceHeader.h>
#include "../utils/Parameters.h"
#include "../utils/Misc.h"
#include "../utils/FrequencyAnalyzer.h"
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

inline static juce::dsp::IIR::Coefficients<SampleType> newBiquadCoeffsForParams(
                                                                    FilterType filterType, 
                                                                    SampleType gain, 
                                                                    SampleType cutoff, 
                                                                    SampleType q, 
                                                                    SampleType sampleRate)
{
    auto gainLinear = juce::Decibels::decibelsToGain(gain);
    juce::dsp::IIR::Coefficients<SampleType> newCoefficients;

    switch (filterType)
    {
        case FilterType::LowPass:
            newCoefficients = *juce::dsp::IIR::Coefficients<SampleType>::makeLowPass  (sampleRate, cutoff, q); 
            break;
        case FilterType::HighPass:
            newCoefficients = *juce::dsp::IIR::Coefficients<SampleType>::makeHighPass  (sampleRate, cutoff, q); 
            break;
        case FilterType::BandPass:
            newCoefficients = *juce::dsp::IIR::Coefficients<SampleType>::makeBandPass  (sampleRate, cutoff, q); 
            break;
        case FilterType::Allpass:
            newCoefficients = *juce::dsp::IIR::Coefficients<SampleType>::makeAllPass  (sampleRate, cutoff, q); 
            break;
        case FilterType::Notch:
            newCoefficients = *juce::dsp::IIR::Coefficients<SampleType>::makeNotch  (sampleRate, cutoff, q); 
            break;
        case FilterType::LowShelf:
            newCoefficients = *juce::dsp::IIR::Coefficients<SampleType>::makeLowShelf (sampleRate, cutoff, q, gainLinear); 
            break;
        case FilterType::HighShelf:
            newCoefficients = *juce::dsp::IIR::Coefficients<SampleType>::makeHighShelf  (sampleRate, cutoff, q, gainLinear); 
            break;
        case FilterType::PeakFilter:
            newCoefficients = *juce::dsp::IIR::Coefficients<SampleType>::makePeakFilter  (sampleRate, cutoff, q, gainLinear); 
            break;
    }

    return newCoefficients;
}


class StereoIIRFilter : private juce::AudioProcessorParameter::Listener, public juce::ChangeBroadcaster
{

public:


    StereoIIRFilter(const FilterParameters& p) : parameters(p) 
    {
        parameters.cutoff.addListener(this);
        parameters.Q.addListener(this);
        parameters.gain.addListener(this);
        parameters.type.addListener(this);
        parameterValueChanged(0, 0.0);

    }

    ~StereoIIRFilter() override
    {
        parameters.cutoff.removeListener(this);
        parameters.Q.removeListener(this);
        parameters.gain.removeListener(this);
        parameters.type.removeListener(this);
        postAnalyzer.stopThread(1000);

    }


    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = (SampleType) spec.sampleRate; 
        postAnalyzer.setupAnalyser (int (sampleRate), float (sampleRate));
        filter.prepare(spec);
    }

    void reset()
    {
        filter.reset();
    }

    void process (const juce::dsp::ProcessContextReplacing<SampleType>& context)
    {
        if(realTimeCoeffsRequireUpdate.load()){
            updateRealtimeCoeffs();
            realTimeCoeffsRequireUpdate.store(false);
        }

        
        filter.process (context);

        // auto buffer = juce::AudioBuffer<float>(2, ) 
        // postAnalyzer.addAudioData(context.getOutputBlock(), 0, (int) context.getOutputBlock().getNumChannels());
    }

    SampleType getPhaseForFrequency(SampleType frequency) const noexcept
    {
        return (SampleType) nonRealtimeCoeffs.getPhaseForFrequency(frequency,  sampleRate);
    }

    SampleType getMagnitudeForFrequency(SampleType frequency) const noexcept
    {
        return (SampleType) nonRealtimeCoeffs.getMagnitudeForFrequency(frequency,  sampleRate);
    }



    const FilterParameters & parameters; 
    Analyzer<SampleType> postAnalyzer; 


private:

    void updateNonRealTimeCoeffs()
    {
        nonRealtimeCoeffs = newBiquadCoeffsForParams(filterType, gain, cutoff, q, sampleRate);
    }

    void updateRealtimeCoeffs()
    {
        *filter.state = newBiquadCoeffsForParams(filterType, gain, cutoff, q, sampleRate);
    }

    void parameterValueChanged(int, float) override
    {
        filterType = static_cast<FilterType>(parameters.type.getIndex()); 
        gain = static_cast<SampleType>(parameters.gain.get()); 
        cutoff = static_cast<SampleType>(parameters.cutoff.get()); 
        q = static_cast<SampleType>(parameters.Q.get());
        realTimeCoeffsRequireUpdate.store(true);

        const auto executeOrDeferToMessageThread = [] (auto&& fn) -> void
        {
            if (juce::MessageManager::getInstance()->isThisTheMessageThread())
                return fn();

            juce::MessageManager::callAsync (std::forward<decltype (fn)> (fn));
        };

        executeOrDeferToMessageThread([this] { updateNonRealTimeCoeffs(); sendSynchronousChangeMessage();});

    }

    void parameterGestureChanged (int, bool) override {}


    FilterType filterType = FilterType::LowPass;
    SampleType gain = 1.0f;
    SampleType cutoff = 1000.0f;
    SampleType q = 0.707f;
    SampleType sampleRate = 44100.0;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<SampleType>, juce::dsp::IIR::Coefficients<SampleType>> filter;
    juce::dsp::IIR::Coefficients<SampleType> nonRealtimeCoeffs = *juce::dsp::IIR::Coefficients<SampleType>::makeLowPass(sampleRate, 100.0f);
    std::atomic<bool> realTimeCoeffsRequireUpdate; 

};

class CutoffResonanceHandle : public juce::Component, private juce::AudioProcessorParameter::Listener
{
public:
    CutoffResonanceHandle(const FilterParameters& params) : cutoff(params.cutoff), Q(params.Q), gain(params.gain), type(params.type)
    {
        setSize(20, 20);
        handleCutoff = cutoff.get(); 
        handleQ = Q.get();
        handleGain = gain.get(); 
        updateYisQ();

        cutoff.addListener(this);
        Q.addListener(this);
        gain.addListener(this);
        type.addListener(this);
    }

    ~CutoffResonanceHandle() override 
    {
        cutoff.removeListener(this);
        Q.removeListener(this);
        gain.removeListener(this); 
        type.removeListener(this);

    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat().reduced(2);
        g.setColour(juce::Colours::white);
        g.fillEllipse(bounds);
        g.setColour(juce::Colours::black);
        g.drawEllipse(bounds, 2);
    }

    float cutoffToRelativeX(float c)
    {
        return juce::mapFromLog10(c, 20.0f, 20000.0f); 
    }

    float XToCutoff(float x)
    {
        return juce::mapToLog10(x / (float) getParentWidth(), 20.0f, 20000.0f); 
    }

    float normalizedYToNormalizedQ(float normY)
    {
        return 1.0f - normY; 
    }

    float QToNormalizedY(float q)
    {
        return 1.0f - Q.convertTo0to1(q); 
    }

    float normYToGain(float normY)
    {
        auto properlyOrderedNormY =  1.0f - normY; //TODO 
        
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        dragger.startDraggingComponent(this, e);
        cutoff.beginChangeGesture();

        if (YisQ)
            Q.beginChangeGesture();
        else gain.beginChangeGesture();
    }

    float addToCurrentQNormalized(float normalizedAmtToAdd)
    {
        return Q.convertTo0to1(Q.get()) + normalizedAmtToAdd; 
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {

        if (YisQ)
        {
            dragger.dragComponent(this, e, nullptr);
            auto x = getBoundsInParent().getCentre().toFloat().getX();
            auto normalizedY = getBoundsInParent().getCentre().toFloat().getY() / (float) getParentHeight(); 

            auto correspondingCutoff = XToCutoff(x);
            handleCutoff = correspondingCutoff; 
            updateCutoff(correspondingCutoff);


            auto correspondingQ = Q.convertFrom0to1(1.0f - normalizedY);
            handleQ = correspondingQ; 
            updateQ(correspondingQ);
        }
        else
        {
            if (isCommandDown())
            {
                // auto centreY = getBoundsInParent().getCentre().toFloat().getY() / (float) getParentHeight(); 
                auto normalizeddragY = (float) e.getDistanceFromDragStartY() / (float) getParentHeight() ; 
                auto correspondingQ = addToCurrentQNormalized(-normalizeddragY);
                handleQ = correspondingQ; 
                updateQ(correspondingQ);

            }
            else
            {
                dragger.dragComponent(this, e, nullptr);
                auto x = getBoundsInParent().getCentre().toFloat().getX();
                auto normalizedY = getBoundsInParent().getCentre().toFloat().getY() / (float) getParentHeight(); 

                auto correspondingCutoff = XToCutoff(x);
                handleCutoff = correspondingCutoff; 
                updateCutoff(correspondingCutoff);

                auto correspondingGain = gain.convertFrom0to1(1.0f - normalizedY); 
                handleGain = correspondingGain; 
                updateGain(correspondingGain);   
            }
        }

    }

    bool isCommandDown()
    {
        return juce::ModifierKeys::currentModifiers.isCommandDown();
    }

    void mouseUp(const juce::MouseEvent&) override 
    {
        cutoff.endChangeGesture();
        if (YisQ)
            Q.endChangeGesture();
        else gain.endChangeGesture();
    }

    void updateQ(float newQ)
    {
        Q.setValueNotifyingHost(Q.convertTo0to1(newQ));
    }

    void updateGain(float newGain)
    {
        gain.setValueNotifyingHost(gain.convertTo0to1(newGain));
    }

    void updateCutoff(float newCutoff)
    {
        cutoff.setValueNotifyingHost(cutoff.convertTo0to1(newCutoff));
    }

    void updateHandlePosition(float newCutoff, float newQ, float newGain)
    {
        auto x = cutoffToRelativeX(newCutoff); 
        float y; 
        if (YisQ)  y = QToNormalizedY(newQ);
        
        else y = 1.0f - gain.convertTo0to1(newGain);
        
        setCentreRelative(x, y);
        handleCutoff = newCutoff; 
        handleQ = newQ;
        handleGain = newGain;
    }

    void updateYisQ()
    {
        auto filterType = static_cast<FilterType> (type.getIndex()); 
        if(filterType == FilterType::LowPass || filterType == FilterType::HighPass || filterType == FilterType::BandPass || filterType == FilterType::Notch) 
            YisQ = true; 
        else YisQ = false; 
    }

private:
    void parameterValueChanged(int parameterIndex, float) override
    {
        if (parameterIndex == type.getParameterIndex()) updateYisQ();
        else
        {
            auto newCutoff = cutoff.get(); 
            auto newQ = Q.get(); 
            auto newGain = gain.get();

            if (newCutoff != handleCutoff || newQ != handleQ || newGain != handleGain)
                updateHandlePosition(newCutoff, newQ, newGain);
        }
    }
    void parameterGestureChanged(int, bool) override 
    {

    }

    bool YisQ = false;  
    juce::ComponentDragger dragger;
    float handleCutoff = 0.0f; 
    float handleQ = 0.0f;
    float handleGain = 0.0f;     
    Parameter& cutoff; 
    Parameter& Q; 
    Parameter& gain; 
    juce::AudioParameterChoice& type; 

    // FilterControls* listener = nullptr;
};

class MagnitudeResponseComponent : public juce::Component, private juce::ChangeListener, private juce::Timer
{
public:
    MagnitudeResponseComponent(StereoIIRFilter& f) : filter(f), cutoffResonanceHandle(f.parameters)
    {
        startTimer(20);
        filter.addChangeListener(this);
        addAndMakeVisible(cutoffResonanceHandle);
        // cutoffResonanceHandle.updateHandlePosition();
        // repaint();
    }
    
    ~MagnitudeResponseComponent() override 
    {
        stopTimer();
        filter.removeChangeListener(this);
    }

    void paint(juce::Graphics& g) override
    {

        g.fillAll(juce::Colours::black);

        const auto bounds = getLocalBounds().toFloat();
        const float width = bounds.getWidth();
        const float height = bounds.getHeight();


        
        if (!plotPhase){
            // Draw magnitude response
            g.setColour(juce::Colours::white);
            juce::Path magnitudePath;
            bool pathStarted = false;

            for (float x = 0; x < width; ++x)
            {
                float freq = juce::mapToLog10(x / width, 20.0f, 20000.0f);
                float magnitude = 0; 
                auto pixelsPerDouble = 2.0f * height / juce::Decibels::decibelsToGain (10.0f);

                magnitude = filter.getMagnitudeForFrequency(freq);
                float y = magnitude > 0 ? (float) (bounds.getCentreY() - pixelsPerDouble * std::log (magnitude) / std::log (2.0)) : bounds.getBottom();
                // float y = juce::jmap(magnitude, -40.0f, 40.0f, height, 0.0f);

                if (!pathStarted)
                {
                    magnitudePath.startNewSubPath(x, y);
                    pathStarted = true;
                }
                else
                {
                    magnitudePath.lineTo(x, y);
                }
            }

            g.strokePath(magnitudePath, juce::PathStrokeType(2.0f));
        }

        else {
            // Draw phase response
            g.setColour(juce::Colours::yellow);
            juce::Path phasePath;
            bool pathStarted = false;

            for (float x = 0; x < width; ++x)
            {
                float freq = juce::mapToLog10(x / width, 20.0f, 20000.0f);
                float phase = filter.getPhaseForFrequency(freq);
                float y = juce::jmap(phase, -juce::MathConstants<float>::pi, juce::MathConstants<float>::pi, height, 0.0f);

                if (!pathStarted)
                {
                    phasePath.startNewSubPath(x, y);
                    pathStarted = true;
                }
                else
                {
                    phasePath.lineTo(x, y);
                }
            }

            g.strokePath(phasePath, juce::PathStrokeType(2.0f));
        }
    

        auto plotFrame = getLocalBounds().toFloat();


        filter.postAnalyzer.createPath(analyzerPath, plotFrame, 20.0f);
        g.setColour (juce::Colours::grey);
        // g.drawFittedText ("Output", plotFrame.reduced (8, 28), juce::Justification::topRight, 1);



        g.strokePath (analyzerPath, juce::PathStrokeType (1.0));
    
    }


    void resized() override
    {
        repaint();
    }



private:


    void changeListenerCallback(juce::ChangeBroadcaster * ) override
    {
        repaint();
    }

    void timerCallback() override
    {
        if (filter.postAnalyzer.checkForNewData()) 
            repaint(); 
    }

    StereoIIRFilter& filter;
    juce::Path analyzerPath; 
    bool plotPhase = false; 
    CutoffResonanceHandle cutoffResonanceHandle; 

};

class FilterResponseComponent : public juce::Component 
{
public: 
    FilterResponseComponent(StereoIIRFilter& f): magnitudeResponseComponent(f)// , phaseResponseComponent(f) 
    {
        addAndMakeVisible(magnitudeResponseComponent);
        // addAndMakeVisible(phaseResponseComponent);
    }

    void resized() override 
    {
        auto bounds = getLocalBounds();
        magnitudeResponseComponent.setBounds(bounds);
        // phaseResponseComponent.setBounds(bounds);
        repaint();
    }

private: 
    MagnitudeResponseComponent magnitudeResponseComponent; 
    // PhaseResponseComponent phaseResponseComponent; 

    
}; 

struct FilterControls: public juce::Component
{
    FilterControls(juce::AudioProcessorEditor& editorIn, StereoIIRFilter& f) 
        : filterResponse(f),
          filterTypeSelector(editorIn, f.parameters.type),
          filterGain(editorIn, f.parameters.gain),
          filterCutoff(editorIn, f.parameters.cutoff),
          filterQ(editorIn, f.parameters.Q)
    { 
        addAllAndMakeVisible(*this, 
                filterResponse, 
                filterTypeSelector, 
                filterGain, 
                filterCutoff, 
                filterQ);
    }
          
    ~FilterControls() override
    {
        // cutoffResonanceHandle.removeListener();
    }

    void resized() override 
    {
        auto r = getLocalBounds();
        auto responseArea = r.removeFromTop((int) (0.6 * getHeight())); 
        filterResponse.setBounds(responseArea);
        performLayout(r, filterTypeSelector, filterCutoff, filterQ, filterGain);
        // updateHandlePosition();
    }

    // void paint(juce::Graphics& g) override
    // {
    //     Component::paint(g);
    // //     // updateHandlePosition();
    // }

private: 
    FilterResponseComponent filterResponse; 
    AttachedCombo filterTypeSelector; 
    AttachedSlider filterGain; 
    AttachedSlider filterCutoff; 
    AttachedSlider filterQ; 


};