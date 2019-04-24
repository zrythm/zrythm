/*
  ==============================================================================

   Juce LV2 Wrapper

  ==============================================================================
*/

// Your project must contain an AppConfig.h file with your project-specific settings in it,
// and your header search path must make it accessible to the module's files.
#include "AppConfig.h"

#include "../utility/juce_CheckSettingMacros.h"
#include "../../juce_core/system/juce_TargetPlatform.h" // for JUCE_LINUX


#if JucePlugin_Build_LV2

/** Plugin requires processing with a fixed/constant block size */
#ifndef JucePlugin_WantsLV2FixedBlockSize
 #define JucePlugin_WantsLV2FixedBlockSize 0
#endif

/** Enable latency port */
#ifndef JucePlugin_WantsLV2Latency
 #define JucePlugin_WantsLV2Latency 1
#endif

/** Use non-parameter states */
#ifndef JucePlugin_WantsLV2State
 #define JucePlugin_WantsLV2State 1
#endif

/** States are strings, needs custom get/setStateInformationString */
#ifndef JucePlugin_WantsLV2StateString
 #define JucePlugin_WantsLV2StateString 0
#endif

/** Export presets */
#ifndef JucePlugin_WantsLV2Presets
 #define JucePlugin_WantsLV2Presets 1
#endif

/** Request time position */
#ifndef JucePlugin_WantsLV2TimePos
 #define JucePlugin_WantsLV2TimePos 1
#endif

/** Using string states require enabling states first */
#if JucePlugin_WantsLV2StateString && ! JucePlugin_WantsLV2State
 #undef JucePlugin_WantsLV2State
 #define JucePlugin_WantsLV2State 1
#endif

#if JUCE_LINUX && ! JUCE_AUDIOPROCESSOR_NO_GUI
 #include <X11/Xlib.h>
 #undef KeyPress
#endif

#include <fstream>
#include <iostream>

// LV2 includes..
#include "includes/lv2.h"
#include "includes/atom.h"
#include "includes/atom-util.h"
#include "includes/buf-size.h"
#include "includes/instance-access.h"
#include "includes/midi.h"
#include "includes/options.h"
#include "includes/parameters.h"
#include "includes/port-props.h"
#include "includes/presets.h"
#include "includes/state.h"
#include "includes/time.h"
#include "includes/ui.h"
#include "includes/urid.h"
#include "includes/lv2_external_ui.h"
#include "includes/lv2_programs.h"

#include "../utility/juce_IncludeModuleHeaders.h"

#define JUCE_LV2_STATE_STRING_URI "urn:juce:stateString"
#define JUCE_LV2_STATE_BINARY_URI "urn:juce:stateBinary"

using namespace juce;

//==============================================================================
// Various helper functions

/** Returns plugin URI */
static const String& getPluginURI()
{
    // JucePlugin_LV2URI might be defined as a function (eg. allowing dynamic URIs based on filename)
    static const String pluginURI(JucePlugin_LV2URI);
    return pluginURI;
}

/** Queries all available plugin audio ports */
void findMaxTotalChannels (AudioProcessor* const filter, int& maxTotalIns, int& maxTotalOuts)
{
    filter->enableAllBuses();

   #ifdef JucePlugin_PreferredChannelConfigurations
    int configs[][2] = { JucePlugin_PreferredChannelConfigurations };
    maxTotalIns = maxTotalOuts = 0;

    for (auto& config : configs)
    {
        maxTotalIns =  jmax (maxTotalIns,  config[0]);
        maxTotalOuts = jmax (maxTotalOuts, config[1]);
    }
   #else
    auto numInputBuses  = filter->getBusCount (true);
    auto numOutputBuses = filter->getBusCount (false);

    if (numInputBuses > 1 || numOutputBuses > 1)
    {
        maxTotalIns = maxTotalOuts = 0;

        for (int i = 0; i < numInputBuses; ++i)
            maxTotalIns  += filter->getChannelCountOfBus (true, i);

        for (int i = 0; i < numOutputBuses; ++i)
            maxTotalOuts += filter->getChannelCountOfBus (false, i);
    }
    else
    {
        maxTotalIns  = numInputBuses  > 0 ? filter->getBus (true,  0)->getMaxSupportedChannels (64) : 0;
        maxTotalOuts = numOutputBuses > 0 ? filter->getBus (false, 0)->getMaxSupportedChannels (64) : 0;
    }
   #endif
}

//==============================================================================
#if JUCE_LINUX

class SharedMessageThread : public Thread
{
public:
    SharedMessageThread()
      : Thread ("Lv2MessageThread"),
        initialised (false)
    {
        startThread (7);

        while (! initialised)
            sleep (1);
    }

    ~SharedMessageThread()
    {
        MessageManager::getInstance()->stopDispatchLoop();
        waitForThreadToExit (5000);
    }

    void run() override
    {
        const ScopedJuceInitialiser_GUI juceInitialiser;

        MessageManager::getInstance()->setCurrentThreadAsMessageThread();
        initialised = true;

        MessageManager::getInstance()->runDispatchLoop();
    }

private:
    volatile bool initialised;
};
#endif

#if ! JUCE_AUDIOPROCESSOR_NO_GUI
//==============================================================================
/**
    Lightweight DocumentWindow subclass for external ui
*/
class JuceLv2ExternalUIWindow : public DocumentWindow
{
public:
    /** Creates a Document Window wrapper */
    JuceLv2ExternalUIWindow (AudioProcessorEditor* editor, const String& title) :
            DocumentWindow (title, Colours::white, DocumentWindow::minimiseButton | DocumentWindow::closeButton, false),
            closed (false),
            lastPos (0, 0)
    {
        setOpaque (true);
        setContentNonOwned (editor, true);
        setSize (editor->getWidth(), editor->getHeight());
        setUsingNativeTitleBar (true);
    }

    /** Close button handler */
    void closeButtonPressed()
    {
        saveLastPos();
        removeFromDesktop();
        closed = true;
    }

    void saveLastPos()
    {
        lastPos = getScreenPosition();
    }

    void restoreLastPos()
    {
        setTopLeftPosition (lastPos.getX(), lastPos.getY());
    }

    Point<int> getLastPos()
    {
        return lastPos;
    }

    bool isClosed()
    {
        return closed;
    }

    void reset()
    {
        closed = false;
    }

private:
    bool closed;
    Point<int> lastPos;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JuceLv2ExternalUIWindow);
};

//==============================================================================
/**
    Juce LV2 External UI handle
*/
class JuceLv2ExternalUIWrapper : public LV2_External_UI_Widget
{
public:
    JuceLv2ExternalUIWrapper (AudioProcessorEditor* editor, const String& title)
        : window (editor, title)
    {
        // external UI calls
        run  = doRun;
        show = doShow;
        hide = doHide;
    }

    ~JuceLv2ExternalUIWrapper()
    {
        if (window.isOnDesktop())
            window.removeFromDesktop();
    }

    void close()
    {
        window.closeButtonPressed();
    }

    bool isClosed()
    {
        return window.isClosed();
    }

    void reset(const String& title)
    {
        window.reset();
        window.setName(title);
    }

    void repaint()
    {
        window.repaint();
    }

    Point<int> getScreenPosition()
    {
        if (window.isClosed())
            return window.getLastPos();
        else
            return window.getScreenPosition();
    }

    void setScreenPos (int x, int y)
    {
        if (! window.isClosed())
            window.setTopLeftPosition(x, y);
    }

    //==============================================================================
    static void doRun (LV2_External_UI_Widget* _this_)
    {
        const MessageManagerLock mmLock;
        JuceLv2ExternalUIWrapper* self = (JuceLv2ExternalUIWrapper*) _this_;

        if (! self->isClosed())
            self->window.repaint();
    }

    static void doShow (LV2_External_UI_Widget* _this_)
    {
        const MessageManagerLock mmLock;
        JuceLv2ExternalUIWrapper* self = (JuceLv2ExternalUIWrapper*) _this_;

        if (! self->isClosed())
        {
            if (! self->window.isOnDesktop())
                self->window.addToDesktop();

            self->window.restoreLastPos();
            self->window.setVisible(true);
        }
    }

    static void doHide (LV2_External_UI_Widget* _this_)
    {
        const MessageManagerLock mmLock;
        JuceLv2ExternalUIWrapper* self = (JuceLv2ExternalUIWrapper*) _this_;

        if (! self->isClosed())
        {
            self->window.saveLastPos();
            self->window.setVisible(false);
        }
    }

private:
    JuceLv2ExternalUIWindow window;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JuceLv2ExternalUIWrapper);
};

//==============================================================================
/**
    Juce LV2 Parent UI container, listens for resize events and passes them to ui-resize
*/
class JuceLv2ParentContainer : public Component
{
public:
    JuceLv2ParentContainer (AudioProcessorEditor* editor, const LV2UI_Resize* uiResize_)
        : uiResize(uiResize_)
    {
        setOpaque (true);
        editor->setOpaque (true);
        setBounds (editor->getBounds());

        editor->setTopLeftPosition (0, 0);
        addAndMakeVisible (editor);
    }

    ~JuceLv2ParentContainer()
    {
    }

    void paint (Graphics&) {}
    void paintOverChildren (Graphics&) {}

    void childBoundsChanged (Component* child)
    {
        const int cw = child->getWidth();
        const int ch = child->getHeight();

#if JUCE_LINUX
        XResizeWindow (display.display, (Window) getWindowHandle(), cw, ch);
#else
        setSize (cw, ch);
#endif

        if (uiResize != nullptr)
            uiResize->ui_resize (uiResize->handle, cw, ch);
    }

    void reset (const LV2UI_Resize* uiResize_)
    {
        uiResize = uiResize_;

        if (uiResize != nullptr)
            uiResize->ui_resize (uiResize->handle, getWidth(), getHeight());
    }

private:
    //==============================================================================
    const LV2UI_Resize* uiResize;
#if JUCE_LINUX
    ScopedXDisplay display;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JuceLv2ParentContainer);
};

//==============================================================================
/**
    Juce LV2 UI handle
*/
class JuceLv2UIWrapper : public AudioProcessorListener,
                         public Timer
{
public:
    JuceLv2UIWrapper (AudioProcessor* filter_, LV2UI_Write_Function writeFunction_, LV2UI_Controller controller_,
                      LV2UI_Widget* widget, const LV2_Feature* const* features, bool isExternal_)
        : filter (filter_),
          writeFunction (writeFunction_),
          controller (controller_),
          isExternal (isExternal_),
          controlPortOffset (0),
          lastProgramCount (0),
          uiTouch (nullptr),
          programsHost (nullptr),
          externalUIHost (nullptr),
          lastExternalUIPos (-1, -1),
          uiResize (nullptr)
    {
        jassert (filter != nullptr);

        filter->addListener(this);

        if (filter->hasEditor())
        {
            editor = filter->createEditorIfNeeded();

            if (editor == nullptr)
            {
                *widget = nullptr;
                return;
            }
        }

        for (int i = 0; features[i] != nullptr; ++i)
        {
            if (strcmp(features[i]->URI, LV2_UI__touch) == 0)
                uiTouch = (const LV2UI_Touch*)features[i]->data;

            else if (strcmp(features[i]->URI, LV2_PROGRAMS__Host) == 0)
                programsHost = (const LV2_Programs_Host*)features[i]->data;
        }

        if (isExternal)
        {
            resetExternalUI (features);

            if (externalUIHost != nullptr)
            {
                String title (filter->getName());

                if (externalUIHost->plugin_human_id != nullptr)
                    title = externalUIHost->plugin_human_id;

                externalUI = new JuceLv2ExternalUIWrapper (editor, title);
                *widget = externalUI;
                startTimer (100);
            }
            else
            {
                *widget = nullptr;
            }
        }
        else
        {
            resetParentUI (features);

            if (parentContainer != nullptr)
               *widget = parentContainer->getWindowHandle();
            else
               *widget = nullptr;
        }

#if (JucePlugin_WantsMidiInput || JucePlugin_WantsLV2TimePos)
        controlPortOffset += 1;
#endif
#if JucePlugin_ProducesMidiOutput
        controlPortOffset += 1;
#endif
        controlPortOffset += 1; // freewheel
#if JucePlugin_WantsLV2Latency
        controlPortOffset += 1;
#endif
        controlPortOffset += filter->getTotalNumInputChannels();
        controlPortOffset += filter->getTotalNumOutputChannels();

        lastProgramCount = filter->getNumPrograms();
    }

    ~JuceLv2UIWrapper()
    {
        PopupMenu::dismissAllActiveMenus();

        filter->removeListener(this);

        parentContainer = nullptr;
        externalUI = nullptr;
        externalUIHost = nullptr;

        if (editor != nullptr)
        {
            filter->editorBeingDeleted (editor);
            editor = nullptr;
        }
    }

    //==============================================================================
    // LV2 core calls

    void lv2Cleanup()
    {
        const MessageManagerLock mmLock;

        if (isExternal)
        {
            if (isTimerRunning())
                stopTimer();

            externalUIHost = nullptr;

            if (externalUI != nullptr)
            {
                lastExternalUIPos = externalUI->getScreenPosition();
                externalUI->close();
            }
        }
        else
        {
            if (parentContainer)
            {
                parentContainer->setVisible (false);
                if (parentContainer->isOnDesktop())
                    parentContainer->removeFromDesktop();
            }
        }
    }

    //==============================================================================
    // Juce calls

    void audioProcessorParameterChanged (AudioProcessor*, int index, float newValue)
    {
        if (writeFunction != nullptr && controller != nullptr)
            writeFunction (controller, index + controlPortOffset, sizeof (float), 0, &newValue);
    }

    void audioProcessorChanged (AudioProcessor*)
    {
        if (filter != nullptr && programsHost != nullptr)
        {
            if (filter->getNumPrograms() != lastProgramCount)
            {
                programsHost->program_changed (programsHost->handle, -1);
                lastProgramCount = filter->getNumPrograms();
            }
            else
                programsHost->program_changed (programsHost->handle, filter->getCurrentProgram());
        }
    }

    void audioProcessorParameterChangeGestureBegin (AudioProcessor*, int parameterIndex)
    {
        if (uiTouch != nullptr)
            uiTouch->touch (uiTouch->handle, parameterIndex + controlPortOffset, true);
    }

    void audioProcessorParameterChangeGestureEnd (AudioProcessor*, int parameterIndex)
    {
        if (uiTouch != nullptr)
            uiTouch->touch (uiTouch->handle, parameterIndex + controlPortOffset, false);
    }

    void timerCallback()
    {
        if (externalUI != nullptr && externalUI->isClosed())
        {
            if (externalUIHost != nullptr)
                externalUIHost->ui_closed (controller);

            if (isTimerRunning())
                stopTimer();
        }
    }

    //==============================================================================
    void resetIfNeeded (LV2UI_Write_Function writeFunction_, LV2UI_Controller controller_, LV2UI_Widget* widget,
                        const LV2_Feature* const* features)
    {
        writeFunction = writeFunction_;
        controller = controller_;
        uiTouch = nullptr;
        programsHost = nullptr;

        for (int i = 0; features[i] != nullptr; ++i)
        {
            if (strcmp(features[i]->URI, LV2_UI__touch) == 0)
                uiTouch = (const LV2UI_Touch*)features[i]->data;

            else if (strcmp(features[i]->URI, LV2_PROGRAMS__Host) == 0)
                programsHost = (const LV2_Programs_Host*)features[i]->data;
        }

        if (isExternal)
        {
            resetExternalUI (features);
            *widget = externalUI;
        }
        else
        {
            resetParentUI (features);
            *widget = parentContainer->getWindowHandle();
        }
    }

    void repaint()
    {
        const MessageManagerLock mmLock;

        if (editor != nullptr)
            editor->repaint();

        if (parentContainer != nullptr)
            parentContainer->repaint();

        if (externalUI != nullptr)
            externalUI->repaint();
    }

private:
    AudioProcessor* const filter;
    ScopedPointer<AudioProcessorEditor> editor;

    LV2UI_Write_Function writeFunction;
    LV2UI_Controller controller;
    const bool isExternal;

    uint32 controlPortOffset;
    int lastProgramCount;

    const LV2UI_Touch* uiTouch;
    const LV2_Programs_Host* programsHost;

    ScopedPointer<JuceLv2ExternalUIWrapper> externalUI;
    const LV2_External_UI_Host* externalUIHost;
    Point<int> lastExternalUIPos;

    ScopedPointer<JuceLv2ParentContainer> parentContainer;
    const LV2UI_Resize* uiResize;

#if JUCE_LINUX
    ScopedXDisplay display;
#endif

    //==============================================================================
    void resetExternalUI (const LV2_Feature* const* features)
    {
        externalUIHost = nullptr;

        for (int i = 0; features[i] != nullptr; ++i)
        {
            if (strcmp(features[i]->URI, LV2_EXTERNAL_UI__Host) == 0)
            {
                externalUIHost = (const LV2_External_UI_Host*)features[i]->data;
                break;
            }
        }

        if (externalUI != nullptr)
        {
            String title(filter->getName());

            if (externalUIHost->plugin_human_id != nullptr)
                title = externalUIHost->plugin_human_id;

            if (lastExternalUIPos.getX() != -1 && lastExternalUIPos.getY() != -1)
                externalUI->setScreenPos(lastExternalUIPos.getX(), lastExternalUIPos.getY());

            externalUI->reset(title);
            startTimer (100);
        }
    }

    void resetParentUI (const LV2_Feature* const* features)
    {
        void* parent = nullptr;
        uiResize = nullptr;

        for (int i = 0; features[i] != nullptr; ++i)
        {
            if (strcmp(features[i]->URI, LV2_UI__parent) == 0)
                parent = features[i]->data;

            else if (strcmp(features[i]->URI, LV2_UI__resize) == 0)
                uiResize = (const LV2UI_Resize*)features[i]->data;
        }

        if (parent != nullptr)
        {
            if (parentContainer == nullptr)
                parentContainer = new JuceLv2ParentContainer (editor, uiResize);

            parentContainer->setVisible (false);

            if (parentContainer->isOnDesktop())
                parentContainer->removeFromDesktop();

            parentContainer->addToDesktop (0, parent);

#if JUCE_LINUX
            Window hostWindow = (Window) parent;
            Window editorWnd  = (Window) parentContainer->getWindowHandle();
            XReparentWindow (display.display, editorWnd, hostWindow, 0, 0);
#endif

            parentContainer->reset (uiResize);
            parentContainer->setVisible (true);
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JuceLv2UIWrapper)
};

#endif /* JUCE_AUDIOPROCESSOR_NO_GUI */

//==============================================================================
/**
    Juce LV2 handle
*/
class JuceLv2Wrapper : public AudioPlayHead
{
public:
    //==============================================================================
    JuceLv2Wrapper (double sampleRate_, const LV2_Feature* const* features)
        : numInChans (0),
          numOutChans (0),
          bufferSize (2048),
          sampleRate (sampleRate_),
          uridMap (nullptr),
          uridAtomBlank (0),
          uridAtomObject (0),
          uridAtomDouble (0),
          uridAtomFloat (0),
          uridAtomInt (0),
          uridAtomLong (0),
          uridAtomSequence (0),
          uridMidiEvent (0),
          uridTimePos (0),
          uridTimeBar (0),
          uridTimeBarBeat (0),
          uridTimeBeatsPerBar (0),
          uridTimeBeatsPerMinute (0),
          uridTimeBeatUnit (0),
          uridTimeFrame (0),
          uridTimeSpeed (0),
          usingNominalBlockLength (false)
    {
        {
            const MessageManagerLock mmLock;
            filter = createPluginFilterOfType (AudioProcessor::wrapperType_LV2);
        }
        jassert (filter != nullptr);

        findMaxTotalChannels (filter, numInChans, numOutChans);

        // You must at least have some channels
        jassert (filter->isMidiEffect() || (numInChans > 0 || numOutChans > 0));

        filter->setPlayConfigDetails (numInChans, numOutChans, 0, 0);
        filter->setPlayHead (this);

#if (JucePlugin_WantsMidiInput || JucePlugin_WantsLV2TimePos)
        portEventsIn = nullptr;
#endif
#if JucePlugin_ProducesMidiOutput
        portMidiOut = nullptr;
#endif

        portFreewheel = nullptr;

#if JucePlugin_WantsLV2Latency
        portLatency = nullptr;
#endif

        portAudioIns.insertMultiple (0, nullptr, numInChans);
        portAudioOuts.insertMultiple (0, nullptr, numOutChans);
        portControls.insertMultiple (0, nullptr, filter->getNumParameters());

        for (int i=0; i < filter->getNumParameters(); ++i)
            lastControlValues.add (filter->getParameter(i));

        curPosInfo.resetToDefault();

        // we need URID_Map first
        for (int i=0; features[i] != nullptr; ++i)
        {
            if (strcmp(features[i]->URI, LV2_URID__map) == 0)
            {
                uridMap = (const LV2_URID_Map*)features[i]->data;
                break;
            }
        }

        // we require uridMap to work properly (it's set as required feature)
        jassert (uridMap != nullptr);

        if (uridMap != nullptr)
        {
            uridAtomBlank = uridMap->map(uridMap->handle, LV2_ATOM__Blank);
            uridAtomObject = uridMap->map(uridMap->handle, LV2_ATOM__Object);
            uridAtomDouble = uridMap->map(uridMap->handle, LV2_ATOM__Double);
            uridAtomFloat = uridMap->map(uridMap->handle, LV2_ATOM__Float);
            uridAtomInt = uridMap->map(uridMap->handle, LV2_ATOM__Int);
            uridAtomLong = uridMap->map(uridMap->handle, LV2_ATOM__Long);
            uridAtomSequence = uridMap->map(uridMap->handle, LV2_ATOM__Sequence);
            uridMidiEvent = uridMap->map(uridMap->handle, LV2_MIDI__MidiEvent);
            uridTimePos = uridMap->map(uridMap->handle, LV2_TIME__Position);
            uridTimeBar = uridMap->map(uridMap->handle, LV2_TIME__bar);
            uridTimeBarBeat = uridMap->map(uridMap->handle, LV2_TIME__barBeat);
            uridTimeBeatsPerBar = uridMap->map(uridMap->handle, LV2_TIME__beatsPerBar);
            uridTimeBeatsPerMinute = uridMap->map(uridMap->handle, LV2_TIME__beatsPerMinute);
            uridTimeBeatUnit = uridMap->map(uridMap->handle, LV2_TIME__beatUnit);
            uridTimeFrame = uridMap->map(uridMap->handle, LV2_TIME__frame);
            uridTimeSpeed = uridMap->map(uridMap->handle, LV2_TIME__speed);

            for (int i=0; features[i] != nullptr; ++i)
            {
                if (strcmp(features[i]->URI, LV2_OPTIONS__options) == 0)
                {
                    const LV2_Options_Option* options = (const LV2_Options_Option*)features[i]->data;

                    for (int j=0; options[j].key != 0; ++j)
                    {
                        if (options[j].key == uridMap->map(uridMap->handle, LV2_BUF_SIZE__nominalBlockLength))
                        {
                            if (options[j].type == uridAtomInt)
                            {
                                bufferSize = *(int*)options[j].value;
                                usingNominalBlockLength = true;
                            }
                            else
                            {
                                std::cerr << "Host provides nominalBlockLength but has wrong value type" << std::endl;
                            }
                            break;
                        }

                        if (options[j].key == uridMap->map(uridMap->handle, LV2_BUF_SIZE__maxBlockLength))
                        {
                            if (options[j].type == uridAtomInt)
                                bufferSize = *(int*)options[j].value;
                            else
                                std::cerr << "Host provides maxBlockLength but has wrong value type" << std::endl;

                            // no break, continue in case host supports nominalBlockLength
                        }
                    }
                    break;
                }
            }
        }

        progDesc.bank = 0;
        progDesc.program = 0;
        progDesc.name = nullptr;
    }

    ~JuceLv2Wrapper ()
    {
        const MessageManagerLock mmLock;

#if ! JUCE_AUDIOPROCESSOR_NO_GUI
        ui = nullptr;
#endif
        filter = nullptr;

        if (progDesc.name != nullptr)
            free((void*)progDesc.name);

        portControls.clear();
        lastControlValues.clear();
    }

    //==============================================================================
    // LV2 core calls

    void lv2ConnectPort (uint32 portId, void* dataLocation)
    {
        uint32 index = 0;

#if (JucePlugin_WantsMidiInput || JucePlugin_WantsLV2TimePos)
        if (portId == index++)
        {
            portEventsIn = (LV2_Atom_Sequence*)dataLocation;
            return;
        }
#endif

#if JucePlugin_ProducesMidiOutput
        if (portId == index++)
        {
            portMidiOut = (LV2_Atom_Sequence*)dataLocation;
            return;
        }
#endif

        if (portId == index++)
        {
            portFreewheel = (float*)dataLocation;
            return;
        }

#if JucePlugin_WantsLV2Latency
        if (portId == index++)
        {
            portLatency = (float*)dataLocation;
            return;
        }
#endif

        for (int i=0; i < numInChans; ++i)
        {
            if (portId == index++)
            {
                portAudioIns.set(i, (float*)dataLocation);
                return;
            }
        }

        for (int i=0; i < numOutChans; ++i)
        {
            if (portId == index++)
            {
                portAudioOuts.set(i, (float*)dataLocation);
                return;
            }
        }

        for (int i=0; i < filter->getNumParameters(); ++i)
        {
            if (portId == index++)
            {
                portControls.set(i, (float*)dataLocation);
                return;
            }
        }
    }

    void lv2Activate()
    {
        jassert (filter != nullptr);

        filter->prepareToPlay (sampleRate, bufferSize);
        filter->setPlayConfigDetails (numInChans, numOutChans, sampleRate, bufferSize);

        channels.calloc (numInChans + numOutChans);

#if (JucePlugin_WantsMidiInput || JucePlugin_ProducesMidiOutput)
        midiEvents.ensureSize (2048);
        midiEvents.clear();
#endif
    }

    void lv2Deactivate()
    {
        jassert (filter != nullptr);

        filter->releaseResources();

        channels.free();
    }

    void lv2Run (uint32 sampleCount)
    {
        jassert (filter != nullptr);

#if JucePlugin_WantsLV2Latency
        if (portLatency != nullptr)
            *portLatency = filter->getLatencySamples();
#endif

        if (portFreewheel != nullptr)
            filter->setNonRealtime (*portFreewheel >= 0.5f);

        if (sampleCount == 0)
        {
            /**
               LV2 pre-roll
               Hosts might use this to force plugins to update its output control ports.
               (plugins can only access port locations during run) */
            return;
        }

        // Check for updated parameters
        {
            float curValue;

            for (int i = 0; i < portControls.size(); ++i)
            {
                if (portControls[i] != nullptr)
                {
                    curValue = *portControls[i];

                    if (lastControlValues[i] != curValue)
                    {
                        filter->setParameter (i, curValue);
                        lastControlValues.setUnchecked (i, curValue);
                    }
                }
            }
        }

        {
            const ScopedLock sl (filter->getCallbackLock());

            if (filter->isSuspended() && false)
            {
                for (int i = 0; i < numOutChans; ++i)
                    zeromem (portAudioOuts[i], sizeof (float) * sampleCount);
            }
            else
            {
                int i;
                for (i = 0; i < numOutChans; ++i)
                {
                    channels[i] = portAudioOuts[i];

                    if (i < numInChans && portAudioIns[i] != portAudioOuts[i])
                        FloatVectorOperations::copy (portAudioOuts [i], portAudioIns[i], sampleCount);
                }

                for (; i < numInChans; ++i)
                    channels [i] = portAudioIns[i];

#if (JucePlugin_WantsMidiInput || JucePlugin_WantsLV2TimePos)
                if (portEventsIn != nullptr)
                {
                    midiEvents.clear();

                    LV2_ATOM_SEQUENCE_FOREACH(portEventsIn, iter)
                    {
                        const LV2_Atom_Event* event = (const LV2_Atom_Event*)iter;

                        if (event == nullptr)
                            continue;
                        if (event->time.frames >= sampleCount)
                            break;

 #if JucePlugin_WantsMidiInput
                        if (event->body.type == uridMidiEvent)
                        {
                            const uint8* data = (const uint8*)(event + 1);
                            midiEvents.addEvent(data, event->body.size, event->time.frames);
                            continue;
                        }
 #endif
 #if JucePlugin_WantsLV2TimePos
                        if (event->body.type == uridAtomBlank || event->body.type == uridAtomObject)
                        {
                            const LV2_Atom_Object* obj = (LV2_Atom_Object*)&event->body;

                            if (obj->body.otype != uridTimePos)
                                continue;

                            LV2_Atom* bar = nullptr;
                            LV2_Atom* barBeat = nullptr;
                            LV2_Atom* beatUnit = nullptr;
                            LV2_Atom* beatsPerBar = nullptr;
                            LV2_Atom* beatsPerMinute = nullptr;
                            LV2_Atom* frame = nullptr;
                            LV2_Atom* speed = nullptr;

                            lv2_atom_object_get (obj,
                                                 uridTimeBar, &bar,
                                                 uridTimeBarBeat, &barBeat,
                                                 uridTimeBeatUnit, &beatUnit,
                                                 uridTimeBeatsPerBar, &beatsPerBar,
                                                 uridTimeBeatsPerMinute, &beatsPerMinute,
                                                 uridTimeFrame, &frame,
                                                 uridTimeSpeed, &speed,
                                                 nullptr);

                            // need to handle this first as other values depend on it
                            if (speed != nullptr)
                            {
                                /**/ if (speed->type == uridAtomDouble)
                                    lastPositionData.speed = ((LV2_Atom_Double*)speed)->body;
                                else if (speed->type == uridAtomFloat)
                                    lastPositionData.speed = ((LV2_Atom_Float*)speed)->body;
                                else if (speed->type == uridAtomInt)
                                    lastPositionData.speed = ((LV2_Atom_Int*)speed)->body;
                                else if (speed->type == uridAtomLong)
                                    lastPositionData.speed = ((LV2_Atom_Long*)speed)->body;

                                curPosInfo.isPlaying = lastPositionData.speed != 0.0;
                            }

                            if (bar != nullptr)
                            {
                                /**/ if (bar->type == uridAtomDouble)
                                    lastPositionData.bar = ((LV2_Atom_Double*)bar)->body;
                                else if (bar->type == uridAtomFloat)
                                    lastPositionData.bar = ((LV2_Atom_Float*)bar)->body;
                                else if (bar->type == uridAtomInt)
                                    lastPositionData.bar = ((LV2_Atom_Int*)bar)->body;
                                else if (bar->type == uridAtomLong)
                                    lastPositionData.bar = ((LV2_Atom_Long*)bar)->body;
                            }

                            if (barBeat != nullptr)
                            {
                                /**/ if (barBeat->type == uridAtomDouble)
                                    lastPositionData.barBeat = ((LV2_Atom_Double*)barBeat)->body;
                                else if (barBeat->type == uridAtomFloat)
                                    lastPositionData.barBeat = ((LV2_Atom_Float*)barBeat)->body;
                                else if (barBeat->type == uridAtomInt)
                                    lastPositionData.barBeat = ((LV2_Atom_Int*)barBeat)->body;
                                else if (barBeat->type == uridAtomLong)
                                    lastPositionData.barBeat = ((LV2_Atom_Long*)barBeat)->body;
                            }

                            if (beatUnit != nullptr)
                            {
                                /**/ if (beatUnit->type == uridAtomDouble)
                                    lastPositionData.beatUnit = ((LV2_Atom_Double*)beatUnit)->body;
                                else if (beatUnit->type == uridAtomFloat)
                                    lastPositionData.beatUnit = ((LV2_Atom_Float*)beatUnit)->body;
                                else if (beatUnit->type == uridAtomInt)
                                    lastPositionData.beatUnit = ((LV2_Atom_Int*)beatUnit)->body;
                                else if (beatUnit->type == uridAtomLong)
                                    lastPositionData.beatUnit = ((LV2_Atom_Long*)beatUnit)->body;

                                if (lastPositionData.beatUnit > 0)
                                    curPosInfo.timeSigDenominator = lastPositionData.beatUnit;
                            }

                            if (beatsPerBar != nullptr)
                            {
                                /**/ if (beatsPerBar->type == uridAtomDouble)
                                    lastPositionData.beatsPerBar = ((LV2_Atom_Double*)beatsPerBar)->body;
                                else if (beatsPerBar->type == uridAtomFloat)
                                    lastPositionData.beatsPerBar = ((LV2_Atom_Float*)beatsPerBar)->body;
                                else if (beatsPerBar->type == uridAtomInt)
                                    lastPositionData.beatsPerBar = ((LV2_Atom_Int*)beatsPerBar)->body;
                                else if (beatsPerBar->type == uridAtomLong)
                                    lastPositionData.beatsPerBar = ((LV2_Atom_Long*)beatsPerBar)->body;

                                if (lastPositionData.beatsPerBar > 0.0f)
                                    curPosInfo.timeSigNumerator = lastPositionData.beatsPerBar;
                            }

                            if (beatsPerMinute != nullptr)
                            {
                                /**/ if (beatsPerMinute->type == uridAtomDouble)
                                    lastPositionData.beatsPerMinute = ((LV2_Atom_Double*)beatsPerMinute)->body;
                                else if (beatsPerMinute->type == uridAtomFloat)
                                    lastPositionData.beatsPerMinute = ((LV2_Atom_Float*)beatsPerMinute)->body;
                                else if (beatsPerMinute->type == uridAtomInt)
                                    lastPositionData.beatsPerMinute = ((LV2_Atom_Int*)beatsPerMinute)->body;
                                else if (beatsPerMinute->type == uridAtomLong)
                                    lastPositionData.beatsPerMinute = ((LV2_Atom_Long*)beatsPerMinute)->body;

                                if (lastPositionData.beatsPerMinute > 0.0f)
                                {
                                    curPosInfo.bpm = lastPositionData.beatsPerMinute;

                                    if (lastPositionData.speed != 0)
                                        curPosInfo.bpm *= std::abs(lastPositionData.speed);
                                }
                            }

                            if (frame != nullptr)
                            {
                                /**/ if (frame->type == uridAtomDouble)
                                    lastPositionData.frame = ((LV2_Atom_Double*)frame)->body;
                                else if (frame->type == uridAtomFloat)
                                    lastPositionData.frame = ((LV2_Atom_Float*)frame)->body;
                                else if (frame->type == uridAtomInt)
                                    lastPositionData.frame = ((LV2_Atom_Int*)frame)->body;
                                else if (frame->type == uridAtomLong)
                                    lastPositionData.frame = ((LV2_Atom_Long*)frame)->body;

                                if (lastPositionData.frame >= 0)
                                {
                                    curPosInfo.timeInSamples = lastPositionData.frame;
                                    curPosInfo.timeInSeconds = double(curPosInfo.timeInSamples)/sampleRate;
                                }
                            }

                            if (lastPositionData.bar >= 0 && lastPositionData.beatsPerBar > 0.0f)
                            {
                                curPosInfo.ppqPositionOfLastBarStart = lastPositionData.bar * lastPositionData.beatsPerBar;

                                if (lastPositionData.barBeat >= 0.0f)
                                    curPosInfo.ppqPosition = curPosInfo.ppqPositionOfLastBarStart + lastPositionData.barBeat;
                            }

                            lastPositionData.extraValid = (lastPositionData.beatsPerMinute > 0.0 &&
                                                           lastPositionData.beatUnit > 0 &&
                                                           lastPositionData.beatsPerBar > 0.0f);
                        }
 #endif
                    }
                }
#endif
                {
                    AudioSampleBuffer chans (channels, jmax (numInChans, numOutChans), sampleCount);
                    filter->processBlock (chans, midiEvents);
                }
            }
        }

#if JucePlugin_WantsLV2TimePos
        // update timePos for next callback
        if (lastPositionData.speed != 0.0)
        {
            if (lastPositionData.speed > 0.0)
            {
                // playing forwards
                lastPositionData.frame += sampleCount;
            }
            else
            {
                // playing backwards
                lastPositionData.frame -= sampleCount;

                if (lastPositionData.frame < 0)
                    lastPositionData.frame = 0;
            }

            curPosInfo.timeInSamples = lastPositionData.frame;
            curPosInfo.timeInSeconds = double(curPosInfo.timeInSamples)/sampleRate;

            if (lastPositionData.extraValid)
            {
                const double beatsPerMinute = lastPositionData.beatsPerMinute * lastPositionData.speed;
                const double framesPerBeat  = 60.0 * sampleRate / beatsPerMinute;
                const double addedBarBeats  = double(sampleCount) / framesPerBeat;

                if (lastPositionData.bar >= 0 && lastPositionData.barBeat >= 0.0f)
                {
                    lastPositionData.bar    += std::floor((lastPositionData.barBeat+addedBarBeats)/
                                                           lastPositionData.beatsPerBar);
                    lastPositionData.barBeat = std::fmod(lastPositionData.barBeat+addedBarBeats,
                                                         lastPositionData.beatsPerBar);

                    if (lastPositionData.bar < 0)
                        lastPositionData.bar = 0;

                    curPosInfo.ppqPositionOfLastBarStart = lastPositionData.bar * lastPositionData.beatsPerBar;
                    curPosInfo.ppqPosition = curPosInfo.ppqPositionOfLastBarStart + lastPositionData.barBeat;
                }

                curPosInfo.bpm = std::abs(beatsPerMinute);
            }
        }
#endif

#if JucePlugin_ProducesMidiOutput
        if (portMidiOut != nullptr)
        {
            const uint32_t capacity = portMidiOut->atom.size;

            portMidiOut->atom.size = sizeof(LV2_Atom_Sequence_Body);
            portMidiOut->atom.type = uridAtomSequence;
            portMidiOut->body.unit = 0;
            portMidiOut->body.pad  = 0;

            if (! midiEvents.isEmpty())
            {
                const uint8* midiEventData;
                int midiEventSize, midiEventPosition;
                MidiBuffer::Iterator i (midiEvents);

                uint32_t size, offset = 0;
                LV2_Atom_Event* aev;

                while (i.getNextEvent (midiEventData, midiEventSize, midiEventPosition))
                {
                    jassert (midiEventPosition >= 0 && midiEventPosition < (int)sampleCount);

                    if (sizeof(LV2_Atom_Event) + midiEventSize > capacity - offset)
                        break;

                    aev = (LV2_Atom_Event*)((char*)LV2_ATOM_CONTENTS(LV2_Atom_Sequence, portMidiOut) + offset);
                    aev->time.frames = midiEventPosition;
                    aev->body.type   = uridMidiEvent;
                    aev->body.size   = midiEventSize;
                    memcpy(LV2_ATOM_BODY(&aev->body), midiEventData, midiEventSize);

                    size    = lv2_atom_pad_size(sizeof(LV2_Atom_Event) + midiEventSize);
                    offset += size;
                    portMidiOut->atom.size += size;
                }

                midiEvents.clear();
            }
        } else
#endif
        if (! midiEvents.isEmpty())
        {
            midiEvents.clear();
        }
    }

    //==============================================================================
    // LV2 extended calls

    uint32_t lv2GetOptions (LV2_Options_Option* options)
    {
        // currently unused
        ignoreUnused(options);

        return LV2_OPTIONS_SUCCESS;
    }

    uint32_t lv2SetOptions (const LV2_Options_Option* options)
    {
        for (int i=0; options[i].key != 0; ++i)
        {
            if (options[i].key == uridMap->map(uridMap->handle, LV2_BUF_SIZE__nominalBlockLength))
            {
                if (options[i].type == uridAtomInt)
                    bufferSize = *(const int32_t*)options[i].value;
                else
                    std::cerr << "Host changed nominalBlockLength but with wrong value type" << std::endl;
            }
            else if (options[i].key == uridMap->map(uridMap->handle, LV2_BUF_SIZE__maxBlockLength) && ! usingNominalBlockLength)
            {
                if (options[i].type == uridAtomInt)
                    bufferSize = *(const int32_t*)options[i].value;
                else
                    std::cerr << "Host changed maxBlockLength but with wrong value type" << std::endl;
            }
            else if (options[i].key == uridMap->map(uridMap->handle, LV2_PARAMETERS__sampleRate))
            {
                if (options[i].type == uridAtomFloat)
                    sampleRate = *(const float*)options[i].value;
                else
                    std::cerr << "Host changed sampleRate but with wrong value type" << std::endl;
            }
        }

        return LV2_OPTIONS_SUCCESS;
    }

    const LV2_Program_Descriptor* lv2GetProgram (uint32_t index)
    {
        jassert (filter != nullptr);

        if (progDesc.name != nullptr)
        {
            free((void*)progDesc.name);
            progDesc.name = nullptr;
        }

        if ((int)index < filter->getNumPrograms())
        {
            progDesc.bank    = index / 128;
            progDesc.program = index % 128;
            progDesc.name    = strdup(filter->getProgramName(index).toUTF8());
            return &progDesc;
        }

        return nullptr;
    }

    void lv2SelectProgram (uint32_t bank, uint32_t program)
    {
        jassert (filter != nullptr);

        int realProgram = bank * 128 + program;

        if (realProgram < filter->getNumPrograms())
        {
            filter->setCurrentProgram(realProgram);

            // update input control ports now
            for (int i = 0; i < portControls.size(); ++i)
            {
                float value = filter->getParameter(i);

                if (portControls[i] != nullptr)
                    *portControls[i] = value;

                lastControlValues.set(i, value);
            }
        }
    }

    LV2_State_Status lv2SaveState (LV2_State_Store_Function store, LV2_State_Handle stateHandle)
    {
        jassert (filter != nullptr);

#if JucePlugin_WantsLV2StateString
        String stateData(filter->getStateInformationString().replace("\r\n","\n"));
        CharPointer_UTF8 charData(stateData.toUTF8());

        store (stateHandle,
               uridMap->map(uridMap->handle, JUCE_LV2_STATE_STRING_URI),
               charData.getAddress(),
               charData.sizeInBytes(),
               uridMap->map(uridMap->handle, LV2_ATOM__String),
               LV2_STATE_IS_POD|LV2_STATE_IS_PORTABLE);
#else
        MemoryBlock chunkMemory;
        filter->getCurrentProgramStateInformation (chunkMemory);

        store (stateHandle,
               uridMap->map(uridMap->handle, JUCE_LV2_STATE_BINARY_URI),
               chunkMemory.getData(),
               chunkMemory.getSize(),
               uridMap->map(uridMap->handle, LV2_ATOM__Chunk),
               LV2_STATE_IS_POD|LV2_STATE_IS_PORTABLE);
#endif

        return LV2_STATE_SUCCESS;
    }

    LV2_State_Status lv2RestoreState (LV2_State_Retrieve_Function retrieve, LV2_State_Handle stateHandle, uint32_t flags)
    {
        jassert (filter != nullptr);

        size_t size = 0;
        uint32 type = 0;
        const void* data = retrieve (stateHandle,
#if JucePlugin_WantsLV2StateString
                                     uridMap->map(uridMap->handle, JUCE_LV2_STATE_STRING_URI),
#else
                                     uridMap->map(uridMap->handle, JUCE_LV2_STATE_BINARY_URI),
#endif
                                     &size, &type, &flags);

        if (data == nullptr || size == 0 || type == 0)
            return LV2_STATE_ERR_UNKNOWN;

#if JucePlugin_WantsLV2StateString
        if (type == uridMap->map (uridMap->handle, LV2_ATOM__String))
        {
            String stateData (CharPointer_UTF8(static_cast<const char*>(data)));
            filter->setStateInformationString (stateData);

           #if ! JUCE_AUDIOPROCESSOR_NO_GUI
            if (ui != nullptr)
                ui->repaint();
           #endif

            return LV2_STATE_SUCCESS;
        }
#else
        if (type == uridMap->map (uridMap->handle, LV2_ATOM__Chunk))
        {
            filter->setCurrentProgramStateInformation (data, size);

           #if ! JUCE_AUDIOPROCESSOR_NO_GUI
            if (ui != nullptr)
                ui->repaint();
           #endif

            return LV2_STATE_SUCCESS;
        }
#endif

        return LV2_STATE_ERR_BAD_TYPE;
    }

    //==============================================================================
    // Juce calls

    bool getCurrentPosition (AudioPlayHead::CurrentPositionInfo& info)
    {
#if JucePlugin_WantsLV2TimePos
        info = curPosInfo;
        return true;
#else
        ignoreUnused(info);
        return false;
#endif
    }

#if ! JUCE_AUDIOPROCESSOR_NO_GUI
    //==============================================================================
    JuceLv2UIWrapper* getUI (LV2UI_Write_Function writeFunction, LV2UI_Controller controller, LV2UI_Widget* widget,
                             const LV2_Feature* const* features, bool isExternal)
    {
        const MessageManagerLock mmLock;

        if (ui != nullptr)
            ui->resetIfNeeded (writeFunction, controller, widget, features);
        else
            ui = new JuceLv2UIWrapper (filter, writeFunction, controller, widget, features, isExternal);

        return ui;
    }
#endif

private:
#if JUCE_LINUX
    SharedResourcePointer<SharedMessageThread> msgThread;
#else
    SharedResourcePointer<ScopedJuceInitialiser_GUI> sharedJuceGUI;
#endif

    ScopedPointer<AudioProcessor> filter;
#if ! JUCE_AUDIOPROCESSOR_NO_GUI
    ScopedPointer<JuceLv2UIWrapper> ui;
#endif
    HeapBlock<float*> channels;
    MidiBuffer midiEvents;
    int numInChans, numOutChans;

#if (JucePlugin_WantsMidiInput || JucePlugin_WantsLV2TimePos)
    LV2_Atom_Sequence* portEventsIn;
#endif
#if JucePlugin_ProducesMidiOutput
    LV2_Atom_Sequence* portMidiOut;
#endif
    float* portFreewheel;
#if JucePlugin_WantsLV2Latency
    float* portLatency;
#endif
    Array<float*> portAudioIns;
    Array<float*> portAudioOuts;
    Array<float*> portControls;

    uint32 bufferSize;
    double sampleRate;
    Array<float> lastControlValues;
    AudioPlayHead::CurrentPositionInfo curPosInfo;

    struct Lv2PositionData {
        int64_t  bar;
        float    barBeat;
        uint32_t beatUnit;
        float    beatsPerBar;
        float    beatsPerMinute;
        int64_t  frame;
        double   speed;
        bool     extraValid;

        Lv2PositionData()
            : bar(-1),
              barBeat(-1.0f),
              beatUnit(0),
              beatsPerBar(0.0f),
              beatsPerMinute(0.0f),
              frame(-1),
              speed(0.0),
              extraValid(false) {}
    };
    Lv2PositionData lastPositionData;

    const LV2_URID_Map* uridMap;
    LV2_URID uridAtomBlank;
    LV2_URID uridAtomObject;
    LV2_URID uridAtomDouble;
    LV2_URID uridAtomFloat;
    LV2_URID uridAtomInt;
    LV2_URID uridAtomLong;
    LV2_URID uridAtomSequence;
    LV2_URID uridMidiEvent;
    LV2_URID uridTimePos;
    LV2_URID uridTimeBar;
    LV2_URID uridTimeBarBeat;
    LV2_URID uridTimeBeatsPerBar;    // timeSigNumerator
    LV2_URID uridTimeBeatsPerMinute; // bpm
    LV2_URID uridTimeBeatUnit;       // timeSigDenominator
    LV2_URID uridTimeFrame;          // timeInSamples
    LV2_URID uridTimeSpeed;

    bool usingNominalBlockLength; // if false use maxBlockLength

    LV2_Program_Descriptor progDesc;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JuceLv2Wrapper)
};

//==============================================================================
// LV2 descriptor functions

static LV2_Handle juceLV2_Instantiate (const LV2_Descriptor*, double sampleRate, const char*, const LV2_Feature* const* features)
{
    return new JuceLv2Wrapper (sampleRate, features);
}

#define handlePtr ((JuceLv2Wrapper*)handle)

static void juceLV2_ConnectPort (LV2_Handle handle, uint32 port, void* dataLocation)
{
    handlePtr->lv2ConnectPort (port, dataLocation);
}

static void juceLV2_Activate (LV2_Handle handle)
{
    handlePtr->lv2Activate();
}

static void juceLV2_Run( LV2_Handle handle, uint32 sampleCount)
{
    handlePtr->lv2Run (sampleCount);
}

static void juceLV2_Deactivate (LV2_Handle handle)
{
    handlePtr->lv2Deactivate();
}

static void juceLV2_Cleanup (LV2_Handle handle)
{
    delete handlePtr;
}

//==============================================================================
// LV2 extended functions

static uint32_t juceLV2_getOptions (LV2_Handle handle, LV2_Options_Option* options)
{
    return handlePtr->lv2GetOptions(options);
}

static uint32_t juceLV2_setOptions (LV2_Handle handle, const LV2_Options_Option* options)
{
    return handlePtr->lv2SetOptions(options);
}

static const LV2_Program_Descriptor* juceLV2_getProgram (LV2_Handle handle, uint32_t index)
{
    return handlePtr->lv2GetProgram(index);
}

static void juceLV2_selectProgram (LV2_Handle handle, uint32_t bank, uint32_t program)
{
    handlePtr->lv2SelectProgram(bank, program);
}

static LV2_State_Status juceLV2_SaveState (LV2_Handle handle, LV2_State_Store_Function store, LV2_State_Handle stateHandle,
                                           uint32_t, const LV2_Feature* const*)
{
    return handlePtr->lv2SaveState(store, stateHandle);
}

static LV2_State_Status juceLV2_RestoreState (LV2_Handle handle, LV2_State_Retrieve_Function retrieve, LV2_State_Handle stateHandle,
                                              uint32_t flags, const LV2_Feature* const*)
{
    return handlePtr->lv2RestoreState(retrieve, stateHandle, flags);
}

#undef handlePtr

static const void* juceLV2_ExtensionData (const char* uri)
{
    static const LV2_Options_Interface options = { juceLV2_getOptions, juceLV2_setOptions };
    static const LV2_Programs_Interface programs = { juceLV2_getProgram, juceLV2_selectProgram };
    static const LV2_State_Interface state = { juceLV2_SaveState, juceLV2_RestoreState };

    if (strcmp(uri, LV2_OPTIONS__interface) == 0)
        return &options;
    if (strcmp(uri, LV2_PROGRAMS__Interface) == 0)
        return &programs;
    if (strcmp(uri, LV2_STATE__interface) == 0)
        return &state;

    return nullptr;
}

#if ! JUCE_AUDIOPROCESSOR_NO_GUI
//==============================================================================
// LV2 UI descriptor functions

static LV2UI_Handle juceLV2UI_Instantiate (LV2UI_Write_Function writeFunction, LV2UI_Controller controller,
                                           LV2UI_Widget* widget, const LV2_Feature* const* features, bool isExternal)
{
    for (int i = 0; features[i] != nullptr; ++i)
    {
        if (strcmp(features[i]->URI, LV2_INSTANCE_ACCESS_URI) == 0 && features[i]->data != nullptr)
        {
            JuceLv2Wrapper* wrapper = (JuceLv2Wrapper*)features[i]->data;
            return wrapper->getUI(writeFunction, controller, widget, features, isExternal);
        }
    }

    std::cerr << "Host does not support instance-access, cannot use UI" << std::endl;
    return nullptr;
}

static LV2UI_Handle juceLV2UI_InstantiateExternal (const LV2UI_Descriptor*, const char*, const char*, LV2UI_Write_Function writeFunction,
                                                   LV2UI_Controller controller, LV2UI_Widget* widget, const LV2_Feature* const* features)
{
    return juceLV2UI_Instantiate(writeFunction, controller, widget, features, true);
}

static LV2UI_Handle juceLV2UI_InstantiateParent (const LV2UI_Descriptor*, const char*, const char*, LV2UI_Write_Function writeFunction,
                                                 LV2UI_Controller controller, LV2UI_Widget* widget, const LV2_Feature* const* features)
{
    return juceLV2UI_Instantiate(writeFunction, controller, widget, features, false);
}

static void juceLV2UI_Cleanup (LV2UI_Handle handle)
{
    ((JuceLv2UIWrapper*)handle)->lv2Cleanup();
}
#endif

//==============================================================================
// static LV2 Descriptor objects

static const LV2_Descriptor JuceLv2Plugin = {
    strdup(getPluginURI().toRawUTF8()),
    juceLV2_Instantiate,
    juceLV2_ConnectPort,
    juceLV2_Activate,
    juceLV2_Run,
    juceLV2_Deactivate,
    juceLV2_Cleanup,
    juceLV2_ExtensionData
};

#if ! JUCE_AUDIOPROCESSOR_NO_GUI
static const LV2UI_Descriptor JuceLv2UI_External = {
    strdup(String(getPluginURI() + "#ExternalUI").toRawUTF8()),
    juceLV2UI_InstantiateExternal,
    juceLV2UI_Cleanup,
    nullptr,
    nullptr
};

static const LV2UI_Descriptor JuceLv2UI_Parent = {
    strdup(String(getPluginURI() + "#ParentUI").toRawUTF8()),
    juceLV2UI_InstantiateParent,
    juceLV2UI_Cleanup,
    nullptr,
    nullptr
};
#endif

static const struct DescriptorCleanup {
    DescriptorCleanup() {}
    ~DescriptorCleanup()
    {
        free((void*)JuceLv2Plugin.URI);
#if ! JUCE_AUDIOPROCESSOR_NO_GUI
        free((void*)JuceLv2UI_External.URI);
        free((void*)JuceLv2UI_Parent.URI);
#endif
    }
} _descCleanup;

#if JUCE_WINDOWS
 #define JUCE_EXPORTED_FUNCTION extern "C" __declspec (dllexport)
#else
 #define JUCE_EXPORTED_FUNCTION extern "C" __attribute__ ((visibility("default")))
#endif

//==============================================================================
// startup code..

JUCE_EXPORTED_FUNCTION const LV2_Descriptor* lv2_descriptor (uint32 index);
JUCE_EXPORTED_FUNCTION const LV2_Descriptor* lv2_descriptor (uint32 index)
{
    return (index == 0) ? &JuceLv2Plugin : nullptr;
}

#if ! JUCE_AUDIOPROCESSOR_NO_GUI
JUCE_EXPORTED_FUNCTION const LV2UI_Descriptor* lv2ui_descriptor (uint32 index);
JUCE_EXPORTED_FUNCTION const LV2UI_Descriptor* lv2ui_descriptor (uint32 index)
{
    switch (index)
    {
    case 0:
        return &JuceLv2UI_External;
    case 1:
        return &JuceLv2UI_Parent;
    default:
        return nullptr;
    }
}
#endif

#ifndef JUCE_LV2_WRAPPER_WITHOUT_EXPORTER
 #include "juce_LV2_Wrapper_Exporter.cpp"
#endif

#endif
