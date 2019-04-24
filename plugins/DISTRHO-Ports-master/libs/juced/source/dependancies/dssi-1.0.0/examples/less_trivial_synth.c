/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/* less_trivial_synth.c

   DSSI Soft Synth Interface
   Constructed by Chris Cannam, Steve Harris and Sean Bolton

   This is an example DSSI synth plugin written by Steve Harris.

   This example file is in the public domain.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <stdint.h>

#include <math.h>
#include <stdio.h>

#include "dssi.h"
#include "ladspa.h"
#include "saw.h"

#define LTS_OUTPUT  0
#define LTS_FREQ    1
#define LTS_ATTACK  2
#define LTS_DECAY   3
#define LTS_SUSTAIN 4
#define LTS_RELEASE 5
#define LTS_TIMBRE  6
#define LTS_COUNT   7 /* must be 1 + highest value above */

#define POLYPHONY   74
#define MIDI_NOTES  128
#define STEP_SIZE   16

#define GLOBAL_GAIN 0.25f

#define TABLE_MODULUS 1024
#define TABLE_SIZE    (TABLE_MODULUS + 1)
#define TABLE_MASK    (TABLE_MODULUS - 1)

#define FP_IN(x) (x.part.in & TABLE_MASK)
#define FP_FR(x) ((float)x.part.fr * 0.0000152587890625f)
#define FP_OMEGA(w) ((double)TABLE_MODULUS * 65536.0 * (w));

#define LERP(f,a,b) ((a) + (f) * ((b) - (a)))

long int lrintf (float x);

static LADSPA_Descriptor *ltsLDescriptor = NULL;
static DSSI_Descriptor *ltsDDescriptor = NULL;

float *table[2];

typedef enum {
    inactive = 0,
    attack,
    decay,
    sustain,
    release
} state_t;

typedef union {
    uint32_t all;
    struct {
#ifdef WORDS_BIGENDIAN
	uint16_t in;
	uint16_t fr;
#else
	uint16_t fr;
	uint16_t in;
#endif
    } part;
} fixp;

typedef struct {
    state_t state;
    int     note;
    float   amp;
    float   env;
    float   env_d;
    fixp    phase;
    int     counter;
    int     next_event;
} voice_data;

typedef struct {
    LADSPA_Data tune;
    LADSPA_Data attack;
    LADSPA_Data decay;
    LADSPA_Data sustain;
    LADSPA_Data release;
    LADSPA_Data timbre;
    LADSPA_Data pitch;
} synth_vals;

typedef struct {
    LADSPA_Data *output;
    LADSPA_Data *tune;
    LADSPA_Data *attack;
    LADSPA_Data *decay;
    LADSPA_Data *sustain;
    LADSPA_Data *release;
    LADSPA_Data *timbre;
    LADSPA_Data pitch;
    voice_data data[POLYPHONY];
    int note2voice[MIDI_NOTES];
    fixp omega[MIDI_NOTES];
    float fs;
} LTS;

static void runLTS(LADSPA_Handle instance, unsigned long sample_count,
		  snd_seq_event_t * events, unsigned long EventCount);

static void run_voice(LTS *p, synth_vals *vals, voice_data *d,
		      LADSPA_Data *out, unsigned int count);

int pick_voice(const voice_data *data);

const LADSPA_Descriptor *ladspa_descriptor(unsigned long index)
{
    switch (index) {
    case 0:
	return ltsLDescriptor;
    default:
	return NULL;
    }
}

const DSSI_Descriptor *dssi_descriptor(unsigned long index)
{
    switch (index) {
    case 0:
	return ltsDDescriptor;
    default:
	return NULL;
    }
}

static void cleanupLTS(LADSPA_Handle instance)
{
    free(instance);
}

static void connectPortLTS(LADSPA_Handle instance, unsigned long port,
			  LADSPA_Data * data)
{
    LTS *plugin;

    plugin = (LTS *) instance;
    switch (port) {
    case LTS_OUTPUT:
	plugin->output = data;
	break;
    case LTS_FREQ:
	plugin->tune = data;
	break;
    case LTS_ATTACK:
	plugin->attack = data;
	break;
    case LTS_DECAY:
	plugin->decay = data;
	break;
    case LTS_SUSTAIN:
	plugin->sustain = data;
	break;
    case LTS_RELEASE:
	plugin->release = data;
	break;
    case LTS_TIMBRE:
	plugin->timbre = data;
	break;
    }
}

static LADSPA_Handle instantiateLTS(const LADSPA_Descriptor * descriptor,
				   unsigned long s_rate)
{
    unsigned int i;

    LTS *plugin_data = (LTS *) malloc(sizeof(LTS));

    plugin_data->fs = s_rate;

    for (i=0; i<MIDI_NOTES; i++) {
	plugin_data->omega[i].all =
		FP_OMEGA(pow(2.0, (i-69.0) / 12.0) / (double)s_rate);
    }

    return (LADSPA_Handle) plugin_data;
}

static void activateLTS(LADSPA_Handle instance)
{
    LTS *plugin_data = (LTS *) instance;
    unsigned int i;

    for (i=0; i<POLYPHONY; i++) {
	plugin_data->data[i].state = inactive;
    }
    for (i=0; i<MIDI_NOTES; i++) {
	plugin_data->note2voice[i] = 0;
    }
    plugin_data->pitch = 1.0f;
}

static void runLTSWrapper(LADSPA_Handle instance,
			 unsigned long sample_count)
{
    runLTS(instance, sample_count, NULL, 0);
}

static void runLTS(LADSPA_Handle instance, unsigned long sample_count,
		  snd_seq_event_t *events, unsigned long event_count)
{
    LTS *plugin_data = (LTS *) instance;
    LADSPA_Data *const output = plugin_data->output;
    synth_vals vals;
    voice_data *data = plugin_data->data;
    unsigned long i;
    unsigned long pos;
    unsigned long count;
    unsigned long event_pos;
    unsigned long voice;

    vals.tune = *(plugin_data->tune);
    vals.attack = *(plugin_data->attack) * plugin_data->fs;
    vals.decay = *(plugin_data->decay) * plugin_data->fs;
    vals.sustain = *(plugin_data->sustain) * 0.01f;
    vals.release = *(plugin_data->release) * plugin_data->fs;

    vals.pitch = plugin_data->pitch;

    for (pos = 0, event_pos = 0; pos < sample_count; pos += STEP_SIZE) {
	vals.timbre = LERP(0.99f, vals.timbre, *(plugin_data->timbre));
	while (event_pos < event_count
	       && pos >= events[event_pos].time.tick) {
	    if (events[event_pos].type == SND_SEQ_EVENT_NOTEON) {
		snd_seq_ev_note_t n = events[event_pos].data.note;

		if (n.velocity > 0) {
		    const int voice = pick_voice(data);

		    plugin_data->note2voice[n.note] = voice;
		    data[voice].note = n.note;
		    data[voice].amp = sqrtf(n.velocity * .0078740157f) *
				      GLOBAL_GAIN;
		    data[voice].state = attack;
		    data[voice].env = 0.0;
		    data[voice].env_d = 1.0f / vals.attack;
		    data[voice].phase.all = 0;
		    data[voice].counter = 0;
		    data[voice].next_event = vals.attack;
		} else {
		    const int voice = plugin_data->note2voice[n.note];

		    data[voice].state = release;
		    data[voice].env_d = -vals.sustain / vals.release;
		    data[voice].counter = 0;
		    data[voice].next_event = vals.release;
		}
	    } else if (events[event_pos].type == SND_SEQ_EVENT_NOTEOFF) {
		snd_seq_ev_note_t n = events[event_pos].data.note;
		const int voice = plugin_data->note2voice[n.note];

		if (data[voice].state != inactive) {
		    data[voice].state = release;
		    data[voice].env_d = -data[voice].env / vals.release;
		    data[voice].counter = 0;
		    data[voice].next_event = vals.release;
		}
	    } else if (events[event_pos].type == SND_SEQ_EVENT_PITCHBEND) {
		vals.pitch =
		    powf(2.0f, (float)(events[event_pos].data.control.value)
			 * 0.0001220703125f * 0.166666666f);
		plugin_data->pitch = vals.pitch;
	    }
	    event_pos++;
	}

	count = (sample_count - pos) > STEP_SIZE ? STEP_SIZE :
		sample_count - pos;
	for (i=0; i<count; i++) {
	    output[pos + i] = 0.0f;
	}
	for (voice = 0; voice < POLYPHONY; voice++) {
	    if (data[voice].state != inactive) {
		run_voice(plugin_data, &vals, &data[voice], output + pos,
			  count);
	    }
	}
    }
}

static void run_voice(LTS *p, synth_vals *vals, voice_data *d, LADSPA_Data *out, unsigned int count)
{
    unsigned int i;

    for (i=0; i<count; i++) {
	d->phase.all += lrintf((float)p->omega[d->note].all * vals->tune *
				vals->pitch);
	d->env += d->env_d;
	out[i] += LERP(vals->timbre,
		       LERP(FP_FR(d->phase), table[0][FP_IN(d->phase)],
			    table[0][FP_IN(d->phase) + 1]),
		       LERP(FP_FR(d->phase), table[1][FP_IN(d->phase)],
			    table[1][FP_IN(d->phase) + 1])) *
		  d->amp * d->env;
    }

    d->counter += count;
    if (d->counter >= d->next_event) {
	switch (d->state) {
	case inactive:
	    break;
            
	case attack:
	    d->state = decay;
	    d->env_d = (vals->sustain - 1.0f) / vals->decay;
	    d->counter = 0;
	    d->next_event = vals->decay;
	    break;

	case decay:
	    d->state = sustain;
	    d->env_d = 0.0f;
	    d->counter = 0;
	    d->next_event = INT_MAX;
	    break;

	case sustain:
	    d->counter = 0;
	    break;

	case release:
	    d->state = inactive;
	    break;

	default:
	    d->state = inactive;
	    break;
	}
    }
}

int getControllerLTS(LADSPA_Handle instance, unsigned long port)
{
    switch (port) {
    case LTS_ATTACK:
        return DSSI_CC(0x49);
    case LTS_DECAY:
        return DSSI_CC(0x4b);
    case LTS_SUSTAIN:
        return DSSI_CC(0x4f);
    case LTS_RELEASE:
        return DSSI_CC(0x48);
    case LTS_TIMBRE:
        return DSSI_CC(0x01);
    }

    return DSSI_NONE;
}

/* find the voice that is least relevant (low note priority)*/

int pick_voice(const voice_data *data)
{
    unsigned int i;
    int highest_note = 0;
    int highest_note_voice = 0;

    /* Look for an inactive voice */
    for (i=0; i<POLYPHONY; i++) {
	if (data[i].state == inactive) {
	    return i;
	}
    }

    /* otherwise find for the highest note and replace that */
    for (i=0; i<POLYPHONY; i++) {
	if (data[i].note > highest_note) {
	    highest_note = data[i].note;
	    highest_note_voice = i;
	}
    }

    return highest_note_voice;
}

#ifdef __GNUC__
__attribute__((constructor)) void init()
#else
void _init()
#endif
{
    unsigned int i;
    char **port_names;
    float *sin_table;
    LADSPA_PortDescriptor *port_descriptors;
    LADSPA_PortRangeHint *port_range_hints;

    sin_table = malloc(sizeof(float) * TABLE_SIZE);
    for (i=0; i<TABLE_SIZE; i++) {
	sin_table[i] = sin(2.0 * M_PI * (double)i / (double)TABLE_MODULUS);
    }
    table[0] = sin_table;
    table[1] = saw_table;

    ltsLDescriptor =
	(LADSPA_Descriptor *) malloc(sizeof(LADSPA_Descriptor));
    if (ltsLDescriptor) {
	ltsLDescriptor->UniqueID = 24;
	ltsLDescriptor->Label = "LTS";
	ltsLDescriptor->Properties = 0;
	ltsLDescriptor->Name = "Less Trivial synth";
	ltsLDescriptor->Maker = "Steve Harris <steve@plugin.org.uk>";
	ltsLDescriptor->Copyright = "Public Domain";
	ltsLDescriptor->PortCount = LTS_COUNT;

	port_descriptors = (LADSPA_PortDescriptor *)
				calloc(ltsLDescriptor->PortCount, sizeof
						(LADSPA_PortDescriptor));
	ltsLDescriptor->PortDescriptors =
	    (const LADSPA_PortDescriptor *) port_descriptors;

	port_range_hints = (LADSPA_PortRangeHint *)
				calloc(ltsLDescriptor->PortCount, sizeof
						(LADSPA_PortRangeHint));
	ltsLDescriptor->PortRangeHints =
	    (const LADSPA_PortRangeHint *) port_range_hints;

	port_names = (char **) calloc(ltsLDescriptor->PortCount, sizeof(char *));
	ltsLDescriptor->PortNames = (const char **) port_names;

	/* Parameters for output */
	port_descriptors[LTS_OUTPUT] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
	port_names[LTS_OUTPUT] = "Output";
	port_range_hints[LTS_OUTPUT].HintDescriptor = 0;

	/* Parameters for tune */
	port_descriptors[LTS_FREQ] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_names[LTS_FREQ] = "A tuning (Hz)";
	port_range_hints[LTS_FREQ].HintDescriptor = LADSPA_HINT_DEFAULT_440 |
			LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
	port_range_hints[LTS_FREQ].LowerBound = 410;
	port_range_hints[LTS_FREQ].UpperBound = 460;

	/* Parameters for attack */
	port_descriptors[LTS_ATTACK] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_names[LTS_ATTACK] = "Attack time (s)";
	port_range_hints[LTS_ATTACK].HintDescriptor =
			LADSPA_HINT_DEFAULT_LOW |
			LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
	port_range_hints[LTS_ATTACK].LowerBound = 0.01f;
	port_range_hints[LTS_ATTACK].UpperBound = 1.0f;

	/* Parameters for decay */
	port_descriptors[LTS_DECAY] = port_descriptors[LTS_ATTACK];
	port_names[LTS_DECAY] = "Decay time (s)";
	port_range_hints[LTS_DECAY].HintDescriptor =
			port_range_hints[LTS_ATTACK].HintDescriptor;
	port_range_hints[LTS_DECAY].LowerBound =
			port_range_hints[LTS_ATTACK].LowerBound;
	port_range_hints[LTS_DECAY].UpperBound =
			port_range_hints[LTS_ATTACK].UpperBound;

	/* Parameters for sustain */
	port_descriptors[LTS_SUSTAIN] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_names[LTS_SUSTAIN] = "Sustain level (%)";
	port_range_hints[LTS_SUSTAIN].HintDescriptor =
			LADSPA_HINT_DEFAULT_HIGH |
			LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
	port_range_hints[LTS_SUSTAIN].LowerBound = 0.0f;
	port_range_hints[LTS_SUSTAIN].UpperBound = 100.0f;

	/* Parameters for release */
	port_descriptors[LTS_RELEASE] = port_descriptors[LTS_ATTACK];
	port_names[LTS_RELEASE] = "Release time (s)";
	port_range_hints[LTS_RELEASE].HintDescriptor =
			LADSPA_HINT_DEFAULT_MIDDLE | LADSPA_HINT_LOGARITHMIC |
			LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
	port_range_hints[LTS_RELEASE].LowerBound =
			port_range_hints[LTS_ATTACK].LowerBound;
	port_range_hints[LTS_RELEASE].UpperBound =
			port_range_hints[LTS_ATTACK].UpperBound * 4.0f;

	/* Parameters for timbre */
	port_descriptors[LTS_TIMBRE] = port_descriptors[LTS_ATTACK];
	port_names[LTS_TIMBRE] = "Timbre";
	port_range_hints[LTS_TIMBRE].HintDescriptor =
			LADSPA_HINT_DEFAULT_MIDDLE |
			LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
	port_range_hints[LTS_TIMBRE].LowerBound = 0.0f;
	port_range_hints[LTS_TIMBRE].UpperBound = 1.0f;

	ltsLDescriptor->activate = activateLTS;
	ltsLDescriptor->cleanup = cleanupLTS;
	ltsLDescriptor->connect_port = connectPortLTS;
	ltsLDescriptor->deactivate = NULL;
	ltsLDescriptor->instantiate = instantiateLTS;
	ltsLDescriptor->run = runLTSWrapper;
	ltsLDescriptor->run_adding = NULL;
	ltsLDescriptor->set_run_adding_gain = NULL;
    }

    ltsDDescriptor = (DSSI_Descriptor *) malloc(sizeof(DSSI_Descriptor));
    if (ltsDDescriptor) {
	ltsDDescriptor->DSSI_API_Version = 1;
	ltsDDescriptor->LADSPA_Plugin = ltsLDescriptor;
	ltsDDescriptor->configure = NULL;
	ltsDDescriptor->get_program = NULL;
	ltsDDescriptor->get_midi_controller_for_port = getControllerLTS;
	ltsDDescriptor->select_program = NULL;
	ltsDDescriptor->run_synth = runLTS;
	ltsDDescriptor->run_synth_adding = NULL;
	ltsDDescriptor->run_multiple_synths = NULL;
	ltsDDescriptor->run_multiple_synths_adding = NULL;
    }
}

#ifdef __GNUC__
__attribute__((destructor)) void fini()
#else
void _fini()
#endif
{
    if (ltsLDescriptor) {
	free((LADSPA_PortDescriptor *) ltsLDescriptor->PortDescriptors);
	free((char **) ltsLDescriptor->PortNames);
	free((LADSPA_PortRangeHint *) ltsLDescriptor->PortRangeHints);
	free(ltsLDescriptor);
    }
    if (ltsDDescriptor) {
	free(ltsDDescriptor);
    }
}
