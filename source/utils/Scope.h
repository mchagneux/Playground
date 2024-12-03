#pragma once

#include "./Misc.h"
#include "choc/containers/choc_SingleReaderSingleWriterFIFO.h"

struct Scope : public juce::Component
{
public:
    Scope()
    {
        fifo.reset (44100);
    }

    ~Scope()
    {
    }

private:
    choc::fifo::SingleReaderSingleWriterFIFO<SampleType> fifo;
};