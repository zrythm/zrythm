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

#ifndef __JUCETICE_MIDI_LEARN_HEADER__
#define __JUCETICE_MIDI_LEARN_HEADER__

class MidiAutomatorManager;


//==============================================================================
/**
     A midi learning object
*/
class MidiAutomatable
{
public:

#if 0
    typedef FastDelegate1<int, float> MidiTransferDelegate;

    //==============================================================================
    /** TODO */
    enum MidiTransferFunction
    {
        Linear = 0,
        InvertedLinear
    };
#endif

    //==============================================================================
    /** Destructor */
    virtual ~MidiAutomatable();

    //==============================================================================
    /** It actually start midi learning */
    void activateLearning ();

    //==============================================================================
    /** Returns the actual controller number */
    int getControllerNumber () const                  { return controllerNumber; }

    /** Set a new controller number */
    void setControllerNumber (const int control);

#if 0
    //==============================================================================
    /** Returns the actual controller number */
    MidiTransferFunction getTransferFunction () const { return transfer; }

    /** Set a new controller number */
    void setTransferFunction (MidiTransferFunction control);
#endif

    //==============================================================================
    /** This is used inside a mouseDown of a corresponding Component */
    void handleMidiPopupMenu (const MouseEvent& e);

    //==============================================================================
    /** Handle a midi message coming in */
    virtual bool handleMidiMessage (const MidiMessage& message) = 0;

protected:

    friend class MidiAutomatorManager;

    int controllerNumber;

#if 0
    MidiTransferFunction transfer;
    MidiTransferDelegate function;
#endif

    MidiAutomatorManager* midiAutomatorManager;

    //==============================================================================
    /** Attach a new manager */
    void setMidiAutomatorManager (MidiAutomatorManager* newManager);
    
    //==============================================================================
    /** Constructor */
    MidiAutomatable ();
};


//==============================================================================
/**
     A midi learning object
*/
class MidiAutomatorManager
{
public:

    //==============================================================================
    /** Constructor */
    MidiAutomatorManager ();

    /** Destructor */
    ~MidiAutomatorManager();

    //==============================================================================
    /** Register a new object

        The function is important because it actually tells to our objects which
        is the actual midi learn manager where we listen to.
        
        It is the first thing to call in order to use the actual learning caps.
    */ 
    void registerMidiAutomatable (MidiAutomatable* object);

    /** Deregister an existing object

        The function is important because it actually tells to our objects which
        is the actual midi learn manager where we listen to.
    */ 
    void removeMidiAutomatable (MidiAutomatable* object);

    //==============================================================================
    /** Deregister all objects listening to a CC number */ 
    void clearMidiAutomatableFromCC (const int ccNumber);

    //==============================================================================
    /** Handle a single midi message */
    bool handleMidiMessage (const MidiMessage& message);

    /** Convenience function that handle multiple midi messages */
    bool handleMidiMessageBuffer (MidiBuffer& buffer);

protected:

    friend class MidiAutomatable;

    //==============================================================================
    /** Tells which is the learning object */
    void setActiveLearner (MidiAutomatable* object);

    Array<Array<void*>*> controllers;
    MidiAutomatable* activeLearner;
};

#endif

