/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/* karplong.c

   DSSI Soft Synth Interface
   Constructed by Chris Cannam, Steve Harris and Sean Bolton

   This is an example DSSI synth plugin written by Chris Cannam.

   It implements the basic Karplus-Strong plucked-string synthesis
   algorithm (Kevin Karplus & Alex Strong, "Digital Synthesis of
   Plucked-String and Drum Timbres", Computer Music Journal 1983).

   My belief is that this algorithm is no longer patented anywhere in
   the world.  The algorithm was protected by US patent no 4,649,783,
   filed in 1984 and granted to Stanford University in 1987.  This
   patent expired in June 2004.  The related European patent EP0124197
   lapsed during the 1990s.

   This also serves as an example of one way to structure a simple
   DSSI plugin in C++.

   This example file is placed in the public domain.
*/

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "dssi.h"
#include "ladspa.h"

//#define DEBUG_KARPLONG 1

#ifdef DEBUG_KARPLONG
#include <iostream>
#endif

class Karplong
{
public:
    static const DSSI_Descriptor *getDescriptor(unsigned long index);

private:
    Karplong(int sampleRate);
    ~Karplong();

    enum {
	OutputPort = 0,
	Sustain    = 1,
	PortCount  = 2
    };

    enum {
	Notes = 128
    };

    static const char *const portNames[PortCount];
    static const LADSPA_PortDescriptor ports[PortCount];
    static const LADSPA_PortRangeHint hints[PortCount];
    static const LADSPA_Properties properties;
    static const LADSPA_Descriptor ladspaDescriptor;
    static const DSSI_Descriptor dssiDescriptor;

    static LADSPA_Handle instantiate(const LADSPA_Descriptor *, unsigned long);
    static void connectPort(LADSPA_Handle, unsigned long, LADSPA_Data *);
    static void activate(LADSPA_Handle);
    static void run(LADSPA_Handle, unsigned long);
    static void deactivate(LADSPA_Handle);
    static void cleanup(LADSPA_Handle);
    static const DSSI_Program_Descriptor *getProgram(LADSPA_Handle, unsigned long);
    static void selectProgram(LADSPA_Handle, unsigned long, unsigned long);
    static int getMidiController(LADSPA_Handle, unsigned long);
    static void runSynth(LADSPA_Handle, unsigned long,
			 snd_seq_event_t *, unsigned long);

    void runImpl(unsigned long, snd_seq_event_t *, unsigned long);
    void addSamples(int, unsigned long, unsigned long);

    float *m_output;
    float *m_sustain;

    int    m_sampleRate;
    long   m_blockStart;

    long   m_ons[Notes];
    long   m_offs[Notes];
    int    m_velocities[Notes];
    float *m_wavetable[Notes];
    float  m_sizes[Notes];
};

const char *const
Karplong::portNames[PortCount] =
{
    "Output",
    "Sustain (on/off)",
};

const LADSPA_PortDescriptor 
Karplong::ports[PortCount] =
{
    LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
    LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
};

const LADSPA_PortRangeHint 
Karplong::hints[PortCount] =
{
    { 0, 0, 0 },
    { LADSPA_HINT_DEFAULT_MINIMUM | LADSPA_HINT_INTEGER |
      LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE, 0, 1 },
};

const LADSPA_Properties
Karplong::properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;

const LADSPA_Descriptor 
Karplong::ladspaDescriptor =
{
    0, // "Unique" ID
    "karplong", // Label
    properties,
    "Simple Karplus-Strong Plucked String Synth", // Name
    "Chris Cannam", // Maker
    "Public Domain", // Copyright
    PortCount,
    ports,
    portNames,
    hints,
    0, // Implementation data
    instantiate,
    connectPort,
    activate,
    run,
    0, // Run adding
    0, // Set run adding gain
    deactivate,
    cleanup
};

const DSSI_Descriptor 
Karplong::dssiDescriptor =
{
    1, // DSSI API version
    &ladspaDescriptor,
    0, // Configure
    0, // Get Program
    0, // Select Program
    getMidiController,
    runSynth,
    0, // Run synth adding
    0, // Run multiple synths
    0, // Run multiple synths adding
};


const DSSI_Descriptor *
Karplong::getDescriptor(unsigned long index)
{
    if (index == 0) return &dssiDescriptor;
    return 0;
}

Karplong::Karplong(int sampleRate) :
    m_output(0),
    m_sustain(0),
    m_sampleRate(sampleRate),
    m_blockStart(0)
{
    for (int i = 0; i < Notes; ++i) {
	float frequency = 440.0f * powf(2.0, (i - 69.0) / 12.0);
	m_sizes[i] = m_sampleRate / frequency;
	m_wavetable[i] = new float[int(m_sizes[i]) + 1];
    }
}

Karplong::~Karplong()
{
    for (int i = 0; i < Notes; ++i) {
	delete[] m_wavetable[i];
    }
}
    
LADSPA_Handle
Karplong::instantiate(const LADSPA_Descriptor *, unsigned long rate)
{
    Karplong *karplong = new Karplong(rate);
    return karplong;
}

void
Karplong::connectPort(LADSPA_Handle handle,
		      unsigned long port, LADSPA_Data *location)
{
    Karplong *karplong = (Karplong *)handle;

    float **ports[PortCount] = {
	&karplong->m_output,
	&karplong->m_sustain,
    };

    *ports[port] = (float *)location;
}

void
Karplong::activate(LADSPA_Handle handle)
{
    Karplong *karplong = (Karplong *)handle;

    karplong->m_blockStart = 0;

    for (size_t i = 0; i < Notes; ++i) {
	karplong->m_ons[i] = -1;
	karplong->m_offs[i] = -1;
	karplong->m_velocities[i] = 0;
    }
}

void
Karplong::run(LADSPA_Handle handle, unsigned long samples)
{
    runSynth(handle, samples, 0, 0);
}

void
Karplong::deactivate(LADSPA_Handle handle)
{
    activate(handle); // both functions just reset the plugin
}

void
Karplong::cleanup(LADSPA_Handle handle)
{
    delete (Karplong *)handle;
}

int
Karplong::getMidiController(LADSPA_Handle, unsigned long port)
{
    int controllers[PortCount] = {
	DSSI_NONE,
	DSSI_CC(64)
    };

    return controllers[port];
}

void
Karplong::runSynth(LADSPA_Handle handle, unsigned long samples,
		       snd_seq_event_t *events, unsigned long eventCount)
{
    Karplong *karplong = (Karplong *)handle;

    karplong->runImpl(samples, events, eventCount);
}

void
Karplong::runImpl(unsigned long sampleCount,
		  snd_seq_event_t *events,
		  unsigned long eventCount)
{
    unsigned long pos;
    unsigned long count;
    unsigned long eventPos;
    snd_seq_ev_note_t n;
    int i;

    for (pos = 0, eventPos = 0; pos < sampleCount; ) {

	while (eventPos < eventCount &&
	       pos >= events[eventPos].time.tick) {

	    switch (events[eventPos].type) {

	    case SND_SEQ_EVENT_NOTEON:
		n = events[eventPos].data.note;
		if (n.velocity > 0) {
		    m_ons[n.note] = m_blockStart + events[eventPos].time.tick;
		    m_offs[n.note] = -1;
		    m_velocities[n.note] = n.velocity;
		}
		break;

	    case SND_SEQ_EVENT_NOTEOFF:
		n = events[eventPos].data.note;
		m_offs[n.note] = m_blockStart + events[eventPos].time.tick;
		break;

	    default:
		break;
	    }

	    ++eventPos;
	}

	count = sampleCount - pos;
	if (eventPos < eventCount &&
	    events[eventPos].time.tick < sampleCount) {
	    count = events[eventPos].time.tick - pos;
	}

	for (i = 0; i < count; ++i) {
	    m_output[pos + i] = 0;
	}

	for (i = 0; i < Notes; ++i) {
	    if (m_ons[i] >= 0) {
		addSamples(i, pos, count);
	    }
	}

	pos += count;
    }

    m_blockStart += sampleCount;
}

void
Karplong::addSamples(int voice, unsigned long offset, unsigned long count)
{
#ifdef DEBUG_KARPLONG
    std::cerr << "Karplong::addSamples(" << voice << ", " << offset
	      << ", " << count << "): on " << m_ons[voice] << ", off "
	      << m_offs[voice] << ", size " << m_sizes[voice]
	      << ", start " << m_blockStart + offset << std::endl;
#endif

    if (m_ons[voice] < 0) return;

    unsigned long on = (unsigned long)(m_ons[voice]);
    unsigned long start = m_blockStart + offset;

    if (start < on) return;

    if (start == on) { 
	for (size_t i = 0; i <= int(m_sizes[voice]); ++i) {
	    m_wavetable[voice][i] = (float(rand()) / float(RAND_MAX)) * 2 - 1;
	}
    }

    size_t i, s;

    float vgain = (float)(m_velocities[voice]) / 127.0f;

    for (i = 0, s = start - on;
	 i < count;
	 ++i, ++s) {

	float gain(vgain);

	if ((!m_sustain || !*m_sustain) &&
	    m_offs[voice] >= 0 &&
	    (unsigned long)(m_offs[voice]) < i + start) {
	    
	    unsigned long release = 1 + (0.01 * m_sampleRate);
	    unsigned long dist = i + start - m_offs[voice];

	    if (dist > release) {
		m_ons[voice] = -1;
		break;
	    }

	    gain = gain * float(release - dist) / float(release);
	}

	size_t sz = int(m_sizes[voice]);
	bool decay = (s > sz);
	size_t index = (s % int(sz));

	float sample = m_wavetable[voice][index];

	if (decay) {
	    if (index == 0) sample += m_wavetable[voice][sz - 1];
	    else sample += m_wavetable[voice][index - 1];
	    sample /= 2;
	    m_wavetable[voice][index] = sample;
	}

	m_output[offset + i] += gain * sample;
    }
}

extern "C" {

const LADSPA_Descriptor *ladspa_descriptor(unsigned long index)
{
    return 0;
}

const DSSI_Descriptor *dssi_descriptor(unsigned long index)
{
    return Karplong::getDescriptor(index);
}

}

