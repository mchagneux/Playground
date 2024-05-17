#pragma once

#include "cmajor/helpers/cmaj_Patch.h"
#include <cstdint>
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <choc/audio/choc_AudioMIDIBlockDispatcher.h>

#include "../Parameters.h"
using namespace juce;

namespace CmajorStereoDSPEffect{

   struct Parameters{
        explicit Parameters (AudioProcessorParameterGroup& layout): enabled (addToLayout<AudioParameterBool> (layout,
                                                        ParameterID { ID::cmajorEnabled, 1 },
                                                        "Cmajor",
                                                        true)) {}
        AudioParameterBool& enabled;

    };  

    struct Processor: private juce::MessageListener{

        struct DummyWorkerContext:public cmaj::Patch::WorkerContext{

            void initialise(std::function<void (const choc::value::ValueView &)> sendMessage, std::function<void (const std::string &)> reportError) override {}
            void sendMessage(const std::string &json, std::function<void (const std::string &)> reportError) override {}
        };

        public: 



            Processor(): dllLoadedSuccessfully (initialiseDLL())  //parameters(state) // probably shouldn't take the state as something that can be modified
            { 

                if (! dllLoadedSuccessfully)
                {
                    setStatusMessage ("Could not load the required Cmajor DLL", true);
                    return;
                }


                patch = std::make_shared<cmaj::Patch>();
                patch->setAutoRebuildOnFileChange (true);
                patch->createEngine = +[] { return cmaj::Engine::create(); };    
                patch->stopPlayback  = [] {};//parameters.enabled = false; }; // might need to do a launch async thing
                patch->startPlayback = [] {};//parameters.enabled = true; }; // might need to do a launch async thing
                patch->patchChanged = [this]
                {
                    const auto executeOrDeferToMessageThread = [] (auto&& fn) -> void
                    {
                        if (juce::MessageManager::getInstance()->isThisTheMessageThread())
                            return fn();

                        juce::MessageManager::callAsync (std::forward<decltype (fn)> (fn));
                    };

                    executeOrDeferToMessageThread ([this] { handlePatchChange(); });
                };

                patch->statusChanged = [this] (const auto& s) { setStatusMessage (s.statusMessage, s.messageList.hasErrors()); };

                patch->handleOutputEvent = [this] (uint64_t frame, std::string_view endpointID, const choc::value::ValueView& v)
                {
                    handleOutputEvent (frame, endpointID, v);
                };
                
                patch->createContextForPatchWorker = []{return std::make_unique<DummyWorkerContext>();};
            }

            ~Processor() override
            {
                patch->patchChanged = [] {};
                patch->unload();
                patch.reset();
            }       

            void loadPatch (const std::filesystem::path& fileToLoad)
            {
                setNewStateAsync (createEmptyState (fileToLoad));
            }

            void loadPatch (const cmaj::PatchManifest& manifest)
            {
                if (dllLoadedSuccessfully)
                {
                    cmaj::Patch::LoadParams loadParams;
                    loadParams.manifest = manifest;
                    patch->loadPatch (loadParams, false);
                }
            }

            bool prepareManifest (cmaj::Patch::LoadParams& loadParams, const juce::ValueTree& newState)
            {
                if (! newState.isValid())

                    return false;

                auto location = newState.getProperty (ids.location).toString().toStdString();


                if (location.empty())

                    return false;

                loadParams.manifest.initialiseWithFile (location);

                if (! patch->isLoaded() || loadParams.manifest.manifestFile == patch->getPatchFile())
                    readParametersFromState (loadParams, newState);
                return true;
            }
            


            void setNewStateAsync (juce::ValueTree&& newState)
            {
                auto m = std::make_unique<NewStateMessage>();
                m->newState = std::move (newState);
                postMessage (m.release());
            }
            //==============================================================================
            juce::ValueTree createEmptyState (std::filesystem::path location) const
            {
                juce::ValueTree state (ids.Cmajor);

                state.setProperty (ids.location, juce::String (location.string()), nullptr);

                return state;
            }


            void unload()
            {
                unload ({}, false);
            }



            void prepare (const dsp::ProcessSpec& spec)
            {
                applyRateAndBlockSize (spec.sampleRate, static_cast<uint32_t> (spec.maximumBlockSize), static_cast<uint32_t> (spec.numChannels));

            }

            void reset()
            {

            }

            // template <typename Context>
            void process (dsp::ProcessContextReplacing<float>& context)
            {

                if (! patch->isPlayable() || context.isBypassed)
                {
                    return;
                }
                auto&& outputBlock = context.getOutputBlock();
                const auto numChannels = outputBlock.getNumChannels();
                const auto numSamples  = outputBlock.getNumSamples();
                auto numFrames = static_cast<choc::buffer::FrameCount> (numSamples);
                


                float * outputChannels [numChannels];
                
                for (size_t channelNb = 0; channelNb < numChannels; ++channelNb){
                    outputChannels[channelNb] = outputBlock.getChannelPointer(channelNb);

                } 
                
                // auto audioOutput = choc::buffer::createChannelArrayView (outputChannels, numOutputChannels, numSamples);

                choc::span<choc::midi::ShortMessage> midiMessages;
                patch->process(outputChannels, numFrames, [&] (uint32_t frame, choc::midi::ShortMessage m){});
                // patch->process ({audioInput, audioOutput, midiMessages, [&] (uint32_t frame, choc::midi::ShortMessage m){}}, true);
// 
            }




            std::shared_ptr<cmaj::Patch> patch;
            std::string statusMessage;
            bool isStatusMessageError = false;
            bool dllLoadedSuccessfully = true;


        // protected: 

        private: 



            std::function<void(CmajorStereoDSPEffect::Processor&)> patchChangeCallback;
            std::function<void(const char*)> handleConsoleMessage;


            static bool initialiseDLL()
            {
                if constexpr (cmaj::Library::isUsingDLL)
                {
                    static bool initialised = false;

                    if (initialised)
                        return true;

                    auto tryLoading = [&] (const juce::File& dll)
                    {
                        if (dll.existsAsFile())
                            initialised = cmaj::Library::initialise (dll.getFullPathName().toStdString());

                        return initialised;
                    };

                    auto exe = juce::File::getSpecialLocation (juce::File::currentExecutableFile);
                    auto dllName = cmaj::Library::getDLLName();

                #if CHOC_OSX
                    auto bundleFolder = juce::File::getSpecialLocation (juce::File::currentApplicationFile);

                    return tryLoading (bundleFolder.getChildFile ("Contents/Resources").getChildFile (dllName))
                                || tryLoading (exe.getSiblingFile (dllName))
                                || tryLoading (bundleFolder.getSiblingFile (dllName));
                #else
                    return tryLoading (exe.getSiblingFile (dllName));
                #endif
                }
                else
                {
                    return true;
                }
            }

            void handleOutputEvent (uint64_t, std::string_view endpointID, const choc::value::ValueView& value)
            {
                if (endpointID == cmaj::getConsoleEndpointID())
                {
                    auto text = cmaj::convertConsoleMessageToString (value);

                    if (handleConsoleMessage != nullptr)
                        handleConsoleMessage (text.c_str());
                    else
                        std::cout << text << std::flush;
                }
            }

            void handlePatchChange()
            {
                // auto changes = AudioProcessorListener::ChangeDetails::getDefaultFlags();

                // auto newLatency = (int) patch->getFramesLatency();

                // changes.latencyChanged           = newLatency != getLatencySamples();
                // changes.parameterInfoChanged     = updateParameters();
                // changes.programChanged           = false;
                // changes.nonParameterStateChanged = true;

                // setLatencySamples (newLatency);
                // notifyEditorPatchChanged();
                // updateHostDisplay (changes);

                if (patchChangeCallback)
                    patchChangeCallback (static_cast<CmajorStereoDSPEffect::Processor&> (*this));
            }
            
            struct NewStateMessage  : public juce::Message
            {
                juce::ValueTree newState;
            };

            void handleMessage (const juce::Message& message) override
            {
                if (auto m = dynamic_cast<const NewStateMessage*> (&message))
                    setNewState (const_cast<NewStateMessage*> (m)->newState);
            }


            void readParametersFromState (cmaj::Patch::LoadParams& loadParams, const juce::ValueTree& newState) const
            {
                if (auto params = newState.getChildWithName (ids.PARAMS); params.isValid())
                    for (auto param : params)
                        if (auto endpointIDProp = param.getPropertyPointer (ids.ID))
                            if (auto endpointID = endpointIDProp->toString().toStdString(); ! endpointID.empty())
                                if (auto valProp = param.getPropertyPointer (ids.V))
                                    loadParams.parameterValues[endpointID] = static_cast<float> (*valProp);
            }

            void setStatusMessage (const std::string& newMessage, bool isError)
            {
                if (statusMessage != newMessage || isStatusMessageError != isError)
                {
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

            void setNewState (const juce::ValueTree& newState){


                if (newState.isValid() && ! newState.hasType (ids.Cmajor))
                    return unload ("Failed to load: invalid state", true);

                cmaj::Patch::LoadParams loadParams;

                try
                {
                    if (! prepareManifest (loadParams, newState))
                        return unload();
                }
                catch (const std::runtime_error& e)
                {
                    return unload (e.what(), true);
                }

                if (isViewResizable())
                {
                    if (auto w = newState.getPropertyPointer (ids.viewWidth))
                        if (w->isInt())
                            lastEditorWidth = *w;

                    if (auto h = newState.getPropertyPointer (ids.viewHeight))
                        if (h->isInt())
                            lastEditorHeight = *h;
                }
                else
                {
                    lastEditorWidth = 0;
                    lastEditorHeight = 0;
                }

                if (auto state = newState.getChildWithName (ids.STATE); state.isValid())
                {
                    for (const auto& v : state)
                    {
                        if (v.hasType (ids.VALUE))
                        {
                            if (auto key = v.getPropertyPointer (ids.key))
                            {
                                if (auto value = v.getPropertyPointer (ids.value))
                                {
                                    if (key->isString() && key->toString().isNotEmpty() && ! value->isVoid())
                                        patch->setStoredStateValue (key->toString().toStdString(), convertVarToValue (*value));
                                }
                            }
                        }
                    }
                }

                // if (getSampleRate() > 0)
                //     applyCurrentRateAndBlockSize();
                patch->loadPatch (loadParams, true);
            }


            void unload (const std::string& message, bool isError)
            {
                // if constexpr (! DerivedType::isPrecompiled)
                // {
                    patch->unload();
                    setStatusMessage (message, isError);
                // }
            }

            cmaj::Patch::PlaybackParams getPlaybackParams (double rate, uint32_t requestedBlockSize, uint32_t numChannels)
            {

                return cmaj::Patch::PlaybackParams (rate, requestedBlockSize,
                                            static_cast<choc::buffer::ChannelCount> (numChannels),
                                            static_cast<choc::buffer::ChannelCount> (numChannels));
            }

            void applyRateAndBlockSize (double sampleRate, uint32_t samplesPerBlock, uint32_t numChannels)
            {
                patch->setPlaybackParams (getPlaybackParams (sampleRate, samplesPerBlock, numChannels));
            }


            bool isViewResizable() const
            {
                if (auto manifest = patch->getManifest())
                    for (auto& v : manifest->views)
                        if (! v.isResizable())
                            return false;

                return true;
            }


            static choc::value::Value convertVarToValue (const juce::var& v)
            {
                if (v.isVoid() || v.isUndefined())  return {};
                if (v.isString())                   return choc::value::createString (v.toString().toStdString());
                if (v.isBool())                     return choc::value::createBool (static_cast<bool> (v));
                if (v.isInt() || v.isInt64())       return choc::value::createInt64 (static_cast<juce::int64> (v));
                if (v.isDouble())                   return choc::value::createFloat64 (static_cast<double> (v));

                if (v.isArray())
                {
                    auto a = choc::value::createEmptyArray();

                    for (auto& i : *v.getArray())
                        a.addArrayElement (convertVarToValue (i));
                }

                if (v.isObject())
                {
                    auto json = juce::JSON::toString (v, juce::JSON::FormatOptions().withSpacing (juce::JSON::Spacing::none));
                    return choc::json::parse (json.toStdString());
                }

                if (v.isBinaryData())
                {
                    auto* block = v.getBinaryData();
                    auto  inputData = choc::value::InputData { (unsigned char *) block->begin(), (unsigned char *) block->end() };
                    return choc::value::Value::deserialise (inputData);
                }

                jassertfalse;
                return {};
            }

            // std::shared_ptr<cmaj::Patch> patch;

            int lastEditorWidth = 0, lastEditorHeight = 0;

            // Parameters& parameters; 
            struct IDs
            {
                const juce::Identifier Cmajor     { "Cmajor" },
                                    PARAMS     { "PARAMS" },
                                    PARAM      { "PARAM" },
                                    ID         { "ID" },
                                    V          { "V" },
                                    STATE      { "STATE" },
                                    VALUE      { "VALUE" },
                                    location   { "location" },
                                    key        { "key" },
                                    value      { "value" },
                                    viewWidth  { "viewWidth" },
                                    viewHeight { "viewHeight" };
            } ids;

    };
};
