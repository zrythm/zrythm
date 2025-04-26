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

#ifndef RUBBERBAND_R3_PITCH_SHIFTER_H
#define RUBBERBAND_R3_PITCH_SHIFTER_H

#ifdef RB_PLUGIN_LADSPA
#ifdef RB_PLUGIN_LV2
#error "Only one of RB_PLUGIN_LADSPA and RB_PLUGIN_LV2 may be defined at once"
#endif
#else
#ifndef RB_PLUGIN_LV2
#error "Including code must define either RB_PLUGIN_LADSPA or RB_PLUGIN_LV2"
#endif
#endif

#ifdef RB_PLUGIN_LADSPA
#include <ladspa.h>
#else
#include <lv2.h>
#endif

#include "common/RingBuffer.h"

namespace RubberBand {
class RubberBandStretcher;
}

class RubberBandR3PitchShifter
{
public:
#ifdef RB_PLUGIN_LADSPA
    static const LADSPA_Descriptor *getDescriptor(unsigned long index);
#else
    static const LV2_Descriptor *getDescriptor(uint32_t index);
#endif
    
protected:
    RubberBandR3PitchShifter(int sampleRate, size_t channels);
    ~RubberBandR3PitchShifter();

    enum {
        LatencyPort      = 0,
	CentsPort        = 1,
	SemitonesPort    = 2,
	OctavesPort      = 3,
	FormantPort      = 4,
	WetDryPort       = 5,
	InputPort1       = 6,
        OutputPort1      = 7,
        PortCountMono    = OutputPort1 + 1,
        InputPort2       = 8,
        OutputPort2      = 9,
        PortCountStereo  = OutputPort2 + 1
    };

#ifdef RB_PLUGIN_LADSPA
    static const char *const portNamesMono[PortCountMono];
    static const LADSPA_PortDescriptor portsMono[PortCountMono];
    static const LADSPA_PortRangeHint hintsMono[PortCountMono];

    static const char *const portNamesStereo[PortCountStereo];
    static const LADSPA_PortDescriptor portsStereo[PortCountStereo];
    static const LADSPA_PortRangeHint hintsStereo[PortCountStereo];

    static const LADSPA_Properties properties;

    static const LADSPA_Descriptor ladspaDescriptorMono;
    static const LADSPA_Descriptor ladspaDescriptorStereo;

    static LADSPA_Handle instantiate(const LADSPA_Descriptor *, unsigned long);
    static void connectPort(LADSPA_Handle, unsigned long, LADSPA_Data *);
    static void activate(LADSPA_Handle);
    static void run(LADSPA_Handle, unsigned long);
    static void deactivate(LADSPA_Handle);
    static void cleanup(LADSPA_Handle);

#else

    static const LV2_Descriptor lv2DescriptorMono;
    static const LV2_Descriptor lv2DescriptorStereo;
    
    static LV2_Handle instantiate(const LV2_Descriptor *, double,
                                  const char *, const LV2_Feature *const *);
    static void connectPort(LV2_Handle, uint32_t, void *);
    static void activate(LV2_Handle);
    static void run(LV2_Handle, uint32_t);
    static void deactivate(LV2_Handle);
    static void cleanup(LV2_Handle);

#endif
    
    void activateImpl();
    void runImpl(uint32_t count);
    void runImpl(uint32_t count, uint32_t offset);
    void updateRatio();
    void updateFormant();

    int getLatency() const;

    float **m_input;
    float **m_output;
    float *m_latency;
    float *m_cents;
    float *m_semitones;
    float *m_octaves;
    float *m_formant;
    float *m_wetDry;
    double m_ratio;
    double m_prevRatio;
    bool m_currentFormant;

    size_t m_blockSize;
    size_t m_reserve;
    size_t m_bufsize;
    size_t m_minfill;

    RubberBand::RubberBandStretcher *m_stretcher;
    RubberBand::RingBuffer<float> **m_outputBuffer;
    RubberBand::RingBuffer<float> **m_delayMixBuffer;
    float **m_scratch;
    float **m_inptrs;

    int m_sampleRate;
    size_t m_channels;
};


#endif
