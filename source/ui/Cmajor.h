#pragma once
#include "./Components.h"
#include "../processors/CmajorStereoDSPEffect.h"
#include "CmajorStereoDSPEffect.h"

namespace CmajorStereoDSPEffect{


    struct ExtraEditorComponent  : public juce::Component,
                                    public juce::FileDragAndDropTarget
    {
        ExtraEditorComponent (CmajorStereoDSPEffect::Processor& p) : plugin (p)
        {
            messageBox.setMultiLine (true);
            messageBox.setReadOnly (true);

            unloadButton.onClick = [this] { plugin.unload(); };

            addAndMakeVisible (messageBox);
            addAndMakeVisible (unloadButton);
        }

        void resized() override
        {
            auto r = getLocalBounds().reduced (4);
            messageBox.setBounds (r);
            unloadButton.setBounds (r.removeFromTop (30).removeFromRight (80));
        }

        void refresh()
        {
            unloadButton.setVisible (plugin.patch->isLoaded());

            juce::Font f (18.0f);
            f.setTypefaceName (juce::Font::getDefaultMonospacedFontName());
            messageBox.setFont (f);

            auto text = plugin.statusMessage;

            if (text.empty())
                text = "Drag-and-drop a .cmajorpatch file here to load it";

            messageBox.setText (text);
        }

        void paintOverChildren (juce::Graphics& g) override
        {
            if (isDragOver)
                g.fillAll (juce::Colours::lightgreen.withAlpha (0.3f));
        }

        bool isInterestedInFileDrag (const juce::StringArray& files) override
        {
            return files.size() == 1 && files[0].endsWith (".cmajorpatch");
        }

        void fileDragEnter (const juce::StringArray&, int, int) override       { setDragOver (true); }
        void fileDragExit (const juce::StringArray&) override                  { setDragOver (false); }

        void filesDropped (const juce::StringArray& files, int, int) override
        {
            setDragOver (false);

            if (isInterestedInFileDrag (files))
                plugin.loadPatch (files[0].toStdString());
        }

        void setDragOver (bool b)
        {
            if (isDragOver != b)
            {
                isDragOver = b;
                repaint();
            }
        }

        //==============================================================================
        CmajorStereoDSPEffect::Processor& plugin;
        bool isDragOver = false;

        juce::TextEditor messageBox;
        juce::TextButton unloadButton { "Unload" };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ExtraEditorComponent)
    };
}