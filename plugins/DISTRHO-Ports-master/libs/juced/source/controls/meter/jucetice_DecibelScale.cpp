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

BEGIN_JUCE_NAMESPACE

//==============================================================================
/*
// Meter level limits (in dB).
#define HQ_METER_MAXDB        (+3.0f)
#define HQ_METER_MINDB        (-70.0f)
// The decay rates (magic goes here :).
// - value decay rate (faster)
#define HQ_METER_DECAY_RATE1    (1.0f - 3E-2f)
// - peak decay rate (slower)
#define HQ_METER_DECAY_RATE2    (1.0f - 3E-6f)
// Number of cycles the peak stays on hold before fall-off.
#define HQ_METER_PEAK_FALLOFF    16
*/

//==============================================================================
DecibelScaleComponent::DecibelScaleComponent ()
  : font (7.0f, Font::plain),
    scale (0.0f),
    lastY (0)
{
    zeromem (levels, sizeof (int) * LevelCount);
}

DecibelScaleComponent::~DecibelScaleComponent ()
{
}

void DecibelScaleComponent::paint (Graphics& g)
{
    g.setFont (font);
    g.setColour (Colours::black);

    lastY = 0;

    drawLabel (g, iecLevel (Level0dB),   "0");
    drawLabel (g, iecLevel (Level3dB),   "3");
    drawLabel (g, iecLevel (Level6dB),   "6");
    drawLabel (g, iecLevel (Level10dB), "10");

    for (float dB = -20.0f; dB > -70.0f; dB -= 10.0f)
        drawLabel (g, DecibelScaleComponent::iecScale (dB), String ((int) -dB));
}

void DecibelScaleComponent::resized ()
{
    scale = 0.85f * getHeight();

    levels [Level0dB]  = iecScale(  0.0f);
    levels [Level3dB]  = iecScale( -3.0f);
    levels [Level6dB]  = iecScale( -6.0f);
    levels [Level10dB] = iecScale(-10.0f);
}

void DecibelScaleComponent::drawLabel (Graphics& g, const int y, const String& label)
{
    int iCurrY = getHeight() - y;
    int iWidth = getWidth();

    int iMidHeight = (int) (font.getHeight () * 0.5f);

    if (font.getStringWidth (label) < iWidth - 5)
    {
        g.drawLine (0, iCurrY, 2, iCurrY);
        g.drawLine (iWidth - 3, iCurrY, iWidth - 1, iCurrY);
    }

    if (iCurrY < iMidHeight || iCurrY > lastY + iMidHeight)
    {
        g.drawText (label,
                    2, iCurrY - iMidHeight, iWidth - 3, (int) font.getHeight (),
                    Justification::centred,
                    false);

        lastY = iCurrY + 1;
    }
}

int DecibelScaleComponent::iecScale (const float dB) const
{
    float fDef = 1.0;

    if (dB < -70.0)
        fDef = 0.0;
    else if (dB < -60.0)
        fDef = (dB + 70.0) * 0.0025;
    else if (dB < -50.0)
        fDef = (dB + 60.0) * 0.005 + 0.025;
    else if (dB < -40.0)
        fDef = (dB + 50.0) * 0.0075 + 0.075;
    else if (dB < -30.0)
        fDef = (dB + 40.0) * 0.015 + 0.15;
    else if (dB < -20.0)
        fDef = (dB + 30.0) * 0.02 + 0.3;
    else // if (dB < 0.0)
        fDef = (dB + 20.0) * 0.025 + 0.5;

    return (int) (fDef * scale);
}

int DecibelScaleComponent::iecLevel (const int index) const
{
    return levels [index];
}

END_JUCE_NAMESPACE
