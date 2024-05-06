#include "PluginProcessor.h"
#include "PluginEditor.h"


#include "processors/Gain.h"
#include "processors/IIRFilter.h"
//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), parameters(*this, 
                       nullptr, 
                       juce::Identifier("Playground"), 
                       {std::make_unique<juce::AudioParameterChoice> ("slot1",   "Slot 1", processorChoices, 0),             // default value
                        std::make_unique<juce::AudioParameterChoice> ("slot2",   "Slot 2",     processorChoices, 0),
                        std::make_unique<juce::AudioParameterChoice> ("slot3",   "Slot 3",     processorChoices, 0)}), mainProcessor  (new juce::AudioProcessorGraph())
{
    

}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
}

//==============================================================================
const juce::String AudioPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AudioPluginAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioPluginAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AudioPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String AudioPluginAudioProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void AudioPluginAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void AudioPluginAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    mainProcessor->setPlayConfigDetails (getMainBusNumInputChannels(),
                                            getMainBusNumOutputChannels(),
                                            sampleRate, samplesPerBlock);

    mainProcessor->prepareToPlay (sampleRate, samplesPerBlock);

    initialiseGraph();
}


void AudioPluginAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
    mainProcessor->releaseResources();

}

bool AudioPluginAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void AudioPluginAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    for (int i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    updateGraph();

    mainProcessor->processBlock (buffer, midiMessages);
}

//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
    return new AudioPluginAudioProcessorEditor (*this);
}

//==============================================================================
void AudioPluginAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::ignoreUnused (destData);
}

void AudioPluginAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    juce::ignoreUnused (data, sizeInBytes);
}


void AudioPluginAudioProcessor::connectAudioNodes()

{
    for (int channel = 0; channel < 2; ++channel)
        mainProcessor->addConnection ({ { audioInputNode->nodeID,  channel },
                                        { audioOutputNode->nodeID, channel } });
}


void AudioPluginAudioProcessor::initialiseGraph()

{
    mainProcessor->clear();

    audioInputNode  = mainProcessor->addNode (std::make_unique<AudioGraphIOProcessor> (AudioGraphIOProcessor::audioInputNode));
    audioOutputNode = mainProcessor->addNode (std::make_unique<AudioGraphIOProcessor> (AudioGraphIOProcessor::audioOutputNode));

    connectAudioNodes();
}

void AudioPluginAudioProcessor::updateGraph()
{
    bool hasChanged = false;

    juce::Array<juce::AudioParameterChoice*> choices {
                                                    static_cast<juce::AudioParameterChoice*>(parameters.getParameter("slot1")), 
                                                    static_cast<juce::AudioParameterChoice*>(parameters.getParameter("slot2")),
                                                    static_cast<juce::AudioParameterChoice*>(parameters.getParameter("slot3"))};

    // juce::Array<juce::AudioParameterBool*> bypasses { bypassSlot1,
    //                                                   bypassSlot2,
    //                                                   bypassSlot3 };

    juce::ReferenceCountedArray<Node> slots;
    slots.add (slot1Node);
    slots.add (slot2Node);
    slots.add (slot3Node);

    for (int i = 0; i < 3; ++i)
    {
        auto& choice = choices.getReference (i);
        auto  slot   = slots  .getUnchecked (i);

        if (choice->getIndex() == 0)            // [1]
        {
            if (slot != nullptr)
            {
                mainProcessor->removeNode (slot.get());
                slots.set (i, nullptr);
                hasChanged = true;
            }
        }
        else if (choice->getIndex() == 1)       // [3]
        {
            if (slot != nullptr)
            {
                if (slot->getProcessor()->getName() == "Gain")
                    continue;

                mainProcessor->removeNode (slot.get());
            }

            slots.set (i, mainProcessor->addNode (std::make_unique<GainProcessor>()));
            hasChanged = true;
        }
        else if (choice->getIndex() == 2)       // [4]
        {
            if (slot != nullptr)
            {
                if (slot->getProcessor()->getName() == "Filter")
                    continue;

                mainProcessor->removeNode (slot.get());
            }

            slots.set (i, mainProcessor->addNode (std::make_unique<FilterProcessor>()));
            hasChanged = true;
        }
    }

    if (hasChanged)
    {
        for (auto connection : mainProcessor->getConnections())     // [5]
            mainProcessor->removeConnection (connection);

        juce::ReferenceCountedArray<Node> activeSlots;

        for (auto slot : slots)
        {
            if (slot != nullptr)
            {
                activeSlots.add (slot);                             // [6]

                slot->getProcessor()->setPlayConfigDetails (getMainBusNumInputChannels(),
                                                            getMainBusNumOutputChannels(),
                                                            getSampleRate(), getBlockSize());
            }
        }

        if (activeSlots.isEmpty())                                  // [7]
        {
            connectAudioNodes();
        }
        else
        {
            for (int i = 0; i < activeSlots.size() - 1; ++i)        // [8]
            {
                for (int channel = 0; channel < 2; ++channel)
                    mainProcessor->addConnection ({ { activeSlots.getUnchecked (i)->nodeID,      channel },
                                                    { activeSlots.getUnchecked (i + 1)->nodeID,  channel } });
            }

            for (int channel = 0; channel < 2; ++channel)           // [9]
            {
                mainProcessor->addConnection ({ { audioInputNode->nodeID,         channel },
                                                { activeSlots.getFirst()->nodeID, channel } });
                mainProcessor->addConnection ({ { activeSlots.getLast()->nodeID,  channel },
                                                { audioOutputNode->nodeID,        channel } });
            }
        }


        for (auto node : mainProcessor->getNodes())                 // [10]
            node->getProcessor()->enableAllBuses();
    }

    // for (int i = 0; i < 3; ++i)
    // {
    //     auto  slot   = slots   .getUnchecked (i);
    //     auto& bypass = bypasses.getReference (i);

    //     if (slot != nullptr)
    //         slot->setBypassed (bypass->get());
    // }

    // audioInputNode->setBypassed (muteInput->get());

    slot1Node = slots.getUnchecked (0);
    slot2Node = slots.getUnchecked (1);
    slot3Node = slots.getUnchecked (2);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}
