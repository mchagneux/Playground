#include "./NeuralProcessor.h"

//==============================================================================
NeuralProcessor::NeuralProcessor(BusesProperties buses, juce::AudioProcessorValueTreeState& a) 
        : AudioProcessor (buses), parameters(a),
        inferenceHandler(prePostProcessor, inferenceConfig),
        dryWetMixer(32768) // 32768 samples of max latency compensation for the dry-wet mixer
{
    for (auto & parameterID : PluginParameters::getPluginParameterList()) {
        parameters.addParameterListener(parameterID, this);
    }
}

NeuralProcessor::~NeuralProcessor()
{
    for (auto & parameterID : PluginParameters::getPluginParameterList()) {
        parameters.removeParameterListener(parameterID, this);
    }
}

//==============================================================================
const juce::String NeuralProcessor::getName() const
{
    return JucePlugin_Name;
}

bool NeuralProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool NeuralProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool NeuralProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double NeuralProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int NeuralProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int NeuralProcessor::getCurrentProgram()
{
    return 0;
}

void NeuralProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String NeuralProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void NeuralProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void NeuralProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec monoSpec {sampleRate,
                                 static_cast<juce::uint32>(samplesPerBlock),
                                 static_cast<juce::uint32>(1)};

    anira::HostAudioConfig monoConfig {
        1,
        (size_t) samplesPerBlock,
        sampleRate
    };

    dryWetMixer.prepare(monoSpec);

    monoBuffer.setSize(1, samplesPerBlock);
    inferenceHandler.prepare(monoConfig);

    auto newLatency = inferenceHandler.getLatency();
    setLatencySamples(newLatency);

    dryWetMixer.setWetLatency(newLatency);

    for (auto & parameterID : PluginParameters::getPluginParameterList()) {
        parameterChanged(parameterID, (float) parameters.getParameterAsValue(parameterID).getValue());
    }
}

void NeuralProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool NeuralProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void NeuralProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                              juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    stereoToMono(monoBuffer, buffer);
    dryWetMixer.pushDrySamples(monoBuffer);

    auto inferenceBuffer = const_cast<float **>(monoBuffer.getArrayOfWritePointers());
    inferenceHandler.process(inferenceBuffer, (size_t) buffer.getNumSamples());

    dryWetMixer.mixWetSamples(monoBuffer);
    monoToStereo(buffer, monoBuffer);
}

//==============================================================================
bool NeuralProcessor::hasEditor() const
{
    return false; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* NeuralProcessor::createEditor()
{
    return nullptr;
}

//==============================================================================
void NeuralProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::ignoreUnused (destData);
}

void NeuralProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    juce::ignoreUnused (data, sizeInBytes);
}

void NeuralProcessor::parameterChanged(const juce::String &parameterID, float newValue) {
    if (parameterID == PluginParameters::DRY_WET_ID.getParamID()) {
        dryWetMixer.setWetMixProportion(newValue);
    } else if (parameterID == PluginParameters::BACKEND_TYPE_ID.getParamID()) {
        const auto paramInt = static_cast<int>(newValue);
        auto paramString = PluginParameters::backendTypes.getReference(paramInt);
#ifdef USE_TFLITE
        if (paramString == "TFLITE") {inferenceHandler.setInferenceBackend(anira::TFLITE);
        std::cout << "TFLITE" << std::endl;}
#endif
#ifdef USE_ONNXRUNTIME
        if (paramString == "ONNXRUNTIME") {inferenceHandler.setInferenceBackend(anira::ONNX);
        std::cout << "ONNXRUNTIME" << std::endl;}
#endif
#ifdef USE_LIBTORCH
        if (paramString == "LIBTORCH") {inferenceHandler.setInferenceBackend(anira::LIBTORCH);
        std::cout << "LIBTORCH" << std::endl;}
#endif
        if (paramString == "NONE") inferenceHandler.setInferenceBackend(anira::NONE);
    }
}
//==============================================================================
// // This creates new instances of the plugin..
// juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
// {
//     return new NeuralProcessor();
// }

anira::InferenceManager &NeuralProcessor::getInferenceManager() {
    return inferenceHandler.getInferenceManager();
}

void NeuralProcessor::stereoToMono(juce::AudioBuffer<float> &targetMonoBlock,
                                             juce::AudioBuffer<float> &sourceBlock) {
    if (sourceBlock.getNumChannels() == 1) {
        targetMonoBlock.makeCopyOf(sourceBlock);
    } else {
        auto nSamples = sourceBlock.getNumSamples();

        auto monoWrite = targetMonoBlock.getWritePointer(0);
        auto lRead = sourceBlock.getReadPointer(0);
        auto rRead = sourceBlock.getReadPointer(1);

        juce::FloatVectorOperations::copy(monoWrite, lRead, nSamples);
        juce::FloatVectorOperations::add(monoWrite, rRead, nSamples);
        juce::FloatVectorOperations::multiply(monoWrite, 0.5f, nSamples);
    }
}

void NeuralProcessor::monoToStereo(juce::AudioBuffer<float> &targetStereoBlock,
                                             juce::AudioBuffer<float> &sourceBlock) {
    if (targetStereoBlock.getNumChannels() == 1) {
        targetStereoBlock.makeCopyOf(sourceBlock);
    } else {
        auto nSamples = sourceBlock.getNumSamples();

        auto lWrite = targetStereoBlock.getWritePointer(0);
        auto rWrite = targetStereoBlock.getWritePointer(1);
        auto monoRead = sourceBlock.getReadPointer(0);

        juce::FloatVectorOperations::copy(lWrite, monoRead, nSamples);
        juce::FloatVectorOperations::copy(rWrite, monoRead, nSamples);
    }
}