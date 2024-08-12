// SPDX-FileCopyrightText: © 2020-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/engine.h"
#include "dsp/kmeter_dsp.h"
#include "dsp/meter.h"
#include "dsp/midi_event.h"
#include "dsp/peak_dsp.h"
#include "dsp/port.h"
#include "dsp/track.h"
#include "dsp/true_peak_dsp.h"
#include "project.h"
#include "utils/math.h"
#include "utils/ring_buffer.h"
#include "zrythm.h"
#include "zrythm_app.h"

void
Meter::get_value (AudioValueFormat format, float * val, float * max)
{
  /* get amplitude */
  float amp = -1.f;
  float max_amp = -1.f;
  if (port_->is_audio () || port_->is_cv ())
    {
      size_t       read_space_avail = port_->audio_ring_->read_space ();
      const size_t block_length = AUDIO_ENGINE->block_length_;
      size_t       blocks_to_read =
        block_length == 0 ? 0 : read_space_avail / block_length;
      /* if no blocks available, skip */
      if (blocks_to_read == 0)
        {
          *val = 1e-20f;
          *max = 1e-20f;
          return;
        }

      z_return_if_fail (tmp_buf_.capacity () >= read_space_avail);
      tmp_buf_.resize (read_space_avail);
      auto samples_read =
        port_->audio_ring_->peek_multiple (tmp_buf_.data (), read_space_avail);
      auto   blocks_read = samples_read / block_length;
      auto   num_cycles = std::min (static_cast<size_t> (4), blocks_read);
      size_t start_index = (blocks_read - num_cycles) * block_length;
      if (blocks_read == 0)
        {
          z_debug ("blocks read for port {} is 0", port_->get_label ());
          *val = 1e-20f;
          *max = 1e-20f;
          return;
        }

      switch (algorithm_)
        {
        case MeterAlgorithm::METER_ALGORITHM_RMS:
          /* not used */
          z_warn_if_reached ();
          amp = math_calculate_rms_amp (
            &tmp_buf_[start_index], (size_t) num_cycles * block_length);
          break;
        case MeterAlgorithm::METER_ALGORITHM_TRUE_PEAK:
          true_peak_processor_->process (&port_->buf_[0], block_length);
          amp = true_peak_processor_->read_f ();
          break;
        case MeterAlgorithm::METER_ALGORITHM_K:
          kmeter_processor_->process (&port_->buf_[0], block_length);
          kmeter_processor_->read (&amp, &max_amp);
          break;
        case MeterAlgorithm::METER_ALGORITHM_DIGITAL_PEAK:
          peak_processor_->process (&port_->buf_[0], block_length);
          peak_processor_->read (&amp, &max_amp);
          break;
        default:
          break;
        }
    }
  else if (port_->is_event ())
    {
      bool on = false;
      auto port = static_cast<MidiPort *> (port_);
      if (port_->write_ring_buffers_)
        {
          MidiEvent event;
          while (port->midi_ring_->peek (event) > 0)
            {
              if (event.systime_ > last_midi_trigger_time_)
                {
                  on = true;
                  last_midi_trigger_time_ = event.systime_;
                  break;
                }
            }
        }
      else
        {
          on = port->last_midi_event_time_ > last_midi_trigger_time_;
          /*g_atomic_int_compare_and_exchange (&port->has_midi_events, 1, 0);*/
          if (on)
            {
              last_midi_trigger_time_ = port->last_midi_event_time_;
            }
        }

      amp = on ? 2.f : 0.f;
      max_amp = amp;
    }

  /* adjust falloff */
  auto now = SteadyClock::now ();
  if (amp < last_amp_)
    {
      /* calculate new value after falloff */
      float falloff =
        static_cast<float> (
          std::chrono::duration_cast<std::chrono::seconds> (now - last_draw_time_)
            .count ())
        *
        /* rgareus says 13.3 is the standard */
        13.3f;

      /* use prev val plus falloff if higher than current val */
      float prev_val_after_falloff =
        math_dbfs_to_amp (math_amp_to_dbfs (last_amp_) - falloff);
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
  last_draw_time_ = now;
  last_amp_ = amp;
  prev_max_ = max_amp;

  switch (format)
    {
    case AudioValueFormat::Amplitude:
      break;
    case AudioValueFormat::DBFS:
      *val = math_amp_to_dbfs (amp);
      *max = math_amp_to_dbfs (max_amp);
      break;
    case AudioValueFormat::Fader:
      *val = math_get_fader_val_from_amp (amp);
      *max = math_get_fader_val_from_amp (max_amp);
      break;
    default:
      break;
    }
}

Meter::Meter (Port &port)
{
  port_ = &port;

  /* master */
  if (port_->is_audio () || port_->is_cv ())
    {
      bool is_master_fader = false;
      if (port_->id_.owner_type_ == PortIdentifier::OwnerType::Track)
        {
          Track * track = port_->get_track (true);
          if (track->is_master ())
            {
              is_master_fader = true;
            }
        }

      if (is_master_fader)
        {
          algorithm_ = MeterAlgorithm::METER_ALGORITHM_K;
          kmeter_processor_ = std::make_unique<KMeterDsp> ();
          kmeter_processor_->init (AUDIO_ENGINE->sample_rate_);
        }
      else
        {
          algorithm_ = MeterAlgorithm::METER_ALGORITHM_DIGITAL_PEAK;
          peak_processor_ = std::make_unique<PeakDsp> ();
          peak_processor_->init (AUDIO_ENGINE->sample_rate_);
        }

      tmp_buf_.reserve (0x4000);
    }
  else if (port_->is_event ())
    {
    }
}