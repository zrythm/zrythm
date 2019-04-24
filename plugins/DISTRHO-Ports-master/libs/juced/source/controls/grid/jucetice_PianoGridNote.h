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

#ifndef __JUCETICE_PIANOGRIDNOTE_HEADER__
#define __JUCETICE_PIANOGRIDNOTE_HEADER__

class MidiGrid;
class PianoGrid;
class AutomationGrid;

//==============================================================================
// Base class for timed midi events, i.e. notes or automation cc events.
// Has a position in beats, and is selectable.
class MidiGridItem : public Component
{
public:

    //==============================================================================
    MidiGridItem ();
    ~MidiGridItem ();

    //==============================================================================
    void setBeat (const float beat_)           { beat = beat_; }
    float getBeat () const                     { return beat; }

    //==============================================================================
    void setSelected (const bool selected_)    { hasBeenSelected = selected_; }
    bool isSelected () const                   { return hasBeenSelected; }

    //==============================================================================
    virtual void startDragging (const MouseEvent& e);
    virtual void continueDragging (const MouseEvent& e);
    virtual void endDragging (const MouseEvent& e);

    //==============================================================================
	struct SortByBeat
	{
		static int compareElements (MidiGridItem* first, MidiGridItem* second)
		{
			return ((10000 * (first->beat + 1)) - (10000 * (second->beat + 1)));
		}
	};
	
protected:

	MidiGrid* owner;
    float beat;
    bool hasBeenSelected;
    ComponentDragger dragger;
    bool isDragging;
};

//==============================================================================
class PianoGridNote : public MidiGridItem
{
public:

    //==============================================================================
    PianoGridNote (PianoGrid* owner);
    ~PianoGridNote ();

    //==============================================================================
    void initialize (const int note,
                     const float beat,
                     const float length,
                     const float velocity);

    //==============================================================================
    void setNote (const int note_)             { note = note_; }
    int getNote () const                       { return note; }

    //==============================================================================
    void setLength (const float length_)       { length = length_; }
    float getLength () const                   { return length; }

    //==============================================================================
    void setVelocity (const float velocity_)   { velocity = velocity_; }
    float getVelocity () const                 { return velocity; }

    //==============================================================================
    bool isNoteToggled (const int note, const float beat);

    //==============================================================================
    /** @internal */
    void mouseMove (const MouseEvent& e);
    /** @internal */
    void mouseDown (const MouseEvent& e);
    /** @internal */
    void mouseDrag (const MouseEvent& e);
    /** @internal */
    void mouseUp (const MouseEvent& e);
    /** @internal */
    void paint (Graphics& g);

protected:

    //==============================================================================
    void continueDragging (const MouseEvent& e);

    //==============================================================================
    void startResizing (const MouseEvent& e);
    void continueResizing (const MouseEvent& e);
    void endResizing (const MouseEvent& e);

    //==============================================================================
    void startVelocity (const MouseEvent& e);
    void continueVelocity (const MouseEvent& e);
    void endVelocity (const MouseEvent& e);

    int note;
    float length;
    float velocity;

    PianoGrid* owner;
    bool isResizing;
    bool isEditingVelocity;
};


class AutomationEvent : public MidiGridItem
{
public:

    //==============================================================================
    AutomationEvent (AutomationGrid* owner);
    ~AutomationEvent ();

    //==============================================================================
    void initialize (const int controller,
					 const double value,
                     const double beat,
					 const Colour colour,
					 const Colour outlineColour);

    //==============================================================================
    void setValue (const double value_);
    double getValue () const                       { return value; }
	void setController(const int ctrl) { controller = ctrl; }; 
	int getController() const { return controller; }; 

    void setColour (const Colour value_)             { colour = value_; }
    Colour getColour () const                       { return colour; }
    void setOutlineColour (const Colour value_)     { outlineColour = value_; }
    Colour getOutlineColour () const                       { return outlineColour; }

    void continueDragging (const MouseEvent& e);

    //==============================================================================
    /** @internal */
    void mouseDown (const MouseEvent& e);
    /** @internal */
    void mouseDrag (const MouseEvent& e);
    /** @internal */
    void mouseUp (const MouseEvent& e);
    /** @internal */
    void paint (Graphics& g);

    AutomationGrid* owner;

	double value;
	int controller;
	Colour colour;
	Colour outlineColour;
};

#endif // __JUCETICE_PIANOGRIDNOTE_HEADER__
