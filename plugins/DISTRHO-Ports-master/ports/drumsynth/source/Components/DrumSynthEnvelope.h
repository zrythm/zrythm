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

#ifndef __JUCETICE_DRUMSYNTHENVELOPE_HEADER__
#define __JUCETICE_DRUMSYNTHENVELOPE_HEADER__

#include "../StandardHeader.h"

class DrumSynthMain;
class DrumSynthPlugin;

#define MAX_ENVELOPE_POINTS    (5)
#define MAX_ENVELOPE_LENGTH    (44100.0f * 2.0f)
#define MAX_ENVELOPE_GAIN      (100.0f)
#define MAX_ENVELOPE_DOT_SIZE  (6)

//==============================================================================
class DrumSynthEnvelope : public Component,
                          public AudioParameterListener
{
public:

    //==============================================================================
    DrumSynthEnvelope (const int envelopeType,
                       DrumSynthMain* owner,
                       DrumSynthPlugin* plugin);

    //==============================================================================
    void parameterChanged (AudioParameter* parameter, const int index) override;
    void updateParameters (const bool repaintComponent = true);

    //==============================================================================
    void mouseDown (const MouseEvent& e) override;
    void mouseDrag (const MouseEvent& e) override;
    void mouseUp (const MouseEvent& e) override;
    void paint (Graphics& g) override;
    void resized () override;

protected:

    int findPointByMousePos (const int x, const int y);

    DrumSynthMain* owner;
    DrumSynthPlugin* plugin;
    int envelopeType;

    int draggingPoint;
    float points [MAX_ENVELOPE_POINTS][2];
    float xDelta, yDelta;
};

#endif

