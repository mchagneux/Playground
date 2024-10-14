#pragma once 
#include <JuceHeader.h>
#include "../utils/Parameters.h"
#include "../utils/Misc.h"
#include "./Compressor.h"
#include "./EQ.h"

struct PostProcessor : public juce::AudioProcessor
{

public: 

    PostProcessor (const PostProcessorParameters& p)
        : juce::AudioProcessor (getBusesProperties()), 
          parameters(p), 
          eq(parameters.eq),
          compressor(parameters.compressor) { }
    
    ~PostProcessor() override {}


//==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override
    {
        // Use this method as the place to do any pre-playback
        // initialisation that you need..
        juce::ignoreUnused (sampleRate, samplesPerBlock);
        const auto channels = juce::jmax (getTotalNumInputChannels(), getTotalNumOutputChannels());

        if (channels == 0)
            return;

        auto spec  = juce::dsp::ProcessSpec{ sampleRate, (juce::uint32) samplesPerBlock, (juce::uint32) channels };
        eq.prepare(spec);
        compressor.prepare(spec);
        // filter.reset();

    }
        
    void reset() final
    {
        eq.reset();
        compressor.reset();
        // chain.reset();
        // update();
    }

    static BusesProperties getBusesProperties()
    {
        return BusesProperties()
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true);
    }


    void releaseResources() final {}

    void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) final
    {

        if (juce::jmax (getTotalNumInputChannels(), getTotalNumOutputChannels()) == 0)
            return;

        juce::ScopedNoDenormals noDenormals;
        const auto totalNumInputChannels  = getTotalNumInputChannels();
        const auto totalNumOutputChannels = getTotalNumOutputChannels();

        const auto numChannels = juce::jmax (totalNumInputChannels, totalNumOutputChannels);

        auto inoutBlock = juce::dsp::AudioBlock<float> (buffer).getSubsetChannelBlock (0, (size_t) numChannels);
        auto context = juce::dsp::ProcessContextReplacing<SampleType> (inoutBlock);

        eq.process (context);
        eq.postAnalyzer.addAudioData(buffer, 0, 2);
        compressor.process(context);
    }

    void processBlock (juce::AudioBuffer<double>&, juce::MidiBuffer&) final {}

    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }

    //==============================================================================
    const juce::String getName() const final { return "PostProcessor"; }

    bool acceptsMidi()  const final { return false; }
    bool producesMidi() const final { return false; }
    bool isMidiEffect() const final { return false; }

    double getTailLengthSeconds() const final { return 0.0; }

    //==============================================================================
    int getNumPrograms()    final { return 1; }
    int getCurrentProgram() final { return 0; }
    void setCurrentProgram (int) final {}
    const juce::String getProgramName (int) final { return "None"; }

    void changeProgramName (int, const juce::String&) final {}

    //==============================================================================
    bool isBusesLayoutSupported (const BusesLayout& layout) const final
    {
        return layout == BusesLayout { { juce::AudioChannelSet::stereo() },
                                       { juce::AudioChannelSet::stereo() } };
    }

    void getStateInformation (juce::MemoryBlock& destData) final
    {
        // copyXmlToBinary (*state.copyState().createXml(), destData);
    }

    void setStateInformation (const void* data, int sizeInBytes) final
    {
        // state.replaceState (juce::ValueTree::fromXml (*getXmlFromBinary (data, sizeInBytes)));
    }

    const PostProcessorParameters& parameters; 

    auto& getEQ()
    {
        return eq; 
    }
    auto& getCompressor()
    {
        return compressor; 
    }
private:

    EQ eq; 
    Compressor compressor; 

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PostProcessor)
};


struct PostProcessorControls final : public juce::Component
{

    explicit PostProcessorControls (juce::AudioProcessorEditor& editor, PostProcessor& pp)
        : eq(editor, pp.getEQ()), 
          compressor(editor, pp.parameters.compressor) 
          
    {
        addAllAndMakeVisible (*this, eq, compressor);
    }

    void resized() override
    {
        auto r = getLocalBounds(); 
        // distortion.setBounds(r.removeFromTop((int) (getHeight() / 2))); 
        eq.setBounds(r.removeFromTop(getHeight() / 2)); 
        compressor.setBounds(r.removeFromTop((int) (r.getHeight() / 2)));
    }

    EQControls eq; 
    CompressorControls compressor; 
    
};
