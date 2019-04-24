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

#ifndef __JUCETICE_HIGHQUALITYMETER_HEADER__
#define __JUCETICE_HIGHQUALITYMETER_HEADER__

class HighQualityMeter;
class HighQualityMeterValue;

//==============================================================================
/**
    Creates VU meters with scale in decibels
*/
class HighQualityMeterValue : public Component
{
public:

    //==============================================================================
    // Constructor.
    HighQualityMeterValue (HighQualityMeter *pMeter);

    // Default destructor.
    ~HighQualityMeterValue();

    //==============================================================================
    // Frame value accessors.
    void setValue (const float fValue);

    //==============================================================================
    // Value refreshment.
    void refresh();

    //==============================================================================
    // Reset peak holder.
    void peakReset();

    //==============================================================================
    /** @internal */
    void paint (Graphics& g);
    /** @internal */
    void resized ();

private:

    // Local instance variables.
    HighQualityMeter *m_pMeter;

    // Running variables.
    float m_fValue;
    int   m_iValueHold;
    float m_fValueDecay;
    int   m_iPeak;
    int   m_iPeakHold;
    float m_fPeakDecay;
    int   m_iPeakColor;
};


//==============================================================================
/**
    Creates VU meters with scale in decibels
*/
class HighQualityMeter : public Component
{
public:

    //==============================================================================
    // Constructor.
    HighQualityMeter (const int numPorts);

    // Default destructor.
    ~HighQualityMeter();

    //==============================================================================
    // Port count accessor.
    int portCount() const;

    //==============================================================================
    // Value proxy.
    void setValue (const int iPort, const float fValue);

    //==============================================================================
    // IEC scale accessors.
    int iec_scale (const float dB) const;
    int iec_level (const int index) const;

    //==============================================================================
    // Slot refreshment.
    void refresh();

    //==============================================================================
    // Peak falloff mode setting.
    void setPeakFalloff (const int peakFalloff);
    int getPeakFalloff() const;

    // Reset peak holder.
    void peakReset();

    //==============================================================================
    // Level indexes.
    enum {
        ColorOver    = 0,
        Color0dB     = 1,
        Color3dB     = 2,
        Color6dB     = 3,
        Color10dB    = 4,
        LevelCount   = 5,
        ColorBack    = 5,
        ColorFore    = 6,
        ColorCount   = 7
    };

    // Common resource accessors.
    const Colour& color (const int index) const;

    //==============================================================================
    // Colors indexes.
    enum ColoursIds
    {
        levelOverColourId    = 0x90900001,
        level0dBColourId     = 0x90900002,
        level3dBColourId     = 0x90900003,
        level6dBColourId     = 0x90900004,
        level10dBColourId    = 0x90900005,
        backgroundColourId   = 0x90900006,
        foregroundColourId   = 0x90900007
    };

    //==============================================================================
    /** @internal */
    void paint (Graphics& g);
    /** @internal */
    void resized ();

private:

    // Local instance variables.
    int m_iPortCount;
    HighQualityMeterValue** m_ppValues;

    float m_fScale;

    int m_levels[LevelCount];
    Colour m_colors[ColorCount];

    // Peak falloff mode setting (0=no peak falloff).
    int m_iPeakFalloff;
};

#endif  // __JUCETICE_HIGHQUALITYMETER_HEADER__
