#include "./neural_configs/RAVE.h"
#include <anira/utils/InferenceBackend.h>

RAVEProcessor::RAVEProcessor (anira::InferenceConfig& inference_config)
    : BackendBase (inference_config)
{
    torch::set_num_threads (1);

    for (unsigned int i = 0; i < m_inference_config.m_num_parallel_processors; ++i)
    {
        m_instances.emplace_back (std::make_shared<Instance> (m_inference_config));
    }
}

RAVEProcessor::~RAVEProcessor()
{
}

void RAVEProcessor::prepare()
{
    for (auto& instance : m_instances)
    {
        instance->prepare();
    }
}

void RAVEProcessor::process (anira::AudioBufferF& input, anira::AudioBufferF& output, std::shared_ptr<anira::SessionElement> session)
{
    while (true)
    {
        for (auto& instance : m_instances)
        {
            if (! (instance->m_processing.exchange (true)))
            {
                instance->process (input, output, session);
                instance->m_processing.exchange (false);
                return;
            }
        }
    }
}

RAVEProcessor::Instance::Instance (anira::InferenceConfig& inference_config)
    : m_inference_config (inference_config)
{
    rave_model_file = inference_config.get_model_path (anira::LIBTORCH);
    try
    {
        m_module = torch::jit::load (m_inference_config.get_model_path (anira::InferenceBackend::LIBTORCH));
    }
    catch (const c10::Error& e)
    {
        std::cerr << "[ERROR] error loading the model\n";
        std::cerr << e.what() << std::endl;
    }
    m_inputs.resize (m_inference_config.m_input_sizes.size());
    m_input_data.resize (m_inference_config.m_input_sizes.size());
    for (size_t i = 0; i < m_inference_config.m_input_sizes.size(); i++)
    {
        m_input_data[i].resize (m_inference_config.m_input_sizes[i]);
        m_inputs[i] = torch::from_blob (m_input_data[i].data(), m_inference_config.get_input_shape (anira::InferenceBackend::LIBTORCH)[i]);
    }

    for (size_t i = 0; i < m_inference_config.m_warm_up; i++)
    {
        m_outputs = m_module.forward (m_inputs);
    }

    auto named_buffers = m_module.named_buffers();
    auto named_attributes = m_module.named_attributes();
    has_prior = false;
    prior_params = torch::zeros ({ 0 });

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
                sr = buf.value.item<int>();
                std::cout << "\tSampling rate: " << sr << std::endl;
            }
            else if (buf.name == "_rave.latent_size")
            {
                latent_size = buf.value.item<int>();
                std::cout << "\tLatent size: " << latent_size << std::endl;
            }
            else if (buf.name == "encode_params")
            {
                encode_params = buf.value;
                std::cout << "\tEncode parameters: " << encode_params
                          << std::endl;
            }
            else if (buf.name == "decode_params")
            {
                decode_params = buf.value;
                std::cout << "\tDecode parameters: " << decode_params
                          << std::endl;
            }
            else if (buf.name == "prior_params")
            {
                prior_params = buf.value;
                has_prior = true;
                std::cout << "\tPrior parameters: " << prior_params << std::endl;
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
                sr = attr.value.toInt();
                std::cout << "\tSampling rate: " << sr << std::endl;
            }
            else if (attr.name == "full_latent_size")
            {
                latent_size = attr.value.toInt();
                std::cout << "\tLatent size: " << latent_size << std::endl;
            }
            else if (attr.name == "encode_params")
            {
                encode_params = attr.value.toTensor();
                std::cout << "\tEncode parameters: " << encode_params
                          << std::endl;
            }
            else if (attr.name == "decode_params")
            {
                decode_params = attr.value.toTensor();
                std::cout << "\tDecode parameters: " << decode_params
                          << std::endl;
            }
            else if (attr.name == "prior_params")
            {
                prior_params = attr.value.toTensor();
                has_prior = true;
                std::cout << "\tPrior parameters: " << prior_params << std::endl;
            }
        }
    }

    std::cout << "\tFull latent size: " << getFullLatentDimensions()
              << std::endl;
    std::cout << "\tRatio: " << getModelRatio() << std::endl;
}

void RAVEProcessor::Instance::prepare()
{
    for (size_t i = 0; i < m_inference_config.m_input_sizes.size(); i++)
    {
        m_input_data[i].clear();
    }
}

void RAVEProcessor::Instance::process (anira::AudioBufferF& input, anira::AudioBufferF& output, std::shared_ptr<anira::SessionElement> session)
{
    for (size_t i = 0; i < m_inference_config.m_input_sizes.size(); i++)
    {
        if (i != m_inference_config.m_index_audio_data[anira::Input])
        {
            for (size_t j = 0; j < m_input_data[i].size(); j++)
            {
                m_input_data[i][j] = session->m_pp_processor.get_input (i, j);
            }
        }
        else
        {
            m_input_data[i].swap_data (input.get_memory_block());
            input.reset_channel_ptr();
        }
        // This is necessary because the tensor data pointers seem to change from inference to inference
        m_inputs[i] = torch::from_blob (m_input_data[i].data(), m_inference_config.get_input_shape (anira::InferenceBackend::LIBTORCH)[i]);
    }

    // Run inference
    m_outputs = m_module.forward (m_inputs);

    // We need to copy the data because we cannot access the data pointer ref of the tensor directly
    if (m_outputs.isTuple())
    {
        for (size_t i = 0; i < m_inference_config.m_output_sizes.size(); i++)
        {
            if (i != m_inference_config.m_index_audio_data[anira::Output])
            {
                for (size_t j = 0; j < m_inference_config.m_output_sizes[i]; j++)
                {
                    session->m_pp_processor.set_output (m_outputs.toTuple()->elements()[i].toTensor().view ({ -1 }).data_ptr<float>()[j], i, j);
                }
            }
            else
            {
                for (size_t j = 0; j < m_inference_config.m_output_sizes[i]; j++)
                {
                    output.get_memory_block()[j] = m_outputs.toTuple()->elements()[i].toTensor().view ({ -1 }).data_ptr<float>()[j];
                }
            }
        }
    }
    else if (m_outputs.isTensorList())
    {
        for (size_t i = 0; i < m_inference_config.m_output_sizes.size(); i++)
        {
            if (i != m_inference_config.m_index_audio_data[anira::Output])
            {
                for (size_t j = 0; j < m_inference_config.m_output_sizes[i]; j++)
                {
                    session->m_pp_processor.set_output (m_outputs.toTensorList().get (i).view ({ -1 }).data_ptr<float>()[j], i, j);
                }
            }
            else
            {
                for (size_t j = 0; j < m_inference_config.m_output_sizes[i]; j++)
                {
                    output.get_memory_block()[j] = m_outputs.toTensorList().get (i).view ({ -1 }).data_ptr<float>()[j];
                }
            }
        }
    }
    else if (m_outputs.isTensor())
    {
        for (size_t i = 0; i < m_inference_config.m_output_sizes[0]; i++)
        {
            output.get_memory_block()[i] = m_outputs.toTensor().view ({ -1 }).data_ptr<float>()[i];
        }
    }
}
