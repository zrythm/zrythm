/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Rubber Band Library
    An audio time-stretching and pitch-shifting library.
    Copyright 2007-2023 Particular Programs Ltd.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.

    Alternatively, if you have a valid commercial licence for the
    Rubber Band Library obtained by agreement with the copyright
    holders, you may redistribute and/or modify it under the terms
    described in that licence.

    If you wish to distribute code using the Rubber Band Library
    under terms other than those of the GNU General Public License,
    you must obtain a valid commercial licence before doing so.
*/

#include "RubberBandLivePitchShifter.h"

#include "RubberBandLiveShifter.h"

#include <iostream>
#include <cmath>

using namespace RubberBand;

using std::cerr;
using std::endl;
using std::string;

#ifdef RB_PLUGIN_LADSPA

const char *const
RubberBandLivePitchShifter::portNamesMono[PortCountMono] =
{
    "latency",
    "Cents",
    "Semitones",
    "Octaves",
    "Formant Preserving",
    "Wet-Dry Mix",
    "Input",
    "Output"
};

const char *const
RubberBandLivePitchShifter::portNamesStereo[PortCountStereo] =
{
    "latency",
    "Cents",
    "Semitones",
    "Octaves",
    "Formant Preserving",
    "Wet-Dry Mix",
    "Input L",
    "Output L",
    "Input R",
    "Output R"
};

const LADSPA_PortDescriptor 
RubberBandLivePitchShifter::portsMono[PortCountMono] =
{
    LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL,
    LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
    LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
    LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
    LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
    LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
    LADSPA_PORT_INPUT  | LADSPA_PORT_AUDIO,
    LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO
};

const LADSPA_PortDescriptor 
RubberBandLivePitchShifter::portsStereo[PortCountStereo] =
{
    LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL,
    LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
    LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
    LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
    LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
    LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
    LADSPA_PORT_INPUT  | LADSPA_PORT_AUDIO,
    LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
    LADSPA_PORT_INPUT  | LADSPA_PORT_AUDIO,
    LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO
};

const LADSPA_PortRangeHint 
RubberBandLivePitchShifter::hintsMono[PortCountMono] =
{
    { 0, 0, 0 },                        // latency
    { LADSPA_HINT_DEFAULT_0 |           // cents
      LADSPA_HINT_BOUNDED_BELOW |
      LADSPA_HINT_BOUNDED_ABOVE,
      -100.0, 100.0 },
    { LADSPA_HINT_DEFAULT_0 |           // semitones
      LADSPA_HINT_BOUNDED_BELOW |
      LADSPA_HINT_BOUNDED_ABOVE |
      LADSPA_HINT_INTEGER,
      -12.0, 12.0 },
    { LADSPA_HINT_DEFAULT_0 |           // octaves
      LADSPA_HINT_BOUNDED_BELOW |
      LADSPA_HINT_BOUNDED_ABOVE |
      LADSPA_HINT_INTEGER,
      -2.0, 2.0 },
    { LADSPA_HINT_DEFAULT_0 |           // formant preserving
      LADSPA_HINT_BOUNDED_BELOW |
      LADSPA_HINT_BOUNDED_ABOVE |
      LADSPA_HINT_TOGGLED,
       0.0, 1.0 },
    { LADSPA_HINT_DEFAULT_0 |           // wet-dry mix
      LADSPA_HINT_BOUNDED_BELOW |
      LADSPA_HINT_BOUNDED_ABOVE,
       0.0, 1.0 },
    { 0, 0, 0 },
    { 0, 0, 0 }
};

const LADSPA_PortRangeHint 
RubberBandLivePitchShifter::hintsStereo[PortCountStereo] =
{
    { 0, 0, 0 },                        // latency
    { LADSPA_HINT_DEFAULT_0 |           // cents
      LADSPA_HINT_BOUNDED_BELOW |
      LADSPA_HINT_BOUNDED_ABOVE,
      -100.0, 100.0 },
    { LADSPA_HINT_DEFAULT_0 |           // semitones
      LADSPA_HINT_BOUNDED_BELOW |
      LADSPA_HINT_BOUNDED_ABOVE |
      LADSPA_HINT_INTEGER,
      -12.0, 12.0 },
    { LADSPA_HINT_DEFAULT_0 |           // octaves
      LADSPA_HINT_BOUNDED_BELOW |
      LADSPA_HINT_BOUNDED_ABOVE |
      LADSPA_HINT_INTEGER,
      -2.0, 2.0 },
    { LADSPA_HINT_DEFAULT_0 |           // formant preserving
      LADSPA_HINT_BOUNDED_BELOW |
      LADSPA_HINT_BOUNDED_ABOVE |
      LADSPA_HINT_TOGGLED,
       0.0, 1.0 },
    { LADSPA_HINT_DEFAULT_0 |           // wet-dry mix
      LADSPA_HINT_BOUNDED_BELOW |
      LADSPA_HINT_BOUNDED_ABOVE,
       0.0, 1.0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 }
};

const LADSPA_Properties
RubberBandLivePitchShifter::properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;

const LADSPA_Descriptor 
RubberBandLivePitchShifter::ladspaDescriptorMono =
{
    29791, // "Unique" ID
    "rubberband-live-pitchshifter-mono", // Label
    properties,
    "Rubber Band Live Mono Pitch Shifter", // Name
    "Breakfast Quay",
    "GPL",
    PortCountMono,
    portsMono,
    portNamesMono,
    hintsMono,
    nullptr, // Implementation data
    instantiate,
    connectPort,
    activate,
    run,
    nullptr, // Run adding
    nullptr, // Set run adding gain
    deactivate,
    cleanup
};

const LADSPA_Descriptor 
RubberBandLivePitchShifter::ladspaDescriptorStereo =
{
    97921, // "Unique" ID
    "rubberband-live-pitchshifter-stereo", // Label
    properties,
    "Rubber Band Live Stereo Pitch Shifter", // Name
    "Breakfast Quay",
    "GPL",
    PortCountStereo,
    portsStereo,
    portNamesStereo,
    hintsStereo,
    nullptr, // Implementation data
    instantiate,
    connectPort,
    activate,
    run,
    nullptr, // Run adding
    nullptr, // Set run adding gain
    deactivate,
    cleanup
};

const LADSPA_Descriptor *
RubberBandLivePitchShifter::getDescriptor(unsigned long index)
{
    if (index == 0) return &ladspaDescriptorMono;
    if (index == 1) return &ladspaDescriptorStereo;
    else return 0;
}

#else

const LV2_Descriptor
RubberBandLivePitchShifter::lv2DescriptorMono =
{
    "http://breakfastquay.com/rdf/lv2-rubberband#livemono",
    instantiate,
    connectPort,
    activate,
    run,
    deactivate,
    cleanup,
    nullptr
};

const LV2_Descriptor
RubberBandLivePitchShifter::lv2DescriptorStereo =
{
    "http://breakfastquay.com/rdf/lv2-rubberband#livestereo",
    instantiate,
    connectPort,
    activate,
    run,
    deactivate,
    cleanup,
    nullptr
};

const LV2_Descriptor *
RubberBandLivePitchShifter::getDescriptor(uint32_t index)
{
    if (index == 0) return &lv2DescriptorMono;
    if (index == 1) return &lv2DescriptorStereo;
    else return 0;
}

#endif

RubberBandLivePitchShifter::RubberBandLivePitchShifter(int sampleRate, size_t channels) :
    m_latency(nullptr),
    m_cents(nullptr),
    m_semitones(nullptr),
    m_octaves(nullptr),
    m_formant(nullptr),
    m_wetDry(nullptr),
    m_ratio(1.0),
    m_prevRatio(1.0),
    m_currentFormant(false),
    m_shifter(new RubberBandLiveShifter
              (sampleRate, channels,
               RubberBandLiveShifter::OptionChannelsTogether)),
    m_sampleRate(sampleRate),
    m_channels(channels),
    m_blockSize(0),
    m_bufferSize(65536),
    m_delay(0)
{
    m_input = new float *[m_channels];
    m_output = new float *[m_channels];

    m_irb = new RingBuffer<float> *[m_channels];
    m_orb = new RingBuffer<float> *[m_channels];

    m_ib = new float *[m_channels];
    m_ob = new float *[m_channels];

    m_delayMixBuffer = new RingBuffer<float> *[m_channels];

    m_blockSize = m_shifter->getBlockSize();
    m_delay = m_shifter->getStartDelay();
    
    for (int c = 0; c < m_channels; ++c) {

        m_irb[c] = new RingBuffer<float>(m_bufferSize);
        m_orb[c] = new RingBuffer<float>(m_bufferSize);
        m_irb[c]->zero(m_blockSize);

        m_ib[c] = new float[m_blockSize];
        m_ob[c] = new float[m_blockSize];
        
        m_delayMixBuffer[c] = new RingBuffer<float>(m_bufferSize + m_delay);
        m_irb[c]->zero(m_delay);
    }

    activateImpl();
}

RubberBandLivePitchShifter::~RubberBandLivePitchShifter()
{
    delete m_shifter;
    for (int c = 0; c < m_channels; ++c) {
        delete m_irb[c];
        delete m_orb[c];
        delete[] m_ib[c];
        delete[] m_ob[c];
        delete m_delayMixBuffer[c];
    }
    delete[] m_irb;
    delete[] m_orb;
    delete[] m_ib;
    delete[] m_ob;
    delete[] m_delayMixBuffer;
    delete[] m_output;
    delete[] m_input;
}

#ifdef RB_PLUGIN_LADSPA

LADSPA_Handle
RubberBandLivePitchShifter::instantiate(const LADSPA_Descriptor *desc, unsigned long rate)
{
    if (desc->PortCount == ladspaDescriptorMono.PortCount) {
        return new RubberBandLivePitchShifter(rate, 1);
    } else if (desc->PortCount == ladspaDescriptorStereo.PortCount) {
        return new RubberBandLivePitchShifter(rate, 2);
    }
    return nullptr;
}

#else

LV2_Handle
RubberBandLivePitchShifter::instantiate(const LV2_Descriptor *desc, double rate,
                                    const char *, const LV2_Feature *const *)
{
    if (rate < 1.0) {
        cerr << "RubberBandLivePitchShifter::instantiate: invalid sample rate "
             << rate << " provided" << endl;
        return nullptr;
    }
    size_t srate = size_t(round(rate));
    if (string(desc->URI) == lv2DescriptorMono.URI) {
        return new RubberBandLivePitchShifter(srate, 1);
    } else if (string(desc->URI) == lv2DescriptorStereo.URI) {
        return new RubberBandLivePitchShifter(srate, 2);
    } else {
        cerr << "RubberBandLivePitchShifter::instantiate: unrecognised URI "
             << desc->URI << " requested" << endl;
        return nullptr;
    }
}

#endif

#ifdef RB_PLUGIN_LADSPA
void
RubberBandLivePitchShifter::connectPort(LADSPA_Handle handle,
				    unsigned long port, LADSPA_Data *location)
#else 
void
RubberBandLivePitchShifter::connectPort(LV2_Handle handle,
                                        uint32_t port, void *location)
#endif
{
    RubberBandLivePitchShifter *shifter = (RubberBandLivePitchShifter *)handle;
    
    float **ports[PortCountStereo] = {
        &shifter->m_latency,
	&shifter->m_cents,
	&shifter->m_semitones,
	&shifter->m_octaves,
	&shifter->m_formant,
	&shifter->m_wetDry,
    	&shifter->m_input[0],
	&shifter->m_output[0],
	&shifter->m_input[1],
	&shifter->m_output[1]
    };

    if (shifter->m_channels == 1) {
        if (port >= PortCountMono) return;
    } else {
        if (port >= PortCountStereo) return;
    }

    *ports[port] = (float *)location;

    if (shifter->m_latency) {
        *(shifter->m_latency) = shifter->getLatency();
    }
}

#ifdef RB_PLUGIN_LADSPA
void
RubberBandLivePitchShifter::activate(LADSPA_Handle handle)
#else
void
RubberBandLivePitchShifter::activate(LV2_Handle handle)
#endif
{
    RubberBandLivePitchShifter *shifter = (RubberBandLivePitchShifter *)handle;
    shifter->activateImpl();
}

#ifdef RB_PLUGIN_LADSPA
void
RubberBandLivePitchShifter::run(LADSPA_Handle handle, unsigned long samples)
#else
void
RubberBandLivePitchShifter::run(LV2_Handle handle, uint32_t samples)
#endif
{
    RubberBandLivePitchShifter *shifter = (RubberBandLivePitchShifter *)handle;
    shifter->runImpl(samples);
}

#ifdef RB_PLUGIN_LADSPA
void
RubberBandLivePitchShifter::deactivate(LADSPA_Handle handle)
#else
void
RubberBandLivePitchShifter::deactivate(LV2_Handle handle)
#endif
{
    activate(handle); // both functions just reset the plugin
}

#ifdef RB_PLUGIN_LADSPA
void
RubberBandLivePitchShifter::cleanup(LADSPA_Handle handle)
#else
void
RubberBandLivePitchShifter::cleanup(LV2_Handle handle)
#endif
{
    delete (RubberBandLivePitchShifter *)handle;
}

void
RubberBandLivePitchShifter::activateImpl()
{
    updateRatio();
    m_prevRatio = m_ratio;
    m_shifter->reset();
    m_shifter->setPitchScale(m_ratio);

    for (int c = 0; c < m_channels; ++c) {
        m_irb[c]->reset();
        m_irb[c]->zero(m_blockSize);
        m_orb[c]->reset();
        m_delayMixBuffer[c]->reset();
        m_delayMixBuffer[c]->zero(m_delay);
    }
}

void
RubberBandLivePitchShifter::updateRatio()
{
    // The octaves, semitones, and cents parameters are supposed to be
    // integral: we want to enforce that, just to avoid
    // inconsistencies between hosts if some respect the hints more
    // than others

    double octaves = round(m_octaves ? *m_octaves : 0.0);
    if (octaves < -2.0) octaves = -2.0;
    if (octaves >  2.0) octaves =  2.0;
    
    double semitones = round(m_semitones ? *m_semitones : 0.0);
    if (semitones < -12.0) semitones = -12.0;
    if (semitones >  12.0) semitones =  12.0;
    
    double cents = round(m_cents ? *m_cents : 0.0);
    if (cents < -100.0) cents = -100.0;
    if (cents >  100.0) cents =  100.0;
    
    m_ratio = pow(2.0,
                  octaves +
                  semitones / 12.0 +
                  cents / 1200.0);
}

void
RubberBandLivePitchShifter::updateFormant()
{
    if (!m_formant) return;

    bool f = (*m_formant > 0.5f);
    if (f == m_currentFormant) return;
    
    RubberBandLiveShifter *s = m_shifter;
    
    s->setFormantOption(f ?
                        RubberBandLiveShifter::OptionFormantPreserved :
                        RubberBandLiveShifter::OptionFormantShifted);

    m_currentFormant = f;
}

int
RubberBandLivePitchShifter::getLatency() const
{
    return m_shifter->getStartDelay() + m_blockSize;
}

void
RubberBandLivePitchShifter::runImpl(uint32_t insamples)
{
    updateRatio();
    if (m_ratio != m_prevRatio) {
        m_shifter->setPitchScale(m_ratio);
        m_prevRatio = m_ratio;
    }
    
    updateFormant();
    
    if (m_latency) {
        *m_latency = getLatency();
    }
    
    for (int c = 0; c < m_channels; ++c) {
        m_irb[c]->write(m_input[c], insamples);
        m_delayMixBuffer[c]->write(m_input[c], insamples);
    }

    while (m_irb[0]->getReadSpace() >= m_blockSize) {

        for (int c = 0; c < m_channels; ++c) {
            m_irb[c]->read(m_ib[c], m_blockSize);
        }

        m_shifter->shift(m_ib, m_ob);

        for (int c = 0; c < m_channels; ++c) {
            m_orb[c]->write(m_ob[c], m_blockSize);
        }
    }

    for (int c = 0; c < m_channels; ++c) {
        m_orb[c]->read(m_output[c], insamples);
    }
    
    float mix = 0.0;
    if (m_wetDry) mix = *m_wetDry;

    for (int c = 0; c < m_channels; ++c) {
        if (mix > 0.0) {
            for (uint32_t i = 0; i < insamples; ++i) {
                float dry = m_delayMixBuffer[c]->readOne();
                m_output[c][i] *= (1.0 - mix);
                m_output[c][i] += dry * mix;
            }
        } else {
            m_delayMixBuffer[c]->skip(insamples);
        }
    }
}

