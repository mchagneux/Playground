#pragma once 
#include "CmajorProcessor.h"

struct CmajorLoaderUI : juce::Component {
    
public:
    CmajorLoaderUI(CmajorProcessor& c):cmajorProcessor(c){ 

        messageBox.setMultiLine (true);
        messageBox.setReadOnly (true);

        addAndMakeVisible(loadButton); 
        addAndMakeVisible(unloadButton);
        addAndMakeVisible(messageBox); 

        loadButton.onClick = [this] { openButtonClicked();};
        unloadButton.onClick = [this] { cmajorProcessor.unload(); };

    }

    bool isValidFile (const juce::String& file)
    {
        return file.endsWith (".cmajorpatch");
    }

        void refresh()
        {
            unloadButton.setVisible (cmajorProcessor.patch->isLoaded());

           #if JUCE_MAJOR_VERSION == 8
            juce::Font f (juce::FontOptions (18.0f));
           #else
            juce::Font f (18.0f);
           #endif

            f.setTypefaceName (juce::Font::getDefaultMonospacedFontName());
            messageBox.setFont (f);

            auto text = cmajorProcessor.statusMessage;

            if (text.empty())
                text = "Click the load button to search for Cmajor patch.";

            messageBox.setText (text);
        }

    void openButtonClicked()
    {
        chooser = std::make_unique<juce::FileChooser> ("Select a Wave file to play...",
                                                       juce::File{},
                                                       "*.cmajorpatch");

        auto chooserFlags = juce::FileBrowserComponent::openMode
                          | juce::FileBrowserComponent::canSelectFiles;

        chooser->launchAsync (chooserFlags, [this] (const juce::FileChooser& fc)
        {
            auto filename = fc.getResult().getFullPathName();
            if (isValidFile(filename)) {
                cmajorProcessor.loadPatch (filename.toStdString());
            }

        });
    }



    void resized() override{
        auto area = getLocalBounds();
        auto messageBoxArea = area.removeFromTop((int) 0.5*getHeight());

        auto loadArea = area.removeFromRight((int) getWidth() / 2); 
        loadButton.setBounds(loadArea);
        unloadButton.setBounds(area);
        messageBox.setBounds(messageBoxArea);
    }

private: 
    juce::TextEditor messageBox;
    std::unique_ptr<juce::FileChooser> chooser;
    CmajorProcessor& cmajorProcessor; 
    juce::TextButton loadButton {"Load"}; 
    juce::TextButton unloadButton {"Unload"}; 

};