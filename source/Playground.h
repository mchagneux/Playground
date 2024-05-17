# pragma once 

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "processors/Ladder.h"
#include "processors/Distortion.h"
#include "ui/Distortion.h"
#include "ui/Ladder.h"
#include "processors/CmajorStereoDSPEffect.h"

// #include "ui/Cmajor.h"

//==============================================================================
class PlaygroundProcessor : public juce::AudioProcessor, private juce::ValueTree::Listener
{
public:
    //==============================================================================
    PlaygroundProcessor();
    ~PlaygroundProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void valueTreePropertyChanged (ValueTree&, const Identifier&) final
    {
        requiresUpdate.store (true);
    }

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    void update();
    void reset() override;

    // void initialiseGraph();
    // void updateGraph();
    // void connectAudioNodes();

    struct ParameterReferences
    {
        explicit ParameterReferences (AudioProcessorValueTreeState::ParameterLayout& layout)
            :  distortion    (addToLayout<AudioProcessorParameterGroup> (layout, "distortion",    "Distortion",    "|")),
            ladder        (addToLayout<AudioProcessorParameterGroup> (layout, "ladder",        "Ladder",        "|")),
            cmajor (addToLayout<AudioProcessorParameterGroup> (layout, "cmajor",        "Cmajor",        "|")) {}
        
        DistortionProcessor::Parameters distortion;
        LadderProcessor::Parameters ladder;
        CmajorStereoDSPEffect::Parameters cmajor;

    };

    CmajorStereoDSPEffect::Processor& getCmajorDSPEffectProcessor() {return dsp::get<cmajorIndex> (chain);}

    const ParameterReferences& getParameterValues() const noexcept { return parameters; }
    // CmajorStereoDSPEffect& getCmajorDSP
private:

    explicit PlaygroundProcessor (AudioProcessorValueTreeState::ParameterLayout layout)
        : AudioProcessor (BusesProperties().withInput ("In",   AudioChannelSet::stereo())
                                           .withOutput ("Out", AudioChannelSet::stereo())),
          parameters { layout },
          apvts { *this, nullptr, "state", std::move (layout) }
    {
        apvts.state.addListener (this);
    }
    
    std::atomic<bool> requiresUpdate { true };

    ParameterReferences parameters; 
    juce::AudioProcessorValueTreeState apvts; 
    using Chain = juce::dsp::ProcessorChain<DistortionProcessor, 
                                            dsp::LadderFilter<float>, 
                                            CmajorStereoDSPEffect::Processor>;

    Chain chain;

    enum ProcessorIndices
    {
        distortionIndex,
        ladderIndex,
        cmajorIndex
    };
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlaygroundProcessor)
};


class PlaygroundEditor final : public AudioProcessorEditor
{
public:
    explicit PlaygroundEditor (PlaygroundProcessor& p)
        : AudioProcessorEditor (&p),
          proc (p) 
    {

        addAllAndMakeVisible (*this,
                              distortionControls, 
                              ladderControls
                              );


        setSize (1280, 720);
        setResizable (true, true);
    }

    //==============================================================================
    void paint (Graphics& g) override
    {
        // auto rect = getLocalBounds();

        // auto currentBound = getBounds();

        // g.setColour (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));
        // g.fillRect (currentBound);

    }

    void resized() override
    {
        auto rect = getLocalBounds();

        auto distortionArea = rect.removeFromLeft(0.3*getWidth());
        distortionControls.setBounds(distortionArea);
        auto ladderArea = rect.removeFromLeft(0.3*getWidth());
        ladderControls.setBounds(ladderArea);


    }

private:

    //==============================================================================
    // static constexpr auto topSize    = 40,
    //                       bottomSize = 40,
    //                       midSize    = 40,
    //                       tabSize    = 155;

    //==============================================================================
    PlaygroundProcessor& proc;
    DistortionControls  distortionControls  { *this, proc.getParameterValues().distortion };
    LadderControls      ladderControls      { *this, proc.getParameterValues().ladder };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PlaygroundEditor)
};

struct Playground final : public PlaygroundProcessor
{
    AudioProcessorEditor* createEditor() override
    {
        return new PlaygroundEditor (*this);
    }

    bool hasEditor() const override { return true; }
};
