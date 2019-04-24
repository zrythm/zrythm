/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/* trivial_synth.c

   DSSI Soft Synth Interface
   Constructed by Chris Cannam, Steve Harris and Sean Bolton

   This is an example DSSI synth plugin written by Steve Harris.

   This example file is in the public domain.
*/

#include <stdlib.h>
#include <string.h>

#include <math.h>
#include <stdio.h>

#include "dssi.h"
#include "ladspa.h"

#define TS_OUTPUT 0
#define TS_FREQ   1
#define TS_VOLUME 2

#define MIDI_NOTES 128

static LADSPA_Descriptor *tsLDescriptor = NULL;
static DSSI_Descriptor *tsDDescriptor = NULL;

static void runTS(LADSPA_Handle instance, unsigned long sample_count,
		  snd_seq_event_t * events, unsigned long EventCount);

typedef struct {
    unsigned int active;
    float amp;
    double phase;
} note_data;

typedef struct {
    LADSPA_Data *output;
    LADSPA_Data *freq;
    LADSPA_Data *vol;
    note_data data[MIDI_NOTES];
    float omega[MIDI_NOTES];
} TS;

const LADSPA_Descriptor *ladspa_descriptor(unsigned long index)
{
    switch (index) {
    case 0:
	return tsLDescriptor;
    default:
	return NULL;
    }
}

const DSSI_Descriptor *dssi_descriptor(unsigned long index)
{
    switch (index) {
    case 0:
	return tsDDescriptor;
    default:
	return NULL;
    }
}

static void cleanupTS(LADSPA_Handle instance)
{
    free(instance);
}

static void connectPortTS(LADSPA_Handle instance, unsigned long port,
			  LADSPA_Data * data)
{
    TS *plugin;

    plugin = (TS *) instance;
    switch (port) {
    case TS_OUTPUT:
	plugin->output = data;
	break;
    case TS_FREQ:
	plugin->freq = data;
	break;
    case TS_VOLUME:
	plugin->vol = data;
	break;
    }
}

static LADSPA_Handle instantiateTS(const LADSPA_Descriptor * descriptor,
				   unsigned long s_rate)
{
    unsigned int i;

    TS *plugin_data = (TS *) malloc(sizeof(TS));
    for (i=0; i<MIDI_NOTES; i++) {
	    plugin_data->omega[i] = M_PI * 2.0 / (double)s_rate *
				    pow(2.0, (i-69.0) / 12.0);
    }

    return (LADSPA_Handle) plugin_data;
}

static void activateTS(LADSPA_Handle instance)
{
    TS *plugin_data = (TS *) instance;
    unsigned int i;

    for (i=0; i<MIDI_NOTES; i++) {
	plugin_data->data[i].active = 0;
    }
}

static void runTSWrapper(LADSPA_Handle instance,
			 unsigned long sample_count)
{
    runTS(instance, sample_count, NULL, 0);
}

static void runTS(LADSPA_Handle instance, unsigned long sample_count,
		  snd_seq_event_t *events, unsigned long event_count)
{
    TS *plugin_data = (TS *) instance;
    LADSPA_Data *const output = plugin_data->output;
    LADSPA_Data freq = *(plugin_data->freq);
    LADSPA_Data vol = *(plugin_data->vol);
    note_data *data = plugin_data->data;
    unsigned long pos;
    unsigned long event_pos;
    unsigned long note;

    if (freq < 1.0) {
	freq = 440.0f;
    }
    if (vol < 0.000001) {
	vol = 1.0f;
    }

    if (event_count > 0) {
	printf("trivial_synth: have %ld events\n", event_count);
    }

    for (pos = 0, event_pos = 0; pos < sample_count; pos++) {

	while (event_pos < event_count
	       && pos == events[event_pos].time.tick) {

	    printf("trivial_synth: event type %d\n", events[event_pos].type);

	    if (events[event_pos].type == SND_SEQ_EVENT_NOTEON) {
		data[events[event_pos].data.note.note].amp =
		    events[event_pos].data.note.velocity / 512.0f;
		data[events[event_pos].data.note.note].
		    active = events[event_pos].data.note.velocity > 0;
		data[events[event_pos].data.note.note].
		    phase = 0.0;
	    } else if (events[event_pos].type == SND_SEQ_EVENT_NOTEOFF) {
		data[events[event_pos].data.note.note].
		    active = 0;
	    }
	    event_pos++;
	}

	/* this is a crazy way to run a synths inner loop, I've
	   just done it this way so its really obvious whats going on */
	output[pos] = 0.0f;
	for (note = 0; note < MIDI_NOTES; note++) {
	    if (data[note].active) {
		output[pos] += sin(data[note].phase) * data[note].amp * vol;
		data[note].phase += plugin_data->omega[note] * freq;
		if (data[note].phase > M_PI * 2.0) {
		    data[note].phase -= M_PI * 2.0;
		}
	    }
	}
    }
}

int getControllerTS(LADSPA_Handle instance, unsigned long port)
{
    switch (port) {
    case TS_VOLUME:
	return 7;
    case TS_FREQ:
	return 9;
    }

    return -1;
}

#ifdef __GNUC__
__attribute__((constructor)) void init()
#else
void _init()
#endif
{
    char **port_names;
    LADSPA_PortDescriptor *port_descriptors;
    LADSPA_PortRangeHint *port_range_hints;

    tsLDescriptor =
	(LADSPA_Descriptor *) malloc(sizeof(LADSPA_Descriptor));
    if (tsLDescriptor) {
	tsLDescriptor->UniqueID = 23;
	tsLDescriptor->Label = "TS";
	tsLDescriptor->Properties = 0;
	tsLDescriptor->Name = "Trivial synth";
	tsLDescriptor->Maker = "Steve Harris <steve@plugin.org.uk>";
	tsLDescriptor->Copyright = "Public Domain";
	tsLDescriptor->PortCount = 3;

	port_descriptors = (LADSPA_PortDescriptor *)
				calloc(tsLDescriptor->PortCount, sizeof
						(LADSPA_PortDescriptor));
	tsLDescriptor->PortDescriptors =
	    (const LADSPA_PortDescriptor *) port_descriptors;

	port_range_hints = (LADSPA_PortRangeHint *)
				calloc(tsLDescriptor->PortCount, sizeof
						(LADSPA_PortRangeHint));
	tsLDescriptor->PortRangeHints =
	    (const LADSPA_PortRangeHint *) port_range_hints;

	port_names = (char **) calloc(tsLDescriptor->PortCount, sizeof(char *));
	tsLDescriptor->PortNames = (const char **) port_names;

	/* Parameters for Output */
	port_descriptors[TS_OUTPUT] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
	port_names[TS_OUTPUT] = "Output";
	port_range_hints[TS_OUTPUT].HintDescriptor = 0;

	/* Parameters for Freq */
	port_descriptors[TS_FREQ] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_names[TS_FREQ] = "Tuning frequency";
	port_range_hints[TS_FREQ].HintDescriptor = LADSPA_HINT_DEFAULT_440 |
			LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
	port_range_hints[TS_FREQ].LowerBound = 420;
	port_range_hints[TS_FREQ].UpperBound = 460;

	/* Parameters for Volume */
	port_descriptors[TS_VOLUME] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_names[TS_VOLUME] = "Volume";
	port_range_hints[TS_VOLUME].HintDescriptor =
			LADSPA_HINT_DEFAULT_MAXIMUM |
			LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
	port_range_hints[TS_VOLUME].LowerBound = 0.0;
	port_range_hints[TS_VOLUME].UpperBound = 1.0;

	tsLDescriptor->activate = activateTS;
	tsLDescriptor->cleanup = cleanupTS;
	tsLDescriptor->connect_port = connectPortTS;
	tsLDescriptor->deactivate = NULL;
	tsLDescriptor->instantiate = instantiateTS;
	tsLDescriptor->run = runTSWrapper;
	tsLDescriptor->run_adding = NULL;
	tsLDescriptor->set_run_adding_gain = NULL;
    }

    tsDDescriptor = (DSSI_Descriptor *) malloc(sizeof(DSSI_Descriptor));
    if (tsDDescriptor) {
	tsDDescriptor->DSSI_API_Version = 1;
	tsDDescriptor->LADSPA_Plugin = tsLDescriptor;
	tsDDescriptor->configure = NULL;
	tsDDescriptor->get_program = NULL;
	tsDDescriptor->get_midi_controller_for_port = getControllerTS;
	tsDDescriptor->select_program = NULL;
	tsDDescriptor->run_synth = runTS;
	tsDDescriptor->run_synth_adding = NULL;
	tsDDescriptor->run_multiple_synths = NULL;
	tsDDescriptor->run_multiple_synths_adding = NULL;
    }
}

#ifdef __GNUC__
__attribute__((destructor)) void fini()
#else
void _fini()
#endif
{
    if (tsLDescriptor) {
	free((LADSPA_PortDescriptor *) tsLDescriptor->PortDescriptors);
	free((char **) tsLDescriptor->PortNames);
	free((LADSPA_PortRangeHint *) tsLDescriptor->PortRangeHints);
	free(tsLDescriptor);
    }
    if (tsDDescriptor) {
	free(tsDDescriptor);
    }
}
