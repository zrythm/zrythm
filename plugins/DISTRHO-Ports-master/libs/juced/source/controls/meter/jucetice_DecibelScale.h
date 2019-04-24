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

   @author  Rui Nuno Capela
   @tweaker Lucio Asnaghi

 ==============================================================================
*/

#ifndef __JUCETICE_DECIBELSCALE_HEADER__
#define __JUCETICE_DECIBELSCALE_HEADER__

//==============================================================================
/**
    Creates a decibel scale using iec lin 2 db scale
*/
class DecibelScaleComponent: public Component
{
public:

    //==============================================================================
    enum DecibelLevels
    {
        LevelOver    = 0,
        Level0dB     = 1,
        Level3dB     = 2,
        Level6dB     = 3,
        Level10dB    = 4,
        LevelCount   = 5
    };

    //==============================================================================
    // Constructor.
    DecibelScaleComponent ();

    // Default destructor.
    ~DecibelScaleComponent ();

    //==============================================================================
    int iecScale (const float dB) const;
    int iecLevel (const int index) const;

    //==============================================================================
    /** @internal */
    void paint (Graphics& g);
    /** @internal */
    void resized ();

protected:

    //==============================================================================
    // Draw IEC scale line and label.
    void drawLabel (Graphics& g, const int y, const String& label);

    // Style font
    Font font;

    // Running variables.
    float scale;
    int lastY;
    int levels [LevelCount];
};


#endif  // __JUCETICE_DECIBELSCALE_HEADER__
