// SPDX-FileCopyrightText: 2006-2016 David Robillard <d@drobilla.net>
// SPDX-FileCopyrightText: 2006 Steve Harris <steve@plugin.org.uk>
// SPDX-License-Identifier: ISC
// Ported from the upstream C source to C++.

/** Include standard C++ headers */
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <thread>

/** Sleeps for the given number of microseconds. */
static void
sleep_us (long us)
{
  std::this_thread::sleep_for (std::chrono::microseconds (us));
}

/**
   LV2 headers are based on the URI of the specification they come from, so a
   consistent convention can be used even for unofficial extensions.  The URI
   of the core LV2 specification is <http://lv2plug.in/ns/lv2core>, by
   replacing `http:/` with `lv2` any header in the specification bundle can be
   included, in this case `lv2.h`.
*/
#include "lv2/atom/atom.h"
#include "lv2/atom/util.h"
#include "lv2/core/lv2.h"
#include "lv2/state/state.h"
#include "lv2/worker/worker.h"

/**
   The URI is the identifier for a plugin, and how the host associates this
   implementation in code with its description in data.  In this plugin it is
   only used once in the code, but defining the plugin URI at the top of the
   file is a good convention to follow.  If this URI does not match that used
   in the data files, the host will fail to load the plugin.
*/
#define AMP_URI "http://lv2plug.in/plugins/eg-amp"

/* Same implementation, exposed under a second URI so the bundle holds a
    plugin whose only UI opens its own window (ui:showInterface) */
#define AMP_FLOAT_WINDOW_URI "http://lv2plug.in/plugins/eg-amp-float-window"

/* Atom events delivered to the message port, observable by tests
   through dlopen() */
static std::atomic<int> g_message_events{ 0 };

LV2_SYMBOL_EXPORT
int
amp_message_event_count (void)
{
  return g_message_events;
}

/* Worker responses delivered back to the plugin, observable by tests
   through dlopen() */
static std::atomic<int>   g_worker_responses{ 0 };
static std::atomic<float> g_worker_last_response{ 0.0f };
static std::atomic<float> g_last_freewheel{ 0.0f };
/* Overlapping work() call detector and the probe value whose job
   holds work() open, observable by tests through dlopen() */
static std::atomic<int> g_work_in_progress{ 0 };
static std::atomic<int> g_concurrent_work{ 0 };
static const float      kSlowJobProbe = 6.0f;
/* When set, set_state also schedules through the cached feature, for
   tests that verify work() serialization */
static std::atomic<int> g_mixed_restore{ 0 };

/* end_run() calls on the float-window twin, observable by tests
   through dlopen() */
static std::atomic<int> g_float_window_end_runs{ 0 };

LV2_SYMBOL_EXPORT
int
amp_worker_response_count (void)
{
  return g_worker_responses;
}

LV2_SYMBOL_EXPORT
float
amp_worker_last_response (void)
{
  return g_worker_last_response;
}

LV2_SYMBOL_EXPORT
float
amp_last_freewheel (void)
{
  return g_last_freewheel;
}

LV2_SYMBOL_EXPORT
int
amp_concurrent_work (void)
{
  return g_concurrent_work.load ();
}

LV2_SYMBOL_EXPORT
void
amp_set_mixed_restore (int enabled)
{
  g_mixed_restore.store (enabled);
}

LV2_SYMBOL_EXPORT
int
amp_float_window_end_run_count (void)
{
  return g_float_window_end_runs;
}

/**
   In code, ports are referred to by index.  An enumeration of port indices
   should be defined for readability.
*/
typedef enum
{
  AMP_GAIN = 0,
  AMP_INPUT = 1,
  AMP_OUTPUT = 2,
  AMP_MUTE = 3,
  AMP_MESSAGE = 4,
  AMP_TRIGGER = 5,
  AMP_FREEWHEEL = 6
} PortIndex;

/**
   Every plugin defines a private structure for the plugin instance.  All data
   associated with a plugin instance is stored here, and is available to
   every instance method.  In this simple plugin, only port buffers need to be
   stored, since there is no additional instance data.
*/
typedef struct
{
  // Port buffers
  const float *            gain;
  const float *            input;
  float *                  output;
  const float *            mute;
  const LV2_Atom_Sequence *message;
  const float *            trigger;
  const float *            freewheel;
  // Host-provided worker feature
  const LV2_Worker_Schedule *schedule;
} Amp;

/**
   The `instantiate()` function is called by the host to create a new plugin
   instance.  The host passes the plugin descriptor, sample rate, and bundle
   path for plugins that need to load additional resources (e.g. waveforms).
   The features parameter contains host-provided features defined in LV2
   extensions, but this simple plugin does not use any.

   This function is in the ``instantiation'' threading class, so no other
   methods on this instance will be called concurrently with it.
*/
static LV2_Handle
instantiate (
  const LV2_Descriptor *      descriptor,
  double                      rate,
  const char *                bundle_path,
  const LV2_Feature * const * features)
{
  (void) descriptor;
  (void) rate;
  (void) bundle_path;

  Amp * amp = (Amp *) calloc (1, sizeof (Amp));

  for (const LV2_Feature * const * f = features; *f != NULL; ++f)
    {
      if (strcmp ((*f)->URI, LV2_WORKER__schedule) == 0)
        {
          amp->schedule = (const LV2_Worker_Schedule *) (*f)->data;
        }
    }

  return (LV2_Handle) amp;
}

/**
   The `connect_port()` method is called by the host to connect a particular
   port to a buffer.  The plugin must store the data location, but data may not
   be accessed except in run().

   This method is in the ``audio'' threading class, and is called in the same
   context as run().
*/
static void
connect_port (LV2_Handle instance, uint32_t port, void * data)
{
  Amp * amp = (Amp *) instance;

  switch ((PortIndex) port)
    {
    case AMP_GAIN:
      amp->gain = (const float *) data;
      break;
    case AMP_INPUT:
      amp->input = (const float *) data;
      break;
    case AMP_OUTPUT:
      amp->output = (float *) data;
      break;
    case AMP_MUTE:
      amp->mute = (const float *) data;
      break;
    case AMP_MESSAGE:
      amp->message = (const LV2_Atom_Sequence *) data;
      break;
    case AMP_TRIGGER:
      amp->trigger = (const float *) data;
      break;
    case AMP_FREEWHEEL:
      amp->freewheel = (const float *) data;
      break;
    }
}

/**
   The `activate()` method is called by the host to initialise and prepare the
   plugin instance for running.  The plugin must reset all internal state
   except for buffer locations set by `connect_port()`.

   This method is in the ``instantiation'' threading class, so no other
   methods on this instance will be called concurrently with it.
*/
static void
activate (LV2_Handle instance)
{
  /* Schedule a job to verify the worker is already running when the
     instance is activated */
  Amp * amp = (Amp *) instance;
  if (amp->schedule != NULL)
    {
      const float activation_probe = 3.0f;
      amp->schedule->schedule_work (
        amp->schedule->handle, sizeof (float), &activation_probe);
    }
}

/** Define a macro for converting a gain in dB to a coefficient. */
#define DB_CO(g) ((g) > -90.0f ? powf (10.0f, (g) *0.05f) : 0.0f)

/**
   The `run()` method is the main process function of the plugin.  It processes
   a block of audio in the audio context.  Since this plugin is
   `lv2:hardRTCapable`, `run()` must be real-time safe, so blocking (e.g. with
   a mutex) or memory allocation are not allowed.
*/
static void
run (LV2_Handle instance, uint32_t n_samples)
{
  Amp * amp = (Amp *) instance;

  const float         gain = *(amp->gain);
  const float * const input = amp->input;
  float * const       output = amp->output;

  const float coef = DB_CO (gain);

  for (uint32_t pos = 0; pos < n_samples; pos++)
    {
      output[pos] = input[pos] * coef;
    }

  /* The message port is only connected for the main eg-amp plugin; the
     float-window twin has no such port in its data */
  if (amp->message != NULL)
    {
      LV2_ATOM_SEQUENCE_FOREACH (amp->message, ev)
        {
          (void) ev;
          ++g_message_events;
        }
    }

  /* The freeWheeling-designated port is host-driven: record what the
     host last wrote so tests can observe it */
  if (amp->freewheel != NULL)
    {
      g_last_freewheel = *amp->freewheel;
    }

  /* The trigger port is a port-props trigger: the host resets it to
     its default after each run, so one high value produces one job
     whose response doubles it */
  if (amp->trigger != NULL && amp->schedule != NULL && *amp->trigger > 0.5f)
    {
      const float trigger = *amp->trigger;
      amp->schedule->schedule_work (
        amp->schedule->handle, sizeof (float), &trigger);
    }
}

/**
   The `deactivate()` method is the counterpart to `activate()`, and is called by
   the host after running the plugin.  It indicates that the host will not call
   `run()` again until another call to `activate()` and is mainly useful for more
   advanced plugins with ``live'' characteristics such as those with auxiliary
   processing threads.  As with `activate()`, this plugin has no use for this
   information so this method does nothing.

   This method is in the ``instantiation'' threading class, so no other
   methods on this instance will be called concurrently with it.
*/
static void
deactivate (LV2_Handle instance)
{
  (void) instance;
}

/**
   Destroy a plugin instance (counterpart to `instantiate()`).

   This method is in the ``instantiation'' threading class, so no other
   methods on this instance will be called concurrently with it.
*/
static void
cleanup (LV2_Handle instance)
{
  free (instance);
}

/**
   The `extension_data()` function returns any extension data supported by the
   plugin.  Note that this is not an instance method, but a function on the
   plugin descriptor.  It is usually used by plugins to implement additional
   interfaces.  This plugin provides the worker interface.

   This method is in the ``discovery'' threading class, so no other functions
   or methods in this plugin library will be called concurrently with it.
*/
static LV2_Worker_Status
work (
  LV2_Handle                  instance,
  LV2_Worker_Respond_Function respond,
  LV2_Worker_Respond_Handle   handle,
  uint32_t                    size,
  const void *                data)
{
  (void) instance;

  if (size < sizeof (float))
    {
      return LV2_WORKER_ERR_UNKNOWN;
    }

  // Detects overlapping work() calls, observable by tests through
  // dlopen()
  if (g_work_in_progress.fetch_add (1) != 0)
    {
      g_concurrent_work.store (1);
    }

  const float probe = * (const float *) data;
  // The restore-path slow job holds work() open long enough for the
  // inline job to overlap it unless the host serializes work() calls
  if (probe == kSlowJobProbe)
    {
      sleep_us (250000);
    }

  const float doubled = probe * 2.0f;
  respond (handle, sizeof (float), &doubled);

  g_work_in_progress.fetch_sub (1);
  return LV2_WORKER_SUCCESS;
}

static LV2_Worker_Status
work_response (LV2_Handle instance, uint32_t size, const void * data)
{
  (void) instance;

  if (size < sizeof (float))
    {
      return LV2_WORKER_ERR_UNKNOWN;
    }

  g_worker_last_response = * (const float *) data;
  ++g_worker_responses;
  return LV2_WORKER_SUCCESS;
}

static LV2_Worker_Status
float_window_end_run (LV2_Handle instance)
{
  (void) instance;
  ++g_float_window_end_runs;
  return LV2_WORKER_SUCCESS;
}

/* The two plugins share work() and work_response() and differ in
   end_run: the main plugin ships none (end_run is optional), the
   float-window twin counts its calls */
static const LV2_Worker_Interface main_worker_interface = {
  work, work_response, NULL
};
static const LV2_Worker_Interface float_window_worker_interface = {
  work, work_response, float_window_end_run
};

/* save() stores no properties: the host still records control port
   values, so state round trips keep their meaning */
static LV2_State_Status
save (LV2_Handle                  instance,
      LV2_State_Store_Function    store,
      LV2_State_Handle            handle,
      uint32_t                    flags,
      const LV2_Feature *const *  features)
{
  (void) instance;
  (void) store;
  (void) handle;
  (void) flags;
  (void) features;
  return LV2_STATE_SUCCESS;
}

/* set_state schedules a worker job through the schedule feature in
   the state feature array: a host that runs work synchronously during
   restore completes it before restore returns. When mixed restore is
   enabled, it also schedules through the cached feature (processed
   by the worker thread): the two jobs only overlap when the host
   fails to serialize work() calls */
static LV2_State_Status
set_state (LV2_Handle                   instance,
           LV2_State_Retrieve_Function  retrieve,
           LV2_State_Handle             handle,
           uint32_t                     flags,
           const LV2_Feature *const *   features)
{
  (void) retrieve;
  (void) handle;
  (void) flags;
  Amp * amp = (Amp *) instance;

  if (g_mixed_restore.load () && amp->schedule != NULL)
    {
      const float slow_probe = kSlowJobProbe;
      amp->schedule->schedule_work (
        amp->schedule->handle, sizeof (float), &slow_probe);

      // Wait (bounded) for the worker thread to enter work() so the
      // inline job below overlaps it deterministically
      for (int i = 0; i < 2000; ++i)
        {
          if (g_work_in_progress.load () != 0)
            break;
          sleep_us (1000);
        }
    }

  for (const LV2_Feature *const * f = features; f != NULL && *f != NULL; ++f)
    {
      if (strcmp ((*f)->URI, LV2_WORKER__schedule) == 0)
        {
          const LV2_Worker_Schedule * schedule =
            (const LV2_Worker_Schedule *) (*f)->data;
          const float restore_probe = 4.0f;
          schedule->schedule_work (
            schedule->handle, sizeof (float), &restore_probe);
        }
    }
  return LV2_STATE_SUCCESS;
}

static const LV2_State_Interface state_interface = { save, set_state };

static const void *
extension_data (const char * uri)
{
  if (strcmp (uri, LV2_WORKER__interface) == 0)
    {
      return &main_worker_interface;
    }
  if (strcmp (uri, LV2_STATE__interface) == 0)
    {
      return &state_interface;
    }
  return NULL;
}

static const void *
float_window_extension_data (const char * uri)
{
  if (strcmp (uri, LV2_WORKER__interface) == 0)
    {
      return &float_window_worker_interface;
    }
  return NULL;
}

/**
   Every plugin must define an `LV2_Descriptor`.  It is best to define
   descriptors statically to avoid leaking memory and non-portable shared
   library constructors and destructors to clean up properly.
*/
static const LV2_Descriptor descriptor = {
  AMP_URI, instantiate, connect_port, activate,
  run,     deactivate,  cleanup,      extension_data
};

static const LV2_Descriptor float_window_descriptor = {
  AMP_FLOAT_WINDOW_URI, instantiate, connect_port, activate,
  run,                   deactivate,  cleanup,      float_window_extension_data
};

/**
   The `lv2_descriptor()` function is the entry point to the plugin library.  The
   host will load the library and call this function repeatedly with increasing
   indices to find all the plugins defined in the library.  The index is not an
   identifier, the URI of the returned descriptor is used to determine the
   identify of the plugin.

   This method is in the ``discovery'' threading class, so no other functions
   or methods in this plugin library will be called concurrently with it.
*/
LV2_SYMBOL_EXPORT
const LV2_Descriptor *
lv2_descriptor (uint32_t index)
{
  switch (index)
    {
    case 0:
      return &descriptor;
    case 1:
      return &float_window_descriptor;
    default:
      return NULL;
    }
}
