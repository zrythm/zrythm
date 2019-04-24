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

BEGIN_JUCE_NAMESPACE

#include "jucetice_PianoGrid.h"


//==============================================================================
MidiGridItem::MidiGridItem ()
  : beat(0),
    hasBeenSelected (false),
    isDragging (false)
{
}

MidiGridItem::~MidiGridItem ()
{
}

//==============================================================================
void MidiGridItem::startDragging (const MouseEvent& e)
{
    isDragging = true;

    dragger.startDraggingComponent (this, e);

    toFront (true);
    repaint ();
}

void MidiGridItem::continueDragging (const MouseEvent& e)
{
    dragger.dragComponent (this, e, 0);
}

void MidiGridItem::endDragging (const MouseEvent& e)
{
    isDragging = false;
    repaint ();
}

//==============================================================================
PianoGridNote::PianoGridNote (PianoGrid* owner_)
  : note (0),
    length (0.0f),
    velocity (1.0f),
    owner (owner_),
    isResizing (false),
    isEditingVelocity (false)
{
    setOpaque (false);
    toFront (false);
}

PianoGridNote::~PianoGridNote()
{
}

//==============================================================================
void PianoGridNote::initialize (const int note_,
                                const float beat_,
                                const float length_,
                                const float velocity_)
{
    note = note_;
    beat = beat_;
    length = length_;
    velocity = velocity_;
}

//==============================================================================
bool PianoGridNote::isNoteToggled (const int note_, const float beat_)
{
    return ((beat_ >= beat && beat_ <= (int)(beat + length)) && (note_ == note));
}

void PianoGridNote::continueDragging (const MouseEvent& e)
{
    int originalX = getX();
    int originalY = getY();

    dragger.dragComponent (this, e, 0);

    int newNote = -1;
    float newBeat = -1;
    if (owner->getRowsColsByMousePosition (getX(), getY(), newNote, newBeat))
    {
        if (newNote != note || newBeat != beat)
        {
            owner->moveNote (this, newNote, newBeat);
        }

        setBounds (owner->getNoteRect (this));
    }
    else
    {
        setTopLeftPosition (originalX, originalY);
    }
}

//==============================================================================
void PianoGridNote::startResizing (const MouseEvent& e)
{
    isResizing = true;

    toFront (true);
    repaint ();
}

void PianoGridNote::continueResizing (const MouseEvent& e)
{
    int newNote = -1;
    float newBeat = -1;
    if (owner->getRowsColsByMousePosition (getX() + e.x, getY() + e.y, newNote, newBeat))
    {
        if (newBeat > beat)
        {
            float newLength = newBeat - beat;

            if (newLength != length)
            {
                owner->resizeNote (this, beat, newLength);
                setBounds (owner->getNoteRect (this));
            }
        }
    }
}

void PianoGridNote::endResizing (const MouseEvent& e)
{
    isResizing = false;
    repaint ();
}

//==============================================================================
void PianoGridNote::startVelocity (const MouseEvent& e)
{
    isEditingVelocity = true;

    setMouseCursor (MouseCursor (MouseCursor::UpDownResizeCursor));

    toFront (true);
    repaint ();
}

void PianoGridNote::continueVelocity (const MouseEvent& e)
{
    float newVelocity = velocity + e.getDistanceFromDragStartY () / 100.0f;

    if (newVelocity != velocity)
    {
        owner->changeNoteVelocity (this, newVelocity);
        repaint();
    }
}

void PianoGridNote::endVelocity (const MouseEvent& e)
{
    isEditingVelocity = false;

    setMouseCursor (MouseCursor (MouseCursor::NormalCursor));

    repaint ();
}


//==============================================================================
void PianoGridNote::mouseMove (const MouseEvent& e)
{
    if (e.x >= getWidth() - 2)
    {
        setMouseCursor (MouseCursor (MouseCursor::LeftRightResizeCursor));
    }
    else
    {
        setMouseCursor (MouseCursor (MouseCursor::NormalCursor));
    }
}

void PianoGridNote::mouseDown (const MouseEvent& e)
{
    if (! owner) return;

    SelectedItemSet<MidiGridItem*> selection = owner->getLassoSelection ();
    if (! selection.isSelected (this))
        owner->selectNote (this, true);

    if (e.mods.isLeftButtonDown())
    {
        if (e.x >= getWidth() - 2)
        {
            for (int i = 0; i < selection.getNumSelected (); i++)
            {
                PianoGridNote* component = dynamic_cast<PianoGridNote*>(selection.getSelectedItem (i));
				if (component)
					component->startResizing (e);
            }
        }
        else
        {
            for (int i = 0; i < selection.getNumSelected (); i++)
            {
                MidiGridItem* component = selection.getSelectedItem (i);
                component->startDragging (component == this ? e : e.getEventRelativeTo (component));
            }
        }

        DBG ("Note: " + String (note) + " " + String (beat) + " " + String (length));
    }
    else if (e.mods.isMiddleButtonDown())
    {
        for (int i = 0; i < selection.getNumSelected (); i++)
        {
            PianoGridNote* component = dynamic_cast<PianoGridNote*>(selection.getSelectedItem (i));
			if (component)
				component->startVelocity (e);
        }
    }
}

void PianoGridNote::mouseDrag (const MouseEvent& e)
{
    if (! owner) return;

    SelectedItemSet<MidiGridItem*> selection = owner->getLassoSelection ();

    if (e.mods.isLeftButtonDown())
    {
        if (isResizing)
        {
            for (int i = 0; i < selection.getNumSelected (); i++)
            {
            PianoGridNote* component = dynamic_cast<PianoGridNote*>(selection.getSelectedItem (i));
			if (component)
                component->continueResizing (e);
            }
        }
        else if (isDragging)
        {
            for (int i = 0; i < selection.getNumSelected (); i++)
            {
                MidiGridItem* component = selection.getSelectedItem (i);
                component->continueDragging (component == this ? e : e.getEventRelativeTo (component));
            }
        }
    }
    else if (e.mods.isMiddleButtonDown())
    {
        if (isEditingVelocity)
        {
            for (int i = 0; i < selection.getNumSelected (); i++)
            {
				PianoGridNote* component = dynamic_cast<PianoGridNote*>(selection.getSelectedItem (i));
				if (component)
					component->continueVelocity (e);
            }
        }
    }
}

void PianoGridNote::mouseUp (const MouseEvent& e)
{
    if (owner)
    {
        SelectedItemSet<MidiGridItem*> selection = owner->getLassoSelection ();

        if (e.mods.isLeftButtonDown())
        {
            if (isResizing)
            {
                for (int i = 0; i < selection.getNumSelected (); i++)
                {
					PianoGridNote* component = dynamic_cast<PianoGridNote*>(selection.getSelectedItem (i));
					if (component)
						component->endResizing (e);
                }
            }
            else if (isDragging)
            {
                for (int i = 0; i < selection.getNumSelected (); i++)
                {
                    MidiGridItem* component = selection.getSelectedItem (i);
                    component->endDragging (e);
                }
            }

            repaint ();
        }
        else if (e.mods.isMiddleButtonDown())
        {
            if (isEditingVelocity)
            {
                for (int i = 0; i < selection.getNumSelected (); i++)
                {
					PianoGridNote* component = dynamic_cast<PianoGridNote*>(selection.getSelectedItem (i));
					if (component)
						component->endVelocity (component == this ? e : e.getEventRelativeTo (component));
                }
            }
        }
        else if (e.mods.isRightButtonDown())
        {
            for (int i = 0; i < selection.getNumSelected (); i++)
            {
				PianoGridNote* component = dynamic_cast<PianoGridNote*>(selection.getSelectedItem (i));
                if (component && component != this)
                    owner->removeNote (component, true);
            }

            owner->removeNote (this, false);
            delete (this);
        }
    }
}

void PianoGridNote::paint (Graphics& g)
{
    Colour fillColour = findColour (PianoGrid::noteFillColourId);
    Colour borderColour = findColour (PianoGrid::noteBorderColourId);

    if (isDragging || isResizing)
    {
        if (hasBeenSelected) {
            fillColour = fillColour.darker (0.4f);
            borderColour = borderColour.darker (0.4f);
        }
        else
        {
            fillColour = fillColour.brighter (0.4f);
            borderColour = borderColour.brighter (0.4f);
        }
    }
    else
    {
        if (hasBeenSelected) {
            fillColour = fillColour.brighter (0.4f);
            borderColour = borderColour.brighter (0.4f);
        }
    }

    float alphaStep = 0.2f + velocity * 0.8f;
    fillColour = fillColour.withAlpha (alphaStep);
    borderColour = borderColour.withAlpha (alphaStep);

    g.fillAll (fillColour);
    g.setColour (borderColour);
    g.drawRect (0, 0, getWidth(), getHeight());
}

//==============================================================================
AutomationEvent::AutomationEvent (AutomationGrid* owner_)
  : owner (owner_),
	value (0),
    controller(0)
{
}

AutomationEvent::~AutomationEvent()
{
}

void AutomationEvent::initialize (const int controller_,
					 const double value_,
                     const double beat_,
					 const Colour colour_,
					 const Colour outlineColour_)
{
	controller = controller_;
	value = value_;
	beat = beat_;
	colour = colour_;
	outlineColour = outlineColour_;
}

void AutomationEvent::setValue (const double value_)
{
	value = value_; 
	value = std::max(value, 0.0); 
	value = std::min(value, 1.0);
}

void AutomationEvent::mouseDown (const MouseEvent& e)
{
    if (! owner) return;

    SelectedItemSet<MidiGridItem*> selection = owner->getLassoSelection ();
    if (! selection.isSelected (this))
        owner->selectNote (this, true);

    if (e.mods.isLeftButtonDown())
    {
		for (int i = 0; i < selection.getNumSelected (); i++)
		{
			MidiGridItem* component = selection.getSelectedItem (i);
			component->startDragging (component == this ? e : e.getEventRelativeTo (component));
			owner->repaint();
		}
    }
}


void AutomationEvent::mouseDrag (const MouseEvent& e)
{
    if (! owner) return;

    SelectedItemSet<MidiGridItem*> selection = owner->getLassoSelection ();

    if (e.mods.isLeftButtonDown())
    {
		if (isDragging)
        {
            for (int i = 0; i < selection.getNumSelected (); i++)
            {
                MidiGridItem* component = selection.getSelectedItem (i);
                component->continueDragging (component == this ? e : e.getEventRelativeTo (component));
				owner->repaint();
            }
        }
    }
}

void AutomationEvent::mouseUp (const MouseEvent& e)
{
    if (owner)
    {
        SelectedItemSet<MidiGridItem*> selection = owner->getLassoSelection ();

        if (e.mods.isLeftButtonDown())
        {
			for (int i = 0; i < selection.getNumSelected (); i++)
			{
				MidiGridItem* component = selection.getSelectedItem (i);
				component->endDragging (e);
				owner->repaint();
			}

            repaint ();
        }
		else if (e.mods.isRightButtonDown())
        {
            for (int i = 0; i < selection.getNumSelected (); i++)
            {
				AutomationEvent* component = dynamic_cast<AutomationEvent*>(selection.getSelectedItem (i));
                if (component && component != this)
                    owner->removeNote (component, true);
            }

            owner->removeNote (this, false);
            delete (this);
        }
    }
}

void AutomationEvent::continueDragging (const MouseEvent& e)
{
    int originalX = getX();
    int originalY = getY();

    dragger.dragComponent (this, e, 0);

    double newValue = -1;
    double newBeat = -1;
    if (owner->getRowsColsByMousePosition (getX(), getY(), newValue, newBeat))
    {
        if (newValue != value || newBeat != beat)
        {
            owner->moveEvent (this, newBeat, newValue);
        }

        setBounds (owner->getNoteRect (this));
    }
    else
    {
        setTopLeftPosition (originalX, originalY);
    }
}

void AutomationEvent::paint (Graphics& g)
{
    Colour fillColour = colour;
    Colour borderColour = outlineColour;
	int borderWidth = 1;
	
	if (hasBeenSelected) 
	{
		fillColour = fillColour.darker (0.6);
		borderColour = borderColour.darker ();
		borderWidth = 2;
	}

    g.setColour (fillColour);
    g.fillEllipse (0, 0, getWidth(), getHeight());	
    g.setColour (borderColour);
    g.drawEllipse (0, 0, getWidth(), getHeight(), borderWidth);
}

END_JUCE_NAMESPACE
