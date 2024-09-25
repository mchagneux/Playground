#pragma once 
#include <JuceHeader.h>
#include "./Misc.h"



template <typename... Components>
void addAllAndMakeVisible (juce::Component& target, Components&... children)
{
    forEach ([&] (juce::Component& child) { target.addAndMakeVisible (child); }, children...);
}


class ComponentWithParamMenu : public juce::Component
{
public:
    ComponentWithParamMenu (juce::AudioProcessorEditor& editorIn, juce::RangedAudioParameter& paramIn)
        : editor (editorIn), param (paramIn) {}

    void mouseUp (const juce::MouseEvent& e) override
    {
        if (e.mods.isRightButtonDown())
            if (auto * c = editor.getHostContext())
                if (auto menuInfo = c->getContextMenuForParameter (&param))
                    menuInfo->getEquivalentPopupMenu().showMenuAsync (juce::PopupMenu::Options{}.withTargetComponent (this)
                                                                                            .withMousePosition());
    }

    int getParameterIndex() const
    {
        return param.getParameterIndex();
    }

private:
    juce::AudioProcessorEditor& editor;
    juce::RangedAudioParameter& param;
};

static inline ComponentWithParamMenu * findParentComponentWithParamMenu (juce::Component * c)
{
    if (c == nullptr)
        return nullptr;

    if (auto* derived = dynamic_cast<ComponentWithParamMenu*> (c))
        return derived;

    return findParentComponentWithParamMenu (c->getParentComponent());
}

class AttachedSlider final : public ComponentWithParamMenu
{
public:
    AttachedSlider (juce::AudioProcessorEditor& editorIn, juce::RangedAudioParameter& paramIn)
        : ComponentWithParamMenu (editorIn, paramIn),
            label ("", paramIn.name),
            attachment (paramIn, slider)
    {
        slider.addMouseListener (this, true);

        addAllAndMakeVisible (*this, slider, label);

        slider.setTextValueSuffix (" " + paramIn.label);

        label.attachToComponent (&slider, false);
        label.setJustificationType (juce::Justification::centred);
    }

    void resized() override { slider.setBounds (getLocalBounds().reduced (0, 40)); }

private:
    juce::Slider slider { juce::Slider::LinearVertical, juce::Slider::TextBoxBelow };
    juce::Label label;
    juce::SliderParameterAttachment attachment;
};

class AttachedToggle final : public ComponentWithParamMenu
{
public:
    AttachedToggle (juce::AudioProcessorEditor& editorIn, juce::RangedAudioParameter& paramIn)
        : ComponentWithParamMenu (editorIn, paramIn),
            toggle (paramIn.name),
            attachment (paramIn, toggle)
    {
        toggle.addMouseListener (this, true);
        addAndMakeVisible (toggle);
    }

    void resized() override { toggle.setBounds (getLocalBounds()); }

private:
    juce::ToggleButton toggle;
    juce::ButtonParameterAttachment attachment;
};

class AttachedCombo final : public ComponentWithParamMenu
{
public:
    AttachedCombo (juce::AudioProcessorEditor& editorIn, juce::RangedAudioParameter& paramIn)
        : ComponentWithParamMenu (editorIn, paramIn),
            combo (paramIn),
            label ("", paramIn.name),
            attachment (paramIn, combo)
    {
        combo.addMouseListener (this, true);

        addAllAndMakeVisible (*this, combo, label);

        label.attachToComponent (&combo, false);
        label.setJustificationType (juce::Justification::centred);
    }

    void resized() override
    {
        combo.setBounds (getLocalBounds().withSizeKeepingCentre (juce::jmin (getWidth(), 150), 24));
    }

private:
    struct ComboWithItems final : public juce::ComboBox
    {
        explicit ComboWithItems (juce::RangedAudioParameter& param)
        {
            // Adding the list here in the constructor means that the combo
            // is already populated when we construct the attachment below
            addItemList (dynamic_cast<juce::AudioParameterChoice&> (param).choices, 1);
        }
    };

    ComboWithItems combo;
    juce::Label label;
    juce::ComboBoxParameterAttachment attachment;
};


struct GetTrackInfo
{
    
    // Combo boxes need a lot of room
    juce::Grid::TrackInfo operator() (AttachedCombo&)             const { return juce::Grid::Px(120); }

    // Toggles are a bit smaller
    juce::Grid::TrackInfo operator() (AttachedToggle&)            const { return juce::Grid::Px(80); }

    // Sliders take up as much room as they can
    juce::Grid::TrackInfo operator() (AttachedSlider&)            const { return juce::Grid::Fr(1); }
};


template <typename... Components>
static void performLayout (const juce::Rectangle<int>& bounds, Components&... components)
{
    juce::Grid grid;
    using Track = juce::Grid::TrackInfo;

    grid.autoColumns     = Track (juce::Grid::Fr(1));
    grid.autoRows        = Track (juce::Grid::Fr(1));
    grid.columnGap       = juce::Grid::Px (10);
    grid.rowGap          = juce::Grid::Px (0);
    grid.autoFlow        = juce::Grid::AutoFlow::column;

    grid.templateColumns = { GetTrackInfo{} (components)... };
    grid.items           = { juce::GridItem (components)... };

    grid.performLayout (bounds);
}
