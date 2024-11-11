#pragma once

#include <JuceHeader.h>
#include "./utils/Parameters.h"

#include "./processors/NeuralProcessor.h"
#include "./processors/CmajorProcessor.h"
#include "./processors/PostProcessor.h"

// #include <melatonin_perfetto/melatonin_perfetto.h>

//==============================================================================
class Plugin final : public juce::AudioProcessor
{
public:
    //==============================================================================
    Plugin()
        : Plugin (juce::AudioProcessorValueTreeState::ParameterLayout {})
    {
    }

    ~Plugin() override;

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

    JITLoaderPlugin& getCmajorProcessor()
    {
        return *cmajorJITLoaderPlugin;
        // return
        // static_cast<CmajorProcessor&>(*cmajorGeneratorNode->getProcessor());
    }

    auto& getPostProcessor() { return postProcessor; }

    BusesProperties getBusesProperties()
    {
        return BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
            .withInput ("Input", juce::AudioChannelSet::stereo(), true)
#endif
            .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
#endif
            ;
    }

    Parameters parameters;

private:
    explicit Plugin (
        juce::AudioProcessorValueTreeState::ParameterLayout layout)
        : AudioProcessor (getBusesProperties())
        , parameters (layout)
        , apvts (*this, nullptr, "Plugin", std::move (layout))
        , postProcessor (parameters.postProcessor)
        , neuralProcessor (parameters.neural)
    {
        auto patch = std::make_shared<cmaj::Patch>();
        patch->setAutoRebuildOnFileChange (true);
        patch->createEngine = +[]
        {
            return cmaj::Engine::create();
        };
        cmajorJITLoaderPlugin = std::make_unique<JITLoaderPlugin> (patch, *this);
        cmajorJITLoaderPlugin->loadPatch (
            "E:\\audio_dev\\Playground\\patches\\Synth\\Synth.cmajorpatch");
    }

    juce::AudioProcessorValueTreeState apvts;

    PostProcessor postProcessor;
    NeuralProcessor neuralProcessor;
    std::unique_ptr<JITLoaderPlugin> cmajorJITLoaderPlugin;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Plugin)
};
