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

#ifndef __JUCETICE_MIDI_FILTER_HEADER__
#define __JUCETICE_MIDI_FILTER_HEADER__

//==============================================================================
class MidiFilter
{
public:

    //==============================================================================
    MidiFilter ();
    virtual ~MidiFilter ();

    //==============================================================================
    void setUseChannelFilter (const bool useFilter);
    bool isUsingChannelFilter () const                 { return useChannelFilter; }

    void clearAllChannels ();

    void setChannel (const int channelNumber);
    void unsetChannel (const int channelNumber);

    bool isChannelSet (const int channelNumber) const  { return channels [channelNumber] == 1; }

    //==============================================================================
    void setUseNoteFilter (const bool useFilter);
    bool isUsingNoteFilter () const                    { return useNoteFilter; }

    void setNoteMin (const int newNoteMin);
    void setNoteMax (const int newNoteMax);

    int getNoteMin () const                            { return noteMin; }
    int getNoteMax () const                            { return noteMax; }

    //==============================================================================
    void setUseVelocityFilter (const bool useFilter);
    bool isUsingVelocityFilter () const                { return useVelocityFilter; }

    void setVelocityMin (const int newVelocityMin);
    void setVelocityMax (const int newVelocityMax);

    int getVelocityMin () const                        { return velocityMin; }
    int getVelocityMax () const                        { return velocityMax; }

    //==============================================================================
    void setUsePitchWeelFilter (const bool useFilter);
    bool isUsingPitchWeelFilter () const               { return usePitchWeelFilter; }

    //==============================================================================
    virtual bool filterEvent (const MidiMessage& message);
    
protected:

    bool useChannelFilter     : 1,
         useNoteFilter        : 1,
         useVelocityFilter    : 1,
         usePitchWeelFilter   : 1;   

    BigInteger channels;
    int noteMin, noteMax;
    int velocityMin, velocityMax;
    int pitchMin, pitchMax;
};

#endif
