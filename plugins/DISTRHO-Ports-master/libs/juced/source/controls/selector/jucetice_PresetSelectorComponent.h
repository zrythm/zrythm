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
 
#ifndef __JUCETICE_PRESETSELECTORCOMPONENT_HEADER__
#define __JUCETICE_PRESETSELECTORCOMPONENT_HEADER__

#include "../../base/jucetice_AudioPlugin.h"


//==============================================================================
// this is the listbox containing the draggable source components..

class PresetSelectorComponent  : public ListBox,
                                 public ListBoxModel
{
public:

    //==============================================================================
    PresetSelectorComponent(AudioPlugin* const plugin_)
        : ListBox ("preset-source", 0),
          plugin (plugin_)
    {
        // tells the ListBox that this object supplies the info about
        // its rows.
        setModel (this);
        setMultipleSelectionEnabled (false);
    }

    ~PresetSelectorComponent()
    {
    }

    //==============================================================================
    // The following methods implement the necessary virtual functions from ListBoxModel,
    // telling the listbox how many rows there are, painting them, etc.
    int getNumRows()
    {
        return plugin->getNumPrograms ();
    }

    void paintListBoxItem (int rowNumber,
                           Graphics& g,
                           int width, int height,
                           bool rowIsSelected)
    {
        if (rowIsSelected)
            g.fillAll (Colours::slategrey);

        g.setColour (Colours::white);
        g.setFont (height * 0.7f);

        g.drawText (plugin->getProgramName (rowNumber),
                    5, 0, width, height,
                    Justification::centredLeft, false);
    }

    var getDragSourceDescription (const SparseSet<int>& selectedRows)
    {
        // for our drag description, we'll just make a list of the selected
        // row numbers - this will be picked up by the drag target and displayed in
        // its box.
        String desc;

        for (int i = 0; i < selectedRows.size(); ++i)
            desc << (selectedRows [i] + 1) << " ";

        return desc.trim();
    }

    //==============================================================================
    // this just fills in the background of the listbox
    void paint (Graphics& g)
    {
        g.fillAll (Colours::black);
    }

    void listBoxItemClicked (int row, const MouseEvent& e)
    {
        plugin->setCurrentProgram (row);
    }
    
protected:

    AudioPlugin* plugin;
};


//==============================================================================
// and this is a component that can have things dropped onto it..

class PresetSelectorTarget  : public Component,
                              public DragAndDropTarget
{
    bool somethingIsBeingDraggedOver;
    String message;

public:
    //==============================================================================
    PresetSelectorTarget()
    {
        somethingIsBeingDraggedOver = false;

        message = "Drag-and-drop onto this component!";
    }

    ~PresetSelectorTarget()
    {
    }

    //==============================================================================
    void paint (Graphics& g)
    {
        g.fillAll (Colours::green.withAlpha (0.2f));

        // draw a red line around the comp if the user's currently dragging something over it..
        if (somethingIsBeingDraggedOver)
        {
            g.setColour (Colours::red);
            g.drawRect (0, 0, getWidth(), getHeight(), 3);
        }

        g.setColour (Colours::black);
        g.setFont (14.0f);
        g.drawFittedText (message, 10, 0, getWidth() - 20, getHeight(), Justification::centred, 4);
    }

    //==============================================================================
    bool isInterestedInDragSource (const String& sourceDescription)
    {
        // normally you'd check the sourceDescription value to see if it's the
        // sort of object that you're interested in before returning true, but for
        // the demo, we'll say yes to anything..
        return true;
    }

    void itemDragEnter (const SourceDetails& dragSourceDetails)
    {
        somethingIsBeingDraggedOver = true;
        repaint();
    }

    void itemDragMove (const SourceDetails& dragSourceDetails)
    {
    }

    void itemDragExit (const SourceDetails& dragSourceDetails)
    {
        somethingIsBeingDraggedOver = false;
        repaint();
    }

    void itemDropped (const SourceDetails& dragSourceDetails,
                      int x, int y)
    {
        message = "last rows dropped: " + (String&) dragSourceDetails.description;

        somethingIsBeingDraggedOver = false;
        repaint();
    }
};


/*
//==============================================================================
class DragAndDropDemo  : public Component,
                         public DragAndDropContainer
{
    //==============================================================================
    PresetSelectorComponent* source;
    PresetSelectorTarget* target;

public:
    //==============================================================================
    DragAndDropDemo()
    {
        setName (T("Drag-and-Drop"));

        source = new PresetSelectorComponent();
        addAndMakeVisible (source);

        target = new PresetSelectorTarget();
        addAndMakeVisible (target);
    }

    ~DragAndDropDemo()
    {
        deleteAllChildren();
    }

    void resized()
    {
        source->setBounds (10, 10, 250, 150);
        target->setBounds (getWidth() - 260, getHeight() - 160, 250, 150);
    }

    //==============================================================================
    // (need to put this in to disambiguate the new/delete operators used in the
    // two base classes).
    juce_UseDebuggingNewOperator
};

*/

#endif
