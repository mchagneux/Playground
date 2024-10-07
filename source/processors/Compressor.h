#pragma once 

#include <JuceHeader.h>
#include "../utils/Parameters.h"
#include "../utils/Components.h"



struct Compressor : private juce::AudioProcessorParameter::Listener
{

public:
    Compressor(const CompressorParameters& p): parameters(p) 
    {
        parameters.attack.addListener(this);
        parameters.threshold.addListener(this);
        parameters.release.addListener(this);
        parameters.ratio.addListener(this);

    }
    ~Compressor() override
    {
        parameters.attack.removeListener(this);
        parameters.threshold.removeListener(this);
        parameters.release.removeListener(this);
        parameters.ratio.removeListener(this);
    }


    void prepare(const juce::dsp::ProcessSpec& spec) 
    {
        compressor.prepare(spec); 
    }

    template <typename Context>
    void process(Context& context)
    {
        if(requiresUpdate.load()) 
            update(); 

        compressor.process(context);
    }

    void update()
    {
        compressor.setThreshold (parameters.threshold.get());
        compressor.setRatio     (parameters.ratio.get());
        compressor.setAttack    (parameters.attack.get());
        compressor.setRelease   (parameters.release.get());
        requiresUpdate.store(false);
    }

    void reset()
    {
        compressor.reset();
    }

    const CompressorParameters& parameters; 

private: 
    void parameterValueChanged(int, float) override 
    {
        requiresUpdate.store(true);
    }

    void parameterGestureChanged(int, bool) override 
    {

    }

    juce::dsp::Compressor<SampleType> compressor; 
    std::atomic<bool> requiresUpdate = false; 

};


struct CompressorControls final : public juce::Component
{
    explicit CompressorControls (juce::AudioProcessorEditor& editor,
                                    const CompressorParameters& state)
        : toggle    (editor, state.enabled),
            threshold (editor, state.threshold),
            ratio     (editor, state.ratio),
            attack    (editor, state.attack),
            release   (editor, state.release)
    {
        addAllAndMakeVisible (*this, toggle, threshold, ratio, attack, release);
    }

    void resized() override
    {
        performLayout (getLocalBounds(), toggle, threshold, ratio, attack, release);
    }

    AttachedToggle toggle;
    AttachedSlider threshold, ratio, attack, release;
};