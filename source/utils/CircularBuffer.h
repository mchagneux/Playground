#pragma once

#include <JuceHeader.h>

template <typename Type>
struct CircularBuffer
{
public:
    CircularBuffer() {}

    ~CircularBuffer() = default;

    void addAudioData (const juce::AudioBuffer<Type>& bufferToCopyFrom)
    {
        if (abstractFifo.getFreeSpace() < bufferToCopyFrom.getNumSamples())
            return;

        int start1, block1, start2, block2;
        abstractFifo.prepareToWrite (bufferToCopyFrom.getNumSamples(), start1, block1, start2, block2);
        internalBuffer.copyFrom (0, start1, bufferToCopyFrom.getReadPointer (0), block1);
        if (block2 > 0)
            internalBuffer.copyFrom (0, start2, bufferToCopyFrom.getReadPointer (0, block1), block2);
        abstractFifo.finishedWrite (block1 + block2);
    }

    bool copyAvailableData (juce::AudioBuffer<Type>& bufferToCopyTo)
    {
        if (abstractFifo.getNumReady() >= bufferToCopyTo.getNumSamples())
        {
            int start1, block1, start2, block2;
            abstractFifo.prepareToRead (bufferToCopyTo.getNumSamples(), start1, block1, start2, block2);
            if (block1 > 0)
                bufferToCopyTo.copyFrom (0, 0, internalBuffer.getReadPointer (0, start1), block1);
            if (block2 > 0)
                bufferToCopyTo.copyFrom (0, block1, internalBuffer.getReadPointer (0, start2), block2);
            abstractFifo.finishedRead (bufferToCopyTo.getNumSamples());
            return true;
        }

        return false;
        // if (abstractFifo.getNumReady() < fft.getSize())
        //     waitForData.wait (100);
    }

    void setup (int fifoSize)
    {
        internalBuffer.setSize (1, fifoSize);
        abstractFifo.setTotalSize (fifoSize);

        // startThread();
    }

private:
    juce::AbstractFifo abstractFifo { 48000 };
    juce::AudioBuffer<Type> internalBuffer { 1, 48000 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CircularBuffer)
};