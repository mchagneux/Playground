#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "CmajorDragArea.h"
#include "processors/CmajorStereoDSPEffect.h"
#include "processors/Distortion.h"
#include "processors/Ladder.h"
#include "processors/NNEngine.h"
#include "ui/Distortion.h"
#include "ui/Ladder.h"

// #include "ui/Cmajor.h"

//==============================================================================
class PlaygroundProcessor : public juce::AudioProcessor,
                            private juce::ValueTree::Listener {
public:
  //==============================================================================
  PlaygroundProcessor();
  ~PlaygroundProcessor() override;

  //==============================================================================
  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;

  bool isBusesLayoutSupported(const BusesLayout &layouts) const override;

  void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;
  using AudioProcessor::processBlock;

  //==============================================================================
  juce::AudioProcessorEditor *createEditor() override { return nullptr; }
  bool hasEditor() const override { return false; }

  //==============================================================================
  const juce::String getName() const override;

  bool acceptsMidi() const override;
  bool producesMidi() const override;
  bool isMidiEffect() const override;
  double getTailLengthSeconds() const override;

  //==============================================================================
  int getNumPrograms() override;
  int getCurrentProgram() override;
  void setCurrentProgram(int index) override;
  const juce::String getProgramName(int index) override;
  void changeProgramName(int index, const juce::String &newName) override;

  void valueTreePropertyChanged(ValueTree &, const Identifier &) final {
    requiresUpdate.store(true);
  }

  //==============================================================================
  void getStateInformation(juce::MemoryBlock &destData) override;
  void setStateInformation(const void *data, int sizeInBytes) override;
  void update();
  void reset() override;

  struct ParameterReferences {
    explicit ParameterReferences(
        AudioProcessorValueTreeState::ParameterLayout &layout)
        : distortion(addToLayout<AudioProcessorParameterGroup>(
              layout, "distortion", "Distortion", "|")),
          ladder(addToLayout<AudioProcessorParameterGroup>(layout, "ladder",
                                                           "Ladder", "|")),
          cmajor(addToLayout<AudioProcessorParameterGroup>(layout, "cmajor",
                                                           "Cmajor", "|")),
          nnEngine(addToLayout<AudioProcessorParameterGroup>(
              layout, "nnEngine", "Neural Network", "|")) {}

    DistortionProcessor::Parameters distortion;
    LadderProcessor::Parameters ladder;
    CmajorStereoDSPEffect::Processor::Parameters cmajor;
    NNEngine::Parameters nnEngine;
  };

  CmajorStereoDSPEffect::Processor &getCmajorDSPEffectProcessor() {
    return dsp::get<cmajorIndex>(chain);
  }

  const ParameterReferences &getParameterValues() const noexcept {
    return parameters;
  }

  bool applyUpdateToCmajorParams(std::function<bool(CmajorStereoDSPEffect::Processor::Parameters&)> updateFn){
    return updateFn(parameters.cmajor);//updateFn(paramNb); // parameters[i]->setPatchParam(params[i]) || changed;
  }


private:
  explicit PlaygroundProcessor(
      AudioProcessorValueTreeState::ParameterLayout layout)
      : AudioProcessor(BusesProperties()
                           .withInput("In", AudioChannelSet::stereo())
                           .withOutput("Out", AudioChannelSet::stereo())),
        parameters{layout}, apvts{*this, nullptr, "state", std::move(layout)} {
    apvts.state.addListener(this);
    getCmajorDSPEffectProcessor().setOwningMainProc(this);
  }

  std::atomic<bool> requiresUpdate{true};

  ParameterReferences parameters;
  juce::AudioProcessorValueTreeState apvts;

  using Chain =
      juce::dsp::ProcessorChain<DistortionProcessor, dsp::LadderFilter<float>,
                                CmajorStereoDSPEffect::Processor, NNEngine>;

  Chain chain;

  enum ProcessorIndices {
    distortionIndex,
    ladderIndex,
    cmajorIndex,
    nnEngineIndex
  };
  //==============================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlaygroundProcessor)
};

class PlaygroundEditor final : public AudioProcessorEditor {
public:
  explicit PlaygroundEditor(PlaygroundProcessor &p)
      : AudioProcessorEditor(&p), proc(p) {

    addAllAndMakeVisible(*this, distortionControls, ladderControls,
                         cmajorControls);

    setSize(1280, 720);
    setResizable(true, true);
  }

  //==============================================================================

  void resized() override {
    auto rect = getLocalBounds();
    auto distortionArea = rect.removeFromLeft(0.5 * getWidth());
    distortionControls.setBounds(distortionArea);
    auto ladderArea = rect.removeFromLeft(0.4 * getWidth());
    ladderControls.setBounds(ladderArea);
    cmajorControls.setBounds(rect);
  }

private:

  //==============================================================================
  PlaygroundProcessor &proc;
  DistortionControls distortionControls{*this,
                                        proc.getParameterValues().distortion};
  LadderControls ladderControls{*this, proc.getParameterValues().ladder};
  CmajorControls cmajorControls{proc.getCmajorDSPEffectProcessor()};
  //==============================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlaygroundEditor)
};

struct Playground final : public PlaygroundProcessor {
  AudioProcessorEditor *createEditor() override {
    return new PlaygroundEditor(*this);
  }

  bool hasEditor() const override { return true; }
};
