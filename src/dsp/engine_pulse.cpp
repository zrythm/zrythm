// SPDX-FileCopyrightText: © 2021-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: © 2020 Ryan Gonzalez <rymg19 at gmail dot com>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#ifdef HAVE_PULSEAUDIO

#  include "dsp/channel.h"
#  include "dsp/engine.h"
#  include "dsp/engine_pulse.h"
#  include "dsp/master_track.h"
#  include "dsp/port.h"
#  include "dsp/router.h"
#  include "dsp/tempo_track.h"
#  include "dsp/tracklist.h"
#  include "dsp/transport.h"
#  include "gui/widgets/main_window.h"
#  include "project.h"
#  include "settings/g_settings_manager.h"
#  include "settings/settings.h"
#  include "utils/ui.h"
#  include "zrythm_app.h"

#  include <glib/gi18n.h>
#  include <gtk/gtk.h>

#  include <pulse/def.h>
#  include <pulse/sample.h>
#  include <pulse/stream.h>

#  define ZRYTHM_PULSE_CLIENT "Zrythm"

#  define FRAMES_TO_BYTES(frames) (((size_t) frames) * sizeof (float) * 2)

#  define BYTES_TO_FRAMES(bytes) ((bytes) / sizeof (float) / 2)

static void
engine_pulse_stream_write_callback (
  pa_stream * stream,
  size_t      bytes,
  void *      userdata)
{
  AudioEngine * self = static_cast<AudioEngine *> (userdata);

  void *   buf;
  gboolean should_free_buffer = FALSE;
  bytes = MIN (bytes, FRAMES_TO_BYTES (self->block_length));
  if (pa_stream_begin_write (stream, &buf, &bytes) != 0)
    {
      g_warning ("Failed to acquire pulse write buffer");

      buf = g_malloc0 (bytes);
      should_free_buffer = TRUE;
    }

  nframes_t num_frames = BYTES_TO_FRAMES (bytes);
  engine_process (self, num_frames);

  memset (buf, 0, (size_t) bytes);
  float * float_buf = (float *) buf;

  for (nframes_t i = 0; i < num_frames; i++)
    {
      float_buf[i * 2] = self->monitor_out->get_l ().buf_[i];
      float_buf[i * 2 + 1] = self->monitor_out->get_r ().buf_[i];
    }

  if (
    pa_stream_write (
      stream, buf, bytes, should_free_buffer ? g_free : NULL, 0, PA_SEEK_RELATIVE)
    != 0)
    {
      g_warning ("Failed to write pulse buffer");
      if (should_free_buffer)
        g_free (buf);
    }
}

static int
engine_pulse_notify_underflow (void * userdata)
{
  if (MAIN_WINDOW)
    {
      ui_show_error_message (
        _ ("Buffer Underflow"),
        _ ("A buffer underflow has occurred. Try increasing the buffer size in the settings to avoid audio glitches."));
    }

  return G_SOURCE_REMOVE;
}

static void
engine_pulse_stream_underflow_callback (pa_stream * stream, void * userdata)
{
  AudioEngine * self = (AudioEngine *) userdata;

  if (self->pulse_notified_underflow)
    return;

  g_message ("Pulse buffer underflow");

  if (ZRYTHM_HAVE_UI && MAIN_WINDOW)
    g_idle_add (engine_pulse_notify_underflow, NULL);

  self->pulse_notified_underflow = TRUE;
}

static void
engine_pulse_stream_state_callback (pa_stream * stream, void * userdata)
{
  pa_threaded_mainloop * mainloop = (pa_threaded_mainloop *) userdata;
  pa_context *           pulse = pa_stream_get_context (stream);
  pa_stream_state_t      state = pa_stream_get_state (stream);

  switch (state)
    {
    case PA_STREAM_READY:
      g_message ("Pulse stream ready");
      break;
    case PA_STREAM_FAILED:
      /* FIXME handle gracefully */
      g_critical (
        "Error in pulse stream: %s", pa_strerror (pa_context_errno (pulse)));
      break;
    case PA_STREAM_CREATING:
      g_message ("Creating pulse stream");
      break;
    case PA_STREAM_TERMINATED:
      g_message ("Pulse stream terminated");
      break;
    case PA_STREAM_UNCONNECTED:
      break;
    }

  if (
    mainloop
    && (state == PA_STREAM_READY || state == PA_STREAM_FAILED || state == PA_STREAM_TERMINATED))
    pa_threaded_mainloop_signal (mainloop, 0);
}

static void
engine_pulse_context_state_callback (pa_context * pulse, void * userdata)
{
  pa_threaded_mainloop * mainloop = (pa_threaded_mainloop *) userdata;
  pa_context_state_t     state = pa_context_get_state (pulse);

  switch (state)
    {
    case PA_CONTEXT_READY:
      g_message ("Pulse context ready");
      break;
    case PA_CONTEXT_FAILED:
      g_warning (
        "Error in pulse context: %s", pa_strerror (pa_context_errno (pulse)));
      break;
    case PA_CONTEXT_CONNECTING:
      g_message ("Pulse context connecting");
      break;
    case PA_CONTEXT_AUTHORIZING:
      g_message ("Pulse context authorizing");
      break;
    case PA_CONTEXT_SETTING_NAME:
      g_message ("Pulse context setting name");
      break;
    case PA_CONTEXT_TERMINATED:
      g_message ("Pulse context terminated");
      break;
    case PA_CONTEXT_UNCONNECTED:
      break;
    }

  if (
    mainloop
    && (state == PA_CONTEXT_READY || state == PA_CONTEXT_FAILED || state == PA_CONTEXT_TERMINATED))
    pa_threaded_mainloop_signal (mainloop, 0);
}

static gboolean
engine_pulse_try_lock_connect_sync (
  pa_threaded_mainloop ** mainloop,
  pa_context **           context,
  char **                 msg)
{
  *mainloop = pa_threaded_mainloop_new ();
  if (pa_threaded_mainloop_start (*mainloop) != 0)
    {
      *msg = g_strdup (_ (
        "PulseAudio error: "
        "Failed to create main loop"));
      return FALSE;
    }

  *context = pa_context_new (
    pa_threaded_mainloop_get_api (*mainloop), ZRYTHM_PULSE_CLIENT);

  pa_context_set_state_callback (
    *context, engine_pulse_context_state_callback, *mainloop);

  pa_threaded_mainloop_lock (*mainloop);

  if (pa_context_connect (*context, NULL, PA_CONTEXT_NOAUTOSPAWN, NULL) != 0)
    {
      *msg = g_strdup_printf (
        _ ("PulseAudio Error: %s"), pa_strerror (pa_context_errno (*context)));
      pa_threaded_mainloop_unlock (*mainloop);
      return FALSE;
    }

  for (;;)
    {
      pa_threaded_mainloop_wait (*mainloop);

      pa_context_state_t state = pa_context_get_state (*context);
      if (state == PA_CONTEXT_READY)
        break;
      else
        {
          *msg = g_strdup (_ (
            "PulseAudio Error: "
            "Failed to connect"));
          pa_threaded_mainloop_unlock (*mainloop);
          return FALSE;
        }
    }

  pa_context_set_state_callback (
    *context, engine_pulse_context_state_callback, NULL);

  return TRUE;
}

/**
 * Set up PulseAudio.
 */
int
engine_pulse_setup (AudioEngine * self)
{
  char * msg = NULL;
  if (!engine_pulse_try_lock_connect_sync (
        &self->pulse_mainloop, &self->pulse_context, &msg))
    {
      g_warning ("%s", msg);
      g_free (msg);
      return 1;
    }

  pa_sample_spec requested_spec;
  requested_spec.channels = 2;
  requested_spec.format = PA_SAMPLE_FLOAT32;
  requested_spec.rate = (uint32_t) engine_samplerate_enum_to_int (
    (AudioEngineSamplerate) g_settings_get_enum (
      S_P_GENERAL_ENGINE, "sample-rate"));

  self->pulse_stream = pa_stream_new (
    self->pulse_context, ZRYTHM_PULSE_CLIENT, &requested_spec, NULL);
  if (self->pulse_stream == NULL)
    {
      g_warning (
        "PulseAudio Error: "
        "Failed to create stream");
      return 1;
    }

  /* Settings based on:
   * https://www.spinics.net/lists/pulse-audio/msg01689.html
   */
  pa_buffer_attr requested_attr;
  requested_attr.fragsize = (uint32_t) -1;
  requested_attr.prebuf = (uint32_t) -1;
  requested_attr.tlength = FRAMES_TO_BYTES (engine_buffer_size_enum_to_int (
    (AudioEngineBufferSize) g_settings_get_enum (
      S_P_GENERAL_ENGINE, "buffer-size")));
  requested_attr.maxlength = requested_attr.tlength;
  requested_attr.minreq = requested_attr.tlength / 2;

  pa_stream_set_state_callback (
    self->pulse_stream, engine_pulse_stream_state_callback,
    self->pulse_mainloop);

  pa_stream_set_underflow_callback (
    self->pulse_stream, engine_pulse_stream_underflow_callback, self);

  if (
    pa_stream_connect_playback (
      self->pulse_stream, NULL, &requested_attr,
      PA_STREAM_START_CORKED | PA_STREAM_ADJUST_LATENCY, NULL, NULL)
    != 0)
    {
      g_warning (
        "PulseAudio Error: "
        "Failed to connect playback stream: %s",
        pa_strerror (pa_context_errno (self->pulse_context)));
      return 1;
    }

  for (;;)
    {
      pa_threaded_mainloop_wait (self->pulse_mainloop);

      pa_stream_state_t state = pa_stream_get_state (self->pulse_stream);
      if (state == PA_STREAM_READY)
        break;
      else if (state == PA_STREAM_FAILED)
        {
          g_warning (
            "PulseAudio Error: "
            "Failed to connect to stream");
          pa_threaded_mainloop_unlock (self->pulse_mainloop);
          return 1;
        }
    }

  pa_stream_set_state_callback (
    self->pulse_stream, engine_pulse_stream_state_callback, NULL);

  const pa_sample_spec * actual_spec =
    pa_stream_get_sample_spec (self->pulse_stream);

  self->sample_rate = actual_spec->rate;
  if (P_TEMPO_TRACK)
    {
      int beats_per_bar = tempo_track_get_beats_per_bar (P_TEMPO_TRACK);
      engine_update_frames_per_tick (
        self, beats_per_bar, tempo_track_get_current_bpm (P_TEMPO_TRACK),
        self->sample_rate, true, true, false);
    }

  const pa_buffer_attr * actual_attr =
    pa_stream_get_buffer_attr (self->pulse_stream);
  self->block_length = BYTES_TO_FRAMES (actual_attr->tlength);

  pa_threaded_mainloop_unlock (self->pulse_mainloop);

  return 0;
}

static void
engine_pulse_stream_cork_callback (
  pa_stream * stream,
  int         success,
  void *      userdata)
{
  pa_threaded_mainloop * mainloop = (pa_threaded_mainloop *) userdata;

  if (!success)
    g_critical ("Failed to cork/uncork stream");

  pa_threaded_mainloop_signal (mainloop, 0);
}

void
engine_pulse_activate (AudioEngine * self, gboolean activate)
{
  if (activate)
    {
      g_message ("activating...");
    }
  else
    {
      g_message ("deactivating...");
    }

  pa_threaded_mainloop_lock (self->pulse_mainloop);

  pa_operation * op = pa_stream_cork (
    self->pulse_stream, !activate, engine_pulse_stream_cork_callback,
    self->pulse_mainloop);

  while (pa_operation_get_state (op) == PA_OPERATION_RUNNING)
    pa_threaded_mainloop_wait (self->pulse_mainloop);

  if (activate)
    {
      /* The write callback is attached late,
       * because otherwise it will be called on
       * stream connect before any of the buffers
       * are set up.
       * https://lists.freedesktop.org/archives/pulseaudio-discuss/2016-July/026552.html
       */
      pa_stream_set_write_callback (
        self->pulse_stream, engine_pulse_stream_write_callback, self);

      size_t bytes = pa_stream_writable_size (self->pulse_stream);
      engine_pulse_stream_write_callback (self->pulse_stream, bytes, self);
    }

  pa_threaded_mainloop_unlock (self->pulse_mainloop);
}

/**
 * Tests if PulseAudio is working properly.
 *
 * Returns 0 if ok, non-null if has errors.
 *
 * If win is not null, it displays error messages
 * to it.
 */
int
engine_pulse_test (GtkWindow * win)
{
  char * msg = NULL;

  pa_context *           context = NULL;
  pa_threaded_mainloop * mainloop = NULL;

  int result = 0;

  if (!engine_pulse_try_lock_connect_sync (&mainloop, &context, &msg))
    {
      if (win)
        {
          ui_show_message_full (
            GTK_WIDGET (win), _ ("Pulseaudio Backend Test Failed"), "%s", msg);
        }
      else
        {
          g_message ("%s", msg);
        }
      g_free (msg);
      result = 1;
    }
  else
    {
      pa_threaded_mainloop_unlock (mainloop);
    }

  if (context != NULL)
    pa_context_unref (context);

  if (mainloop != NULL)
    {
      pa_threaded_mainloop_stop (mainloop);
      pa_threaded_mainloop_free (mainloop);
    }

  return result;
}

/**
 * Closes PulseAudio.
 */
void
engine_pulse_tear_down (AudioEngine * engine)
{
  if (engine->pulse_stream != NULL)
    pa_stream_unref (engine->pulse_stream);

  if (engine->pulse_context != NULL)
    pa_context_unref (engine->pulse_context);

  if (engine->pulse_mainloop != NULL)
    {
      pa_threaded_mainloop_stop (engine->pulse_mainloop);
      pa_threaded_mainloop_free (engine->pulse_mainloop);
    }
}

#endif // HAVE_PULSEAUDIO
