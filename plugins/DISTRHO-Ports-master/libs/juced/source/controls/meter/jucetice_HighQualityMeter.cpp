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
// Meter level limits (in dB).
#define HQ_METER_MAXDB        (+4.0f)
#define HQ_METER_MINDB        (-70.0f)
// The decay rates (magic goes here :).
// - value decay rate (faster)
#define HQ_METER_DECAY_RATE1    (1.0f - 3E-2f)
// - peak decay rate (slower)
#define HQ_METER_DECAY_RATE2    (1.0f - 3E-6f)
// Number of cycles the peak stays on hold before fall-off.
#define HQ_METER_PEAK_FALLOFF    16


//==============================================================================
HighQualityMeterValue::HighQualityMeterValue (HighQualityMeter *pMeter)
  : m_pMeter (pMeter),
    m_fValue (0.0f),
    m_iValueHold (0),
    m_fValueDecay (HQ_METER_DECAY_RATE1),
    m_iPeak (0),
    m_iPeakHold (0),
    m_fPeakDecay (HQ_METER_DECAY_RATE2),
    m_iPeakColor (HighQualityMeter::Color6dB)
{
}

HighQualityMeterValue::~HighQualityMeterValue ()
{
}

void HighQualityMeterValue::setValue (const float fValue)
{
    m_fValue = fValue;
}

void HighQualityMeterValue::peakReset ()
{
    m_iPeak = 0;
}

void HighQualityMeterValue::refresh ()
{
    if (m_fValue > 0.001f || m_iPeak > 0)
        repaint();
}

void HighQualityMeterValue::paint (Graphics& g)
{
    int w = getWidth();
    int h = getHeight();
    int y;

    if (isEnabled())
    {
        g.setColour (m_pMeter->color (HighQualityMeter::ColorBack));
        g.fillRect (0, 0, w, h);

        y = m_pMeter->iec_level (HighQualityMeter::Color0dB);

        g.setColour (m_pMeter->color (HighQualityMeter::ColorFore));
        g.drawLine (0, h - y, w, h - y);
    }
    else
    {
        g.setColour (Colours::black);
        g.fillRect (0, 0, w, h);
    }

    float dB = HQ_METER_MINDB;
    if (m_fValue > 0.0f)
        dB = 20.0f * log10f (m_fValue);

    if (dB < HQ_METER_MINDB)
        dB = HQ_METER_MINDB;
    else if (dB > HQ_METER_MAXDB)
        dB = HQ_METER_MAXDB;

    int y_over = 0;
    int y_curr = 0;

    y = m_pMeter->iec_scale (dB);
    if (m_iValueHold < y)
    {
        m_iValueHold = y;
        m_fValueDecay = HQ_METER_DECAY_RATE1;
    }
    else
    {
        m_iValueHold = int (float (m_iValueHold * m_fValueDecay));
        if (m_iValueHold < y)
            m_iValueHold = y;
        else {
            m_fValueDecay *= m_fValueDecay;
            y = m_iValueHold;
        }
    }

    int iLevel;
    for (iLevel = HighQualityMeter::Color10dB;
         iLevel > HighQualityMeter::ColorOver && y >= y_over; iLevel--)
    {
        y_curr = m_pMeter->iec_level (iLevel);

//        g.setColour (m_pMeter->color (iLevel));

        g.setGradientFill (ColourGradient (m_pMeter->color (iLevel), 0, h - y_over,
                                           m_pMeter->color (iLevel-1), 0, h - y_curr,
                                           false));

        if (y < y_curr)
            g.fillRect(0, h - y, w, y - y_over);
        else
            g.fillRect(0, h - y_curr, w, y_curr - y_over);

        y_over = y_curr;
    }

    if (y > y_over)
    {
        g.setColour (m_pMeter->color (HighQualityMeter::ColorOver));
        g.fillRect (0, h - y, w, y - y_over);
    }

    if (m_iPeak < y)
    {
        m_iPeak = y;
        m_iPeakHold = 0;
        m_fPeakDecay = HQ_METER_DECAY_RATE2;
        m_iPeakColor = iLevel;
    }
    else if (++m_iPeakHold > m_pMeter->getPeakFalloff())
    {
        m_iPeak = int (float (m_iPeak * m_fPeakDecay));
        if (m_iPeak < y) {
            m_iPeak = y;
        } else {
            if (m_iPeak < m_pMeter->iec_level (HighQualityMeter::Color10dB))
                m_iPeakColor = HighQualityMeter::Color6dB;
            m_fPeakDecay *= m_fPeakDecay;
        }
    }

    g.setColour (m_pMeter->color (m_iPeakColor));
    g.drawLine (0, h - m_iPeak, w, h - m_iPeak);
}

void HighQualityMeterValue::resized ()
{
    m_iPeak = 0;
}


//==============================================================================
HighQualityMeter::HighQualityMeter (const int numPorts)
  : m_iPortCount (numPorts), // FIXME: Default port count.
    m_ppValues (0),
    m_fScale (0.0f),
    m_iPeakFalloff (HQ_METER_PEAK_FALLOFF)
{
    for (int i = 0; i < LevelCount; i++)
        m_levels[i] = 0;

    m_colors[ColorOver] = findColour (levelOverColourId);
    m_colors[Color0dB]  = findColour (level0dBColourId);
    m_colors[Color3dB]  = findColour (level3dBColourId);
    m_colors[Color6dB]  = findColour (level6dBColourId);
    m_colors[Color10dB] = findColour (level10dBColourId);
    m_colors[ColorBack] = findColour (backgroundColourId);
    m_colors[ColorFore] = findColour (foregroundColourId);

    if (m_iPortCount > 0)
    {
        m_ppValues = new HighQualityMeterValue* [m_iPortCount];

        for (int iPort = 0; iPort < m_iPortCount; iPort++)
        {
            m_ppValues[iPort] = new HighQualityMeterValue (this);
            addAndMakeVisible (m_ppValues[iPort]);
        }
    }
}


// Default destructor.
HighQualityMeter::~HighQualityMeter (void)
{
    for (int iPort = 0; iPort < m_iPortCount; iPort++)
        delete m_ppValues[iPort];

    delete [] m_ppValues;
}

void HighQualityMeter::resized ()
{
    m_fScale = 0.85f * getHeight();

    m_levels[Color0dB]  = iec_scale (  0.0f);
    m_levels[Color3dB]  = iec_scale ( -3.0f);
    m_levels[Color6dB]  = iec_scale ( -6.0f);
    m_levels[Color10dB] = iec_scale (-10.0f);

    int size = getWidth () / m_iPortCount;

    for (int iPort = 0; iPort < m_iPortCount; iPort++)
    {
        m_ppValues[iPort]->setBounds (iPort * size, 0, size, getHeight ());
    }
}

void HighQualityMeter::paint (Graphics& g)
{
    LookAndFeel_V2::drawBevel (g,
                            0, 0, getWidth(), getHeight(), 1,
                            Colours::black.withAlpha(0.2f),
                            Colours::white.withAlpha(0.2f));
}


// Child widget accessors.
int HighQualityMeter::iec_scale (const float dB) const
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

    return (int) (fDef * m_fScale);
}

int HighQualityMeter::iec_level (const int index) const
{
    return m_levels[index];
}

int HighQualityMeter::portCount () const
{
    return m_iPortCount;
}

void HighQualityMeter::setPeakFalloff (const int iPeakFalloff)
{
    m_iPeakFalloff = iPeakFalloff;
}

int HighQualityMeter::getPeakFalloff () const
{
    return m_iPeakFalloff;
}

void HighQualityMeter::peakReset ()
{
    for (int iPort = 0; iPort < m_iPortCount; iPort++)
        m_ppValues[iPort]->peakReset();
}

void HighQualityMeter::refresh ()
{
    for (int iPort = 0; iPort < m_iPortCount; iPort++)
        m_ppValues[iPort]->refresh();
}

void HighQualityMeter::setValue (const int iPort, const float fValue)
{
    m_ppValues[iPort]->setValue(fValue);
}

const Colour& HighQualityMeter::color (const int index) const
{
    return m_colors[index];
}

END_JUCE_NAMESPACE
