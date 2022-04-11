// SPDX-FileCopyrightText: © 2020-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "audio/engine.h"
#include "audio/kmeter_dsp.h"
#include "audio/meter.h"
#include "audio/midi_event.h"
#include "audio/peak_dsp.h"
#include "audio/port.h"
#include "audio/track.h"
#include "audio/true_peak_dsp.h"
#include "project.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "zrythm_app.h"

#include "ext/zix/zix/ring.h"

/**
 * Get the current meter value.
 *
 * This should only be called once in a draw
 * cycle.
 */
void
meter_get_value (
  Meter *          self,
  AudioValueFormat format,
  float *          val,
  float *          max)
{
  Port * port = self->port;
  g_return_if_fail (IS_PORT_AND_NONNULL (port));

  /* get amplitude */
  float amp = -1.f;
  float max_amp = -1.f;
  if (
    port->id.type == TYPE_AUDIO
    || port->id.type == TYPE_CV)
    {
      g_return_if_fail (port->audio_ring);
      int    num_cycles = 4;
      size_t read_space_avail =
        zix_ring_read_space (port->audio_ring);
      size_t size =
        sizeof (float)
        * (size_t) AUDIO_ENGINE->block_length;
      size_t blocks_to_read =
        size == 0 ? 0 : read_space_avail / size;
      /* if no blocks available, skip */
      if (blocks_to_read == 0)
        {
          *val = 1e-20f;
          *max = 1e-20f;
          return;
        }

      float  buf[read_space_avail];
      size_t blocks_read = zix_ring_peek (
        port->audio_ring, &buf[0],
        read_space_avail);
      blocks_read /= size;
      num_cycles =
        MIN (num_cycles, (int) blocks_read);
      size_t start_index =
        (blocks_read - (size_t) num_cycles)
        * AUDIO_ENGINE->block_length;
      g_return_if_fail (IS_PORT_AND_NONNULL (port));
      if (blocks_read == 0)
        {
          g_message (
            "%s: blocks read for port %s is 0",
            __func__, port->id.label);
          *val = 1e-20f;
          *max = 1e-20f;
          return;
        }

      switch (self->algorithm)
        {
        case METER_ALGORITHM_RMS:
          /* not used */
          g_warn_if_reached ();
          amp = math_calculate_rms_amp (
            &buf[start_index],
            (size_t) num_cycles
              * AUDIO_ENGINE->block_length);
          break;
        case METER_ALGORITHM_TRUE_PEAK:
          true_peak_dsp_process (
            self->true_peak_processor,
            &port->buf[0],
            (int) AUDIO_ENGINE->block_length);
          amp = true_peak_dsp_read_f (
            self->true_peak_processor);
          break;
        case METER_ALGORITHM_K:
          kmeter_dsp_process (
            self->kmeter_processor, &port->buf[0],
            (int) AUDIO_ENGINE->block_length);
          kmeter_dsp_read (
            self->kmeter_processor, &amp, &max_amp);
          break;
        case METER_ALGORITHM_DIGITAL_PEAK:
          peak_dsp_process (
            self->peak_processor, &port->buf[0],
            (int) AUDIO_ENGINE->block_length);
          peak_dsp_read (
            self->peak_processor, &amp, &max_amp);
          break;
        default:
          break;
        }
    }
  else if (port->id.type == TYPE_EVENT)
    {
      bool on = false;
      if (port->write_ring_buffers)
        {
          MidiEvent event;
          while (
            zix_ring_peek (
              port->midi_ring, &event,
              sizeof (MidiEvent))
            > 0)
            {
              if (
                event.systime
                > self->last_midi_trigger_time)
                {
                  on = true;
                  self->last_midi_trigger_time =
                    event.systime;
                  break;
                }
            }
        }
      else
        {
          on =
            port->last_midi_event_time
            > self->last_midi_trigger_time;
          /*g_atomic_int_compare_and_exchange (*/
          /*&port->has_midi_events, 1, 0);*/
          if (on)
            {
              self->last_midi_trigger_time =
                port->last_midi_event_time;
              /*g_get_monotonic_time ();*/
            }
        }

      amp = on ? 2.f : 0.f;
      max_amp = amp;
    }

  /* adjust falloff */
  gint64 now = g_get_monotonic_time ();
  if (amp < self->last_amp)
    {
      /* calculate new value after falloff */
      float falloff =
        ((float) (now - self->last_draw_time)
         / 1000000.f)
        *
        /* rgareus says 13.3 is the standard */
        13.3f;

      /* use prev val plus falloff if higher than
       * current val */
      float prev_val_after_falloff =
        math_dbfs_to_amp (
          math_amp_to_dbfs (self->last_amp)
          - falloff);
      if (prev_val_after_falloff > amp)
        {
          amp = prev_val_after_falloff;
        }
    }

  /* if this is a peak value, set to current falloff
   * if peak is lower */
  if (max_amp < amp)
    max_amp = amp;

  /* remember vals */
  self->last_draw_time = now;
  self->last_amp = amp;
  self->prev_max = max_amp;

  switch (format)
    {
    case AUDIO_VALUE_AMPLITUDE:
      break;
    case AUDIO_VALUE_DBFS:
      *val = math_amp_to_dbfs (amp);
      *max = math_amp_to_dbfs (max_amp);
      break;
    case AUDIO_VALUE_FADER:
      *val = math_get_fader_val_from_amp (amp);
      *max = math_get_fader_val_from_amp (max_amp);
      break;
    default:
      break;
    }
}

Meter *
meter_new_for_port (Port * port)
{
  Meter * self = object_new (Meter);

  self->port = port;

  /* master */
  if (
    port->id.type == TYPE_AUDIO
    || port->id.type == TYPE_CV)
    {
      bool is_master_fader = false;
      if (port->id.owner_type == PORT_OWNER_TYPE_TRACK)
        {
          Track * track =
            port_get_track (port, true);
          if (track->type == TRACK_TYPE_MASTER)
            {
              is_master_fader = true;
            }
        }

      if (is_master_fader)
        {
          self->algorithm = METER_ALGORITHM_K;
          self->kmeter_processor =
            kmeter_dsp_new ();
          kmeter_dsp_init (
            self->kmeter_processor,
            AUDIO_ENGINE->sample_rate);
        }
      else
        {
          self->algorithm =
            METER_ALGORITHM_DIGITAL_PEAK;
          self->peak_processor = peak_dsp_new ();
          peak_dsp_init (
            self->peak_processor,
            AUDIO_ENGINE->sample_rate);
        }
    }
  else if (port->id.type == TYPE_EVENT)
    {
    }

  return self;
}

void
meter_free (Meter * self)
{
#define FREE_DSP(x, name) \
  if (self->x) \
    { \
      name##_free (self->x); \
    }

  FREE_DSP (true_peak_processor, true_peak_dsp);
  FREE_DSP (true_peak_max_processor, true_peak_dsp);
  FREE_DSP (kmeter_processor, kmeter_dsp);
  FREE_DSP (peak_processor, peak_dsp);

#undef FREE_DSP

  free (self);
}
