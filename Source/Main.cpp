#include "../JuceLibraryCode/JuceHeader.h"

Component* createMainContentComponent();

//==============================================================================
class audioTestApplication  : public JUCEApplication
{
public:
    //==============================================================================
    audioTestApplication() {}

    const String getApplicationName() override       { return ProjectInfo::projectName; }
    const String getApplicationVersion() override    { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override       { return true; }

    //==============================================================================
    void initialise (const String& commandLine) override
    {
        mainWindow = new MainWindow (getApplicationName());
    }

    void shutdown() override
    {
        mainWindow = nullptr; // (deletes our window)
    }

    //==============================================================================
    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted (const String& commandLine) override
    {}

    //==============================================================================
    
    //This class implements the desktop window that contains an instance of MainContentComponent class.
    class MainWindow
    {
    public:
        MainWindow (String name)
        {
            mMainComponent = createMainContentComponent();
            mMainComponent->setVisible(true);
            mMainComponent->addToDesktop(ComponentPeer::windowHasDropShadow);
        }
        
        ~MainWindow()
        {
            delete mMainComponent;
        }

//        void closeButtonPressed() override
//        {
//            JUCEApplication::getInstance()->systemRequestedQuit();
//        }
        
//        void buttonClicked(Button* button) override
//        {
//            if (button == &closeButton) JUCEApplication::getInstance()->systemRequestedQuit();
//        }

    private:
        Component* mMainComponent;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

private:
    ScopedPointer<MainWindow> mainWindow;
};

//==============================================================================
// This macro generates the main() routine that launches the app.
START_JUCE_APPLICATION (audioTestApplication)
