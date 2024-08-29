#pragma once

// #include <juce_audio_processors/juce_audio_processors.h>
#include <JuceHeader.h>
#include "CmajorProcessor.h"


//==============================================================================
class AudioPluginAudioProcessor final : public juce::AudioProcessor
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

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    CmajorProcessor& getCmajorProcessor(){
        return static_cast<CmajorProcessor&>(*cmajorGeneratorNode->getProcessor()); 
    }

    BusesProperties getBusesProperties() 
    {
        return BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                    ;
    }

    void initialiseGraph()
    {
        mainProcessor->clear();

        audioOutputNode = mainProcessor->addNode (std::make_unique<AudioGraphIOProcessor> (AudioGraphIOProcessor::audioOutputNode));
        midiInputNode   = mainProcessor->addNode (std::make_unique<AudioGraphIOProcessor> (AudioGraphIOProcessor::midiInputNode));


        auto patch = std::make_shared<cmaj::Patch>();
        patch->setAutoRebuildOnFileChange (true);
        patch->createEngine = +[] { return cmaj::Engine::create(); };

        cmajorGeneratorNode = mainProcessor->addNode(std::make_unique<CmajorProcessor>(getBusesProperties(), std::move(patch)));

        connectAudioNodes();
        connectMidiNodes();


    }

    void connectAudioNodes()
    {
        for (int channel = 0; channel < 2; ++channel)
            mainProcessor->addConnection ({ { cmajorGeneratorNode->nodeID,  channel },
                                            { audioOutputNode->nodeID, channel } });
    }

    void connectMidiNodes()
    {
        mainProcessor->addConnection ({ { midiInputNode->nodeID,  juce::AudioProcessorGraph::midiChannelIndex },
                                        { cmajorGeneratorNode->nodeID, juce::AudioProcessorGraph::midiChannelIndex } });
    }


private:
    Node::Ptr audioOutputNode;
    Node::Ptr midiInputNode;
    Node::Ptr cmajorGeneratorNode;

    std::unique_ptr<AudioProcessorGraph> mainProcessor;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessor)
};
