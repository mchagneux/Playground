#pragma once

#include <JuceHeader.h>
#include "./Components.h"
#include "./CircularBuffer.h"

struct VisualizationManager : private juce::Timer

{
public:
    VisualizationManager (CircularBuffer<float>& fifoToFillBufferFrom, juce::Array<AudioReactiveComponent*> componentsToManage)
        : fifo (fifoToFillBufferFrom)
        , managedComponents (componentsToManage)
    {
        startTimer (20);
    }

private:
    CircularBuffer<float>& fifo;
    juce::AudioBuffer<float> visualizationBuffer { 1, 1000 };
    juce::Array<AudioReactiveComponent*> managedComponents;

    void timerCallback() override
    {
        if (fifo.copyAvailableData (visualizationBuffer))
        {
            for (auto component : managedComponents)
            {
                if (component->drawnPrevious)
                {
                    component->updateFromBuffer (visualizationBuffer);
                    component->repaint();
                }
            }
        }
    }
};