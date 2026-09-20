// Copyright 2011-2016 David Robillard <d@drobilla.net>
// Copyright 2011 Gabriel M. Beddingfield <gabriel@teuton.org>
// Copyright 2011 James Morris <jwm.art.net@gmail.com>
// SPDX-License-Identifier: ISC

#include "atom_sink.h"
#include "peaks.h"
#include "uris.h"

#include <lv2/atom/atom.h>
#include <lv2/atom/forge.h>
#include <lv2/atom/util.h>
#include <lv2/core/lv2.h>
#include <lv2/core/lv2_util.h>
#include <lv2/log/log.h>
#include <lv2/log/logger.h>
#include <lv2/midi/midi.h>
#include <lv2/state/state.h>
#include <lv2/urid/urid.h>
#include <lv2/worker/worker.h>

#include <samplerate.h>
#include <sndfile.h>

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { SAMPLER_CONTROL = 0, SAMPLER_NOTIFY = 1, SAMPLER_OUT = 2 };

typedef struct {
  SF_INFO  info;     // Info about sample from sndfile
  float*   data;     // Sample data in float
  char*    path;     // Path of file
  uint32_t path_len; // Length of path
} Sample;

typedef struct {
  // Features
  LV2_URID_Map*        map;
  LV2_Worker_Schedule* schedule;
  LV2_Log_Logger       logger;

  // Ports
  const LV2_Atom_Sequence* control_port;
  LV2_Atom_Sequence*       notify_port;
  float*                   output_port;

  // Communication utilities
  LV2_Atom_Forge_Frame notify_frame; ///< Cached for worker replies
  LV2_Atom_Forge       forge;        ///< Forge for writing atoms in run thread
  PeaksSender          psend;        ///< Audio peaks sender

  // URIs
  SamplerURIs uris;

  // Playback state
  Sample*    sample;
  uint32_t   frame_offset;
  float      gain;
  float      gain_dB;
  sf_count_t frame;
  bool       play;
  bool       activated;
  bool       gain_changed;
  bool       sample_changed;
  int        sample_rate;
} Sampler;

/**
   An atom-like message used internally to apply/free samples.

   This is only used internally to communicate with the worker, it is never
   sent to the outside world via a port since it is not POD.  It is convenient
   to use an Atom header so actual atoms can be easily sent through the same
   ringbuffer.
*/
typedef struct {
  LV2_Atom atom;
  Sample*  sample;
} SampleMessage;

/**
   Convert an interleaved audio buffer to mono.

   This simply ignores the data on all channels but the first.
*/
static sf_count_t
convert_to_mono(float* data, sf_count_t num_input_frames, uint32_t channels)
{
  sf_count_t num_output_frames = 0;

  for (sf_count_t i = 0; i < num_input_frames * channels; i += channels) {
    data[num_output_frames++] = data[i];
  }

  return num_output_frames;
}

/**
   Load a new sample and return it.

   Since this is of course not a real-time safe action, this is called in the
   worker thread only.  The sample is loaded and returned only, plugin state is
   not modified.
*/
static Sample*
load_sample(LV2_Log_Logger* logger, const char* path, const int sample_rate)
{
  lv2_log_trace(logger, "Loading %s\n", path);

  const size_t   path_len = strlen(path);
  Sample* const  sample   = (Sample*)calloc(1, sizeof(Sample));
  SF_INFO* const info     = &sample->info;
  SNDFILE* const sndfile  = sf_open(path, SFM_READ, info);
  float*         data     = NULL;
  bool           error    = true;
  if (!sndfile || !info->frames) {
    lv2_log_error(logger, "Failed to open %s\n", path);
  } else if (!(data = (float*)malloc(sizeof(float) * info->frames *
                                     info->channels))) {
    lv2_log_error(logger, "Failed to allocate memory for sample\n");
  } else {
    error = false;
  }

  if (error) {
    free(sample);
    free(data);
    sf_close(sndfile);
    return NULL;
  }

  sf_seek(sndfile, 0UL, SEEK_SET);
  sf_read_float(sndfile, data, info->frames * info->channels);
  sf_close(sndfile);

  if (info->channels != 1) {
    info->frames   = convert_to_mono(data, info->frames, info->channels);
    info->channels = 1;
  }

  if (info->samplerate != sample_rate) {
    lv2_log_trace(logger,
                  "Converting from %d Hz to %d Hz\n",
                  info->samplerate,
                  sample_rate);

    const double src_ratio     = (double)sample_rate / (double)info->samplerate;
    const double output_length = ceil((double)info->frames * src_ratio);
    const long   output_frames = (long)output_length;
    float* const output_buffer = (float*)malloc(sizeof(float) * output_frames);

    SRC_DATA src_data = {
      data,
      output_buffer,
      info->frames,
      output_frames,
      0,
      0,
      0,
      src_ratio,
    };

    if (src_simple(&src_data, SRC_SINC_BEST_QUALITY, 1) != 0) {
      lv2_log_error(logger, "Sample rate conversion failed\n");
      free(output_buffer);
    } else {
      // Replace original data with converted buffer
      free(data);
      data         = output_buffer;
      info->frames = src_data.output_frames_gen;
    }
  } else {
    lv2_log_trace(
      logger, "Sample matches the current rate of %d Hz\n", sample_rate);
  }

  // Fill sample struct and return it
  sample->data     = data;
  sample->path     = (char*)malloc(path_len + 1);
  sample->path_len = (uint32_t)path_len;
  memcpy(sample->path, path, path_len + 1);

  return sample;
}

static void
free_sample(Sampler* self, Sample* sample)
{
  if (sample) {
    lv2_log_trace(&self->logger, "Freeing %s\n", sample->path);
    free(sample->path);
    free(sample->data);
    free(sample);
  }
}

/**
   Do work in a non-realtime thread.

   This is called for every piece of work scheduled in the audio thread using
   self->schedule->schedule_work().  A reply can be sent back to the audio
   thread using the provided `respond` function.
*/
static LV2_Worker_Status
work(LV2_Handle                  instance,
     LV2_Worker_Respond_Function respond,
     LV2_Worker_Respond_Handle   handle,
     uint32_t                    size,
     const void*                 data)
{
  Sampler*        self = (Sampler*)instance;
  const LV2_Atom* atom = (const LV2_Atom*)data;
  if (atom->type == self->uris.eg_freeSample) {
    // Free old sample
    const SampleMessage* msg = (const SampleMessage*)data;
    free_sample(self, msg->sample);
  } else if (atom->type == self->forge.Object) {
    // Handle set message (load sample).
    const LV2_Atom_Object* obj  = (const LV2_Atom_Object*)data;
    const char*            path = read_set_file(&self->uris, obj);
    if (!path) {
      lv2_log_error(&self->logger, "Malformed set file request\n");
      return LV2_WORKER_ERR_UNKNOWN;
    }

    // Load sample.
    Sample* sample = load_sample(&self->logger, path, self->sample_rate);
    if (sample) {
      // Send new sample to run() to be applied
      respond(handle, sizeof(Sample*), &sample);
    }
  }

  return LV2_WORKER_SUCCESS;
}

/**
   Handle a response from work() in the audio thread.

   When running normally, this will be called by the host after run().  When
   freewheeling, this will be called immediately at the point the work was
   scheduled.
*/
static LV2_Worker_Status
work_response(LV2_Handle instance, uint32_t size, const void* data)
{
  Sampler*      self       = (Sampler*)instance;
  Sample*       old_sample = self->sample;
  const Sample* new_sample = *(Sample* const*)data;

  // Install the new sample
  self->sample = *(Sample* const*)data;

  // Stop playing previous sample, which can be larger than new one
  self->frame = 0;
  self->play  = false;

  // Schedule work to free the old sample
  SampleMessage msg = {{sizeof(Sample*), self->uris.eg_freeSample}, old_sample};
  self->schedule->schedule_work(self->schedule->handle, sizeof(msg), &msg);

  // Send a notification that we're using a new sample
  lv2_atom_forge_frame_time(&self->forge, self->frame_offset);
  write_set_file(
    &self->forge, &self->uris, new_sample->path, new_sample->path_len);

  return LV2_WORKER_SUCCESS;
}

static void
connect_port(LV2_Handle instance, uint32_t port, void* data)
{
  Sampler* self = (Sampler*)instance;
  switch (port) {
  case SAMPLER_CONTROL:
    self->control_port = (const LV2_Atom_Sequence*)data;
    break;
  case SAMPLER_NOTIFY:
    self->notify_port = (LV2_Atom_Sequence*)data;
    break;
  case SAMPLER_OUT:
    self->output_port = (float*)data;
    break;
  default:
    break;
  }
}

static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    rate,
            const char*               path,
            const LV2_Feature* const* features)
{
  // Allocate and initialise instance structure.
  Sampler* self = (Sampler*)calloc(1, sizeof(Sampler));
  if (!self) {
    return NULL;
  }

  // Get host features
  // clang-format off
  const char* missing = lv2_features_query(
    features,
    LV2_LOG__log,         &self->logger.log, false,
    LV2_URID__map,        &self->map,        true,
    LV2_WORKER__schedule, &self->schedule,   true,
    NULL);
  // clang-format on

  lv2_log_logger_set_map(&self->logger, self->map);
  if (missing) {
    lv2_log_error(&self->logger, "Missing feature <%s>\n", missing);
    free(self);
    return NULL;
  }

  // Map URIs and initialise forge
  map_sampler_uris(self->map, &self->uris);
  lv2_atom_forge_init(&self->forge, self->map);
  peaks_sender_init(&self->psend, self->map);

  self->gain        = 1.0f;
  self->gain_dB     = 0.0f;
  self->sample_rate = (int)rate;

  return (LV2_Handle)self;
}

static void
cleanup(LV2_Handle instance)
{
  Sampler* self = (Sampler*)instance;
  free_sample(self, self->sample);
  free(self);
}

static void
activate(LV2_Handle instance)
{
  ((Sampler*)instance)->activated = true;
}

static void
deactivate(LV2_Handle instance)
{
  ((Sampler*)instance)->activated = false;
}

/** Define a macro for converting a gain in dB to a coefficient. */
#define DB_CO(g) ((g) > -90.0f ? powf(10.0f, (g) * 0.05f) : 0.0f)

/**
   Handle an incoming event in the audio thread.

   This performs any actions triggered by an event, such as the start of sample
   playback, a sample change, or responding to requests from the UI.
*/
static void
handle_event(Sampler* self, LV2_Atom_Event* ev)
{
  SamplerURIs* uris       = &self->uris;
  PeaksURIs*   peaks_uris = &self->psend.uris;

  if (ev->body.type == uris->midi_Event) {
    const uint8_t* const msg = (const uint8_t*)(ev + 1);
    switch (lv2_midi_message_type(msg)) {
    case LV2_MIDI_MSG_NOTE_ON:
      self->frame = 0;
      self->play  = true;
      break;
    default:
      break;
    }
  } else if (lv2_atom_forge_is_object_type(&self->forge, ev->body.type)) {
    const LV2_Atom_Object* obj = (const LV2_Atom_Object*)&ev->body;
    if (obj->body.otype == uris->patch_Set) {
      // Get the property and value of the set message
      const LV2_Atom* property = NULL;
      const LV2_Atom* value    = NULL;

      // clang-format off
      lv2_atom_object_get(obj,
                          uris->patch_property, &property,
                          uris->patch_value,    &value,
                          0);
      // clang-format on

      if (!property) {
        lv2_log_error(&self->logger, "Set message with no property\n");
        return;
      }

      if (property->type != uris->atom_URID) {
        lv2_log_error(&self->logger, "Set property is not a URID\n");
        return;
      }

      const uint32_t key = ((const LV2_Atom_URID*)property)->body;
      if (key == uris->eg_sample) {
        // Sample change, send it to the worker.
        lv2_log_trace(&self->logger, "Scheduling sample change\n");
        self->schedule->schedule_work(
          self->schedule->handle, lv2_atom_total_size(&ev->body), &ev->body);
      } else if (key == uris->param_gain) {
        // Gain change
        if (value->type == uris->atom_Float) {
          self->gain_dB = ((LV2_Atom_Float*)value)->body;
          self->gain    = DB_CO(self->gain_dB);
        }
      }
    } else if (obj->body.otype == uris->patch_Get && self->sample) {
      const LV2_Atom_URID* accept  = NULL;
      const LV2_Atom_Int*  n_peaks = NULL;

      // clang-format off
      lv2_atom_object_get_typed(
        obj,
        uris->patch_accept,      &accept,  uris->atom_URID,
        peaks_uris->peaks_total, &n_peaks, peaks_uris->atom_Int,
        0);
      // clang-format on

      if (accept && accept->body == peaks_uris->peaks_PeakUpdate) {
        // Received a request for peaks, prepare for transmission
        peaks_sender_start(&self->psend,
                           self->sample->data,
                           self->sample->info.frames,
                           n_peaks->body);
      } else {
        // Received a get message, emit our state (probably to UI)
        lv2_atom_forge_frame_time(&self->forge, self->frame_offset);
        write_set_file(&self->forge,
                       &self->uris,
                       self->sample->path,
                       self->sample->path_len);
      }
    } else {
      lv2_log_trace(&self->logger, "Unknown object type %u\n", obj->body.otype);
    }
  } else {
    lv2_log_trace(&self->logger, "Unknown event type %u\n", ev->body.type);
  }
}

/**
   Output audio for a slice of the current cycle.
*/
static void
render(Sampler* self, uint32_t start, uint32_t end)
{
  float* output = self->output_port;

  if (self->play && self->sample) {
    // Start/continue writing sample to output
    for (; start < end; ++start) {
      output[start] = self->sample->data[self->frame] * self->gain;
      if (++self->frame == self->sample->info.frames) {
        self->play = false; // Reached end of sample
        break;
      }
    }
  }

  // Write silence to remaining buffer
  for (; start < end; ++start) {
    output[start] = 0.0f;
  }
}

static void
run(LV2_Handle instance, uint32_t sample_count)
{
  Sampler* self = (Sampler*)instance;

  // Set up forge to write directly to notify output port.
  const uint32_t notify_capacity = self->notify_port->atom.size;
  lv2_atom_forge_set_buffer(
    &self->forge, (uint8_t*)self->notify_port, notify_capacity);

  // Start a sequence in the notify output port.
  lv2_atom_forge_sequence_head(&self->forge, &self->notify_frame, 0);

  // Send update to UI if gain has changed due to state restore
  if (self->gain_changed) {
    lv2_atom_forge_frame_time(&self->forge, 0);
    write_set_gain(&self->forge, &self->uris, self->gain_dB);
    self->gain_changed = false;
  }

  // Send update to UI if sample has changed due to state restore
  if (self->sample_changed) {
    lv2_atom_forge_frame_time(&self->forge, 0);
    write_set_file(
      &self->forge, &self->uris, self->sample->path, self->sample->path_len);
    self->sample_changed = false;
  }

  // Iterate over incoming events, emitting audio along the way
  self->frame_offset = 0;
  LV2_ATOM_SEQUENCE_FOREACH (self->control_port, ev) {
    // Render output up to the time of this event
    render(self, self->frame_offset, ev->time.frames);

    /* Update current frame offset to this event's time.  This is stored in
       the instance because it is used for synchronous worker event
       execution.  This allows a sample load event to be executed with
       sample accuracy when running in a non-realtime context (such as
       exporting a session). */
    self->frame_offset = ev->time.frames;

    // Process this event
    handle_event(self, ev);
  }

  // Use available space after any emitted events to send peaks
  peaks_sender_send(
    &self->psend, &self->forge, sample_count, self->frame_offset);

  // Render output for the rest of the cycle past the last event
  render(self, self->frame_offset, sample_count);
}

static LV2_State_Status
save(LV2_Handle                instance,
     LV2_State_Store_Function  store,
     LV2_State_Handle          handle,
     uint32_t                  flags,
     const LV2_Feature* const* features)
{
  Sampler* self = (Sampler*)instance;
  if (!self->sample) {
    return LV2_STATE_SUCCESS;
  }

  LV2_State_Map_Path* map_path =
    (LV2_State_Map_Path*)lv2_features_data(features, LV2_STATE__mapPath);
  if (!map_path) {
    return LV2_STATE_ERR_NO_FEATURE;
  }

  // Map absolute sample path to an abstract state path
  char* apath = map_path->abstract_path(map_path->handle, self->sample->path);

  // Store eg:sample = abstract path
  store(handle,
        self->uris.eg_sample,
        apath,
        strlen(apath) + 1,
        self->uris.atom_Path,
        LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);

  free(apath);

  // Store the gain value
  store(handle,
        self->uris.param_gain,
        &self->gain_dB,
        sizeof(self->gain_dB),
        self->uris.atom_Float,
        LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);

  return LV2_STATE_SUCCESS;
}

static LV2_State_Status
restore(LV2_Handle                  instance,
        LV2_State_Retrieve_Function retrieve,
        LV2_State_Handle            handle,
        uint32_t                    flags,
        const LV2_Feature* const*   features)
{
  Sampler* self = (Sampler*)instance;

  // Get host features
  LV2_Worker_Schedule* schedule = NULL;
  LV2_State_Map_Path*  paths    = NULL;

  // clang-format off
  const char* missing = lv2_features_query(
    features,
    LV2_STATE__mapPath,   &paths,    true,
    LV2_WORKER__schedule, &schedule, false,
    NULL);
  // clang-format on

  if (missing) {
    lv2_log_error(&self->logger, "Missing feature <%s>\n", missing);
    return LV2_STATE_ERR_NO_FEATURE;
  }

  // Get eg:sample from state
  size_t      size     = 0;
  uint32_t    type     = 0;
  uint32_t    valflags = 0;
  const void* value =
    retrieve(handle, self->uris.eg_sample, &size, &type, &valflags);

  if (!value) {
    lv2_log_error(&self->logger, "Missing eg:sample\n");
    return LV2_STATE_ERR_NO_PROPERTY;
  }

  if (type != self->uris.atom_Path) {
    lv2_log_error(&self->logger, "Non-path eg:sample\n");
    return LV2_STATE_ERR_BAD_TYPE;
  }

  // Map abstract state path to absolute path
  const char* apath = (const char*)value;
  char*       path  = paths->absolute_path(paths->handle, apath);

  // Replace current sample with the new one
  if (!self->activated || !schedule) {
    // No scheduling available, load sample immediately
    lv2_log_trace(&self->logger, "Synchronous restore\n");
    Sample* sample = load_sample(&self->logger, path, self->sample_rate);
    if (sample) {
      free_sample(self, self->sample);
      self->sample         = sample;
      self->sample_changed = true;
    }
  } else {
    // Schedule sample to be loaded by the provided worker
    lv2_log_trace(&self->logger, "Scheduling restore\n");
    LV2_Atom_Forge forge;
    LV2_Atom*      buf = (LV2_Atom*)calloc(1, strlen(path) + 128);
    lv2_atom_forge_init(&forge, self->map);
    lv2_atom_forge_set_sink(&forge, atom_sink, atom_sink_deref, buf);
    write_set_file(&forge, &self->uris, path, strlen(path));

    const uint32_t msg_size = lv2_atom_pad_size(buf->size);
    schedule->schedule_work(self->schedule->handle, msg_size, buf + 1);
    free(buf);
  }

  free(path);

  // Get param:gain from state
  value = retrieve(handle, self->uris.param_gain, &size, &type, &valflags);

  if (!value) {
    // Not an error, since older versions did not save this property
    lv2_log_note(&self->logger, "Missing param:gain\n");
    return LV2_STATE_SUCCESS;
  }

  if (type != self->uris.atom_Float) {
    lv2_log_error(&self->logger, "Non-float param:gain\n");
    return LV2_STATE_ERR_BAD_TYPE;
  }

  self->gain_dB      = *(const float*)value;
  self->gain         = DB_CO(self->gain_dB);
  self->gain_changed = true;

  return LV2_STATE_SUCCESS;
}

static const void*
extension_data(const char* uri)
{
  static const LV2_State_Interface  state  = {save, restore};
  static const LV2_Worker_Interface worker = {work, work_response, NULL};

  if (!strcmp(uri, LV2_STATE__interface)) {
    return &state;
  }

  if (!strcmp(uri, LV2_WORKER__interface)) {
    return &worker;
  }

  return NULL;
}

static const LV2_Descriptor descriptor = {EG_SAMPLER_URI,
                                          instantiate,
                                          connect_port,
                                          activate,
                                          run,
                                          deactivate,
                                          cleanup,
                                          extension_data};

LV2_SYMBOL_EXPORT const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
  return index == 0 ? &descriptor : NULL;
}
