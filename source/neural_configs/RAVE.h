
#include <anira/anira.h>

inline anira::InferenceConfig RAVEConfig (
    "E:\\audio_dev\\Playground\\models\\rave\\models\\sol_ordinario_fast.ts", // Model path
    { 1, 1, 1024 },                                                           // Input shape
    { 1, 1, 1024 },
    42.66f // Maximum inference time in ms
);
