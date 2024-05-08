#pragma once 

#include "Base.h"
#include "../Parameters.h"

using namespace juce;

struct LadderGroup
{
    explicit LadderGroup (AudioProcessorParameterGroup& layout)
        : enabled (addToLayout<AudioParameterBool> (layout,
                                                    ParameterID { ID::ladderEnabled, 1 },
                                                    "Ladder",
                                                    false)),
            mode (addToLayout<AudioParameterChoice> (layout,
                                                    ParameterID { ID::ladderMode, 1 },
                                                    "Mode",
                                                    StringArray { "LP12", "LP24", "HP12", "HP24", "BP12", "BP24" },
                                                    1)),
            cutoff (addToLayout<Parameter> (layout,
                                            ParameterID { ID::ladderCutoff, 1 },
                                            "Frequency",
                                            NormalisableRange<float> (10.0f, 22000.0f, 0.0f, 0.25f),
                                            1000.0f,
                                            getHzAttributes())),
            resonance (addToLayout<Parameter> (layout,
                                                ParameterID { ID::ladderResonance, 1 },
                                                "Resonance",
                                                NormalisableRange<float> (0.0f, 100.0f),
                                                0.0f,
                                                getPercentageAttributes())),
            drive (addToLayout<Parameter> (layout,
                                            ParameterID { ID::ladderDrive, 1 },
                                            "Drive",
                                            NormalisableRange<float> (0.0f, 40.0f),
                                            0.0f,
                                            getDbAttributes())) {}

    AudioParameterBool& enabled;
    AudioParameterChoice& mode;
    Parameter& cutoff;
    Parameter& resonance;
    Parameter& drive;
};