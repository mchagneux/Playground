
#include "anira/utils/InferenceBackend.h"
#include <juce_dsp/juce_dsp.h>
#include <anira/anira.h>
#include "../Parameters.h"

using namespace juce;



#ifdef INSTALL_VERSION
#if __linux__
    static std::string home = getenv("HOME");
    static std::string path_prefix_pytorch = home + std::string("/.config/nn-inference-template/GuitarLSTM/pytorch-version/models/");
    static std::string path_prefix_tflite = home + std::string("/.config/nn-inference-template/GuitarLSTM/tensorflow-version/models/");
#elif __APPLE__
    static std::string home = getenv("HOME");
    static std::string path_prefix_pytorch = home + std::string("/Library/Application Support/nn-inference-template/GuitarLSTM/pytorch-version/models/");
    static std::string path_prefix_tflite = home + std::string("/Library/Application Support/nn-inference-template/GuitarLSTM/tensorflow-version/models/");
#elif _WIN32
    #include <filesystem>
    static std::string path_prefix_pytorch = std::filesystem::path {getenv("APPDATA")}.string() + std::string("/nn-inference-template/GuitarLSTM/pytorch-version/models/");
    static std::string path_prefix_tflite = std::filesystem::path {getenv("APPDATA")}.string() + std::string("/nn-inference-template/GuitarLSTM/tensorflow-version/models/");
#endif

#else
static std::string path_prefix_pytorch = std::string(GUITARLSTM_MODELS_PATH_PYTORCH);
static std::string path_prefix_tflite = std::string(GUITARLSTM_MODELS_PATH_TENSORFLOW);
#endif

static anira::InferenceConfig hybridNNConfig(
#ifdef USE_LIBTORCH
        path_prefix_pytorch + std::string("model_0/GuitarLSTM-dynamic.pt"),
        {256, 1, 150},
        {256, 1},
#endif
#ifdef USE_ONNXRUNTIME
        path_prefix_pytorch + std::string("model_0/GuitarLSTM-libtorch-dynamic.onnx"),
        {256, 1, 150},
        {256, 1},
#endif
#ifdef USE_TFLITE
        path_prefix_tflite + std::string("model_0/GuitarLSTM-256.tflite"),
        {256, 150, 1},
        {256, 1},
#endif
        256,
        150,
        1,
        10.66f,
        0,
        true,
        0.5f,
        false
);


class HybridNNPrePostProcessor : public anira::PrePostProcessor
{
public:
    virtual void preProcess(anira::RingBuffer& input, anira::AudioBufferF& output, [[maybe_unused]] anira::InferenceBackend currentInferenceBackend) override {
        for (size_t batch = 0; batch < config.m_batch_size; batch++) {
            size_t baseIdx = batch * config.m_model_input_size;
            popSamplesFromBuffer(input, output, config.m_model_output_size, config.m_model_input_size-config.m_model_output_size, baseIdx);
        }
    };
    
    anira::InferenceConfig config = hybridNNConfig;
};



struct NNEngine{

    struct Parameters{

     explicit Parameters (AudioProcessorParameterGroup& layout)
            : enabled (addToLayout<AudioParameterBool> (layout,
                                                        ParameterID { ID::nnEnabled, 1 },
                                                        "Neural Network",
                                                        true)),
                type (addToLayout<AudioParameterChoice> (layout,
                                                        ParameterID { ID::nnBackend, 1 },
                                                        "Backend",
                                                        StringArray { "Libtorch", "ONNX", "Tensorflow Lite" },
                                                        0)),
                mix (addToLayout<Parameter> (layout,
                                            ParameterID { ID::nnMix, 1 },
                                            "Mix",
                                            NormalisableRange<float> (0.0f, 100.0f),
                                            100.0f,
                                            getPercentageAttributes())) {}
        AudioParameterBool& enabled;
        AudioParameterChoice& type;
        Parameter& mix;

    };



    NNEngine(): inferenceHandler(prePostProcessor, inferenceConfig), dryWetMixer(32768) {}

    void setBackend(anira::InferenceBackend backend){

        inferenceHandler.setInferenceBackend(backend); // TODO: asynchronous call on the message queue
    }

    int getLatency(){
        return inferenceHandler.getLatency();
    }
    void prepare (const dsp::ProcessSpec& spec) {

        juce::dsp::ProcessSpec monoSpec {spec.sampleRate,
                            static_cast<juce::uint32>(spec.maximumBlockSize),
                            static_cast<juce::uint32>(1)};

        anira::HostAudioConfig monoConfig {
            1,
            (size_t) spec.maximumBlockSize,
            spec.sampleRate
        };

        dryWetMixer.prepare(monoSpec);

        monoBuffer.setSize(1, spec.maximumBlockSize);
        inferenceHandler.prepare(monoConfig);

        dryWetMixer.setWetLatency(getLatency());
        // inferenceHandler.setInferenceBackend(anira::LIBTORCH);
        // for (auto & parameterID : PluginParameters::getPluginParameterList()) {
        //     parameterChanged(parameterID, (float) parameters.getParameterAsValue(parameterID).getValue());
        // }
    }

    template <typename Context>
    void process(Context& context){ //Context&){
        // juce::ignoreUnused (midiMessages);
        // return;
        if (context.isBypassed)
            return;

        auto&& inputBlock = context.getInputBlock();
        auto&& outputBlock = context.getOutputBlock();


        outputBlock.copyFrom(inputBlock);
        stereoToMono(monoBuffer, inputBlock);
        dryWetMixer.pushDrySamples(monoBuffer);

        auto inferenceBuffer = const_cast<float **>(monoBuffer.getArrayOfWritePointers());
        inferenceHandler.process(inferenceBuffer, (size_t) inputBlock.getNumSamples());

        dryWetMixer.mixWetSamples(monoBuffer);
        monoToStereo(outputBlock, monoBuffer);
    }
    void reset() {}
    
    juce::AudioBuffer<float> monoBuffer;
    // void parameterChanged (const juce::String& parameterID, float newValue) override;
    template <typename SampleType>

    void stereoToMono(juce::AudioBuffer<float> &targetMonoBlock,
                                                const juce::dsp::AudioBlock<SampleType> &sourceBlock) {
        if (sourceBlock.getNumChannels() == 1) {
            targetMonoBlock.copyFrom(0, 0, sourceBlock.getChannelPointer(0), sourceBlock.getNumSamples());
        } else {
        auto nSamples = sourceBlock.getNumSamples();

        auto monoWrite = targetMonoBlock.getWritePointer(0);
        auto lRead = sourceBlock.getChannelPointer(0);
        auto rRead = sourceBlock.getChannelPointer(0);

        juce::FloatVectorOperations::copy(monoWrite, lRead, nSamples);
        juce::FloatVectorOperations::add(monoWrite, rRead, nSamples);
        juce::FloatVectorOperations::multiply(monoWrite, 0.5f, nSamples);
        }
    }

    template <typename SampleType>
    void monoToStereo(juce::dsp::AudioBlock<SampleType> &targetStereoBlock, juce::AudioBuffer<float> &sourceBlock) {

        if (targetStereoBlock.getNumChannels() == 1) {
            targetStereoBlock.copyFrom(sourceBlock);
        } else {
            auto nSamples = sourceBlock.getNumSamples();

            auto lWrite = targetStereoBlock.getChannelPointer(0);
            auto rWrite = targetStereoBlock.getChannelPointer(1);
            auto monoRead = sourceBlock.getReadPointer(0);

            juce::FloatVectorOperations::copy(lWrite, monoRead, nSamples);
            juce::FloatVectorOperations::copy(rWrite, monoRead, nSamples);
        }
    }

    anira::InferenceConfig inferenceConfig = hybridNNConfig;
    HybridNNPrePostProcessor prePostProcessor;
    anira::InferenceHandler inferenceHandler;
    juce::dsp::DryWetMixer<float> dryWetMixer;

//==============================================================================
JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NNEngine)

};