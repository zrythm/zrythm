/*
 ==============================================================================

 This file is part of the JUCETICE project - Copyright 2008 by Lucio Asnaghi.

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

#include "../DrumSynthPlugin.h"
#include "../DrumSynthComponent.h"

#include "DrumSynthEnvelope.h"
#include "DrumSynthMain.h"


//==============================================================================
DrumSynthEnvelope::DrumSynthEnvelope (const int envelopeType_,
                                      DrumSynthMain* owner_,
                                      DrumSynthPlugin* plugin_)
  : owner (owner_),
    plugin (plugin_),
    envelopeType (envelopeType_),
    draggingPoint (-1)
{
}

//==============================================================================void DrumSynthEnvelope::resized ()
{
    xDelta = (getWidth ()) / MAX_ENVELOPE_LENGTH;
    yDelta = (getHeight ()) / MAX_ENVELOPE_GAIN;
    
    updateParameters (false);
}

//==============================================================================void DrumSynthEnvelope::paint (Graphics& g)
{
    const int dotSize = MAX_ENVELOPE_DOT_SIZE;
    const int halfDotSize = dotSize / 2;

    g.fillAll (Colour (0x47ffffff));

    Path myPath;
    myPath.startNewSubPath (0, points[0][1]);
    for (int i = 1; i < MAX_ENVELOPE_POINTS; i++)
        myPath.lineTo (points[i][0], points[i][1]);

    g.setColour (Colours::white);
    g.strokePath (myPath, PathStrokeType (2.0f));

    g.setColour (Colours::black);
    for (int i = 0; i < MAX_ENVELOPE_POINTS; i++)
    {
        int x = (int) (points[i][0]);
        int y = (int) (points[i][1]);
        
        g.setColour (Colours::white);
        g.fillEllipse (x - halfDotSize, y - halfDotSize, dotSize, dotSize);
        
        if (draggingPoint == i) {
            g.setColour (Colours::red);
            g.drawEllipse (x - halfDotSize, y - halfDotSize, dotSize, dotSize, 2);
        } else {
            g.setColour (Colours::black);
            g.drawEllipse (x - halfDotSize, y - halfDotSize, dotSize, dotSize, 1);
        }
    }
}

//==============================================================================void DrumSynthEnvelope::mouseDown (const MouseEvent& e)
{
    draggingPoint = findPointByMousePos (e.x, e.y);
    if (draggingPoint != -1)
    {
    }

    repaint ();
}

void DrumSynthEnvelope::mouseDrag (const MouseEvent& e)
{
    int currentDrum = plugin->getCurrentDrum();
    if (currentDrum < 0)
        return;

    if (draggingPoint != -1)
    {
        int prevPoint = jmax (0, draggingPoint - 1);
        int nextPoint = jmin (MAX_ENVELOPE_POINTS, draggingPoint + 1);

        // calculate X
        if (draggingPoint == 0)
            points[draggingPoint][0] = 0;
        else if (draggingPoint > 0 && draggingPoint < MAX_ENVELOPE_POINTS - 1)
            points[draggingPoint][0] = jmax (points[prevPoint][0],
                                             jmin (points[nextPoint][0], (float) e.x));
        else
            points[draggingPoint][0] = jmax (points[prevPoint][0],
                                             (float) jmin (getWidth(), e.x));

        // calculate Y
        points[draggingPoint][1] = jmax (0, jmin (e.y, getHeight()));

        int noteNumber = currentDrum;
        int paramNumber = envelopeType + draggingPoint * 2;
        
        plugin->setParameterMapped (PPAR(noteNumber, paramNumber++), points[draggingPoint][0] / xDelta);
        plugin->setParameterMapped (PPAR(noteNumber, paramNumber++), (getHeight() - points[draggingPoint][1]) / yDelta);
        
        repaint ();
    }
}

void DrumSynthEnvelope::mouseUp (const MouseEvent& e)
{
    draggingPoint = -1;

    repaint ();
}

//==============================================================================int DrumSynthEnvelope::findPointByMousePos (const int x, const int y)
{
    const int pixelSnap = MAX_ENVELOPE_DOT_SIZE / 2;

    for (int i = 0; i < MAX_ENVELOPE_POINTS; i++)
    {
        if ((x >= (points [i][0] - pixelSnap) && x <= (points [i][0] + pixelSnap))
            && (y >= (points [i][1] - pixelSnap) && y <= (points [i][1] + pixelSnap)))
        {
            return i;
        }
    }
    return -1;
}

//==============================================================================void DrumSynthEnvelope::parameterChanged (AudioParameter* parameter, const int index)
{
    updateParameters (true);
}

void DrumSynthEnvelope::updateParameters (const bool repaintComponent)
{
    int currentDrum = plugin->getCurrentDrum();
    if (currentDrum < 0)
        return;

    int noteNumber = currentDrum;
    int paramNumber = envelopeType;

    for (int i = 0; i < MAX_ENVELOPE_POINTS; i++)
    {
        float x = plugin->getParameterMapped (PPAR(noteNumber, paramNumber++));
        float y = plugin->getParameterMapped (PPAR(noteNumber, paramNumber++)) / MAX_ENVELOPE_GAIN;
    
        points [i][0] = x * xDelta;
        points [i][1] = getHeight() * (1.0f - y);
    }
    
    if (repaintComponent)
        repaint ();
    
}


