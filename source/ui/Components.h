#pragma once 

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "../Utils.h"

// #include <juce_core/juce_core.h>
// #include "./SliderLookAndFeel.h"

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
            // slider.setLookAndFeel(new CustomSliderLookAndFeel());
            addAllAndMakeVisible (*this, slider, label);

            slider.setTextValueSuffix (" " + paramIn.label);

            label.attachToComponent (&slider, false);
            label.setJustificationType (Justification::centred);
        }

        void resized() override { slider.setBounds (getLocalBounds().reduced (0, 40)); }

    private:
        Slider slider { Slider::LinearBarVertical, Slider::TextBoxBelow };
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



// class CustomSliderLookAndFeel : public juce::LookAndFeel_V4
// {
// public:
//     CustomSliderLookAndFeel()
//     {

//     }

//     void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
//                            float sliderPos, float minSliderPos, float maxSliderPos,
//                            const juce::Slider::SliderStyle style, juce::Slider& slider) override
//     {
//         juce::ignoreUnused(x, minSliderPos, maxSliderPos, style, slider);

//         auto trackWidth = (float) height * 0.025f;

//         juce::Point<float> startPoint ((float) width * 0.05f, (float) y + (float) height * 0.5f);
//         juce::Point<float> endPoint ((float) width * 0.95f, startPoint.y);
//         float distance = endPoint.x - startPoint.x;

//         juce::Path backgroundTrack;
//         backgroundTrack.startNewSubPath (startPoint);
//         backgroundTrack.lineTo (endPoint);

//         g.setColour (juce::Colour{ 0xff3E3846 });
//         g.strokePath (backgroundTrack, { trackWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded });


//         float thumbWidth = (float) width / 8.f;
//         float thumbHeight = thumbWidth / 2.f;


//         float thumbX = startPoint.getX() + sliderPos / (float) width * (distance - 20.f);
//         juce::Point<float> thumbCenter = { thumbX, (float) y + (float) height * 0.5f };

//         juce::Rectangle<float> thumbRec = { thumbCenter.getX() - thumbWidth / 2.f, thumbCenter.getY() - thumbHeight / 2, thumbWidth, thumbHeight };
//         g.setColour (juce::Colour { 0xff9F0E5D });
//         g.fillRoundedRectangle(thumbRec, thumbHeight / 2.f);

//         float expansionRatio = 1.2f;

//         thumbRec.setWidth(thumbRec.getWidth() * expansionRatio);
//         thumbRec.setHeight(thumbRec.getHeight() * expansionRatio);
//         thumbRec.setCentre(thumbCenter);

//         juce::Image offscreenImage = juce::Image(juce::Image::ARGB, width, height, true);

//         juce::Graphics offscreenGraphics(offscreenImage);

//         offscreenGraphics.setColour(juce::Colour { 0xff9F0E5D });
//         offscreenGraphics.fillRoundedRectangle(thumbRec, thumbHeight / 2.0f * expansionRatio);

//         juce::ImageConvolutionKernel kernel(15);
//         kernel.createGaussianBlur(3.0f);
//         kernel.applyToImage(offscreenImage, offscreenImage, offscreenImage.getBounds());

//         g.drawImageAt(offscreenImage, 0, 0);
//     }
// };

// class CustomVerticalSlider: public juce::Slider
// {
// public: 
//     CustomVerticalSlider() {
//        setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
//        setTextBoxStyle(juce::Slider::TextBoxBelow, false, 0, 0);
//        setLookAndFeel(&customSliderLookAndFeel);
//     }
//     ~CustomVerticalSlider() {
//         setLookAndFeel(nullptr);
//     }
// private:
//     SliderLookAndFeel customSliderLookAndFeel;

// };