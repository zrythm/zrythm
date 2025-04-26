/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*-  vi:set ts=8 sts=4 sw=4: */

/*
    Rubber Band Library
    An audio time-stretching and pitch-shifting library.
    Copyright 2007-2024 Particular Programs Ltd.

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

#include "RubberBandPitchShifter.h"

#include "RubberBandStretcher.h"

#include <iostream>
#include <cmath>

using namespace RubberBand;

using std::cout;
using std::cerr;
using std::endl;
using std::min;
using std::string;

#ifdef RB_PLUGIN_LADSPA

const char *const
RubberBandPitchShifter::portNamesMono[PortCountMono] =
{
    "latency",
    "Cents",
    "Semitones",
    "Octaves",
    "Crispness",
    "Formant Preserving",
    "Wet-Dry Mix",
    "Input",
    "Output"
};

const char *const
RubberBandPitchShifter::portNamesStereo[PortCountStereo] =
{
    "latency",
    "Cents",
    "Semitones",
    "Octaves",
    "Crispness",
    "Formant Preserving",
    "Wet-Dry Mix",
    "Input L",
    "Output L",
    "Input R",
    "Output R"
};

const LADSPA_PortDescriptor 
RubberBandPitchShifter::portsMono[PortCountMono] =
{
    LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL,
    LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
    LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
    LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
    LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
    LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
    LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
    LADSPA_PORT_INPUT  | LADSPA_PORT_AUDIO,
    LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO
};

const LADSPA_PortDescriptor 
RubberBandPitchShifter::portsStereo[PortCountStereo] =
{
    LADSPA_PORT_OUTPUT | LADSPA_PORT_CONTROL,
    LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
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
RubberBandPitchShifter::hintsMono[PortCountMono] =
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
    { LADSPA_HINT_DEFAULT_MAXIMUM |     // crispness
      LADSPA_HINT_BOUNDED_BELOW |
      LADSPA_HINT_BOUNDED_ABOVE |
      LADSPA_HINT_INTEGER,
       0.0, 3.0 },
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
RubberBandPitchShifter::hintsStereo[PortCountStereo] =
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
    { LADSPA_HINT_DEFAULT_MAXIMUM |     // crispness
      LADSPA_HINT_BOUNDED_BELOW |
      LADSPA_HINT_BOUNDED_ABOVE |
      LADSPA_HINT_INTEGER,
       0.0, 3.0 },
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
RubberBandPitchShifter::properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;

const LADSPA_Descriptor 
RubberBandPitchShifter::ladspaDescriptorMono =
{
    2979, // "Unique" ID
    "rubberband-pitchshifter-mono", // Label
    properties,
    "Rubber Band Mono Pitch Shifter", // Name
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
RubberBandPitchShifter::ladspaDescriptorStereo =
{
    9792, // "Unique" ID
    "rubberband-pitchshifter-stereo", // Label
    properties,
    "Rubber Band Stereo Pitch Shifter", // Name
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
RubberBandPitchShifter::getDescriptor(unsigned long index)
{
    if (index == 0) return &ladspaDescriptorMono;
    if (index == 1) return &ladspaDescriptorStereo;
    else return 0;
}

#else

const LV2_Descriptor
RubberBandPitchShifter::lv2DescriptorMono =
{
    "http://breakfastquay.com/rdf/lv2-rubberband#mono",
    instantiate,
    connectPort,
    activate,
    run,
    deactivate,
    cleanup,
    nullptr
};

const LV2_Descriptor
RubberBandPitchShifter::lv2DescriptorStereo =
{
    "http://breakfastquay.com/rdf/lv2-rubberband#stereo",
    instantiate,
    connectPort,
    activate,
    run,
    deactivate,
    cleanup,
    nullptr
};

const LV2_Descriptor *
RubberBandPitchShifter::getDescriptor(uint32_t index)
{
    if (index == 0) return &lv2DescriptorMono;
    if (index == 1) return &lv2DescriptorStereo;
    else return 0;
}

#endif

RubberBandPitchShifter::RubberBandPitchShifter(int sampleRate, size_t channels) :
    m_latency(nullptr),
    m_cents(nullptr),
    m_semitones(nullptr),
    m_octaves(nullptr),
    m_crispness(nullptr),
    m_formant(nullptr),
    m_wetDry(nullptr),
    m_ratio(1.0),
    m_prevRatio(1.0),
    m_currentCrispness(-1),
    m_currentFormant(false),
    m_blockSize(1024),
    m_reserve(8192),
    m_bufsize(0),
    m_minfill(0),
    m_stretcher(new RubberBandStretcher
                (sampleRate, channels,
                 RubberBandStretcher::OptionProcessRealTime |
                 RubberBandStretcher::OptionPitchHighConsistency)),
    m_sampleRate(sampleRate),
    m_channels(channels)
{
    m_input = new float *[m_channels];
    m_output = new float *[m_channels];

    m_outputBuffer = new RingBuffer<float> *[m_channels];
    m_delayMixBuffer = new RingBuffer<float> *[m_channels];
    m_scratch = new float *[m_channels];
    m_inptrs = new float *[m_channels];
    
    m_bufsize = m_blockSize + m_reserve + 8192;

    for (size_t c = 0; c < m_channels; ++c) {

        m_input[c] = 0;
        m_output[c] = 0;

        m_outputBuffer[c] = new RingBuffer<float>(m_bufsize);
        m_delayMixBuffer[c] = new RingBuffer<float>(m_bufsize);

        m_scratch[c] = new float[m_bufsize];
        for (size_t i = 0; i < m_bufsize; ++i) {
            m_scratch[c][i] = 0.f;
        }

        m_inptrs[c] = 0;
    }

    activateImpl();
}

RubberBandPitchShifter::~RubberBandPitchShifter()
{
    delete m_stretcher;
    for (size_t c = 0; c < m_channels; ++c) {
        delete m_outputBuffer[c];
        delete m_delayMixBuffer[c];
        delete[] m_scratch[c];
    }
    delete[] m_outputBuffer;
    delete[] m_delayMixBuffer;
    delete[] m_inptrs;
    delete[] m_scratch;
    delete[] m_output;
    delete[] m_input;
}

#ifdef RB_PLUGIN_LADSPA

LADSPA_Handle
RubberBandPitchShifter::instantiate(const LADSPA_Descriptor *desc, unsigned long rate)
{
    if (desc->PortCount == ladspaDescriptorMono.PortCount) {
        return new RubberBandPitchShifter(rate, 1);
    } else if (desc->PortCount == ladspaDescriptorStereo.PortCount) {
        return new RubberBandPitchShifter(rate, 2);
    }
    return nullptr;
}

#else

LV2_Handle
RubberBandPitchShifter::instantiate(const LV2_Descriptor *desc, double rate,
                                    const char *, const LV2_Feature *const *)
{
    if (rate < 1.0) {
        cerr << "RubberBandPitchShifter::instantiate: invalid sample rate "
             << rate << " provided" << endl;
        return nullptr;
    }
    size_t srate = size_t(round(rate));
    if (string(desc->URI) == lv2DescriptorMono.URI) {
        return new RubberBandPitchShifter(srate, 1);
    } else if (string(desc->URI) == lv2DescriptorStereo.URI) {
        return new RubberBandPitchShifter(srate, 2);
    } else {
        cerr << "RubberBandPitchShifter::instantiate: unrecognised URI "
             << desc->URI << " requested" << endl;
        return nullptr;
    }
}

#endif

#ifdef RB_PLUGIN_LADSPA
void
RubberBandPitchShifter::connectPort(LADSPA_Handle handle,
				    unsigned long port, LADSPA_Data *location)
#else 
void
RubberBandPitchShifter::connectPort(LV2_Handle handle,
				    uint32_t port, void *location)
#endif
{
    RubberBandPitchShifter *shifter = (RubberBandPitchShifter *)handle;

    float **ports[PortCountStereo] = {
        &shifter->m_latency,
	&shifter->m_cents,
	&shifter->m_semitones,
	&shifter->m_octaves,
        &shifter->m_crispness,
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
RubberBandPitchShifter::activate(LADSPA_Handle handle)
#else
void
RubberBandPitchShifter::activate(LV2_Handle handle)
#endif
{
    RubberBandPitchShifter *shifter = (RubberBandPitchShifter *)handle;
    shifter->activateImpl();
}

#ifdef RB_PLUGIN_LADSPA
void
RubberBandPitchShifter::run(LADSPA_Handle handle, unsigned long samples)
#else
void
RubberBandPitchShifter::run(LV2_Handle handle, uint32_t samples)
#endif
{
    RubberBandPitchShifter *shifter = (RubberBandPitchShifter *)handle;
    shifter->runImpl(samples);
}

#ifdef RB_PLUGIN_LADSPA
void
RubberBandPitchShifter::deactivate(LADSPA_Handle handle)
#else
void
RubberBandPitchShifter::deactivate(LV2_Handle handle)
#endif
{
    activate(handle); // both functions just reset the plugin
}

#ifdef RB_PLUGIN_LADSPA
void
RubberBandPitchShifter::cleanup(LADSPA_Handle handle)
#else
void
RubberBandPitchShifter::cleanup(LV2_Handle handle)
#endif
{
    delete (RubberBandPitchShifter *)handle;
}

int
RubberBandPitchShifter::getLatency() const
{
    return m_reserve;
}

void
RubberBandPitchShifter::activateImpl()
{
    updateRatio();
    m_prevRatio = m_ratio;
    m_stretcher->reset();
    m_stretcher->setPitchScale(m_ratio);

    for (size_t c = 0; c < m_channels; ++c) {
        m_outputBuffer[c]->reset();
    }

    for (size_t c = 0; c < m_channels; ++c) {
        m_delayMixBuffer[c]->reset();
        m_delayMixBuffer[c]->zero(getLatency());
    }
    
    for (size_t c = 0; c < m_channels; ++c) {
        for (size_t i = 0; i < m_bufsize; ++i) {
            m_scratch[c][i] = 0.f;
        }
    }

    m_minfill = 0;

    m_stretcher->process(m_scratch, m_reserve, false);
}

void
RubberBandPitchShifter::updateRatio()
{
    // The octaves, semitones, and cents parameters are supposed to be
    // integral: we want to enforce that, just to avoid
    // inconsistencies between hosts if some respect the hints more
    // than others
    
#ifdef RB_PLUGIN_LADSPA

    // But we don't want to change the long-standing behaviour of the
    // LADSPA plugin, so let's leave this as-is and only do "the right
    // thing" for LV2
    double oct = (m_octaves ? *m_octaves : 0.0);
    oct += (m_semitones ? *m_semitones : 0.0) / 12;
    oct += (m_cents ? *m_cents : 0.0) / 1200;
    m_ratio = pow(2.0, oct);

#else

    // LV2

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
#endif
}

void
RubberBandPitchShifter::updateCrispness()
{
    if (!m_crispness) return;
    
    int c = lrintf(*m_crispness);
    if (c == m_currentCrispness) return;
    if (c < 0 || c > 3) return;
    RubberBandStretcher *s = m_stretcher;

    switch (c) {
    case 0:
        s->setPhaseOption(RubberBandStretcher::OptionPhaseIndependent);
        s->setTransientsOption(RubberBandStretcher::OptionTransientsSmooth);
        break;
    case 1:
        s->setPhaseOption(RubberBandStretcher::OptionPhaseLaminar);
        s->setTransientsOption(RubberBandStretcher::OptionTransientsSmooth);
        break;
    case 2:
        s->setPhaseOption(RubberBandStretcher::OptionPhaseLaminar);
        s->setTransientsOption(RubberBandStretcher::OptionTransientsMixed);
        break;
    case 3:
        s->setPhaseOption(RubberBandStretcher::OptionPhaseLaminar);
        s->setTransientsOption(RubberBandStretcher::OptionTransientsCrisp);
        break;
    }

    m_currentCrispness = c;
}

void
RubberBandPitchShifter::updateFormant()
{
    if (!m_formant) return;

    bool f = (*m_formant > 0.5f);
    if (f == m_currentFormant) return;
    
    RubberBandStretcher *s = m_stretcher;
    
    s->setFormantOption(f ?
                        RubberBandStretcher::OptionFormantPreserved :
                        RubberBandStretcher::OptionFormantShifted);

    m_currentFormant = f;
}

void
RubberBandPitchShifter::runImpl(uint32_t insamples)
{
    for (size_t c = 0; c < m_channels; ++c) {
        m_delayMixBuffer[c]->write(m_input[c], insamples);
    }

    size_t offset = 0;

    // We have to break up the input into chunks like this because
    // insamples could be arbitrarily large and our output buffer is
    // of limited size

    while (offset < insamples) {

        size_t block = m_blockSize;
        if (offset + block > insamples) {
            block = insamples - offset;
        }

        runImpl(block, offset);

        offset += block;
    }

    float mix = 0.0;
    if (m_wetDry) mix = *m_wetDry;

    for (size_t c = 0; c < m_channels; ++c) {
        if (mix > 0.0) {
            for (size_t i = 0; i < insamples; ++i) {
                float dry = m_delayMixBuffer[c]->readOne();
                m_output[c][i] *= (1.0 - mix);
                m_output[c][i] += dry * mix;
            }
        } else {
            m_delayMixBuffer[c]->skip(insamples);
        }
    }
}

void
RubberBandPitchShifter::runImpl(uint32_t insamples, uint32_t offset)
{
    updateRatio();
    if (m_ratio != m_prevRatio) {
        m_stretcher->setPitchScale(m_ratio);
        m_prevRatio = m_ratio;
    }

    if (m_latency) {
        *m_latency = getLatency();
    }

    updateCrispness();
    updateFormant();

    const int samples = insamples;
    int processed = 0;

    while (processed < samples) {

        // never feed more than the minimum necessary number of
        // samples at a time; ensures nothing will overflow internally
        // and we don't need to call setMaxProcessSize

        int toCauseProcessing = m_stretcher->getSamplesRequired();
        int inchunk = min(samples - processed, toCauseProcessing);

        for (size_t c = 0; c < m_channels; ++c) {
            m_inptrs[c] = &(m_input[c][offset + processed]);
        }

        m_stretcher->process(m_inptrs, inchunk, false);

        processed += inchunk;

        int avail = m_stretcher->available();
        int writable = m_outputBuffer[0]->getWriteSpace();

        int outchunk = avail;
        if (outchunk > writable) {
            cerr << "RubberBandPitchShifter::runImpl: buffer is not large enough: size = " << m_outputBuffer[0]->getSize() << ", chunk = " << outchunk << ", space = " << writable << " (buffer contains " << m_outputBuffer[0]->getReadSpace() << " unread)" << endl;
            outchunk = writable;
        }
        
        size_t actual = m_stretcher->retrieve(m_scratch, outchunk);

        for (size_t c = 0; c < m_channels; ++c) {
            m_outputBuffer[c]->write(m_scratch[c], actual);
        }
    }
    
    for (size_t c = 0; c < m_channels; ++c) {
        int toRead = m_outputBuffer[c]->getReadSpace();
        if (toRead < samples && c == 0) {
            cerr << "RubberBandPitchShifter::runImpl: buffer underrun: required = " << samples << ", available = " << toRead << endl;
        }
        int chunk = min(toRead, samples);
        m_outputBuffer[c]->read(&(m_output[c][offset]), chunk);
    }

    size_t fill = m_outputBuffer[0]->getReadSpace();
    if (fill < m_minfill || m_minfill == 0) {
        m_minfill = fill;
//        cerr << "minfill = " << m_minfill << endl;
    }
}

