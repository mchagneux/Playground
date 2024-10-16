#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================

MainProcessor::~MainProcessor()
{
    // #if PERFETTO
    //     MelatoninPerfetto::get().endSession();
    // #endif
}

//==============================================================================
const juce::String MainProcessor::getName() const { return JucePlugin_Name; }

bool MainProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool MainProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool MainProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double MainProcessor::getTailLengthSeconds() const { return 0.0; }

int MainProcessor::getNumPrograms()
{
    return 1; // NB: some hosts don't cope very well if you tell them there are 0
              // programs, so this should be at least 1, even if you're not really
              // implementing programs.
}

int MainProcessor::getCurrentProgram() { return 0; }

void MainProcessor::setCurrentProgram (int index) { juce::ignoreUnused (index); }

const juce::String MainProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void MainProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void MainProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    // mainProcessor->setPlayConfigDetails (getMainBusNumInputChannels(),
    //                                         getMainBusNumOutputChannels(),
    //                                         sampleRate, samplesPerBlock);
    // for (auto node : mainProcessor->getNodes())
    //     node->getProcessor()->enableAllBuses();

    // mainProcessor->prepareToPlay (sampleRate, samplesPerBlock);
    cmajorJITLoaderPlugin->prepareToPlay (sampleRate, samplesPerBlock);
    neuralProcessor.prepareToPlay (sampleRate, samplesPerBlock);
    postProcessor.prepareToPlay (sampleRate, samplesPerBlock);
}

void MainProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
    // mainProcessor->releaseResources();
    cmajorJITLoaderPlugin->releaseResources();
    neuralProcessor.releaseResources();
    postProcessor.releaseResources();
}

bool MainProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
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

void MainProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                  juce::MidiBuffer& midiMessages)
{
    auto message = juce::MidiMessage::noteOn (1, 28, 1.0f);
    // message.setTimeStamp (juce::Time::getMillisecondCounterHiRes() * 0.001);
    midiMessages.addEvent (message, 0);
    // juce::ignoreUnused (midiMessages);

    // mainProcessor->processBlock (buffer, midiMessages);
    cmajorJITLoaderPlugin->processBlock (buffer, midiMessages);
    neuralProcessor.processBlock (buffer, midiMessages);
    postProcessor.processBlock (buffer, midiMessages);
}

//==============================================================================
bool MainProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* MainProcessor::createEditor()
{
    return new MainProcessorEditor (*this);
    // return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void MainProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::ignoreUnused (destData);
}

void MainProcessor::setStateInformation (const void* data, int sizeInBytes)
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
    return new MainProcessor();
}
