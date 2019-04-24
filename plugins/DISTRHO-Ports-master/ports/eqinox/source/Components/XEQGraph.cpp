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

#include "XEQGraph.h"

//==============================================================================
/** Constructor */
EQGraph::EQGraph ()
  : eff (0),
    maxdB (30),
    backColour (Colours::black),
    gridColour (Colours::darkgrey),
    lineColour (Colours::lightblue),
    fillColour (Colours::lightblue.withAlpha (0.65f)),
    drawStyle (FillAndStroke),
    strokeThickness (1.5f)
{
}

//==============================================================================
void EQGraph::setEqualizer (Equalizer *eff_)
{
    eff = eff_;
}

//==============================================================================
void EQGraph::setBackgroundColour (const Colour& colour)
{
    backColour = colour;
}

void EQGraph::setGridColour (const Colour& colour)
{
    gridColour = colour;
}

void EQGraph::setLineColour (const Colour& colour)
{
    lineColour = colour;
}

void EQGraph::setFillColour (const Colour& colour)
{
    fillColour = colour;
}

//==============================================================================
void EQGraph::setDrawStyle (const DrawStyle style)
{
    drawStyle = style;
}

//==============================================================================
void EQGraph::setStrokeThickness (const float stroke)
{
    strokeThickness = stroke;
}

//==============================================================================
void EQGraph::paintFreqLine (Graphics& g, float freq)
{
    double freqx = getFrequencyPosition (freq);
    if (freqx > 0.0 && freqx < 1.0)
    {
        g.drawLine (0 + (int) (freqx * getWidth()),
                    0,
                    0 + (int) (freqx * getWidth()),
                    0 + getHeight());
    }
}

void EQGraph::paintBackgroundGrid (Graphics& g)
{
    int i;
    int lx = getWidth(), ly = getHeight();

    // paint grid !
    if (gridColour != backColour)
    {
        g.setColour (gridColour);

        for (i = 1; i < 10; i++)
        {
            paintFreqLine (g, i * 10.0);
            paintFreqLine (g, i * 100.0);
            paintFreqLine (g, i * 1000.0);
        }
        // paintFreqLine (g, 10000.0, 0);
        // paintFreqLine (g, 20000.0, 1);

        g.drawLine (2,
                    ly / 2,
                    lx - 2,
                    ly / 2, 2.0f);

        int GY = 6;
        if (ly < GY * 3) GY = -1;
        for (i = 1; i < GY; i++)
        {
            int tmp = (int)(ly / (float) GY * i);
            g.drawLine (2,
                        tmp,
                        lx - 2,
                        tmp);
        }
    }
}

void EQGraph::paint (Graphics& g)
{
    int i, lx = getWidth(), ly = getHeight();

    g.fillAll (backColour);

    // paint grid !
    paintBackgroundGrid (g);

    // draw the frequency response
    float frq;
    int iy, oiy = getResponse (ly, getFrequencyX (0.0));
    int halfSampleRate = (int) eff->getSampleRate () / 2;

    Path path;
    path.startNewSubPath (0.0f,
                          (float)(ly - oiy));

    for (i = 2; i < lx; i++)
    {
        frq = getFrequencyX (i / (float) lx);
        if (frq > halfSampleRate) break;
        
        iy = getResponse (ly, frq);

        path.lineTo ((float)(i),
                     (float)(ly - iy));
                     
        oiy = iy;
    }

    // fill path if we want it to
    if (drawStyle != StrokeOnly)
    {
        Path pathToFill (path);

        pathToFill.lineTo ((float) getWidth(), (float) getHeight());
        pathToFill.lineTo (0.0f, (float) getHeight());
        pathToFill.closeSubPath ();

        g.setColour (fillColour);
        g.fillPath (pathToFill);
    }

    // stroke path if we want it to
    if (drawStyle == StrokeOnly
        || drawStyle == FillAndStroke)
    {
        g.setColour (lineColour);
        g.strokePath (path, PathStrokeType (strokeThickness));
    }

    // draw glass overlay
    g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.55f),
                                       320.0f, (float) (-158),
                                       Colours::transparentBlack,
                                       304.0f, 158.0f,
                                       true));
    g.fillEllipse (0.0f, (float) (-80), getWidth (), 158.0f);

    // draw bevel
    LookAndFeel_V2::drawBevel (g,
                               0, 0,
                               getWidth(), getHeight(),
                               2,
                               Colour (0xff828282).darker (0.5f),
                               Colour (0xff828282).brighter (0.5f),
                               true);
}

void EQGraph::resized ()
{
    repaint ();
}

void EQGraph::parameterChanged (AudioParameter* parameter, const int index)
{
    repaint ();
}


