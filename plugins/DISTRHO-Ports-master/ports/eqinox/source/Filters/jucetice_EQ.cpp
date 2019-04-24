/*
 ==============================================================================

 This file is part of the JUCETICE project - Copyright 2007 by Lucio Asnaghi.

 JUCETICE is based around the JUCE library - "Jules' Utility Class Extensions"
 Copyright 2007 by Julian Storer.

 ------------------------------------------------------------------------------

 JUCE and JUCETICE can be redistributed and/or modified under the terms of
 the GNU Lesser General Public License, as published by the Free Software
 Foundation; either version 2 of the License, or (at your option) any later
 version.

 JUCE and JUCETICE are distributed in the hope that they will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU Lesser General Public License
 along with JUCE and JUCETICE; if not, visit www.gnu.org/licenses or write to
 Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 Boston, MA 02111-1307 USA

 ==============================================================================

   @author  Paul Nasca
   @tweaker Lucio Asnaghi

 ==============================================================================
*/

#include "jucetice_EQ.h"


//==============================================================================
Equalizer::Equalizer (int insertion_)
{
    sampleRate = 44100;
    insertion = insertion_;

    for (int i = 0; i < MAX_EQ_BANDS; i++)
    {
        filter[i].Ptype = 0;
        filter[i].Pfreq = 64;
        filter[i].Pgain = 64;
        filter[i].Pq = 64;
        filter[i].Pstages = 0;
        filter[i].l = new AnalogFilter (6, 1000.0, 1.0, 0);
        filter[i].r = new AnalogFilter (6, 1000.0, 1.0, 0);
    }

    //default values
    Ppreset = 0;
    Pvolume = 0;
    Pdrywet = 127;

    setPreset (Ppreset);
    clean();
}

Equalizer::~Equalizer()
{
    for (int i = 0; i < MAX_EQ_BANDS; i++)
    {
        delete filter[i].l;
        delete filter[i].r;
    }
}

//==============================================================================
void Equalizer::clean()
{
    for (int i = 0; i < MAX_EQ_BANDS; i++)
    {
        filter[i].l->cleanup();
        filter[i].r->cleanup();
    }
}

//==============================================================================
void Equalizer::prepareToPlay (float sampleRate_, int samplesPerBlock_)
{
    sampleRate = sampleRate_;

    for (int i = 0; i < MAX_EQ_BANDS; i++)
    {
        filter[i].l->prepareToPlay (sampleRate_, samplesPerBlock_);
        filter[i].r->prepareToPlay (sampleRate_, samplesPerBlock_);
    }
}

void Equalizer::releaseResources ()
{
    for (int i = 0; i < MAX_EQ_BANDS; i++)
    {
        filter[i].l->releaseResources ();
        filter[i].r->releaseResources ();
    }
}

//==============================================================================
void Equalizer::out (const float *insmpsl,
                     const float *insmpsr,
                     float *outsmpsl,
                     float *outsmpsr,
                     float *smpsl,
                     float *smpsr,
                     const int numSamples)
{
    for (int i = 0; i < numSamples; i++)
    {
        smpsl[i] = insmpsl[i] * volume;
        smpsr[i] = insmpsr[i] * volume;
    }

    for (int i = 0; i < MAX_EQ_BANDS; i++)
    {
        if (filter[i].Ptype == 0) continue;
        filter[i].l->filterOut (smpsl, numSamples);
        filter[i].r->filterOut (smpsr, numSamples);
    }

    for (int i = 0; i < numSamples; i++)
    {
        outsmpsl[i] = insmpsl[i] * (1.0f - drywet) + smpsl[i] * drywet;
        outsmpsr[i] = insmpsr[i] * (1.0f - drywet) + smpsr[i] * drywet;
    }
}

//==============================================================================
void Equalizer::setParameter (int npar, uint8 value)
{
    switch (npar)
    {
    case 0: setvolume(value); break;
    case 1: setdrywet(value); break;
    }
    if (npar < 10) return;

    int nb = (npar - 10) / 5; // number of the band (filter)
    if (nb >= MAX_EQ_BANDS) return;
    int bp = npar % 5; // band paramenter

    float tmp;
    switch (bp)
    {
    case 0: if (value > 9) value = 0; // has to be changed if more filters will be added
            filter[nb].Ptype = value;
            if (value != 0)
            {
                filter[nb].l->setType (value - 1);
                filter[nb].r->setType (value - 1);
            }
            break;
    case 1: filter[nb].Pfreq = value;
            tmp = 600.0 * pow (30.0,(value - 64.0) / 64.0);
            filter[nb].l->setFreq (tmp);
            filter[nb].r->setFreq (tmp);
            break;
    case 2: filter[nb].Pgain = value;
            tmp = 30.0 * (value - 64.0) / 82.0; // original was: - 64.0) / 64.0
            filter[nb].l->setGain (tmp);
            filter[nb].r->setGain (tmp);
            break;
    case 3: filter[nb].Pq = value;
            tmp = pow (30.0, (value - 64.0) / 64.0);
            filter[nb].l->setQ (tmp);
            filter[nb].r->setQ (tmp);
            break;
    case 4: value = jmin (value, (uint8) MAX_ANALOG_FILTER_STAGES);
            filter[nb].Pstages = value;
            filter[nb].l->setStages (value);
            filter[nb].r->setStages (value);
            break;
    }
}

uint8 Equalizer::getParameter (int npar)
{
    switch (npar)
    {
    case 0: return Pvolume;  break;
    case 1: return Pdrywet;  break;
    }

    if (npar < 10) return 0;

    int nb = (npar - 10) / 5; //number of the band (filter)
    if (nb >= MAX_EQ_BANDS) return(0);
    int bp = npar % 5; //band paramenter

    switch (bp)
    {
    case 0: return filter[nb].Ptype;      break;
    case 1: return filter[nb].Pfreq;      break;
    case 2: return filter[nb].Pgain;      break;
    case 3: return filter[nb].Pq;         break;
    case 4: return filter[nb].Pstages;    break;
    }

    return 0;
}

//==============================================================================
void Equalizer::setvolume (uint8 Pvolume_)
{
    Pvolume = Pvolume_;

    outvolume = pow (0.005, (1.0 - Pvolume / 127.0)) * 10.0;
    volume = (insertion == 0) ? 1.0 : outvolume;
}

//==============================================================================
void Equalizer::setdrywet (uint8 Pdrywet_)
{
    Pdrywet = Pdrywet_;
    drywet = Pdrywet / 127.0f;
}

//==============================================================================
void Equalizer::setPreset (uint8 npreset)
{
    const int PRESET_SIZE = 2;
    const int NUM_PRESETS = 2;
    uint8 presets[NUM_PRESETS][PRESET_SIZE] = {
        //EQ 1
        {64, 127},
        //EQ 2
        {64, 127}
    };

    if (npreset >= NUM_PRESETS) npreset = NUM_PRESETS - 1;
    for (int n = 0; n < PRESET_SIZE; n++) setParameter (n, presets[npreset][n]);
    Ppreset = npreset;
}

//==============================================================================
void Equalizer::addToXML (XmlElement* xml)
{
    xml->setAttribute ("prst", Ppreset);

    {
    XmlElement* e = new XmlElement ("fxpar");
    for (int n = 0; n < 128; n++)
    {
        int par = getParameter (n);
        if (par == 0) continue;

        XmlElement* pe = new XmlElement ("p" + String (n));
        pe->setAttribute ("v", par);
        e->addChildElement (pe);
    }
    xml->addChildElement (e);
    }
}

void Equalizer::updateFromXML (XmlElement *xml)
{
    Ppreset = xml->getIntAttribute ("prst", Ppreset);

    XmlElement* e = xml->getChildByName ("fxpar");
    if (e)
    {
        for (int n = 0; n < 128; n++)
        {
            setParameter (n, 0);

            XmlElement* pe = e->getChildByName ("p" + String (n));
            if (pe)
                setParameter (n, pe->getIntAttribute ("v", getParameter (n)));
        }
    }

    clean();
}

//==============================================================================
float Equalizer::getFrequencyResponse (float freq)
{
    float resp = 1.0;

    for (int i = 0; i < MAX_EQ_BANDS; i++)
    {
        if (filter[i].Ptype == 0) continue;
        resp *= filter[i].l->H (freq);
    }
    return 20 * log (resp * outvolume) / double_Log10;
}

float Equalizer::getSampleRate ()
{
    return sampleRate;
}

