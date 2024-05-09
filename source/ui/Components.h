#pragma once 

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
// #include <juce_core/juce_core.h>

using namespace juce; 

class ComponentWithParamMenu : public Component
{
    public:
        ComponentWithParamMenu (AudioProcessorEditor& editorIn, RangedAudioParameter& paramIn)
            : editor (editorIn), param (paramIn) {}

        void mouseUp (const MouseEvent& e) override
        {
            if (e.mods.isRightButtonDown())
                if (auto* c = editor.getHostContext())
                    if (auto menuInfo = c->getContextMenuForParameter (&param))
                        menuInfo->getEquivalentPopupMenu().showMenuAsync (PopupMenu::Options{}.withTargetComponent (this)
                                                                                                .withMousePosition());
        }

    private:
        AudioProcessorEditor& editor;
        RangedAudioParameter& param;
};

class AttachedSlider final : public ComponentWithParamMenu
{
    public:
        AttachedSlider (AudioProcessorEditor& editorIn, RangedAudioParameter& paramIn)
            : ComponentWithParamMenu (editorIn, paramIn),
                label ("", paramIn.name),
                attachment (paramIn, slider)
        {
            slider.addMouseListener (this, true);

            addAllAndMakeVisible (*this, slider, label);

            slider.setTextValueSuffix (" " + paramIn.label);

            label.attachToComponent (&slider, false);
            label.setJustificationType (Justification::centred);
        }

        void resized() override { slider.setBounds (getLocalBounds().reduced (0, 40)); }

    private:
        Slider slider { Slider::RotaryVerticalDrag, Slider::TextBoxBelow };
        Label label;
        SliderParameterAttachment attachment;
};

class AttachedToggle final : public ComponentWithParamMenu
{
    public:
        AttachedToggle (AudioProcessorEditor& editorIn, RangedAudioParameter& paramIn)
            : ComponentWithParamMenu (editorIn, paramIn),
                toggle (paramIn.name),
                attachment (paramIn, toggle)
        {
            toggle.addMouseListener (this, true);
            addAndMakeVisible (toggle);
        }

        void resized() override { toggle.setBounds (getLocalBounds()); }

    private:
        ToggleButton toggle;
        ButtonParameterAttachment attachment;
};


class AttachedCombo final : public ComponentWithParamMenu
{
    public:
        AttachedCombo (AudioProcessorEditor& editorIn, RangedAudioParameter& paramIn)
            : ComponentWithParamMenu (editorIn, paramIn),
                combo (paramIn),
                label ("", paramIn.name),
                attachment (paramIn, combo)
        {
            combo.addMouseListener (this, true);

            addAllAndMakeVisible (*this, combo, label);

            label.attachToComponent (&combo, false);
            label.setJustificationType (Justification::centred);
        }

        void resized() override
        {
            combo.setBounds (getLocalBounds().withSizeKeepingCentre (jmin (getWidth(), 150), 24));
        }

    private:
        struct ComboWithItems final : public ComboBox
        {
            explicit ComboWithItems (RangedAudioParameter& param)
            {
                // Adding the list here in the constructor means that the combo
                // is already populated when we construct the attachment below
                addItemList (dynamic_cast<AudioParameterChoice&> (param).choices, 1);
            }
        };

        ComboWithItems combo;
        Label label;
        ComboBoxParameterAttachment attachment;
};


struct GetTrackInfo
{
        // Combo boxes need a lot of room
        Grid::TrackInfo operator() (AttachedCombo&)             const { return 120_px; }

        // Toggles are a bit smaller
        Grid::TrackInfo operator() (AttachedToggle&)            const { return 80_px; }

        // Sliders take up as much room as they can
        Grid::TrackInfo operator() (AttachedSlider&)            const { return 1_fr; }
    };

    template <typename... Components>
    static void performLayout (const Rectangle<int>& bounds, Components&... components)
    {
        Grid grid;
        using Track = Grid::TrackInfo;

        grid.autoColumns     = Track (1_fr);
        grid.autoRows        = Track (1_fr);
        grid.columnGap       = Grid::Px (10);
        grid.rowGap          = Grid::Px (0);
        grid.autoFlow        = Grid::AutoFlow::column;

        grid.templateColumns = { GetTrackInfo{} (components)... };
        grid.items           = { GridItem (components)... };

        grid.performLayout (bounds);
}