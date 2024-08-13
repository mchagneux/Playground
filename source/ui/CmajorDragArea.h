#pragma once 

#include "../processors/CmajorStereoDSPEffect.h"
#include <juce_gui_basics/juce_gui_basics.h>


struct CmajorControls: public juce::Component{

    explicit CmajorControls (CmajorStereoDSPEffect::Processor& cmajorProcIn):cmajorProc(cmajorProcIn){
        addAndMakeVisible (&loadButton);
        loadButton.setButtonText ("Load");
        loadButton.onClick = [this] { openButtonClicked();};

        addAndMakeVisible (&unloadButton);
        unloadButton.setButtonText ("Unload");
        unloadButton.onClick = [this] { cmajorProc.unload();  };
    }



    void resized() override {
        auto area = getLocalBounds();
        auto loadButtonArea = area.removeFromTop(0.5*getHeight()).reduced(0.05*getWidth(), 0.05*getHeight());
        auto unloadButtonArea = area.reduced(0.05*getWidth(), 0.05*getHeight());
        loadButton.setBounds(loadButtonArea);
        unloadButton.setBounds(unloadButtonArea);
    }

    bool isValidFile (const juce::String& file)
    {
        return file.endsWith (".cmajorpatch");
    }



    void refresh()
    {
        unloadButton.setVisible (cmajorProc.patch->isLoaded());
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
                std::cout << filename << std::endl;
                cmajorProc.loadPatch (filename.toStdString());
            }

        });
    }


    private:
        CmajorStereoDSPEffect::Processor& cmajorProc;
        juce::TextButton loadButton;
        juce::TextButton unloadButton;
        std::unique_ptr<juce::FileChooser> chooser;
    
};
