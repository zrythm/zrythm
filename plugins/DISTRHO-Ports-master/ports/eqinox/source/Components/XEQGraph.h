/*
 ==============================================================================

 This file is part of the JUCETICE project - Copyright 2004 by Lucio Asnaghi.

 JUCETICE is based around the JUCE library - "Jules' Utility Class Extensions"
 Copyright 2004 by Julian Storer.

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

 ------------------------------------------------------------------------------

 If you'd like to release a closed-source product which uses JUCE, commercial
 licenses are also available: visit www.rawmaterialsoftware.com/juce for
 more information.

 ==============================================================================
*/

#ifndef __JUCETICE_EQGRAPH_HEADER__
#define __JUCETICE_EQGRAPH_HEADER__

#include "../StandardHeader.h"
#include "../Filters/jucetice_EQ.h"


//==============================================================================
/**

    An auto-generated component, created by the Jucer.

    Describe your class and how it works here!

*/
class EQGraph : public Component,
                public AudioParameterListener
{
public:

    //==============================================================================
    /** Constructor */
    EQGraph ();

    //==============================================================================
    void setEqualizer (Equalizer *eff_);

    //==============================================================================
    void setBackgroundColour (const Colour& colour);
    void setGridColour (const Colour& colour);
    void setLineColour (const Colour& colour);
    void setFillColour (const Colour& colour);

    //==============================================================================
    enum DrawStyle
    {
        StrokeOnly    = 0,
        FillOnly      = 1,
        FillAndStroke = 2
    };

    void setDrawStyle (const DrawStyle style);

    //==============================================================================
    void setStrokeThickness (const float stroke);

    //==============================================================================
    void paint (Graphics& g) override;
    void resized () override;

    //==============================================================================
    void parameterChanged (AudioParameter* parameter, const int index) override;

protected:

    //==============================================================================
    void paintFreqLine (Graphics& g, float freq);
    void paintBackgroundGrid (Graphics& g);

    //==============================================================================
    inline int getResponse (int maxy, float freq)
    {
        float dbresp = eff->getFrequencyResponse (freq);
        return (int) ((dbresp / maxdB + 1.0) * maxy * 0.5);
    }

/*
    inline int getBandResponse (int band, int maxy, float freq)
    {
        float dbresp = eff->getBandFrequencyResponse (band, freq);
        return (int) ((dbresp / maxdB + 1.0) * maxy * 0.5);
    }
*/

    inline float getFrequencyX (float x)
    {
        return 20.0 * pow (1000.0f, jmin (x, 1.0f));
    }

    inline float getFrequencyPosition (float freq)
    {
        return log (jmax (freq, 0.00001f) / 20.0) / log (1000.0);
    }

    Equalizer *eff;
    int maxdB;

    Colour backColour;
    Colour gridColour;
    Colour lineColour;
    Colour fillColour;

    DrawStyle drawStyle;
    float strokeThickness;
};


#endif
