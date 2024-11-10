#include <anira/anira.h>

static anira::InferenceConfig hybridNNConfig (
    MODELS_PATH + std::string ("/model_0/GuitarLSTM-dynamic.pt"),
    { 256, 1, 150 },
    { 256, 1 },

    5.33f,
    0,
    false,
    0.f,
    false);