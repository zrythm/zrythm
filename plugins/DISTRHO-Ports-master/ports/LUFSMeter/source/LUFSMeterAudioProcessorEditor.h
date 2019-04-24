/*
 ===============================================================================
 
 LUFSMeterAudioProcessorEditor.h
 
 
 This file is part of the LUFS Meter audio measurement plugin.
 Copyright 2011-12 by Klangfreund, Samuel Gaehwiler.
 
 -------------------------------------------------------------------------------
 
 The LUFS Meter can be redistributed and/or modified under the terms of the GNU 
 General Public License Version 2, as published by the Free Software Foundation.
 A copy of the license is included with these source files. It can also be found
 at www.gnu.org/licenses.
 
 The LUFS Meter is distributed WITHOUT ANY WARRANTY.
 See the GNU General Public License for more details.
 
 -------------------------------------------------------------------------------
 
 To release a closed-source product which uses the LUFS Meter or parts of it,
 a commercial license is available. Visit www.klangfreund.com/lufsmeter for more
 information.
 
 ===============================================================================
 */


#ifndef __LUFS_METER_AUDIO_PROCESSOR_EDITOR__
#define __LUFS_METER_AUDIO_PROCESSOR_EDITOR__

#include "MacrosAndJuceHeaders.h"
#include "LUFSMeterAudioProcessor.h"
#include "gui/BackgroundGrid.h"
#include "gui/BackgroundGridCaption.h"
#include "gui/BackgroundVerticalLinesAndCaption.h"
#include "gui/LoudnessBar.h"
#include "gui/LoudnessNumeric.h"
#include "gui/LoudnessHistory.h"
#include "gui/LoudnessHistoryGroup.h"
#include "gui/LoudnessRangeBar.h"
#include "gui/LoudnessRangeHistory.h"
#include "gui/MultiChannelLoudnessBar.h"
#include "gui/PreferencesPane.h"


//==============================================================================
/**
*/
class LUFSMeterAudioProcessorEditor   : public AudioProcessorEditor,
                                        public Timer,
                                        public ButtonListener,
                                        public Value::Listener
{
public:
    LUFSMeterAudioProcessorEditor (LUFSMeterAudioProcessor* ownerFilter);
    ~LUFSMeterAudioProcessorEditor();

    //==========================================================================
    /** The standard Juce paint method. */
    void paint (Graphics& g);
    
    void resized ();
    
    void timerCallback ();
    
    /** The ButtonListener method. */
    void buttonClicked (Button* button);
    
    /** The value listener method. */
    void valueChanged (Value & value);
    
private:
    LUFSMeterAudioProcessor* getProcessor() const;
    
    /** Recalculates the position and size of all GUI components, based on
        the width for the loudness bars.
     */
    void resizeGuiComponents();
    
    void displayPositionInfo (const AudioPlayHead::CurrentPositionInfo& pos);
    
    AudioPlayHead::CurrentPositionInfo lastDisplayedPosition;
    
    LookAndFeel_V3 lookAndFeelV3;
    
    /** Shared values. */
    Value momentaryLoudnessValue;
    // There is no momentaryLoudnessValues here, because the Value object is
    // not suited to be updated at e.g. 20Hz since it deallocates and allocates
    // new memory every time it gets updated.
    Value shortTermLoudnessValue;
    Value loudnessRangeStartValue;
    Value loudnessRangeEndValue;
    Value loudnessRangeValue;
    Value integratedLoudnessValue;
    
    /** GUI parameters. */
    int distanceBetweenLoudnessBarAndTop;
    int distanceBetweenLoudnessBarAndBottom;
    
    BackgroundGrid backgroundGrid;
    BackgroundGridCaption backgroundGridCaption;
    BackgroundVerticalLinesAndCaption backgroundVerticalLinesAndCaption;
    
    /* Level meter bars. */
    MultiChannelLoudnessBar momentaryLoudnessBar;
    LoudnessBar momentaryLoudnessBarSum;
    LoudnessBar shortTermLoudnessBar;
    LoudnessRangeBar loudnessRangeBar;
    LoudnessBar integratedLoudnessBar;
    
    /* Level values numeric. */
    LoudnessNumeric momentaryLoudnessNumeric;
    LoudnessNumeric shortTermLoudnessNumeric;
    LoudnessNumeric loudnessRangeNumeric;
    LoudnessNumeric integratedLoudnessNumeric;
    
    /** Level meter captions. */
    Label momentaryLoudnessCaption;
    Label shortTermLoudnessCaption;
    Label loudnessRangeCaption;
    Label integratedLoudnessCaption;
    
    /** Level history graph. */
    LoudnessHistoryGroup loudnessHistoryGroup;
    LoudnessHistory momentaryLoudnessHistory;
    LoudnessHistory shortTermLoudnessHistory;
    LoudnessRangeHistory loudnessRangeHistory;
    LoudnessHistory integratedLoudnessHistory;
    
    
    /** Preferences pane. */
    PreferencesPane preferencesPane;
    
    Label infoLabel;
    
    TextButton resetButton;
    
    ScopedPointer<ResizableCornerComponent> resizer;    
    /** Specifies the maximum size of the window. */
    ComponentBoundsConstrainer resizeLimits;
};


#endif  // __LUFS_METER_AUDIO_PROCESSOR_EDITOR__
