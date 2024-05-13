#include "Playground.h"


//==============================================================================
PlaygroundProcessor::PlaygroundProcessor()
     : PlaygroundProcessor(AudioProcessorValueTreeState::ParameterLayout{})
{

}

PlaygroundProcessor::~PlaygroundProcessor()
{
}

//==============================================================================
const juce::String PlaygroundProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PlaygroundProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool PlaygroundProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool PlaygroundProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double PlaygroundProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PlaygroundProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int PlaygroundProcessor::getCurrentProgram()
{
    return 0;
}

void PlaygroundProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String PlaygroundProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void PlaygroundProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void PlaygroundProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..

    const auto channels = jmax (getTotalNumInputChannels(), getTotalNumOutputChannels());

    if (channels == 0)
        return;

    chain.prepare ({ sampleRate, (uint32) samplesPerBlock, (uint32) channels });

    reset();

}


void PlaygroundProcessor::reset()
{
    chain.reset();
    update();
}

void PlaygroundProcessor::update()
{
    {

        DistortionProcessor& distortion = dsp::get<distortionIndex> (chain);

        if (distortion.currentIndexOversampling != parameters.distortion.oversampler.getIndex())
        {
            distortion.currentIndexOversampling = parameters.distortion.oversampler.getIndex();
            prepareToPlay (getSampleRate(), getBlockSize());
            return;
        }

        distortion.currentIndexWaveshaper = parameters.distortion.type.getIndex();
        distortion.lowpass .setCutoffFrequency (parameters.distortion.lowpass.get());
        distortion.highpass.setCutoffFrequency (parameters.distortion.highpass.get());
        distortion.distGain.setGainDecibels (parameters.distortion.inGain.get());
        distortion.compGain.setGainDecibels (parameters.distortion.compGain.get());
        distortion.mixer.setWetMixProportion (parameters.distortion.mix.get() / 100.0f);
        dsp::setBypassed<distortionIndex> (chain, ! parameters.distortion.enabled);
    }

    {
        dsp::LadderFilter<float>& ladder = dsp::get<ladderIndex> (chain);

        ladder.setCutoffFrequencyHz (parameters.ladder.cutoff.get());
        ladder.setResonance         (parameters.ladder.resonance.get() / 100.0f);
        ladder.setDrive (Decibels::decibelsToGain (parameters.ladder.drive.get()));

        ladder.setMode ([&]
        {
            switch (parameters.ladder.mode.getIndex())
            {
                case 0: return dsp::LadderFilterMode::LPF12;
                case 1: return dsp::LadderFilterMode::LPF24;
                case 2: return dsp::LadderFilterMode::HPF12;
                case 3: return dsp::LadderFilterMode::HPF24;
                case 4: return dsp::LadderFilterMode::BPF12;

                default: break;
            }

            return dsp::LadderFilterMode::BPF24;
        }());

        dsp::setBypassed<ladderIndex> (chain, ! parameters.ladder.enabled);

        dsp::setBypassed<cmajorIndex> (chain, ! parameters.cmajor.enabled);

    }

    requiresUpdate.store (false);
}





void PlaygroundProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.

}

bool PlaygroundProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void PlaygroundProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer&)
{
    if (jmax (getTotalNumInputChannels(), getTotalNumOutputChannels()) == 0)
        return;

    ScopedNoDenormals noDenormals;

    if (requiresUpdate.load())
        update();


    const auto totalNumInputChannels  = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    setLatencySamples (dsp::isBypassed<distortionIndex> (chain) ? 0 : roundToInt (dsp::get<distortionIndex> (chain).getLatency()));

    const auto numChannels = jmax (totalNumInputChannels, totalNumOutputChannels);

    auto inoutBlock = dsp::AudioBlock<float> (buffer).getSubsetChannelBlock (0, (size_t) numChannels);
    chain.process (dsp::ProcessContextReplacing<float> (inoutBlock));
}

//==============================================================================
void PlaygroundProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::ignoreUnused (destData);
}

void PlaygroundProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    juce::ignoreUnused (data, sizeInBytes);

}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Playground();
}
