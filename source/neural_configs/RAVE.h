
#include <anira/anira.h>
#include <anira/utils/InferenceBackend.h>

static std::vector<anira::ModelData> model_data_config = {
    { std::string ("E:\\audio_dev\\Playground\\models\\rave\\models\\sol_ordinario_fast.ts"), anira::InferenceBackend::LIBTORCH },

};

static std::vector<anira::TensorShape> tensor_shape_config = {
    { { { 1, 1, 1024 } }, { { 1, 1, 1024 } }, anira::InferenceBackend::LIBTORCH },

};

static anira::InferenceConfig RAVEConfig (
    model_data_config,
    tensor_shape_config,
    42.66f);
