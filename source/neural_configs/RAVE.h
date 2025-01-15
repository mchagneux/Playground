#pragma once

#include <ATen/core/TensorBody.h>
#ifndef ANIRA_LIBTORCHPROCESSOR_H
#define ANIRA_LIBTORCHPROCESSOR_H

#ifdef USE_LIBTORCH
#define MAX_LATENT_BUFFER_SIZE 32
#define BUFFER_LENGTH 32768
// Avoid min/max macro conflicts on Windows for LibTorch compatibility
#ifdef _WIN32
#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif
#endif

#include <anira/anira.h>

// LibTorch headers trigger many warnings; disabling for cleaner build logs
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4244 4267 4996)
#elif defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#endif

#include <torch/script.h>
#include <torch/torch.h>

#ifdef _MSC_VER
#pragma warning(pop)
#elif defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

class RAVEProcessor : public anira::BackendBase
{
public:
    RAVEProcessor (anira::InferenceConfig& inference_config);
    ~RAVEProcessor();

    void prepare() override;
    void process (anira::AudioBufferF& input, anira::AudioBufferF& output, std::shared_ptr<anira::SessionElement> session) override;

private:
    struct Instance
    {
        Instance (anira::InferenceConfig& inference_config);
        void prepare();
        void process (anira::AudioBufferF& input, anira::AudioBufferF& output, std::shared_ptr<anira::SessionElement> session);

        torch::jit::script::Module m_module;

        std::vector<anira::MemoryBlock<float>> m_input_data;

        std::vector<c10::IValue> m_inputs;
        c10::IValue m_outputs;

        anira::InferenceConfig& m_inference_config;
        std::atomic<bool> m_processing { false };

        bool stereo = false;
        bool has_prior = false;
        int sr = 44100;
        int latent_size = 64;

        at::Tensor encode_params;
        at::Tensor decode_params;
        at::Tensor prior_params;
        std::string rave_model_file;
        std::vector<torch::jit::IValue> inputs_rave;
        at::Tensor latent_buffer;

        int getFullLatentDimensions() { return latent_size; }

        int getModelRatio() { return encode_params.index ({ 3 }).item<int>(); }

        torch::Tensor sample_prior (const int n_steps, const float temperature)
        {
            c10::InferenceMode guard;
            inputs_rave[0] = torch::ones ({ 1, 1, n_steps }) * temperature;
            torch::Tensor prior =
                m_module.get_method ("prior") (inputs_rave).toTensor();
            return prior;
        }

        torch::Tensor encode (const torch::Tensor input)
        {
            c10::InferenceMode guard;
            inputs_rave[0] = input;
            auto y = m_module.get_method ("encode") (inputs_rave).toTensor();
            return y;
        }

        std::vector<torch::Tensor> encode_amortized (const torch::Tensor input)
        {
            c10::InferenceMode guard;
            inputs_rave[0] = input;
            auto stats = m_module.get_method ("encode_amortized") (inputs_rave)
                             .toTuple()
                             .get()
                             ->elements();
            torch::Tensor mean = stats[0].toTensor();
            torch::Tensor std = stats[1].toTensor();
            std::vector<torch::Tensor> mean_std = { mean, std };
            return mean_std;
        }

        torch::Tensor decode (const torch::Tensor input)
        {
            c10::InferenceMode guard;
            inputs_rave[0] = input;
            auto y = m_module.get_method ("decode") (inputs_rave).toTensor();
            return y;
        }

        int getInputBatches() { return encode_params.index ({ 1 }).item<int>(); }

        int getOutputBatches() { return decode_params.index ({ 3 }).item<int>(); }

        void resetLatentBuffer() { latent_buffer = torch::zeros ({ 0 }); }

        void writeLatentBuffer (at::Tensor latent)
        {
            if (latent_buffer.size (0) == 0)
            {
                latent_buffer = latent;
            }
            else
            {
                latent_buffer = torch::cat ({ latent_buffer, latent }, 2);
            }
            if (latent_buffer.size (2) > MAX_LATENT_BUFFER_SIZE)
            {
                latent_buffer = latent_buffer.index (
                    { "...", at::indexing::Slice (-MAX_LATENT_BUFFER_SIZE, at::indexing::None, at::indexing::None) });
            }
        }
    };

    std::vector<std::shared_ptr<Instance>> m_instances;
};

#endif
#endif // ANIRA_LIBTORCHPROCESSOR_H

static std::vector<anira::ModelData> model_data_config = {
    { std::string (RAVE_MODELS_PATH_PYTORCH) + std::string ("/sol_ordinario_fast.ts"), anira::InferenceBackend::LIBTORCH },

};

static std::vector<anira::TensorShape> tensor_shape_config = {
    { { { 1, 1, 1024 } }, { { 1, 1, 1024 } }, anira::InferenceBackend::LIBTORCH },

};

static anira::InferenceConfig RAVEConfig (
    model_data_config,
    tensor_shape_config,
    42.66f);
