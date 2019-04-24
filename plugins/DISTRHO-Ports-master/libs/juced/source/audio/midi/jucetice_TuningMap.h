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

#ifndef __JUCETICE_TUNINGMAP_HEADER__
#define __JUCETICE_TUNINGMAP_HEADER__

//==============================================================================
/**
    This holds information on a tuning map, useful if you want to use
    different scales when playing a midi synth.

    In fact this class permit to store a single scale, reading it from a Scala
    or VAZ .tun files, and permits to handle microtuning for your synth

*/
class TuningMap
{
public:

    //==============================================================================
    /** Creates an empty TuningMap object. */
    TuningMap();

    /** Destructor */
    ~TuningMap();

    //==============================================================================
    /** Returns the base freq in Hertz of the current scale. */
    double getBaseFreq() const                          { return m_dblBaseFreq; }

    /** Returns the absolute tune in Hertz of the selected note.

        If the note is < 0 or > 127 then a default value is returned.
    */
    double getNoteFreq (int nNoteIndex) const
    {
        return m_dblBaseFreq * pow (2.0, getRelativeTune (nNoteIndex) / 1200.0);
    }

    /** Returns the relative tune in Cents of the selected note.

        If the note is < 0 or > 127 then a default value is returned.
    */
    double getRelativeTune (int nNoteIndex) const
    {
        // First make sure, that the note index is in the valid range
        // If not, return a "standard value"
        if ( (nNoteIndex >= 0) && (nNoteIndex <= 127) )
            return m_dblTunes[nNoteIndex];
        else
            return 100 * nNoteIndex;
    }

    //==============================================================================
    /** Set the absolute base frequency in Hertz for the selected tuning map. */
    bool setBaseFreq (double dblBaseFreq);

    /** Set the relative base frequency in cents for the selected tuning map. */
    bool setRelativeTune (int nNoteIndex, double dblTune);

    //==============================================================================
    /** Read the tuning map from an input stream.

        The format must be Scala or VAZ Plus 1.5 tuning map files, otherwise
        false is returned and no tuning map is loaded.
    */
    bool readFromStream (InputStream& in);

    /** Write the tuning map out to a output stream. */
    bool writeToStream (OutputStream& out, bool bSaveBaseFreq = false);

    /** Reset the current tuning map. */
    void reset();

    //==============================================================================
    juce_UseDebuggingNewOperator

private:

    //==============================================================================
    double m_dblTunes[128];
    double m_dblBaseFreq;
};

#endif