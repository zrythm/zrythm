/*
  ==============================================================================

   This file is part of the JUCE library - "Jules' Utility Class Extensions"
   Copyright 2004-11 by Raw Material Software Ltd.

  ------------------------------------------------------------------------------

   JUCE can be redistributed and/or modified under the terms of the GNU General
   Public License (Version 2), as published by the Free Software Foundation.
   A copy of the license is included in the JUCE distribution, or can be found
   online at www.gnu.org/licenses.

   JUCE is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

  ------------------------------------------------------------------------------

   To release a closed-source product which uses JUCE, commercial licenses are
   available: visit www.rawmaterialsoftware.com/juce for more information.

  ==============================================================================
*/

#include "JucePluginMain.h"
#include "modules/juce_audio_plugin_client/utility/juce_PluginUtilities.cpp"

// resources
#include "icon/DistrhoIcon.h"

using namespace juce;

//==============================================================================
/**
    A class that can be used to run a simple standalone application containing your filter.

    Just create one of these objects in your JUCEApplication::initialise() method, and
    let it do its work. It will create your filter object using the same createFilter() function
    that the other plugin wrappers use.
*/
class StandaloneFilterWindow    : public DocumentWindow,
                                  public MenuBarModel
{
public:
    //==============================================================================
    /** Creates a window with a given title and colour.
        The settings object can be a PropertySet that the class should use to
        store its settings - the object that is passed-in will be owned by this
        class and deleted automatically when no longer needed. (It can also be null)
    */
    StandaloneFilterWindow (const String& title,
                            const Colour& backgroundColour,
                            PropertySet* settingsToUse,
                            const String& commandLine)
        : DocumentWindow (title, backgroundColour, DocumentWindow::minimiseButton | DocumentWindow::closeButton, false),
          settings (settingsToUse),
          nativeTitleBarCheck (false)
    {
        createFilter();

        if (filter == nullptr)
        {
            jassertfalse    // Your filter didn't create correctly! In a standalone app that's not too great.
            JUCEApplication::quit();
        }

        filter->setPlayConfigDetails (JucePlugin_MaxNumInputChannels,
                                      JucePlugin_MaxNumOutputChannels,
                                      44100, 512);

        deviceManager = new AudioDeviceManager();
        deviceManager->addAudioCallback (&player);
        deviceManager->addMidiInputCallback (String::empty, &player);

        player.setProcessor (filter);

        ScopedPointer<XmlElement> savedState;

        if (settings != nullptr)
            savedState = settings->getXmlValue ("audioSetup");

        deviceManager->initialise (filter->getNumInputChannels(),
                                   filter->getNumOutputChannels(),
                                   savedState,
                                   true);

        if (commandLine.isNotEmpty())
        {
            File saveFile(commandLine);

            if (saveFile.existsAsFile())
            {
                saveFileName = commandLine;
                MemoryBlock data;

                if (saveFile.loadFileAsData (data))
                {
                    filter->setStateInformation (data.getData(), data.getSize());
                }
                else
                {
                    AlertWindow::showMessageBox (AlertWindow::WarningIcon,
                                                TRANS("Error whilst loading"),
                                                TRANS("Couldn't read from the specified file!"));
                }
            }
        }
        else
        {
            if (settings != nullptr)
            {
                MemoryBlock data;

                if (data.fromBase64Encoding (settings->getValue ("filterState"))
                    && data.getSize() > 0)
                {
                    filter->setStateInformation (data.getData(), data.getSize());
                }
            }
        }

        setContentOwned (filter->createEditorIfNeeded(), true);

        if (settings != nullptr)
        {
            const int x = settings->getIntValue ("windowX", -100);
            const int y = settings->getIntValue ("windowY", -100);
            const bool native = settings->getBoolValue("nativeTitleBar", true);

            setUsingNativeTitleBar (native);

            if (! native)
                setDropShadowEnabled (false);

            if (x != -100 && y != -100)
                setBoundsConstrained (Rectangle<int> (x, y, getWidth(), getHeight()));
            else
                centreWithSize (getWidth(), getHeight());
        }
        else
        {
            setUsingNativeTitleBar (true);
            centreWithSize (getWidth(), getHeight());
        }

#if JUCE_MAC
        setMacMainMenu (this);
#else
        setMenuBar (this);
#endif

        addToDesktop();
    }

    ~StandaloneFilterWindow()
    {
        if (settings != nullptr)
        {
            settings->setValue ("windowX", getX());
            settings->setValue ("windowY", getY());

            if (deviceManager != nullptr)
            {
                ScopedPointer<XmlElement> xml (deviceManager->createStateXml());
                settings->setValue ("audioSetup", xml);
            }

            if (nativeTitleBarCheck == false)
                settings->setValue("nativeTitleBar", isUsingNativeTitleBar());
        }

        deviceManager->removeMidiInputCallback (String::empty, &player);
        deviceManager->removeAudioCallback (&player);
        deviceManager = nullptr;

        if (settings != nullptr && filter != nullptr)
        {
            MemoryBlock data;
            filter->getStateInformation (data);

            settings->setValue ("filterState", data.toBase64Encoding());
        }

#if JUCE_MAC
        setMacMainMenu (0);
#else
        setMenuBar (0);
#endif

        deleteFilter();
    }

    void initIcon()
    {
        if (ComponentPeer* const peer = getPeer())
        {
            icon = ImageFileFormat::loadFrom (DistrhoIcon::logo_png, DistrhoIcon::logo_pngSize);
            peer->setIcon (icon);
        }
    }

    //==============================================================================
    StringArray getMenuBarNames()
    {
        const char* const names[] = { "File", "Presets", "Options", nullptr };

        return StringArray (names);
    }

    PopupMenu getMenuForIndex (int topLevelMenuIndex, const String& /*menuName*/)
    {
        PopupMenu menu;

        if (topLevelMenuIndex == 0)
        {
            menu.addItem (1, TRANS("Load a saved state..."));
            menu.addItem (2, TRANS("Save current state"));
            menu.addItem (3, TRANS("Save current state as..."));
            menu.addSeparator();
            menu.addItem (4, TRANS("Reset to default state"));
            menu.addSeparator();
            menu.addItem (5, TRANS("Quit"));
        }
        else if (topLevelMenuIndex == 1)
        {
            if (filter != nullptr && filter->getNumPrograms() > 0)
            {
                for (int i=0; i < filter->getNumPrograms(); i++)
                    menu.addItem (11+i, filter->getProgramName(i));
            }
            else
                menu.addItem (10, TRANS("(none)"), false);
        }
        else if (topLevelMenuIndex == 2)
        {
            menu.addItem (6, TRANS("Audio Settings..."));
            menu.addSeparator();
            if (isUsingNativeTitleBar())
                menu.addItem (7, TRANS("Use JUCE Titlebar"));
            else
                menu.addItem (7, TRANS("Use Native Titlebar"));
        }
        return menu;
    }

    void menuItemSelected (int menuItemID, int /*topLevelMenuIndex*/)
    {
        if (menuItemID == 1)
          loadState();
        else if (menuItemID == 2)
          saveState();
        else if (menuItemID == 3)
          saveStateAs();
        else if (menuItemID == 4)
          resetFilter();
        else if (menuItemID == 5)
          JUCEApplication::quit();
        else if (menuItemID == 6)
          showAudioSettingsDialog();
        else if (menuItemID == 7)
        {
            if (settings)
            {
                nativeTitleBarCheck = true;
                settings->setValue("nativeTitleBar", !isUsingNativeTitleBar());
                AlertWindow::showMessageBoxAsync(AlertWindow::InfoIcon, "Need restart", "You need to restart this plugin to apply the changes", TRANS("Ok"), this);
            }
        }
        else if (menuItemID >= 11)
          filter->setCurrentProgram(menuItemID-11);
    }

    //==============================================================================
    AudioProcessor* getAudioProcessor() const noexcept      { return filter; }
    AudioDeviceManager* getDeviceManager() const noexcept   { return deviceManager; }

    void createFilter()
    {
        AudioProcessor::setTypeOfNextNewPlugin (AudioProcessor::wrapperType_Standalone);
        filter = createPluginFilter();
        AudioProcessor::setTypeOfNextNewPlugin (AudioProcessor::wrapperType_Undefined);
    }

    /** Deletes and re-creates the filter and its UI. */
    void resetFilter()
    {
        deleteFilter();
        createFilter();

        if (filter != nullptr)
        {
            if (deviceManager != nullptr)
                player.setProcessor (filter);

            setContentOwned (filter->createEditorIfNeeded(), true);
        }

        if (settings != nullptr)
            settings->removeValue ("filterState");
    }

    /** Save state replacing old file. */
    void saveState()
    {
        if (saveFileName.isNotEmpty())
        {
            File saveFile(saveFileName);

            if (saveFile.existsAsFile())
            {
                MemoryBlock data;
                filter->getStateInformation (data);

                if (! saveFile.replaceWithData (data.getData(), data.getSize()))
                {
                    AlertWindow::showMessageBox (AlertWindow::WarningIcon,
                                                TRANS("Error whilst saving"),
                                                TRANS("Couldn't write to the current save file!"));
                }
            }
        }
        else
        {
            saveStateAs();
        }
    }

    /** Pops up a dialog letting the user save the filter's state to a file. */
    void saveStateAs()
    {
        FileChooser fc (TRANS("Save current state"),
                        settings != nullptr ? File (settings->getValue ("lastStateFile"))
                                            : File::nonexistent);

        if (fc.browseForFileToSave (true))
        {
            saveFileName = fc.getResult().getFileName();

            MemoryBlock data;
            filter->getStateInformation (data);

            if (! fc.getResult().replaceWithData (data.getData(), data.getSize()))
            {
                AlertWindow::showMessageBox (AlertWindow::WarningIcon,
                                             TRANS("Error whilst saving"),
                                             TRANS("Couldn't write to the specified file!"));
            }
        }
    }

    /** Pops up a dialog letting the user re-load the filter's state from a file. */
    void loadState()
    {
        FileChooser fc (TRANS("Load a saved state"),
                        settings != nullptr ? File (settings->getValue ("lastStateFile"))
                                            : File::nonexistent);

        if (fc.browseForFileToOpen())
        {
            saveFileName = fc.getResult().getFileName();

            MemoryBlock data;

            if (fc.getResult().loadFileAsData (data))
            {
                filter->setStateInformation (data.getData(), data.getSize());
            }
            else
            {
                AlertWindow::showMessageBox (AlertWindow::WarningIcon,
                                             TRANS("Error whilst loading"),
                                             TRANS("Couldn't read from the specified file!"));
            }
        }
    }

    /** Shows the audio properties dialog box modally. */
    void showAudioSettingsDialog()
    {
        DialogWindow::LaunchOptions o;
        o.content.setOwned (new AudioDeviceSelectorComponent (*deviceManager,
                                                              filter->getNumInputChannels(),
                                                              filter->getNumInputChannels(),
                                                              filter->getNumOutputChannels(),
                                                              filter->getNumOutputChannels(),
                                                              true, false, true, false));
        o.content->setSize (500, 450);

        o.dialogTitle                   = TRANS("Audio Settings");
        o.dialogBackgroundColour        = Colours::lightgrey;
        o.escapeKeyTriggersCloseButton  = true;
        o.useNativeTitleBar             = isUsingNativeTitleBar();
        o.resizable                     = false;

        o.launchAsync();
    }

    //==============================================================================
    /** @internal */
    void closeButtonPressed()
    {
        JUCEApplication::quit();
    }

private:
    //==============================================================================
    PropertySet* settings;
    ScopedPointer<AudioProcessor> filter;
    ScopedPointer<AudioDeviceManager> deviceManager;
    AudioProcessorPlayer player;
    bool nativeTitleBarCheck;
    String saveFileName;
    Image icon;

    void deleteFilter()
    {
        player.setProcessor (nullptr);

        if (filter != nullptr && getContentComponent() != nullptr)
        {
            filter->editorBeingDeleted (dynamic_cast <AudioProcessorEditor*> (getContentComponent()));
            clearContentComponent();
        }

        filter = nullptr;
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (StandaloneFilterWindow)
};
