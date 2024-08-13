#pragma once

#include "cmajor/helpers/cmaj_Patch.h"
#include <choc/audio/choc_AudioMIDIBlockDispatcher.h>
#include <cstdint>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <memory>
#include "../Parameters.h"
using namespace juce;


namespace CmajorStereoDSPEffect {

struct Processor : private juce::MessageListener {

  struct DummyWorkerContext : public cmaj::Patch::WorkerContext {

    void initialise(std::function<void(const choc::value::ValueView &)> sendMessage,
               std::function<void(const std::string &)> reportError) override {}
    void sendMessage(const std::string &json,
                std::function<void(const std::string &)> reportError) override {
    }
  };

  Processor() // parameters(state) // probably shouldn't take the state as
              // something that can be modified
  {

    patch = std::make_shared<cmaj::Patch>();
    patch->setAutoRebuildOnFileChange(true);
    patch->createEngine = +[] { return cmaj::Engine::create(); };
    patch->stopPlayback = [] {}; // parameters.enabled = false; }; // might need
                                 // to do a launch async thing
    patch->startPlayback = [] {}; // parameters.enabled = true; }; // might need to do a launch async thing
    patch->patchChanged = [this] {
      const auto executeOrDeferToMessageThread = [](auto &&fn) -> void {
        if (juce::MessageManager::getInstance()->isThisTheMessageThread())
          return fn();

        juce::MessageManager::callAsync(std::forward<decltype(fn)>(fn));
      };

      executeOrDeferToMessageThread([this] { handlePatchChange(); });
    };

    patch->statusChanged = [this](const auto &s) {
      setStatusMessage(s.statusMessage, s.messageList.hasErrors());
    };

    patch->handleOutputEvent = [this](uint64_t frame,
                                      std::string_view endpointID,
                                      const choc::value::ValueView &v) {
      handleOutputEvent(frame, endpointID, v);
    };

    patch->createContextForPatchWorker = [] {
      return std::make_unique<DummyWorkerContext>();
    };
  }

  ~Processor() override {
    patch->patchChanged = [] {};
    patch->unload();
    patch.reset();
  }

  void loadPatch(const std::filesystem::path &fileToLoad) {
    setNewStateAsync(createEmptyState(fileToLoad));
  }

  void loadPatch(const cmaj::PatchManifest &manifest) {

    cmaj::Patch::LoadParams loadParams;
    loadParams.manifest = manifest;
    patch->loadPatch(loadParams, false);
  }

  bool prepareManifest(cmaj::Patch::LoadParams &loadParams,
                       const juce::ValueTree &newState) {
    if (!newState.isValid()) {

      return false;
    }

    auto location = newState.getProperty(ids.location).toString().toStdString();

    if (location.empty()) {
      return false;
    }

    loadParams.manifest.initialiseWithFile(location);

    if (!patch->isLoaded() ||
        loadParams.manifest.manifestFile == patch->getPatchFile()) {
      readParametersFromState(loadParams, newState);
    }

    return true;
  }

  void setNewStateAsync(juce::ValueTree &&newState) {
    auto m = std::make_unique<NewStateMessage>();
    m->newState = std::move(newState);
    postMessage(m.release());
  }
  //==============================================================================
  juce::ValueTree createEmptyState(std::filesystem::path location) const {
    juce::ValueTree state(ids.Cmajor);

    state.setProperty(ids.location, juce::String(location.string()), nullptr);

    return state;
  }

  void unload() { unload({}, false); }

  void prepare(const dsp::ProcessSpec &spec) {
    applyRateAndBlockSize(spec.sampleRate,
                          static_cast<uint32_t>(spec.maximumBlockSize),
                          static_cast<uint32_t>(spec.numChannels));
  }

  void reset() {}

  // template <typename Context>
  void process(dsp::ProcessContextReplacing<float> &context) {

    if (!patch->isPlayable() || context.isBypassed) {
      return;
    }
    auto &&outputBlock = context.getOutputBlock();
    const auto numChannels = outputBlock.getNumChannels();
    const auto numSamples = outputBlock.getNumSamples();
    auto numFrames = static_cast<choc::buffer::FrameCount>(numSamples);

    float *outputChannels[numChannels];

    for (size_t channelNb = 0; channelNb < numChannels; ++channelNb) {
      outputChannels[channelNb] = outputBlock.getChannelPointer(channelNb);
    }

    // auto audioOutput = choc::buffer::createChannelArrayView (outputChannels,
    // numOutputChannels, numSamples);

    choc::span<choc::midi::ShortMessage> midiMessages;
    patch->process(outputChannels, numFrames,
                   [&](uint32_t frame, choc::midi::ShortMessage m) {});
    // patch->process ({audioInput, audioOutput, midiMessages, [&] (uint32_t
    // frame, choc::midi::ShortMessage m){}}, true);
    //
  }

  void setOwningMainProc(juce::AudioProcessor *mainProcIn) {
    mainProc = mainProcIn;
  }

  struct Parameter : public juce::RangedAudioParameter {

    Parameter (juce::String &&pID): juce::RangedAudioParameter (pID, "Param" + pID) {
      range = std::make_unique<juce::NormalisableRange<float>>(); 
    }

    ~Parameter() override { detach(); }


    const NormalisableRange<float> & getNormalisableRange() const override {
      return *range;
    }

    bool setPatchParam(cmaj::PatchParameterPtr p) {
      if (patchParam == p)
        return false;

      detach();
      patchParam = std::move(p);

      patchParam->valueChanged = [this](float v) {
        sendValueChangedMessageToListeners(
            patchParam->properties.convertTo0to1(v));
      };

      patchParam->gestureStart = [this] { beginChangeGesture(); };
      patchParam->gestureEnd = [this] { endChangeGesture(); };
      return true;
    }

    void detach() {
      if (patchParam != nullptr) {
        patchParam->valueChanged = [](float) {};
        patchParam->gestureStart = [] {};
        patchParam->gestureEnd = [] {};
      }
    }

    void forceValueChanged() {
      if (patchParam != nullptr)
        patchParam->valueChanged(patchParam->currentValue);
    }

    // juce::String getParameterID() const { return paramID; }
    juce::String getName(int maxLength) const override {
      return patchParam == nullptr
                 ? "unknown"
                 : patchParam->properties.name.substr(0, (size_t)maxLength);
    }
    juce::String getLabel() const override {
      return patchParam == nullptr ? juce::String()
                                   : patchParam->properties.unit;
    }
    Category getCategory() const override { return Category::genericParameter; }
    bool isDiscrete() const override {
      return patchParam != nullptr && patchParam->properties.discrete;
    }
    bool isBoolean() const override {
      return patchParam != nullptr && patchParam->properties.boolean;
    }
    bool isAutomatable() const override {
      return patchParam == nullptr || patchParam->properties.automatable;
    }
    bool isMetaParameter() const override {
      return patchParam != nullptr && patchParam->properties.hidden;
    }

    juce::StringArray getAllValueStrings() const override {
      juce::StringArray result;

      if (patchParam != nullptr)
        for (auto &s : patchParam->properties.valueStrings)
          result.add(s);

      return result;
    }

    float getDefaultValue() const override {
      return patchParam != nullptr ? patchParam->properties.convertTo0to1(
                                         patchParam->properties.defaultValue)
                                   : 0.0f;
    }
    float getValue() const override {
      return patchParam != nullptr ? patchParam->properties.convertTo0to1(
                                         patchParam->currentValue)
                                   : 0.0f;
    }
    void setValue(float newValue) override {
      if (patchParam != nullptr)
        patchParam->setValue(patchParam->properties.convertFrom0to1(newValue),
                             false, -1, 0);
    }

    juce::String getText(float v, int length) const override {
      if (patchParam == nullptr)
        return "0";

      juce::String result = patchParam->properties.getValueAsString(
          patchParam->properties.convertFrom0to1(v));
      return length > 0 ? result.substring(0, length) : result;
    }

    float getValueForText(const juce::String &text) const override {
      if (patchParam != nullptr) {
        if (auto value =
                patchParam->properties.getStringAsValue(text.toStdString()))
          return *value;

        return patchParam->properties.defaultValue;
      }

      return 0;
    }

    int getNumSteps() const override {
      if (patchParam != nullptr)
        if (auto steps = patchParam->properties.getNumDiscreteOptions())
          return static_cast<int>(steps);

      return AudioProcessor::getDefaultNumParameterSteps();
    }

    cmaj::PatchParameterPtr patchParam;
    std::unique_ptr<juce::NormalisableRange<float>> range; 
  };


  struct Parameters {
    explicit Parameters(AudioProcessorParameterGroup &layout)
        : enabled(addToLayout<AudioParameterBool>(
              layout, ParameterID{ID::cmajorEnabled, 1}, "Cmajor", false)) {
      while (parameters.size() < 100) {
        Parameter * p = new Parameter("P" + juce::String(parameters.size()));
        parameters.push_back(p);
        add(layout, rawToUniquePtr(p));
      }
    }
    AudioParameterBool &enabled;
    std::vector<Parameter*> parameters;

  };

  void handleOutputEvent(uint64_t, std::string_view endpointID,
                         const choc::value::ValueView &value) {
    if (endpointID == cmaj::getConsoleEndpointID()) {
      auto text = cmaj::convertConsoleMessageToString(value);

      if (handleConsoleMessage != nullptr)
        handleConsoleMessage(text.c_str());
      else
        std::cout << text << std::flush;
    }
  }

  void handlePatchChange() {
    auto changes = AudioProcessorListener::ChangeDetails::getDefaultFlags();

    // auto newLatency = (int) patch->getFramesLatency();

    changes.latencyChanged = false; // newLatency != getLatencySamples();
    changes.parameterInfoChanged = updateParameters();
    changes.programChanged = false;
    changes.nonParameterStateChanged = true;

    // setLatencySamples (newLatency);
    // notifyEditorPatchChanged();
    mainProc->updateHostDisplay(changes);

    if (patchChangeCallback)
      patchChangeCallback(
          static_cast<CmajorStereoDSPEffect::Processor &>(*this));
  }

  struct NewStateMessage : public juce::Message {
    juce::ValueTree newState;
  };

  void handleMessage(const juce::Message &message) override {
    if (auto m = dynamic_cast<const NewStateMessage *>(&message)) {
      setNewState(const_cast<NewStateMessage *>(m)->newState);
    }
  }

  void readParametersFromState(cmaj::Patch::LoadParams &loadParams,
                               const juce::ValueTree &newState) const {
    if (auto params = newState.getChildWithName(ids.PARAMS); params.isValid())
      for (auto param : params)
        if (auto endpointIDProp = param.getPropertyPointer(ids.ID))
          if (auto endpointID = endpointIDProp->toString().toStdString();
              !endpointID.empty())
            if (auto valProp = param.getPropertyPointer(ids.V))
              loadParams.parameterValues[endpointID] =
                  static_cast<float>(*valProp);
  }

  void setStatusMessage(const std::string &newMessage, bool isError) {
    if (statusMessage != newMessage || isStatusMessageError != isError) {
      std::cout << newMessage << std::endl;
      statusMessage = newMessage;
      isStatusMessageError = isError;
      // notifyEditorStatusMessageChanged();
    }
  }

  // void notifyEditorStatusMessageChanged()
  // {
  //     if (auto e = dynamic_cast<Editor*> (getActiveEditor()))
  //         e->statusMessageChanged();
  // }

  void setNewState(const juce::ValueTree &newState) {

    if (newState.isValid() && !newState.hasType(ids.Cmajor))
      return unload("Failed to load: invalid state", true);

    cmaj::Patch::LoadParams loadParams;

    try {
      if (!prepareManifest(loadParams, newState))
        return unload();
    } catch (const std::runtime_error &e) {
      return unload(e.what(), true);
    }

    if (isViewResizable()) {
      if (auto w = newState.getPropertyPointer(ids.viewWidth))
        if (w->isInt())
          lastEditorWidth = *w;

      if (auto h = newState.getPropertyPointer(ids.viewHeight))
        if (h->isInt())
          lastEditorHeight = *h;
    } else {
      lastEditorWidth = 0;
      lastEditorHeight = 0;
    }

    if (auto state = newState.getChildWithName(ids.STATE); state.isValid()) {
      for (const auto &v : state) {
        if (v.hasType(ids.VALUE)) {
          if (auto key = v.getPropertyPointer(ids.key)) {
            if (auto value = v.getPropertyPointer(ids.value)) {
              if (key->isString() && key->toString().isNotEmpty() &&
                  !value->isVoid())
                patch->setStoredStateValue(key->toString().toStdString(),
                                           convertVarToValue(*value));
            }
          }
        }
      }
    }

    // if (getSampleRate() > 0)
    //     applyCurrentRateAndBlockSize();
    patch->loadPatch(loadParams, true);
  }

  juce::ValueTree getUpdatedState() {
    auto state = createEmptyState(patch->getManifestFile());

    if (isViewResizable() && lastEditorWidth != 0 && lastEditorHeight != 0) {
      state.setProperty(ids.viewWidth, lastEditorWidth, nullptr);
      state.setProperty(ids.viewHeight, lastEditorHeight, nullptr);
    }

    if (const auto &values = patch->getStoredStateValues(); !values.empty()) {
      juce::ValueTree stateValues(ids.STATE);

      for (auto &v : values) {
        juce::ValueTree value(ids.VALUE);
        value.setProperty(
            ids.key, juce::String(v.first.data(), v.first.length()), nullptr);
        auto serialised = v.second.serialise();
        value.setProperty(
            ids.value,
            juce::var(serialised.data.data(), serialised.data.size()), nullptr);
        stateValues.appendChild(value, nullptr);
      }

      state.appendChild(stateValues, nullptr);
    }

    juce::ValueTree paramList(ids.PARAMS);

    for (auto &p : patch->getParameterList())
      paramList.appendChild(
          juce::ValueTree(ids.PARAM,
                          {{ids.ID, juce::String(p->properties.endpointID)},
                           {ids.V, p->currentValue}}),
          nullptr);

    state.appendChild(paramList, nullptr);
    return state;
  }

  void unload(const std::string &message, bool isError) {
    // if constexpr (! DerivedType::isPrecompiled)
    // {
    patch->unload();
    setStatusMessage(message, isError);
    // }
  }

  cmaj::Patch::PlaybackParams getPlaybackParams(double rate,
                                                uint32_t requestedBlockSize,
                                                uint32_t numChannels) {

    return cmaj::Patch::PlaybackParams(
        rate, requestedBlockSize,
        static_cast<choc::buffer::ChannelCount>(numChannels),
        static_cast<choc::buffer::ChannelCount>(numChannels));
  }

  void applyRateAndBlockSize(double sampleRate, uint32_t samplesPerBlock,
                             uint32_t numChannels) {
    patch->setPlaybackParams(
        getPlaybackParams(sampleRate, samplesPerBlock, numChannels));
  }

  bool isViewResizable() const {
    if (auto manifest = patch->getManifest())
      for (auto &v : manifest->views)
        if (!v.isResizable())
          return false;

    return true;
  }

  static choc::value::Value convertVarToValue(const juce::var &v) {
    if (v.isVoid() || v.isUndefined())
      return {};
    if (v.isString())
      return choc::value::createString(v.toString().toStdString());
    if (v.isBool())
      return choc::value::createBool(static_cast<bool>(v));
    if (v.isInt() || v.isInt64())
      return choc::value::createInt64(static_cast<juce::int64>(v));
    if (v.isDouble())
      return choc::value::createFloat64(static_cast<double>(v));

    if (v.isArray()) {
      auto a = choc::value::createEmptyArray();

      for (auto &i : *v.getArray())
        a.addArrayElement(convertVarToValue(i));
    }

    if (v.isObject()) {
      auto json = juce::JSON::toString(
          v,
          juce::JSON::FormatOptions().withSpacing(juce::JSON::Spacing::none));
      return choc::json::parse(json.toStdString());
    }

    if (v.isBinaryData()) {
      auto *block = v.getBinaryData();
      auto inputData = choc::value::InputData{(unsigned char *)block->begin(),
                                              (unsigned char *)block->end()};
      return choc::value::Value::deserialise(inputData);
    }

    jassertfalse;
    return {};
  }

  void createParameterTree() {}

  bool updateParameters();


  std::shared_ptr<cmaj::Patch> patch;
  std::string statusMessage;
  bool isStatusMessageError = false;

  int lastEditorWidth = 0, lastEditorHeight = 0;
  juce::AudioProcessor * mainProc = nullptr;

  std::function<void(CmajorStereoDSPEffect::Processor &)> patchChangeCallback;
  std::function<void(const char *)> handleConsoleMessage;
  // Parameters& parameters;
  struct IDs {
    const juce::Identifier Cmajor{"Cmajor"}, PARAMS{"PARAMS"}, PARAM{"PARAM"},
        ID{"ID"}, V{"V"}, STATE{"STATE"}, VALUE{"VALUE"}, location{"location"},
        key{"key"}, value{"value"}, viewWidth{"viewWidth"},
        viewHeight{"viewHeight"};
  } ids;
};
}; // namespace CmajorStereoDSPEffect
