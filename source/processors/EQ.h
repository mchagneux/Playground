#pragma once 

#include <JuceHeader.h>
#include "../utils/Misc.h"
#include "../utils/FrequencyAnalyzer.h"
#include "../utils/Parameters.h"
#include "./MultiTypeFilter.h"



using FilterChain = juce::dsp::ProcessorChain<StereoIIRFilter, StereoIIRFilter>;

enum filterIDs 
{
    filter0,
    filter1
};


struct EQ
{

public: 

    EQ(const EQParameters& parameterRefs)
    {
        filterChain.get<filter0>().connectToParameters(parameterRefs.filter0); 
        filterChain.get<filter1>().connectToParameters(parameterRefs.filter1); 
    }

    ~EQ()
    {
        postAnalyzer.stopThread(1000);
    }

    void prepare(juce::dsp::ProcessSpec& spec) 
    {
        sampleRate = (SampleType) spec.sampleRate; 
        postAnalyzer.setupAnalyser (int (sampleRate), float (sampleRate));
        filterChain.prepare(spec); 
    }

    void reset()
    {
        filterChain.reset(); 
    }

    template <typename Context>
    void process(Context& context) 
    {
        filterChain.process(context); 
    }


    Analyzer<SampleType> postAnalyzer; 
    FilterChain filterChain; 

private: 


    SampleType sampleRate; 

};


class EQControls : public juce::Component, private juce::ChangeListener, private juce::Timer
{
public:
    EQControls(juce::AudioProcessorEditor& editorIn, EQ& eqIn) 
        : eq(eqIn), 
          handle0(editorIn, *eq.filterChain.get<0>().parameters),
          handle1(editorIn, *eq.filterChain.get<1>().parameters)
    {
        startTimer(20);
        eq.filterChain.get<0>().addChangeListener(this);
        eq.filterChain.get<1>().addChangeListener(this);

        addAndMakeVisible(handle0);
        addAndMakeVisible(handle1);

        // FilterHandle.updateHandlePosition();
        // repaint();
    }
    
    ~EQControls() override 
    {
        stopTimer();
        eq.filterChain.get<0>().removeChangeListener(this);
        eq.filterChain.get<1>().removeChangeListener(this);

    }

    void paint(juce::Graphics& g) override
    {

        g.fillAll(juce::Colours::black);

        const auto bounds = getLocalBounds().toFloat();
        const float width = bounds.getWidth();
        const float height = bounds.getHeight();


        
        g.setColour(juce::Colours::white);
        juce::Path magnitudePath;
        bool pathStarted = false;

        for (float x = 0; x < width; ++x)
        {
            float freq = juce::mapToLog10(x / width, 20.0f, 20000.0f);
            float magnitude = 1.0f; 
            auto pixelsPerDouble = height / juce::Decibels::decibelsToGain (10.0f);


            magnitude *= eq.filterChain.get<0>().getMagnitudeForFrequency(freq);
            magnitude *= eq.filterChain.get<1>().getMagnitudeForFrequency(freq);

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


        auto plotFrame = getLocalBounds().toFloat();


        eq.postAnalyzer.createPath(analyzerPath, plotFrame, 20.0f);
        g.setColour (juce::Colours::grey);

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
        if (eq.postAnalyzer.checkForNewData()) 
            repaint(); 
    }

    EQ& eq;

    juce::Path analyzerPath; 
    FilterHandle handle0;
    FilterHandle handle1;  

};
