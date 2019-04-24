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

//==============================================================================
void MidiGridSelectedItems::itemSelected (MidiGridItem* item)
{
    item->setSelected (true);
    item->repaint ();
}

void MidiGridSelectedItems::itemDeselected (MidiGridItem* item)
{
    item->setSelected (false);
    item->repaint ();
}


//==============================================================================
static const int noteAdjustOffset [12] = {
    /*C*/0,
        /*C#*/1,
    /*D*/1,
        /*D#*/1,
    /*E*/0,
    /*F*/1,
        /*F#*/2,
    /*G*/0,
        /*G#*/2,
    /*A*/0,
        /*A#*/2,
    /*B*/1
};


//==============================================================================
MidiGrid::MidiGrid ()
  : divDenominator (4),
    snapQuantize (1),
    numBars (1),
    barWidth (12),
    header (0),
    indicator (0)
{
    addAndMakeVisible (header = new PianoGridHeader (this));
    addAndMakeVisible (indicator = new PianoGridIndicator (this));
    addAndMakeVisible (lassoComponent = new LassoComponent<MidiGridItem*> ());

    setIndicatorPosition (0);
}

MidiGrid::~MidiGrid()
{
    deleteAndZero (header);
    deleteAndZero (indicator);
    deleteAndZero (lassoComponent);
}

//==============================================================================
void MidiGrid::setNumBars (const int newBarsCount)
{
    numBars = newBarsCount;

    updateSize ();
}

void MidiGrid::setBarWidth (const int newBarWidth)
{
    barWidth = newBarWidth;

    updateSize ();
}

int MidiGrid::getVisibleBars () const
{
    int count = 0;

    for (int b = 0, x = 0; (b < numBars) && (x <= getWidth ()); b++)
    {
        x += barWidth;
        count++;
    }

    return count;
}

//==============================================================================
void PianoGrid::setNumRows (const int newRowsCount)
{
    numRows = newRowsCount;

    setRowsOffset (rowsOffset);
}

void PianoGrid::setRowsOffset (const int newRowsOffset)
{
    rowsOffset = jmin (jmax (newRowsOffset, 0), numRows);
}

void PianoGrid::setRowHeight (const int newRowHeight)
{
    rowHeight = newRowHeight;
}

int PianoGrid::getVisibleRows () const
{
    float currentHeight = rowHeight - noteAdjustOffset [0];
    float pos_y = getHeight() - currentHeight;

    for (int row = rowsOffset; row < numRows; row++)
    {
        // check if we are inside
        if (pos_y <= 0)
            return row - rowsOffset;

        // offset for next height
        int octaveNumber = row / 12;
        int nextNoteNumber = (row + 1) % 12;
        int nextOctaveNumber = (row + 1) / 12;
        currentHeight = rowHeight
                        - noteAdjustOffset [nextNoteNumber]
                        - ((nextOctaveNumber != octaveNumber) ? 1 : 0);

        pos_y -= currentHeight;
    }

    return 0;
}

//==============================================================================
void MidiGrid::setTimeDivision (const int denominator)
{
    divDenominator = denominator;
}

int MidiGrid::getTimeDivision () const
{
    return divDenominator;
}

void MidiGrid::setSnapQuantize (const int quantize)
{
    snapQuantize = quantize;
}

int MidiGrid::getSnapQuantize () const
{
    return snapQuantize;
}

//==============================================================================
void MidiGrid::updateSize ()
{
    setSize (numBars * barWidth, getHeight());
}

void MidiGrid::resized()
{
    header->setBounds (0, 0, getWidth(), getHeaderHeight());
    indicator->setBounds (indicator->getX(), 0, indicator->getWidth(), getHeight());

    repaint ();
}


//==============================================================================
void MidiGrid::paintBarLines (Graphics& g)
{
    Colour blackKey = findColour (MidiGrid::blackKeyColourId);
    Colour blackKeyBright = findColour (MidiGrid::blackKeyBrightColourId);
    Colour whiteKey = findColour (MidiGrid::whiteKeyColourId);
    Colour whiteKeyBright = findColour (MidiGrid::whiteKeyBrightColourId);
    Colour rowGrid = findColour (MidiGrid::backGridColourId);
    Colour rowGridDark = rowGrid.darker (1.0f);
    Colour rowGridBright = rowGrid.brighter (1.0f);

    // draw columns
    float pos_x = 0.0f;
    int dynamicGridSize = 1;

    if (barWidth < 20)
        dynamicGridSize = 1;
    else if (barWidth < 80)
        dynamicGridSize = divDenominator * 1;
    else if (barWidth < 200)
        dynamicGridSize = divDenominator * 2;
    else if (barWidth < 440)
        dynamicGridSize = divDenominator * 4;
    else if (barWidth < 920)
        dynamicGridSize = divDenominator * 8;
    else if (barWidth < 1050)
        dynamicGridSize = divDenominator * 16;

    int beatDivider = jmax (1, dynamicGridSize / divDenominator);
    float deltaWidth = barWidth / (float) (dynamicGridSize);

    for (int b = 0; (b < numBars) && (pos_x <= getWidth ()); b++)
    {
        for (int i = 0; i < dynamicGridSize; i++)
        {
            int columnIsBarStart = (i == 0);
            int columnIsBeatStart = (i % beatDivider != 0);

            if (columnIsBarStart)
                g.setColour (rowGridDark);
            else if (columnIsBeatStart)
                g.setColour (rowGridBright);
            else
                g.setColour (rowGrid);

            g.drawVerticalLine ((int) pos_x, 0, getHeight());

            pos_x += deltaWidth;
        }
    }

    g.setColour (rowGrid);
    g.drawVerticalLine ((int) pos_x, 0, getHeight());
}

void MidiGrid::paint (Graphics& g)
{
	paintBarLines(g);
}

//==============================================================================
bool MidiGrid::isAddOrResizeEvent(const MouseEvent& e)
{
    return (e.mods.isLeftButtonDown() && e.mods.isCommandDown());
}

bool MidiGrid::isLassoEvent(const MouseEvent& e)
{
    return (e.mods.isLeftButtonDown() && !e.mods.isCommandDown());
}

void MidiGrid::mouseDown (const MouseEvent& e)
{
	if (isLassoEvent(e))
        lassoComponent->beginLasso (e, this);
}

void MidiGrid::mouseDrag (const MouseEvent& e)
{
	if (isLassoEvent(e))
        lassoComponent->dragLasso (e);
}

void MidiGrid::mouseUp (const MouseEvent& e)
{
	if (isLassoEvent(e))
        lassoComponent->endLasso ();
}

void MidiGrid::mouseMove (const MouseEvent& e)
{
}

void MidiGrid::mouseExit (const MouseEvent& e)
{
}

//==============================================================================
void MidiGrid::selectNote (MidiGridItem* note, const bool clearAllOthers)
{
    if (clearAllOthers)
        selectedNotes.deselectAll ();

    if (note)
        selectedNotes.addToSelection (note);
}

void MidiGrid::clearNotesSelection ()
{
    selectedNotes.deselectAll ();
}

//==============================================================================
float MidiGrid::getIndicatorPosition () const
{
    return indicator->getX() / (float) getWidth();
}

void MidiGrid::setIndicatorPosition (const float newPosition)
{
    int newX = roundFloatToInt (newPosition * getWidth());

    indicator->setBounds (newX, 0, 2, getHeight());
    indicator->toFront (false);
}

void MidiGrid::showIndicator (const bool isVisible)
{
    indicator->setVisible (isVisible);
}

//==============================================================================
void MidiGrid::findLassoItemsInArea (Array <MidiGridItem*>& itemsFound,
									 const Rectangle<int>& area)
{
#if 0
    const Rectangle lasso (area.getX(), area.getY(), area.getWidth(), area.getHeight());
    for (int i = 0; i < notes.size (); i++)
    {
        MidiGridItem* note = notes.getUnchecked (i);
        if (note->getBounds().intersects (lasso))
        {
            itemsFound.addIfNotAlreadyThere (note);
            selectedNotes.addToSelection (note);
        }
        else
        {
            selectedNotes.deselect (note);
        }
    }
#endif

    for (int i = 0; i < notes.size (); i++)
    {
        MidiGridItem* note = notes.getUnchecked (i);
        if ((note->getX() >= area.getX() && note->getX() < area.getX() + area.getWidth())
            && (note->getY() >= area.getY() && note->getY() < area.getY() + area.getHeight()))
        {
            itemsFound.addIfNotAlreadyThere (note);
            selectedNotes.addToSelection (note);
        }
        else
        {
            selectedNotes.deselect (note);
        }
    }
}

SelectedItemSet<MidiGridItem*>& MidiGrid::getLassoSelection ()
{
    return selectedNotes;
}


void MidiGrid::notifyListenersOfTimeSignatureChange ()
{
    if (listener && listener->timeSignatureChanged (getNumBars(), divDenominator))
    {
    }
}

void MidiGrid::notifyListenersOfPlayingPositionChanged (const float newPosition)
{
    if (listener && listener->playingPositionChanged (newPosition))
    {
        setIndicatorPosition (newPosition);
    }
}

//==============================================================================
PianoGrid::PianoGrid()
  : draggingNote (0),
    draggingRow (-1),
    draggingColumn (-1),
    numRows (128),
    rowsOffset (0),
    rowHeight (8),
    defaultNoteLength (1.0f)
{
}

PianoGrid::~PianoGrid()
{
    // do not notify listeners, or they will clear their notes too !
    notes.clear (true);
}

Rectangle<int> PianoGrid::getNoteRect (PianoGridNote* note)
{
    // update rows number
    int x, w;
    float snapsPerBeat = 1.0f / (snapQuantize / (float) divDenominator);
    float snapWidth = barWidth / (float) snapQuantize;
    float beatWidth = barWidth / (float) divDenominator;

    if (snapQuantize == 0)
    {
        x = roundFloatToInt (beatWidth * note->getBeat());
        w = roundFloatToInt (beatWidth * note->getLength());
    }
    else
    {
        x = roundFloatToInt (snapWidth * (note->getBeat() / snapsPerBeat));
        w = roundFloatToInt (snapWidth * (note->getLength() / snapsPerBeat));
    }

    float currentHeight = rowHeight - noteAdjustOffset [0];
    float previousHeight = 0;
    float pos_y = getHeight() - currentHeight;

    for (int row = rowsOffset;
             (row < numRows) && ((pos_y + previousHeight) >= 12.0f);
             row++)
    {
        int octaveNumber = row / 12;
        previousHeight = currentHeight;

        // check if we are inside
        if (row == note->getNote ())
            return Rectangle<int> (x, (int)pos_y + 1, w, (int)previousHeight - 1);

        // offset for next height
        int nextNoteNumber = (row + 1) % 12;
        int nextOctaveNumber = (row + 1) / 12;
        currentHeight = rowHeight
                        - noteAdjustOffset [nextNoteNumber]
                        - ((nextOctaveNumber != octaveNumber) ? 1 : 0);

        pos_y -= currentHeight;
    }

    return Rectangle<int> (0,0,1,1);
}

//==============================================================================
void PianoGrid::paint (Graphics& g)
{
    Colour blackKey = findColour (MidiGrid::blackKeyColourId);
    Colour blackKeyBright = findColour (MidiGrid::blackKeyBrightColourId);
    Colour whiteKey = findColour (MidiGrid::whiteKeyColourId);
    Colour whiteKeyBright = findColour (MidiGrid::whiteKeyBrightColourId);
    Colour rowGrid = findColour (MidiGrid::backGridColourId);
    Colour rowGridDark = rowGrid.darker (1.0f);
    Colour rowGridBright = rowGrid.brighter (1.0f);

    float currentHeight = rowHeight - noteAdjustOffset [0];
    float previousHeight = 0;
    float pos_y = getHeight() - currentHeight;

    // draw rows
    for (int i = rowsOffset; (i < numRows) && ((pos_y + previousHeight) >= 0.0f); i++)
    {
        int noteNumber = i % 12;
        int octaveNumber = i / 12;
        previousHeight = currentHeight;

        switch (noteNumber)
        {
        case 1:
        case 3:
        case 6:
        case 8:
        case 10: // black keys
            g.setColour (blackKeyBright);
            break;

        default: // white keys
            g.setColour (whiteKeyBright);
            break;
        }

        // fill background
        g.fillRect (0, (int) pos_y + 1, getWidth(), (int) previousHeight - 1);

        // fill divider line
        g.setColour (rowGridBright);
        g.drawHorizontalLine ((int) pos_y, 0, getWidth());

        // offset for next height
        int nextNoteNumber = (i + 1) % 12;
        int nextOctaveNumber = (i + 1) / 12;
        currentHeight = rowHeight
                        - noteAdjustOffset [nextNoteNumber]
                        - ((nextOctaveNumber != octaveNumber) ? 1 : 0);

        pos_y -= currentHeight;
    }

	MidiGrid::paintBarLines(g);
}

//==============================================================================
void PianoGrid::addNote (const int newNote,
                         const float newBeat,
                         const float newLength)
{
    PianoGridNote* note;
    addAndMakeVisible (note = new PianoGridNote (this));
    note->initialize (newNote, newBeat, newLength, 1.0f);
    note->setBounds (getNoteRect (note));

    notes.add (note);
}

void PianoGrid::addNote (PianoGridNote* note)
{
	PianoGridListener* listener = getListener();
    if (listener && listener->noteAdded (note->getNote(), note->getBeat (), note->getLength ()))
    {
        addAndMakeVisible (note);
        notes.add (note);

        selectedNotes.selectOnly (note);
    }
}

void PianoGrid::removeNote (PianoGridNote* note, const bool alsoFreeObject)
{
	PianoGridListener* listener = getListener();
    if (listener && listener->noteRemoved (note->getNote(),
                                           note->getBeat (),
                                           note->getLength ()))
    {
        selectedNotes.deselect (note);

        notes.removeObject (note, alsoFreeObject);
    }
}

void PianoGrid::moveNote (PianoGridNote* note, const int newNote, const float newBeat)
{
    const int oldNote = note->getNote ();
    const float oldBeat = note->getBeat ();

	PianoGridListener* listener = getListener();
    if (listener && listener->noteMoved (oldNote,
                                         oldBeat,
                                         newNote,
                                         newBeat,
                                         note->getLength()))
    {
        note->setNote (newNote);

        float maxBeat = (numBars * divDenominator) - note->getLength();
        note->setBeat (jmin (jmax (newBeat, 0.0f), maxBeat));
    }
}

void PianoGrid::resizeNote (PianoGridNote* note, const float beatNumber, const float newLength)
{
	PianoGridListener* listener = getListener();
    if (listener && listener->noteResized (note->getNote(),
                                           note->getBeat(),
                                           note->getLength()))
    {
        float minLength = 0.0001f;
        float maxLength = (numBars * divDenominator) - beatNumber;

        if (snapQuantize > 0)
            minLength = defaultNoteLength;

        note->setLength (jmin (jmax (newLength, minLength), maxLength));
    }
}

void PianoGrid::changeNoteVelocity (PianoGridNote* note, const float newVelocity)
{
/*
	PianoGridListener* listener = getListener();
    if (listener && listener->noteChangeVelocity (note->getNote(),
                                                  note->getBeat(),
                                                  note->getLength(),
                                                  note->getVelocity ()))
*/
    {
        note->setVelocity (jmin (jmax (newVelocity, 0.0f), 1.0f));
    }
}

void PianoGrid::removeAllNotes (const bool notifyListeners)
{
    selectedNotes.deselectAll ();

    notes.clear (true);

    if (notifyListeners)
    {
		PianoGridListener* listener = getListener();
        if (listener)
            listener->allNotesRemoved ();
    }
}

//==============================================================================
bool PianoGrid::getRowsColsByMousePosition (const int x, const int y,
                                            int& noteNumber, float& beatNumber)
{
    bool returnOK = false;

    // update beats number
    float snapsPerBeat = 1.0f / (snapQuantize / (float) divDenominator);
    float snapWidth = barWidth / (float) snapQuantize;
    float beatWidth = barWidth / (float) divDenominator;

    if (snapQuantize == 0)
        beatNumber = x / beatWidth;
    else
        beatNumber = snapsPerBeat * (int) (x / snapWidth);

    // update rows number
    noteNumber = -1;

    float currentHeight = rowHeight - noteAdjustOffset [0];
    float previousHeight = 0;
    float pos_y = getHeight() - currentHeight;

    for (int row = rowsOffset;
             (row < numRows) && ((pos_y + previousHeight) >= 12.0f);
             row++)
    {
        previousHeight = currentHeight;

        // check if we are inside
        if (y >= pos_y && y < pos_y + previousHeight)
        {
            noteNumber = row;
            returnOK = true;
            break;
        }

        // offset for next height
        int octaveNumber = row / 12;
        int nextNoteNumber = (row + 1) % 12;
        int nextOctaveNumber = (row + 1) / 12;
        currentHeight = rowHeight
                        - noteAdjustOffset [nextNoteNumber]
                        - ((nextOctaveNumber != octaveNumber) ? 1 : 0);

        pos_y -= currentHeight;
    }

    beatNumber = jmin (jmax (beatNumber, 0.0f), (float) (numBars * divDenominator));
    noteNumber = jmin (jmax (noteNumber, 0), numRows - 1);

    return returnOK;
}

int PianoGrid::getNoteIndex (const int noteNumber, const float beatNumber)
{
    for (int i = notes.size(); --i >= 0;)
        if (getNote (i)->isNoteToggled (noteNumber, beatNumber))
            return i;
    return -1;
}

PianoGridNote* PianoGrid::getNote (const int noteNumber, const float beatNumber)
{
    for (int i = notes.size(); --i >= 0;)
        if (getNote (i)->isNoteToggled (noteNumber, beatNumber))
            return getNote (i);
    return 0;
}

void PianoGrid::resized ()
{
    for (int i = notes.size(); --i >= 0;)
    {
        PianoGridNote* note = getNote (i);
        note->setBounds (getNoteRect (note));
    }
	
	MidiGrid::resized();
}

void PianoGrid::mouseDown (const MouseEvent& e)
{
    if (isAddOrResizeEvent(e))
    {
        if (getRowsColsByMousePosition (e.x, e.y, draggingRow, draggingColumn))
        {
            draggingNote = new PianoGridNote (this);
            draggingNote->initialize (draggingRow, draggingColumn, defaultNoteLength, 1.0f);
            draggingNote->setBounds (getNoteRect (draggingNote));
            draggingNote->toFront (true);

            addNote (draggingNote);
        }
    }
    
	MidiGrid::mouseDown (e);
}

void PianoGrid::mouseDrag (const MouseEvent& e)
{
    if (isAddOrResizeEvent(e))
    {
        int newNote = -1;
        float newBeat = -1;

        if (draggingNote != 0 && getRowsColsByMousePosition (e.x, e.y, newNote, newBeat))
        {
            if (newBeat > draggingColumn)
            {
                float newLength = newBeat - draggingColumn;

                if (newLength != draggingNote->getLength())
                {
                    draggingNote->setLength (newLength);
                    draggingNote->setBounds (getNoteRect (draggingNote));
                }
            }
        }
    }

	MidiGrid::mouseDrag(e);
}

void PianoGrid::mouseUp (const MouseEvent& e)
{
    if (isAddOrResizeEvent(e))
    {
        if (draggingNote != 0)
        {
            resizeNote (draggingNote, draggingNote->getBeat(), draggingNote->getLength());

            draggingRow = -1;
            draggingColumn = -1;
            draggingNote = 0;
        }
    }
     
	MidiGrid::mouseUp(e);
}

void PianoGrid::mouseMove (const MouseEvent& e)
{
}

void PianoGrid::mouseExit (const MouseEvent& e)
{
}

//==============================================================================
AutomationGrid::AutomationGrid()
: draggingNote(NULL),
  templateEvent(NULL)
  
{

}

void AutomationGrid::setCurrentController(int controllerNum, Colour fill, Colour border)
{
	for (int i = 0; i < notes.size(); i++)
	{
		AutomationEvent* tmp = getEvent(i);
		// highlight the currently-selected events
		if (tmp->getController() == controllerNum)
		{
			tmp->setColour(fill);
			tmp->setOutlineColour(border);
			tmp->toFront(false);
		}
		// dim the previously-current events
		else if (tmp->getController() == templateEvent.getController())
		{
			tmp->setColour(templateEvent.getColour().brighter());
			tmp->setOutlineColour(Colour(0.0f, 0.0f, 0.0f, 0.0f)); // invisible border
		}
	}

	// set the default stuff
	templateEvent.setController(controllerNum);
	templateEvent.setColour(fill);
	templateEvent.setOutlineColour(border);

	repaint();
}

void AutomationGrid::paint (Graphics& g)
{
    Colour whiteKeyBright = findColour (MidiGrid::whiteKeyBrightColourId);

	g.setColour (whiteKeyBright);
	g.fillRect (0, 0, getWidth(), getHeight());

	MidiGrid::paintBarLines(g);

	// paint lines between each note
	Array<int> uniqueControllers;
	Array<Colour> matchingColours;
	MidiGridItem::SortByBeat sorter;
	notes.sort(sorter, true);
	for (int i=0; i<notes.size(); i++)
	{
		if (!uniqueControllers.contains(getEvent(i)->getController()) && (!draggingNote || (draggingNote->getController() != getEvent(i)->getController())))
		{
			uniqueControllers.add(getEvent(i)->getController());
			matchingColours.add(getEvent(i)->getColour());
		}
	}
	AutomationEvent *cur = NULL, *prev = NULL;
	for (int i=0; i<uniqueControllers.size(); i++)
	{
		prev = NULL;
		for (int j=0; j<notes.size(); j++)
		{
			cur = getEvent(j);
			if (cur->getController() == uniqueControllers[i])
			{
				if (cur && prev)
				{
					g.setColour (matchingColours[i]);
					int radius = cur->getHeight() / 2;
					g.drawLine(prev->getX() + radius, prev->getY() + radius, cur->getX() + radius, cur->getY() + radius, cur->getController() == templateEvent.getController() ? 2 : 1);
				}
				prev = cur;
			}
		}
	}
}

void AutomationGrid::resized ()
{
	// this code could be commonalified
	//  - getEventRect is virtual on MidiGrid, takes MidiGridItem* replaces getNoteRect)
	//  - getEvent could be common too
    for (int i = notes.size(); --i >= 0;)
    {
        AutomationEvent* note = getEvent (i);
        note->setBounds (getNoteRect (note));
    }
	
	MidiGrid::resized();
}

void AutomationGrid::mouseDown (const MouseEvent& e)
{
	double beat = 1;
	double value = 64;

    if (isAddOrResizeEvent(e))
    {
        if (getRowsColsByMousePosition (e.x, e.y, value, beat))
        {
			AutomationEvent* draggingNote;
            draggingNote = new AutomationEvent (this);
            draggingNote->initialize (templateEvent.getController(), value, beat, templateEvent.getColour(), templateEvent.getOutlineColour());
            draggingNote->setBounds (getNoteRect (draggingNote));
            draggingNote->toFront (true);

            addNote (draggingNote);
        }
    }
    
	MidiGrid::mouseDown (e);
	repaint();
}

void AutomationGrid::mouseDrag (const MouseEvent& e)
{
	MidiGrid::mouseDrag(e);
}

void AutomationGrid::mouseUp (const MouseEvent& e)
{
	MidiGrid::mouseUp(e);
	
	draggingNote = NULL;
}

//==============================================================================
bool AutomationGrid::getRowsColsByMousePosition (const int x, const int y,
                                            double& value, double& beatNumber)
{
    bool returnOK = true;

    // update beats number
    float snapsPerBeat = 1.0f / (snapQuantize / (float) divDenominator);
    float snapWidth = barWidth / (float) snapQuantize;
    float beatWidth = barWidth / (float) divDenominator;

    if (snapQuantize == 0)
        beatNumber = x / beatWidth;
    else
        //beatNumber = snapsPerBeat * (int)round(x / snapWidth);
		beatNumber = snapsPerBeat * (int)(x / snapWidth);
	
	int fullh = getAvailableHeight(); // account for header & space for item's circle at top or bottom
	
	value = (getHeight() - static_cast<double>(y)) / fullh; // flip upside down
	
	value = std::min(value, 1.0);
	value = std::max(value, 0.0);

    return returnOK;
}

Rectangle<int> AutomationGrid::getNoteRect (AutomationEvent* note)
{
	const int diameter = getEventDiameter();

    int x;
    float snapsPerBeat = 1.0f / (snapQuantize / (float) divDenominator);
    float snapWidth = barWidth / (float) snapQuantize;
    float beatWidth = barWidth / (float) divDenominator;

    if (snapQuantize == 0)
        x = roundFloatToInt (beatWidth * note->getBeat());
    else
        //x = roundFloatToInt (snapWidth * round(note->getBeat() / snapsPerBeat));
		x = roundFloatToInt (snapWidth * (note->getBeat() / snapsPerBeat));

	int fullh = getAvailableHeight(); // account for header & space for item's circle at top or bottom
	int y = fullh - (note->value * fullh); // upside down flip
	y += getEventDiameter() + getHeaderHeight();

	return Rectangle<int> (x-diameter, y-diameter, diameter*2, diameter*2);
}

//==============================================================================
AutomationEvent* AutomationGrid::addEvent (const int controller, const double value, const double newBeat, const Colour& colour, const Colour& outlineColour)
{
    AutomationEvent* note;
    addAndMakeVisible (note = new AutomationEvent (this));
    note->initialize (controller, value, newBeat, colour, outlineColour);
    note->setBounds (getNoteRect (note));

    notes.add (note);
	
	return note;
}

void AutomationGrid::removeAllEvents (const bool notifyListeners)
{
    selectedNotes.deselectAll ();

    notes.clear (true);

    if (notifyListeners)
    {
		AutomationGridListener* listener = getListener();
        if (listener)
            listener->allEventsRemoved ();
    }
	repaint();
}

void AutomationGrid::removeNote (AutomationEvent* note, const bool alsoFreeObject)
{
	AutomationGridListener* listener = getListener();
    if (listener && listener->eventRemoved (note->getController(), note->getValue(), note->getBeat ()))
    {
        selectedNotes.deselect (note);

        notes.removeObject (note, alsoFreeObject);
		repaint();
    }
}

void AutomationGrid::removeAllEvents (const int controller)
{
    for (int i = notes.size ()-1; i >= 0; i--)
    {
        AutomationEvent* note = getEvent (i);
		if (note->getController() == controller)
			removeNote(note, true);
	}
}

void AutomationGrid::addNote (AutomationEvent* note)
{
	AutomationGridListener* listener = getListener();
    if (listener && listener->eventAdded (note->getController(), note->getValue(), note->getBeat ()))
    {
        addAndMakeVisible (note);
        notes.add (note);

        selectedNotes.selectOnly (note);
    }
}

void AutomationGrid::moveEvent (AutomationEvent* note, const double beatNumber, const double newValue)
{
    const double oldNote = note->getValue ();
    const double oldBeat = note->getBeat ();

	AutomationGridListener* listener = getListener();
    if (listener && listener->eventMoved (note->getController(), oldNote, oldBeat, newValue, beatNumber))
    {
        note->setValue (newValue);
        note->setBeat (beatNumber);
    }
}

void AutomationGrid::findLassoItemsInArea (Array <MidiGridItem*>& itemsFound,
										   const Rectangle<int>& area)
{
#if 0
    const Rectangle lasso (area.getX(), area.getY(), area.getWidth(), area.getHeight());
    for (int i = 0; i < notes.size (); i++)
    {
        MidiGridItem* note = notes.getUnchecked (i);
        if (note->getBounds().intersects (lasso))
        {
            itemsFound.addIfNotAlreadyThere (note);
            selectedNotes.addToSelection (note);
        }
        else
        {
            selectedNotes.deselect (note);
        }
    }
#endif

    for (int i = 0; i < notes.size (); i++)
    {
        AutomationEvent* note = getEvent (i);
        if ((note->getX() >= area.getX() && note->getX() < area.getX() + area.getWidth())
            && (note->getY() >= area.getY() && note->getY() < area.getY() + area.getHeight())
			&& note->getController() == templateEvent.getController()) // overriding this method so we can only select events with same CC
        {
            itemsFound.addIfNotAlreadyThere (note);
            selectedNotes.addToSelection (note);
        }
        else
        {
            selectedNotes.deselect (note);
        }
    }
}

END_JUCE_NAMESPACE
