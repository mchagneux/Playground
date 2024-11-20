#include "Plugin.h"
#include "PluginEditor.h"

//==============================================================================

Plugin::~Plugin()
{
    // #if PERFETTO
    //     MelatoninPerfetto::get().endSession();
    // #endif
}

//==============================================================================
const juce::String Plugin::getName() const { return JucePlugin_Name; }

bool Plugin::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool Plugin::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool Plugin::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double Plugin::getTailLengthSeconds() const { return 0.0; }

int Plugin::getNumPrograms()
{
    return 1; // NB: some hosts don't cope very well if you tell them there are 0
              // programs, so this should be at least 1, even if you're not really
              // implementing programs.
}

int Plugin::getCurrentProgram() { return 0; }

void Plugin::setCurrentProgram (int index) { juce::ignoreUnused (index); }

const juce::String Plugin::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void Plugin::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void Plugin::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    auto processSpec = juce::dsp::ProcessSpec {
        sampleRate,
        (juce::uint32) samplesPerBlock,
        2
    };

    cmajorJITProcessor->prepare (processSpec);
    neuralProcessor.prepare (processSpec);
    postProcessor.prepare (processSpec);
}

void Plugin::releaseResources()
{
    cmajorJITProcessor->reset();
    neuralProcessor.reset();
    postProcessor.reset();
}

bool Plugin::isBusesLayoutSupported (const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
#else

    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

        // This checks if the input layout matches the output layout
#if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}

void Plugin::processBlock (juce::AudioBuffer<float>& buffer,
                           juce::MidiBuffer& midiMessages)
{
    auto message = juce::MidiMessage::noteOn (1, 28, 1.0f);
    midiMessages.addEvent (message, 0);

    cmajorJITProcessor->process (buffer, midiMessages);

    auto block = juce::dsp::AudioBlock<float> (buffer);
    auto context = juce::dsp::ProcessContextReplacing<float> (block);
    neuralProcessor.process (context);
    postProcessor.process (context);
}

//==============================================================================
bool Plugin::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* Plugin::createEditor()
{
    return new PluginEditor (*this);
    // return new juce::GenericAudioProcessorEditor (*this);
}

//==============================================================================
void Plugin::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::ignoreUnused (destData);
}

void Plugin::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory
    // block, whose contents will have been created by the getStateInformation()
    // call.
    juce::ignoreUnused (data, sizeInBytes);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Plugin();
}
