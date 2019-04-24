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

#ifndef __JUCETICE_MIDI_MANIPULATOR_HEADER__
#define __JUCETICE_MIDI_MANIPULATOR_HEADER__

#include "jucetice_MidiFilter.h"
#include "jucetice_MidiTransform.h"

//==============================================================================
class MidiManipulator
{
public:

    MidiManipulator ();
    virtual ~MidiManipulator ();

    //==============================================================================
    void setMidiFilter (MidiFilter* newMidiFilter);
    void setMidiTransform (MidiTransform* newMidiTransform);

    //==============================================================================
    virtual void prepareToPlay (const double sampleRate_, const int blockSize_);
    virtual void releaseResources ();

    //==============================================================================
    virtual void processEvents (MidiBuffer& midiMessages, const int blockSize);
    
protected:
    
    MidiFilter* filter;
    MidiTransform* transform;
    
    double sampleRate;
    int blockSize;
};

#endif