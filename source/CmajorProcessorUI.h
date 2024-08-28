#pragma once

#include "CmajorProcessor.h"
#include "../3rd_party/cmajor/include/cmajor/helpers/cmaj_PatchWebView.h"


struct CmajorComponent : public juce::Component, 
                        public juce::ChangeListener

{
    CmajorComponent (CmajorProcessor& p, juce::AudioProcessorEditor& e): owner (p), mainEditor(e),
            patchWebView (std::make_unique<cmaj::PatchWebView> (*p.patch, derivePatchViewSize (p)))
    {

        owner.addChangeListener(this);

        patchWebViewHolder = choc::ui::createJUCEWebViewHolder (patchWebView->getWebView());
        patchWebViewHolder->setSize ((int) patchWebView->width, (int) patchWebView->height);

        // mainEditor.setResizeLimits (250, 160, 32768, 32768);

        patchWebView->setActive (true);

        onPatchChanged (false);


        statusMessageChanged();

        juce::Font::setDefaultMinimumHorizontalScaleFactor (1.0f);


    }

    ~CmajorComponent() override
    {
        // owner.editorBeingDeleted (this);
        // setLookAndFeel (nullptr);
        patchWebViewHolder.reset();
        patchWebView.reset();
        owner.removeChangeListener(this);
    }

    void changeListenerCallback (juce::ChangeBroadcaster * source) override {
        if (source == &owner){
            if (owner.editorsShouldUpdatePatch) {
                onPatchChanged();
                owner.editorsShouldUpdatePatch = false;
            }
            else if (owner.editorsShouldUpdateMessage){

                statusMessageChanged();
                owner.editorsShouldUpdateMessage = false; 
            }
        };
    }

    void statusMessageChanged()
    {
        patchWebView->setStatusMessage (owner.statusMessage);
    }

    static cmaj::PatchManifest::View derivePatchViewSize (const CmajorProcessor& owner)
    {
        auto view = cmaj::PatchManifest::View
        {
            choc::json::create ("width", owner.lastEditorWidth,
                                "height", owner.lastEditorHeight)
        };

        if (auto manifest = owner.patch->getManifest())
            if (auto v = manifest->findDefaultView())
                view = *v;

        if (view.getWidth()  == 0)  view.view.setMember ("width", defaultWidth);
        if (view.getHeight() == 0)  view.view.setMember ("height", defaultHeight);

        return view;
    }

    void onPatchChanged (bool forceReload = true)
    {
        if (owner.isViewVisible())
        {
            std::cout << "View visible" << std::endl;
            patchWebView->setActive (true);
            patchWebView->update (derivePatchViewSize (owner));
            patchWebViewHolder->setSize ((int) patchWebView->width, (int) patchWebView->height);

            mainEditor.setResizable (patchWebView->resizable, false);

            addAndMakeVisible (*patchWebViewHolder);
            childBoundsChanged (nullptr);
        }
        else
        {

            removeChildComponent (patchWebViewHolder.get());

            patchWebView->setActive (false);
            patchWebViewHolder->setVisible (false);

            setSize (defaultWidth, defaultHeight);
            mainEditor.setResizable (true, false);
        }

        if (forceReload)
            patchWebView->reload();
    }

    void childBoundsChanged (Component*) override
    {
        if (! isResizing && patchWebViewHolder->isVisible())
            setSize (std::max (50, patchWebViewHolder->getWidth()),
                        std::max (50, patchWebViewHolder->getHeight()));// + CmajorProcessor::extraCompHeight));
    }

    void paint(juce::Graphics& g) override {
        g.fillAll (juce::Colours::white);

    }

    void resized() override
    {
        isResizing = true;
        juce::Component::resized();

        auto r = getLocalBounds();

        if (patchWebViewHolder->isVisible())
        {
            patchWebViewHolder->setBounds (r.removeFromTop (getHeight()));
            if (getWidth() > 0 && getHeight() > 0)
            {
                owner.lastEditorWidth = patchWebViewHolder->getWidth();
                owner.lastEditorHeight = patchWebViewHolder->getHeight();
            }
        }
        isResizing = false;
    }


    //==============================================================================
    CmajorProcessor& owner;
    juce::AudioProcessorEditor& mainEditor; 

    std::unique_ptr<cmaj::PatchWebView> patchWebView;
    std::unique_ptr<juce::Component> patchWebViewHolder;//, extraComp;

    bool isResizing = false;

    static constexpr int defaultWidth = 500, defaultHeight = 400;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CmajorComponent)
};
