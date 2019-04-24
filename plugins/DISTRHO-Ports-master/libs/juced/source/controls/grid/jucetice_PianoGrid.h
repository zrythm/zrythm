/*
 ==============================================================================

 This file is part of the JUCETICE project - Copyright 2009 by Lucio Asnaghi.

 JUCETICE is based around the JUCE library - "Jules' Utility Class Extensions"
 Copyright 2007 by Julian Storer.

 ------------------------------------------------------------------------------

 JUCE and JUCETICE can be redistributed and/or modified under the terms of
 the GNU General Public License, as published by the Free Software Foundation;
 either version 2 of the License, or (at your option) any later version.

 JUCE and JUCETICE are distributed in the hope that they will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with JUCE and JUCETICE; if not, visit www.gnu.org/licenses or write to
 Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 Boston, MA 02111-1307 USA

 ==============================================================================
*/

#ifndef __JUCETICE_PIANOGRID_HEADER__
#define __JUCETICE_PIANOGRID_HEADER__

#include "jucetice_PianoGridNote.h"
#include "jucetice_PianoGridHeader.h"
#include "jucetice_PianoGridIndicator.h"
#include "jucetice_PianoGridKeyboard.h"

class MidiGrid;

//==============================================================================
class MidiGridListener
{
public:

    virtual ~MidiGridListener () {}

    virtual bool timeSignatureChanged (const int barsCount,
                                       const int timeDenominator) = 0;

    virtual bool playingPositionChanged (const float absolutePosition) = 0;

protected:

    MidiGridListener () {}
};



//==============================================================================
class PianoGridListener
{
public:

    virtual ~PianoGridListener () {}

    virtual bool noteAdded (const int noteNumber,
                            const float beatNumber,
                            const float noteLength) = 0;

    virtual bool noteRemoved (const int noteNumber,
                              const float beatNumber,
                              const float noteLength) = 0;

    virtual bool noteMoved (const int oldNote,
                            const float oldBeat,
                            const int noteNumber,
                            const float beatNumber,
                            const float noteLength) = 0;

    virtual bool noteResized (const int noteNumber,
                              const float beatNumber,
                              const float noteLength) = 0;

    virtual bool allNotesRemoved () = 0;

protected:

    PianoGridListener () {}
};

//==============================================================================
class AutomationGridListener
{
public:

    virtual ~AutomationGridListener () {}

    virtual bool eventAdded (const int controller, const double automationValue,
                            const float beatNumber) = 0;

    virtual bool eventRemoved (const int controller, const double automationValue,
                            const float beatNumber) = 0;

    virtual bool eventMoved (const int controllerNum,const double oldValue,
                            const float oldBeat,
                            const double automationValue,
                            const float beatNumber) = 0;

    virtual bool allEventsRemoved () = 0;

protected:

    AutomationGridListener () {}
};

//==============================================================================
class MidiGridSelectedItems   : public SelectedItemSet<MidiGridItem*>
{
public:
    void itemSelected (MidiGridItem* item);
    void itemDeselected (MidiGridItem* item);
};


//==============================================================================
// Base class for midi grid editor components, e.g. pianoroll, cc automation etc.
// Handles the beat grid and a list of generic events (e.g. notes or automation points).
class MidiGrid : public Component,
                 public LassoSource<MidiGridItem*>
{
public:

    //==============================================================================
    MidiGrid ();
    ~MidiGrid ();

    //==============================================================================
    void setNumBars (const int newBarNumber);
    int getNumBars() const                              { return numBars; }
    void setBarWidth (const int newBarWidth);
    int getBarWidth () const                            { return barWidth; }
    int getVisibleBars () const;

    //==============================================================================
    void setTimeDivision (const int denominator);
    int getTimeDivision () const;

    void setSnapQuantize (const int snapQuantize);
    int getSnapQuantize () const;

    //==============================================================================
    void updateSize ();

    //==============================================================================
    void selectNote (MidiGridItem* note, const bool clearAllOthers);
    void clearNotesSelection ();

    //==============================================================================
    enum ColourIds
    {
        blackKeyColourId                 = 0x99002001,
        blackKeyBrightColourId           = 0x99002002,
        whiteKeyColourId                 = 0x99002003,
        whiteKeyBrightColourId           = 0x99002004,
        backGridColourId                 = 0x99002005,
        headerColourId                   = 0x99002006,
        indicatorColourId                = 0x99002007,
        noteFillColourId                 = 0x99002008,
        noteBorderColourId               = 0x99002009
    };

    //==============================================================================
	int getHeaderHeight() { return 12; };
    float getIndicatorPosition () const;
    void setIndicatorPosition (const float newPosition);
    void showIndicator (const bool isVisible);

    //==============================================================================
    void setListener (MidiGridListener* listener_) { listener = listener_; };

    //==============================================================================
    void notifyListenersOfTimeSignatureChange ();
    void notifyListenersOfPlayingPositionChanged (const float newPosition);

    //==============================================================================
    SelectedItemSet<MidiGridItem*> & getLassoSelection ();
    void findLassoItemsInArea (Array <MidiGridItem*>& itemsFound,
							   const Rectangle<int>& area);

    void paintBarLines(Graphics& g);

    //==============================================================================
    /** @internal */
    void paint(Graphics& g);
    /** @internal */
    void resized();
    /** @internal */
    void mouseDown(const MouseEvent& e);
    /** @internal */
    void mouseDrag(const MouseEvent& e);
    /** @internal */
    void mouseUp(const MouseEvent& e);
    /** @internal */
    void mouseMove(const MouseEvent& e);
    /** @internal */
    void mouseExit(const MouseEvent& e);

protected:

    OwnedArray<MidiGridItem> notes;

    bool isAddOrResizeEvent(const MouseEvent& e);
    bool isLassoEvent(const MouseEvent& e);

    //==============================================================================
    int divDenominator;
    int snapQuantize;

    int numBars;

    int barWidth;

    float beatDelta;

    PianoGridHeader* header;
    PianoGridIndicator* indicator;

    MidiGridSelectedItems selectedNotes;
    LassoComponent<MidiGridItem*>* lassoComponent;
	
	MidiGridListener* listener;
};

class PianoGrid : public MidiGrid
{
public:

    //==============================================================================
    PianoGrid ();
    ~PianoGrid ();

    //==============================================================================
    void setNumRows (const int newRowsCount);
    int getNumRows () const                             { return numRows; }
    void setRowsOffset (const int newRowsOffset);
    int getRowsOffset () const                          { return rowsOffset; }
    void setRowHeight (const int newRowHeight);
    int getRowHeight () const                           { return rowHeight; }
    int getVisibleRows () const;

    //==============================================================================
    bool getRowsColsByMousePosition (const int x, const int y,
                                     int& noteNumber, float& beatNumber);

    //==============================================================================
    void setNoteLengthInBeats (const float beats)       { defaultNoteLength = beats; }
    float getNoteLengthInBeats () const                 { return defaultNoteLength; }

    //==============================================================================
    void addNote (const int noteNumber,
                  const float beatNumber,
                  const float newLength);

    void removeAllNotes (const bool notifyListeners = true);

    void addNote (PianoGridNote* note);
    void moveNote (PianoGridNote* note, const int noteNumber, const float beatNumber);
    void resizeNote (PianoGridNote* note, const float beatNumber, const float length);
    void changeNoteVelocity (PianoGridNote* note, const float newVelocity);
    void removeNote (PianoGridNote* note, const bool alsoFreeObject = true);

    //==============================================================================
    int getNoteIndex (const int noteNumber, const float beatNumber);
    PianoGridNote* getNote (const int noteNumber, const float beatNumber);
    Rectangle<int> getNoteRect (PianoGridNote* note);
    
    //==============================================================================
    /** @internal */
    void paint(Graphics& g);
    /** @internal */
    void resized();
    /** @internal */
    void mouseDown(const MouseEvent& e);
    /** @internal */
    void mouseDrag(const MouseEvent& e);
    /** @internal */
    void mouseUp(const MouseEvent& e);
    /** @internal */
    void mouseMove(const MouseEvent& e);
    /** @internal */
    void mouseExit(const MouseEvent& e);

protected:

    friend class PianoGridNote;
    friend class PianoGridIndicator;

	PianoGridNote* getNote(int i) { return dynamic_cast<PianoGridNote*>(notes.getUnchecked (i)); }

    PianoGridNote* draggingNote;
    int draggingRow;
    float draggingColumn;

    int numRows;
    int rowsOffset;
    int rowHeight;
    float defaultNoteLength;

	PianoGridListener* getListener() const { return dynamic_cast<PianoGridListener*>(listener); };
};

class AutomationGrid : public MidiGrid
{
public:
	AutomationGrid();

    //==============================================================================
	int getEventDiameter() { return 4; };
	int getAvailableHeight() { return getHeight() - (2 * getEventDiameter()) - getHeaderHeight(); };
    bool getRowsColsByMousePosition (const int x, const int y, double& value, double& beatNumber);

    //==============================================================================
	// sets the default parameters for events added by mouse clicks
	void setCurrentController(int controllerNum, Colour fill, Colour border);

    //==============================================================================
    AutomationEvent* addEvent (const int controller, const double value, const double newBeat, const Colour& colour, const Colour& outlineColour = Colour(0.0f, 0.0f, 0.0f, 0.0f));

    void removeAllEvents (const bool notifyListeners = true);

    void addNote (AutomationEvent* note);
    Rectangle<int> getNoteRect (AutomationEvent* note);
	void moveEvent (AutomationEvent* note, const double beatNumber, const double newValue);
	void removeNote (AutomationEvent* note, const bool alsoFreeObject);
	void removeAllEvents (const int controller);

    //==============================================================================
	void findLassoItemsInArea (Array <MidiGridItem*>& itemsFound,
							   const Rectangle<int>& area);
    
	//==============================================================================
	AutomationEvent* getEvent(int i) { return dynamic_cast<AutomationEvent*>(notes.getUnchecked (i)); }

    //==============================================================================
    /** @internal */
    void paint(Graphics& g);
    /** @internal */
    void mouseDown(const MouseEvent& e);
    /** @internal */
    void mouseDrag(const MouseEvent& e);
    /** @internal */
    void mouseUp(const MouseEvent& e);
    void resized();

	AutomationGridListener* getListener() const { return dynamic_cast<AutomationGridListener*>(listener); };
	
	AutomationEvent* draggingNote;
	AutomationEvent templateEvent;
    double draggingRow;
    double draggingColumn;
};

#endif // __JUCETICE_PIANOGRID_HEADER__
