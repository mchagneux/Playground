#include "CmajorProcessor.h" 
#include "../3rd_party/cmajor/include/cmajor/helpers/cmaj_PatchWebView.h"


struct CmajorComponent : public juce::Component, 
                        public juce::ChangeListener

{
    CmajorComponent (CmajorProcessor& p): owner (p),
            patchWebView (std::make_unique<cmaj::PatchWebView> (*p.patch, derivePatchViewSize (p)))
    {

        owner.addChangeListener(this);

        patchWebViewHolder = choc::ui::createJUCEWebViewHolder (patchWebView->getWebView());
        // patchWebViewHolder->setSize ((int) patchWebView->width, (int) patchWebView->height);

        // mainEditor.setResizeLimits (250, 160, 32768, 32768);

        // patchWebView->setActive (true);

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
            patchWebView->setActive (true);
            patchWebView->update (derivePatchViewSize (owner));
            // patchWebViewHolder->setSize ((int) patchWebView->width, (int) patchWebView->height);

            addAndMakeVisible (*patchWebViewHolder);
            // childBoundsChanged (nullptr);
        }
        else
        {

            removeChildComponent (patchWebViewHolder.get());

            patchWebView->setActive (false);
            patchWebViewHolder->setVisible (false);

            // setSize (defaultWidth, defaultHeight);
        }

        if (forceReload)
            patchWebView->reload();
    }

    // void childBoundsChanged (Component*) override
    // {
    //     if (! isResizing && patchWebViewHolder->isVisible())
    //         setSize (std::max (50, patchWebViewHolder->getWidth()),
    //                     std::max (50, patchWebViewHolder->getHeight()));// + CmajorProcessor::extraCompHeight));
    // }

    void paint(juce::Graphics& g) override {
        g.fillAll (juce::Colours::white);

    }

    void resized() override
    {
        // isResizing = true;
        // juce::Component::resized();

        // auto r = getLocalBounds();

        patchWebViewHolder->setBounds(getLocalBounds());

        // if (patchWebViewHolder->isVisible())
        // {
        //     patchWebViewHolder->setBounds (r.removeFromTop (getHeight()));
        //     if (getWidth() > 0 && getHeight() > 0)
        //     {
        //         owner.lastEditorWidth = patchWebViewHolder->getWidth();
        //         owner.lastEditorHeight = patchWebViewHolder->getHeight();
        //     }
        // }
        // isResizing = false;
    }


    //==============================================================================
    CmajorProcessor& owner;


    std::unique_ptr<cmaj::PatchWebView> patchWebView;
    std::unique_ptr<juce::Component> patchWebViewHolder;//, extraComp;

    bool isResizing = false;

    static constexpr int defaultWidth = 500, defaultHeight = 400;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CmajorComponent)
};


struct LoaderComponent : public juce::Component, 
                        public juce::ChangeListener 
{
    
    public:
        LoaderComponent(CmajorProcessor& c):cmajorProcessor(c){ 

            cmajorProcessor.addChangeListener(this);
            messageBox.setMultiLine (true);
            messageBox.setReadOnly (true);

            addAndMakeVisible(loadButton); 
            addAndMakeVisible(unloadButton);
            addAndMakeVisible(messageBox); 

            loadButton.onClick = [this] { openButtonClicked();};
            unloadButton.onClick = [this] { cmajorProcessor.unload(); };
        }


        ~LoaderComponent() override
        {
            cmajorProcessor.removeChangeListener(this);
        }


        void changeListenerCallback(juce::ChangeBroadcaster * source ) override {
            if (source==&cmajorProcessor){
                if(cmajorProcessor.editorsShouldUpdateMessage)
                    refresh();
            }
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
            auto messageBoxArea = area.removeFromTop((int) (0.5*getHeight()));

            auto loadArea = area.removeFromRight((int) (getWidth() / 2)); 
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

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LoaderComponent)

};



struct CmajorJITComponent: public juce::Component
{

public:
    CmajorJITComponent(CmajorProcessor& c): loaderComponent(c), cmajorComponent(c)
    {
        addAndMakeVisible(loaderComponent);
        addAndMakeVisible(cmajorComponent);
    }


    void resized() override
    {
        auto area = getLocalBounds();
        loaderComponent.setBounds(area.removeFromBottom((int)(getHeight() / 10)));
        cmajorComponent.setBounds(area);
    }

private:  
    LoaderComponent loaderComponent; 
    CmajorComponent cmajorComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CmajorJITComponent)

};