#pragma once
#include <JuceHeader.h>
#include <torch/script.h>
#include <torch/torch.h>

#define MAX_LATENT_BUFFER_SIZE 32
#define BUFFER_LENGTH 32768
using namespace torch::indexing;

class RAVE : public juce::ChangeBroadcaster
{
public:
    RAVE()
        : juce::ChangeBroadcaster()
    {
        torch::jit::getProfilingMode() = false;
        c10::InferenceMode guard;
        torch::jit::setGraphExecutorOptimize (true);
    }

    void load_model (const std::string& rave_model_file)
    {
        try
        {
            c10::InferenceMode guard;
            this->model = torch::jit::load (rave_model_file);
        }
        catch (const c10::Error& e)
        {
            std::cerr << e.what();
            std::cerr << e.msg();
            std::cerr << "error loading the model\n";
            return;
        }

        this->model_path = juce::String (rave_model_file);
        auto named_buffers = this->model.named_buffers();
        auto named_attributes = this->model.named_attributes();
        this->has_prior = false;
        this->prior_params = torch::zeros ({ 0 });

        std::cout << "[ ] RAVE - Model successfully loaded: " << rave_model_file
                  << std::endl;

        bool found_model_as_attribute = false;
        bool found_stereo_attribute = false;
        for (const auto& attr : named_attributes)
        {
            if (attr.name == "_rave")
            {
                found_model_as_attribute = true;
                std::cout << "Found _rave model as named attribute" << std::endl;
            }
            else if (attr.name == "stereo" || attr.name == "_rave.stereo")
            {
                found_stereo_attribute = true;
                stereo = attr.value.toBool();
                std::cout << "Stereo?" << (stereo ? "true" : "false") << std::endl;
            }
        }

        if (! found_stereo_attribute)
        {
            stereo = false;
        }

        if (found_model_as_attribute)
        {
            // Use named buffers within _rave
            for (const auto& buf : named_buffers)
            {
                if (buf.name == "_rave.sampling_rate")
                {
                    this->sr = buf.value.item<int>();
                    std::cout << "\tSampling rate: " << this->sr << std::endl;
                }
                else if (buf.name == "_rave.latent_size")
                {
                    this->latent_size = buf.value.item<int>();
                    std::cout << "\tLatent size: " << this->latent_size << std::endl;
                }
                else if (buf.name == "encode_params")
                {
                    this->encode_params = buf.value;
                    std::cout << "\tEncode parameters: " << this->encode_params
                              << std::endl;
                }
                else if (buf.name == "decode_params")
                {
                    this->decode_params = buf.value;
                    std::cout << "\tDecode parameters: " << this->decode_params
                              << std::endl;
                }
                else if (buf.name == "prior_params")
                {
                    this->prior_params = buf.value;
                    this->has_prior = true;
                    std::cout << "\tPrior parameters: " << this->prior_params << std::endl;
                }
            }
        }
        else
        {
            // Use top-level named attributes
            for (const auto& attr : named_attributes)
            {
                if (attr.name == "sampling_rate")
                {
                    this->sr = attr.value.toInt();
                    std::cout << "\tSampling rate: " << this->sr << std::endl;
                }
                else if (attr.name == "full_latent_size")
                {
                    this->latent_size = attr.value.toInt();
                    std::cout << "\tLatent size: " << this->latent_size << std::endl;
                }
                else if (attr.name == "encode_params")
                {
                    this->encode_params = attr.value.toTensor();
                    std::cout << "\tEncode parameters: " << this->encode_params
                              << std::endl;
                }
                else if (attr.name == "decode_params")
                {
                    this->decode_params = attr.value.toTensor();
                    std::cout << "\tDecode parameters: " << this->decode_params
                              << std::endl;
                }
                else if (attr.name == "prior_params")
                {
                    this->prior_params = attr.value.toTensor();
                    this->has_prior = true;
                    std::cout << "\tPrior parameters: " << this->prior_params << std::endl;
                }
            }
        }

        std::cout << "\tFull latent size: " << getFullLatentDimensions()
                  << std::endl;
        std::cout << "\tRatio: " << getModelRatio() << std::endl;
        c10::InferenceMode guard;
        inputs_rave.clear();
        inputs_rave.push_back (torch::ones ({ 1, 1, getModelRatio() }));
        resetLatentBuffer();
        sendChangeMessage();
    }

    torch::Tensor sample_prior (const int n_steps, const float temperature)
    {
        c10::InferenceMode guard;
        inputs_rave[0] = torch::ones ({ 1, 1, n_steps }) * temperature;
        torch::Tensor prior =
            this->model.get_method ("prior") (inputs_rave).toTensor();
        return prior;
    }

    torch::Tensor encode (const torch::Tensor input)
    {
        c10::InferenceMode guard;
        inputs_rave[0] = input;
        auto y = this->model.get_method ("encode") (inputs_rave).toTensor();
        return y;
    }

    std::vector<torch::Tensor> encode_amortized (const torch::Tensor input)
    {
        c10::InferenceMode guard;
        inputs_rave[0] = input;
        auto stats = this->model.get_method ("encode_amortized") (inputs_rave)
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
        auto y = this->model.get_method ("decode") (inputs_rave).toTensor();
        return y;
    }

    juce::Range<float> getValidBufferSizes()
    {
        return juce::Range<float> (getModelRatio(), BUFFER_LENGTH);
    }

    unsigned int getLatentDimensions()
    {
        int tmp = decode_params.index ({ 0 }).item<int>();
        assert (tmp >= 0);
        return (unsigned int) tmp;
    }

    unsigned int getEncodeChannels()
    {
        int tmp = encode_params.index ({ 0 }).item<int>();
        assert (tmp >= 0);
        return (unsigned int) tmp;
    }

    unsigned int getDecodeChannels()
    {
        int tmp = decode_params.index ({ 3 }).item<int>();
        assert (tmp >= 0);
        return (unsigned int) tmp;
    }

    int getModelRatio() { return encode_params.index ({ 3 }).item<int>(); }

    float zPerSeconds() { return encode_params.index ({ 3 }).item<float>() / sr; }

    int getFullLatentDimensions() { return latent_size; }

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
                { "...", Slice (-MAX_LATENT_BUFFER_SIZE, None, None) });
        }
    }

    bool hasPrior() { return has_prior; }

    bool isStereo() const { return stereo; }

    at::Tensor getLatentBuffer() { return latent_buffer; }

    bool hasMethod (const std::string& method_name) const
    {
        return this->model.find_method (method_name).has_value();
    }

private:
    torch::jit::Module model;
    int sr;
    int latent_size;
    bool has_prior = false;
    bool stereo = false;
    juce::String model_path;
    at::Tensor encode_params;
    at::Tensor decode_params;
    at::Tensor prior_params;
    at::Tensor latent_buffer;
    std::vector<torch::jit::IValue> inputs_rave;
    juce::Range<float> validBufferSizeRange;
};
