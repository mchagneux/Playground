#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "processors/Ladder.h"
#include "processors/Distortion.h"

//==============================================================================
class AudioPluginAudioProcessor final : public juce::AudioProcessor, private juce::ValueTree::Listener
{
public:
    using AudioGraphIOProcessor = juce::AudioProcessorGraph::AudioGraphIOProcessor;
    using Node = juce::AudioProcessorGraph::Node;
    //==============================================================================
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

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

    // void initialiseGraph();
    // void updateGraph();
    // void connectAudioNodes();

    struct ParameterReferences
    {
        explicit ParameterReferences (AudioProcessorValueTreeState::ParameterLayout& layout)
            :  distortion    (addToLayout<AudioProcessorParameterGroup> (layout, "distortion",    "Distortion",    "|")),
            ladder        (addToLayout<AudioProcessorParameterGroup> (layout, "ladder",        "Ladder",        "|")) {}
        
        DistortionProcessor::Parameters distortion;
        LadderProcessor::Parameters ladder;

    };

    const ParameterReferences& getParameterValues() const noexcept { return parameters; }

private:

    explicit AudioPluginAudioProcessor (AudioProcessorValueTreeState::ParameterLayout layout)
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
    using Chain = juce::dsp::ProcessorChain<DistortionProcessor, dsp::LadderFilter<float>>;


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};
