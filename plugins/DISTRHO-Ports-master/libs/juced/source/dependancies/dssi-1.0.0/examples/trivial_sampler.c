/* -*- c-basic-offset: 4 -*-  vi:set ts=8 sts=4 sw=4: */

/* trivial_sampler.c

   DSSI Soft Synth Interface
   Constructed by Chris Cannam, Steve Harris and Sean Bolton

   A straightforward DSSI plugin sampler.

   This example file is in the public domain.
*/

#define _BSD_SOURCE    1
#define _SVID_SOURCE   1
#define _ISOC99_SOURCE 1

#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <math.h>

#include "dssi.h"
#include "ladspa.h"

#include <sndfile.h>
#include <samplerate.h>
#include <pthread.h>

#include "trivial_sampler.h"

static LADSPA_Descriptor *samplerMonoLDescriptor = NULL;
static LADSPA_Descriptor *samplerStereoLDescriptor = NULL;

static DSSI_Descriptor *samplerMonoDDescriptor = NULL;
static DSSI_Descriptor *samplerStereoDDescriptor = NULL;

typedef struct {
    LADSPA_Data *output[2];
    LADSPA_Data *retune;
    LADSPA_Data *basePitch;
    LADSPA_Data *sustain;
    LADSPA_Data *release;
    LADSPA_Data *balance;
    int          channels;
    float       *sampleData[2];
    size_t       sampleCount;
    int          sampleRate;
    long         ons[Sampler_NOTES];
    long         offs[Sampler_NOTES];
    char         velocities[Sampler_NOTES];
    long         sampleNo;
    char        *projectDir;
    pthread_mutex_t mutex;
} Sampler;

static void runSampler(LADSPA_Handle instance, unsigned long sample_count,
		       snd_seq_event_t *events, unsigned long EventCount);

const LADSPA_Descriptor *ladspa_descriptor(unsigned long index)
{
    switch (index) {
    case 0:
	return samplerStereoLDescriptor;
    case 1:
	return samplerMonoLDescriptor;
    default:
	return NULL;
    }
}

const DSSI_Descriptor *dssi_descriptor(unsigned long index)
{
    switch (index) {
    case 0:
	return samplerStereoDDescriptor;
    case 1:
	return samplerMonoDDescriptor;
    default:
	return NULL;
    }
}

static void cleanupSampler(LADSPA_Handle instance)
{
    Sampler *plugin = (Sampler *)instance;
    free(plugin);
}

static void connectPortSampler(LADSPA_Handle instance, unsigned long port,
			       LADSPA_Data * data)
{
    Sampler *plugin;

    plugin = (Sampler *) instance;
    switch (port) {
    case Sampler_OUTPUT_LEFT:
	plugin->output[0] = data;
	break;
    case Sampler_RETUNE:
	plugin->retune = data;
	break;
    case Sampler_BASE_PITCH:
	plugin->basePitch = data;
	break;
    case Sampler_SUSTAIN:
	plugin->sustain = data;
	break;
    case Sampler_RELEASE:
	plugin->release = data;
	break;
    default:
	break;
    }

    if (plugin->channels == 2) {
	switch (port) {
	case Sampler_OUTPUT_RIGHT:
	    plugin->output[1] = data;
	    break;
	case Sampler_BALANCE:
	    plugin->balance = data;
	    break;
	default:
	    break;
	}
    }
}

static LADSPA_Handle instantiateSampler(const LADSPA_Descriptor * descriptor,
					unsigned long s_rate)
{
    Sampler *plugin_data = (Sampler *) malloc(sizeof(Sampler));
    pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

    //!!! check rv from malloc throughout

    plugin_data->output[0] = 0;
    plugin_data->output[1] = 0;
    plugin_data->retune = 0;
    plugin_data->basePitch = 0;
    plugin_data->sustain = 0;
    plugin_data->release = 0;
    plugin_data->balance = 0;
    plugin_data->channels = 1;
    plugin_data->sampleData[0] = 0;
    plugin_data->sampleData[1] = 0;
    plugin_data->sampleCount = 0;
    plugin_data->sampleRate = s_rate;
    plugin_data->projectDir = 0;

    if (!strcmp(descriptor->Label, Sampler_Stereo_LABEL)) {
	plugin_data->channels = 2;
    }
    
    memcpy(&plugin_data->mutex, &m, sizeof(pthread_mutex_t));

    return (LADSPA_Handle) plugin_data;
}

static void activateSampler(LADSPA_Handle instance)
{
    Sampler *plugin_data = (Sampler *) instance;
    unsigned int i;

    pthread_mutex_lock(&plugin_data->mutex);

    plugin_data->sampleNo = 0;

    for (i = 0; i < Sampler_NOTES; i++) {
	plugin_data->ons[i] = -1;
	plugin_data->offs[i] = -1;
	plugin_data->velocities[i] = 0;
    }

    pthread_mutex_unlock(&plugin_data->mutex);
}

static void addSample(Sampler *plugin_data, int n,
		      unsigned long pos, unsigned long count)
{
    float ratio = 1.0;
    float gain = 1.0;
    unsigned long i, ch, s;

    if (plugin_data->retune && *plugin_data->retune) {
	if (plugin_data->basePitch && n != *plugin_data->basePitch) {
	    ratio = powf(1.059463094, n - *plugin_data->basePitch);
	}
    }

    if (pos + plugin_data->sampleNo < plugin_data->ons[n]) return;

    gain = (float)plugin_data->velocities[n] / 127.0f;

    for (i = 0, s = pos + plugin_data->sampleNo - plugin_data->ons[n];
	 i < count;
	 ++i, ++s) {

	float         lgain = gain;
	float         rs = s * ratio;
	unsigned long rsi = lrintf(floor(rs));

	if (rsi >= plugin_data->sampleCount) {
	    plugin_data->ons[n] = -1;
	    break;
	}

	if (plugin_data->offs[n] >= 0 &&
	    pos + i + plugin_data->sampleNo > plugin_data->offs[n]) {

	    unsigned long dist =
		pos + i + plugin_data->sampleNo - plugin_data->offs[n];

	    unsigned long releaseFrames = 200;
	    if (plugin_data->release) {
		releaseFrames = *plugin_data->release * plugin_data->sampleRate;
	    }

	    if (dist > releaseFrames) {
		plugin_data->ons[n] = -1;
		break;
	    } else {
		lgain = lgain * (float)(releaseFrames - dist) / (float)releaseFrames;
	    }
	}
	
	for (ch = 0; ch < plugin_data->channels; ++ch) {

	    float sample = plugin_data->sampleData[ch][rsi] +
		((plugin_data->sampleData[ch][rsi + 1] -
		  plugin_data->sampleData[ch][rsi]) *
		 (rs - (float)rsi));

	    if (plugin_data->balance) {
		if (ch == 0 && *plugin_data->balance > 0) {
		    sample *= 1.0 - *plugin_data->balance;
		} else if (ch == 1 && *plugin_data->balance < 0) {
		    sample *= 1.0 + *plugin_data->balance;
		}
	    }

	    plugin_data->output[ch][pos + i] += lgain * sample;
	}
    }
}

static void runSampler(LADSPA_Handle instance, unsigned long sample_count,
		       snd_seq_event_t *events, unsigned long event_count)
{
    Sampler *plugin_data = (Sampler *) instance;
    unsigned long pos;
    unsigned long count;
    unsigned long event_pos;
    int i;

    for (i = 0; i < plugin_data->channels; ++i) {
	memset(plugin_data->output[i], 0, sample_count * sizeof(float));
    }

    if (pthread_mutex_trylock(&plugin_data->mutex)) {
	return;
    }

    if (!plugin_data->sampleData || !plugin_data->sampleCount) {
	plugin_data->sampleNo += sample_count;
	pthread_mutex_unlock(&plugin_data->mutex);
	return;
    }

    for (pos = 0, event_pos = 0; pos < sample_count; ) {

	while (event_pos < event_count
	       && pos >= events[event_pos].time.tick) {

	    if (events[event_pos].type == SND_SEQ_EVENT_NOTEON) {
		snd_seq_ev_note_t n = events[event_pos].data.note;
		if (n.velocity > 0) {
		    plugin_data->ons[n.note] =
			plugin_data->sampleNo + events[event_pos].time.tick;
		    plugin_data->offs[n.note] = -1;
		    plugin_data->velocities[n.note] = n.velocity;
		} else {
		    if (!plugin_data->sustain || (*plugin_data->sustain < 0.001)) {
			plugin_data->offs[n.note] = 
			    plugin_data->sampleNo + events[event_pos].time.tick;
		    }
		}
	    } else if (events[event_pos].type == SND_SEQ_EVENT_NOTEOFF &&
		       (!plugin_data->sustain || (*plugin_data->sustain < 0.001))) {
		snd_seq_ev_note_t n = events[event_pos].data.note;
		plugin_data->offs[n.note] = 
		    plugin_data->sampleNo + events[event_pos].time.tick;
	    }

	    ++event_pos;
	}

	count = sample_count - pos;
	if (event_pos < event_count &&
	    events[event_pos].time.tick < sample_count) {
	    count = events[event_pos].time.tick - pos;
	}

	for (i = 0; i < Sampler_NOTES; ++i) {
	    if (plugin_data->ons[i] >= 0) {
		addSample(plugin_data, i, pos, count);
	    }
	}

	pos += count;
    }

    plugin_data->sampleNo += sample_count;
    pthread_mutex_unlock(&plugin_data->mutex);
}

static void runSamplerWrapper(LADSPA_Handle instance,
			 unsigned long sample_count)
{
    runSampler(instance, sample_count, NULL, 0);
}

int getControllerSampler(LADSPA_Handle instance, unsigned long port)
{
    if (port == Sampler_RETUNE) return DSSI_CC(12);
    else if (port == Sampler_BASE_PITCH) return DSSI_CC(13);
    else if (port == Sampler_SUSTAIN) return DSSI_CC(64);
    else if (port == Sampler_RELEASE) return DSSI_CC(72);
    else {
	Sampler *plugin_data = (Sampler *) instance;
	if (plugin_data->channels == 2) {
	    if (port == Sampler_BALANCE) return DSSI_CC(10);
	}
    }
    return DSSI_NONE;
}

char *
dssi_configure_message(const char *fmt, ...)
{
    va_list args;
    char buffer[256];

    va_start(args, fmt);
    vsnprintf(buffer, 256, fmt, args);
    va_end(args);
    return strdup(buffer);
}

char *samplerLoad(Sampler *plugin_data, const char *path)
{
    SF_INFO info;
    SNDFILE *file;
    size_t samples = 0;
    float *tmpFrames, *tmpSamples[2], *tmpResamples, *tmpOld[2];
    char *revisedPath = 0;
    size_t i;

    info.format = 0;
    file = sf_open(path, SFM_READ, &info);

    if (!file) {

	const char *filename = strrchr(path, '/');
	if (filename) ++filename;
	else filename = path;

	if (*filename && plugin_data->projectDir) {
	    revisedPath = (char *)malloc(strlen(filename) +
					 strlen(plugin_data->projectDir) + 2);
	    sprintf(revisedPath, "%s/%s", plugin_data->projectDir, filename);
	    file = sf_open(revisedPath, SFM_READ, &info);
	    if (!file) {
		free(revisedPath);
	    }
	}

	if (!file) {
	    return dssi_configure_message
		("error: unable to load sample file '%s'", path);
	}
    }
    
    if (info.frames > Sampler_FRAMES_MAX) {
	return dssi_configure_message
	    ("error: sample file '%s' is too large (%ld frames, maximum is %ld)",
	     path, info.frames, Sampler_FRAMES_MAX);
    }

    //!!! complain also if more than 2 channels

    samples = info.frames;

    tmpFrames = (float *)malloc(info.frames * info.channels * sizeof(float));
    sf_readf_float(file, tmpFrames, info.frames);
    sf_close(file);

    tmpResamples = 0;

    if (info.samplerate != plugin_data->sampleRate) {
	
	double ratio = (double)plugin_data->sampleRate / (double)info.samplerate;
	size_t target = (size_t)(info.frames * ratio);
	SRC_DATA data;

	tmpResamples = (float *)malloc(target * info.channels * sizeof(float));
	memset(tmpResamples, 0, target * info.channels * sizeof(float));

	data.data_in = tmpFrames;
	data.data_out = tmpResamples;
	data.input_frames = info.frames;
	data.output_frames = target;
	data.src_ratio = ratio;

	if (!src_simple(&data, SRC_SINC_BEST_QUALITY, info.channels)) {
	    free(tmpFrames);
	    tmpFrames = tmpResamples;
	    samples = target;
	} else {
	    free(tmpResamples);
	}
    }

    /* add an extra sample for linear interpolation */
    tmpSamples[0] = (float *)malloc((samples + 1) * sizeof(float));

    if (plugin_data->channels == 2) {
	tmpSamples[1] = (float *)malloc((samples + 1) * sizeof(float));
    } else {
	tmpSamples[1] = NULL;
    }


    if (plugin_data->channels == 2) {
	for (i = 0; i < samples; ++i) {
	    int j;
	    for (j = 0; j < 2; ++j) {
		if (j == 1 && info.frames < 2) {
		    tmpSamples[j][i] = tmpSamples[0][i];
		} else {
		    tmpSamples[j][i] = tmpFrames[i * info.channels + j];
		}
	    }
	}
    } else {
	for (i = 0; i < samples; ++i) {
	    int j;
	    tmpSamples[0][i] = 0.0f;
	    for (j = 0; j < info.channels; ++j) {
		tmpSamples[0][i] += tmpFrames[i * info.channels + j];
	    }
	}
    }

    free(tmpFrames);

    /* add an extra sample for linear interpolation */
    tmpSamples[0][samples] = 0.0f;
    if (plugin_data->channels == 2) {
	tmpSamples[1][samples] = 0.0f;
    }
    
    pthread_mutex_lock(&plugin_data->mutex);

    tmpOld[0] = plugin_data->sampleData[0];
    tmpOld[1] = plugin_data->sampleData[1];
    plugin_data->sampleData[0] = tmpSamples[0];
    plugin_data->sampleData[1] = tmpSamples[1];
    plugin_data->sampleCount = samples;

    for (i = 0; i < Sampler_NOTES; ++i) {
	plugin_data->ons[i] = -1;
	plugin_data->offs[i] = -1;
	plugin_data->velocities[i] = 0;
    }

    pthread_mutex_unlock(&plugin_data->mutex);

    if (tmpOld[0]) free(tmpOld[0]);
    if (tmpOld[1]) free(tmpOld[1]);

    printf("%s: loaded %s (%ld samples from original %ld channels resampled from %ld frames at %ld Hz)\n", (plugin_data->channels == 2 ? Sampler_Stereo_LABEL : Sampler_Mono_LABEL), path, (long)samples, (long)info.channels, (long)info.frames, (long)info.samplerate);

    if (revisedPath) {
	char *message = dssi_configure_message("warning: sample file '%s' not found: loading from '%s' instead", path, revisedPath);
	free(revisedPath);
	return message;
    }

    return NULL;
}

char *samplerConfigure(LADSPA_Handle instance, const char *key, const char *value)
{
    Sampler *plugin_data = (Sampler *)instance;

    if (!strcmp(key, "load")) {
	return samplerLoad(plugin_data, value);
    } else if (!strcmp(key, DSSI_PROJECT_DIRECTORY_KEY)) {
	if (plugin_data->projectDir) free(plugin_data->projectDir);
	plugin_data->projectDir = strdup(value);
	return 0;
    }

    return strdup("error: unrecognized configure key");
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
    int channels;

    samplerMonoLDescriptor =
	(LADSPA_Descriptor *) malloc(sizeof(LADSPA_Descriptor));

    samplerStereoLDescriptor =
	(LADSPA_Descriptor *) malloc(sizeof(LADSPA_Descriptor));

    samplerMonoDDescriptor =
	(DSSI_Descriptor *) malloc(sizeof(DSSI_Descriptor));

    samplerStereoDDescriptor =
	(DSSI_Descriptor *) malloc(sizeof(DSSI_Descriptor));

    //!!! malloc rv

    for (channels = 1; channels <= 2; ++channels) {

	int stereo = (channels == 2);

	LADSPA_Descriptor *desc =
	    (stereo ? samplerStereoLDescriptor : samplerMonoLDescriptor);

	desc->UniqueID = channels;
	desc->Label = (stereo ? Sampler_Stereo_LABEL : Sampler_Mono_LABEL);
	desc->Properties = LADSPA_PROPERTY_HARD_RT_CAPABLE;
	desc->Name = (stereo ? "Simple Stereo Sampler" : "Simple Mono Sampler");
	desc->Maker = "Chris Cannam <cannam@all-day-breakfast.com>";
	desc->Copyright = "GPL";
	desc->PortCount = (stereo ? Sampler_Stereo_COUNT : Sampler_Mono_COUNT);

	port_descriptors = (LADSPA_PortDescriptor *)
	    calloc(desc->PortCount, sizeof(LADSPA_PortDescriptor));
	desc->PortDescriptors =
	    (const LADSPA_PortDescriptor *) port_descriptors;

	port_range_hints = (LADSPA_PortRangeHint *)
	    calloc(desc->PortCount, sizeof (LADSPA_PortRangeHint));
	desc->PortRangeHints =
	    (const LADSPA_PortRangeHint *) port_range_hints;

	port_names = (char **) calloc(desc->PortCount, sizeof(char *));
	desc->PortNames = (const char **) port_names;

	/* Parameters for output left */
	port_descriptors[Sampler_OUTPUT_LEFT] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
	port_names[Sampler_OUTPUT_LEFT] = "Output L";
	port_range_hints[Sampler_OUTPUT_LEFT].HintDescriptor = 0;

	/* Parameters for retune */
	port_descriptors[Sampler_RETUNE] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_names[Sampler_RETUNE] = "Tuned (on/off)";
	port_range_hints[Sampler_RETUNE].HintDescriptor =
	    LADSPA_HINT_DEFAULT_MAXIMUM | LADSPA_HINT_INTEGER |
	    LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
	port_range_hints[Sampler_RETUNE].LowerBound = 0;
	port_range_hints[Sampler_RETUNE].UpperBound = 1;

	/* Parameters for base pitch */
	port_descriptors[Sampler_BASE_PITCH] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_names[Sampler_BASE_PITCH] = "Base pitch (MIDI)";
	port_range_hints[Sampler_BASE_PITCH].HintDescriptor =
	    LADSPA_HINT_INTEGER | LADSPA_HINT_DEFAULT_MIDDLE |
	    LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
	port_range_hints[Sampler_BASE_PITCH].LowerBound = Sampler_BASE_PITCH_MIN;
	port_range_hints[Sampler_BASE_PITCH].UpperBound = Sampler_BASE_PITCH_MAX;

	/* Parameters for sustain */
	port_descriptors[Sampler_SUSTAIN] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_names[Sampler_SUSTAIN] = "Sustain (on/off)";
	port_range_hints[Sampler_SUSTAIN].HintDescriptor =
	    LADSPA_HINT_DEFAULT_MINIMUM | LADSPA_HINT_INTEGER |
	    LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
	port_range_hints[Sampler_SUSTAIN].LowerBound = 0.0f;
	port_range_hints[Sampler_SUSTAIN].UpperBound = 1.0f;

	/* Parameters for release */
	port_descriptors[Sampler_RELEASE] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	port_names[Sampler_RELEASE] = "Release time (s)";
	port_range_hints[Sampler_RELEASE].HintDescriptor =
	    LADSPA_HINT_DEFAULT_MINIMUM | LADSPA_HINT_LOGARITHMIC |
	    LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
	port_range_hints[Sampler_RELEASE].LowerBound = Sampler_RELEASE_MIN;
	port_range_hints[Sampler_RELEASE].UpperBound = Sampler_RELEASE_MAX;

	if (stereo) {

	    /* Parameters for output right */
	    port_descriptors[Sampler_OUTPUT_RIGHT] = LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO;
	    port_names[Sampler_OUTPUT_RIGHT] = "Output R";
	    port_range_hints[Sampler_OUTPUT_RIGHT].HintDescriptor = 0;

	    /* Parameters for balance */
	    port_descriptors[Sampler_BALANCE] = LADSPA_PORT_INPUT | LADSPA_PORT_CONTROL;
	    port_names[Sampler_BALANCE] = "Pan / Balance";
	    port_range_hints[Sampler_BALANCE].HintDescriptor =
		LADSPA_HINT_DEFAULT_MIDDLE |
		LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE;
	    port_range_hints[Sampler_BALANCE].LowerBound = -1.0f;
	    port_range_hints[Sampler_BALANCE].UpperBound = 1.0f;
	}

	desc->activate = activateSampler;
	desc->cleanup = cleanupSampler;
	desc->connect_port = connectPortSampler;
	desc->deactivate = activateSampler; // sic
	desc->instantiate = instantiateSampler;
	desc->run = runSamplerWrapper;
	desc->run_adding = NULL;
	desc->set_run_adding_gain = NULL;
    }

    samplerMonoDDescriptor->DSSI_API_Version = 1;
    samplerMonoDDescriptor->LADSPA_Plugin = samplerMonoLDescriptor;
    samplerMonoDDescriptor->configure = samplerConfigure;
    samplerMonoDDescriptor->get_program = NULL;
    samplerMonoDDescriptor->get_midi_controller_for_port = getControllerSampler;
    samplerMonoDDescriptor->select_program = NULL;
    samplerMonoDDescriptor->run_synth = runSampler;
    samplerMonoDDescriptor->run_synth_adding = NULL;
    samplerMonoDDescriptor->run_multiple_synths = NULL;
    samplerMonoDDescriptor->run_multiple_synths_adding = NULL;

    samplerStereoDDescriptor->DSSI_API_Version = 1;
    samplerStereoDDescriptor->LADSPA_Plugin = samplerStereoLDescriptor;
    samplerStereoDDescriptor->configure = samplerConfigure;
    samplerStereoDDescriptor->get_program = NULL;
    samplerStereoDDescriptor->get_midi_controller_for_port = getControllerSampler;
    samplerStereoDDescriptor->select_program = NULL;
    samplerStereoDDescriptor->run_synth = runSampler;
    samplerStereoDDescriptor->run_synth_adding = NULL;
    samplerStereoDDescriptor->run_multiple_synths = NULL;
    samplerStereoDDescriptor->run_multiple_synths_adding = NULL;
}

#ifdef __GNUC__
__attribute__((destructor)) void fini()
#else
void _fini()
#endif
{
    if (samplerMonoLDescriptor) {
	free((LADSPA_PortDescriptor *) samplerMonoLDescriptor->PortDescriptors);
	free((char **) samplerMonoLDescriptor->PortNames);
	free((LADSPA_PortRangeHint *) samplerMonoLDescriptor->PortRangeHints);
	free(samplerMonoLDescriptor);
    }
    if (samplerMonoDDescriptor) {
	free(samplerMonoDDescriptor);
    }
    if (samplerStereoLDescriptor) {
	free((LADSPA_PortDescriptor *) samplerStereoLDescriptor->PortDescriptors);
	free((char **) samplerStereoLDescriptor->PortNames);
	free((LADSPA_PortRangeHint *) samplerStereoLDescriptor->PortRangeHints);
	free(samplerStereoLDescriptor);
    }
    if (samplerStereoDDescriptor) {
	free(samplerStereoDDescriptor);
    }
}
