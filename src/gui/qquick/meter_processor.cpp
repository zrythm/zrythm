// SPDX-FileCopyrightText: © 2020-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>

#include "dsp/audio_port.h"
#include "dsp/engine.h"
#include "dsp/kmeter_dsp.h"
#include "dsp/midi_event.h"
#include "dsp/midi_port.h"
#include "gui/qquick/meter_processor.h"
#include "utils/math.h"
#include "utils/ring_buffer.h"

namespace zrythm::gui::qquick
{
MeterProcessor::MeterProcessor (QObject * parent) : QObject (parent) { }

void
MeterProcessor::setChannel (int channel)
{
  channel_ = channel;
}

void
MeterProcessor::setPort (QVariant port_var)
{
  if (port_obj_)
    {
      ring_buffer_reader_.reset ();
      port_obj_.clear ();
    }
  port_obj_ = port_var.value<QObject *> ();
  if (port_obj_)
    {
      QObject::connect (port_obj_, &QObject::destroyed, this, [this] () {
        ring_buffer_reader_.reset ();
        port_obj_.clear ();
      });

      std::visit (
        [&] (auto &&port_) {
          z_trace ("setting port to {}", port_->get_label ());

          /* master */
          if (port_->is_audio () || port_->is_cv ())
            {
              bool is_master_fader =
                port_->get_uuid ()
                == P_MASTER_TRACK->channel ()->audioOutPort ()->get_uuid ();

              if (is_master_fader)
                {
                  algorithm_ = MeterAlgorithm::METER_ALGORITHM_K;
                  kmeter_processor_ =
                    std::make_unique<zrythm::dsp::KMeterDsp> ();
                  kmeter_processor_->init (
                    audio_engine_->get_sample_rate ().in (units::sample_rate));
                }
              else
                {
                  algorithm_ = MeterAlgorithm::METER_ALGORITHM_DIGITAL_PEAK;
                  peak_processor_ = std::make_unique<zrythm::dsp::PeakDsp> ();
                  peak_processor_->init (
                    audio_engine_->get_sample_rate ().in (units::sample_rate));
                }

              tmp_buf_.reserve (dsp::AudioPort::AUDIO_RING_SIZE);

              ring_buffer_reader_.emplace (*port_);
            }
          else if (port_->is_midi ())
            {
              ring_buffer_reader_.emplace (*port_);
            }
        },
        convert_to_variant<MeterPortPtrVariant> (port_obj_.get ()));

      auto * timer = new QTimer (this);
      timer->setInterval (1000 / 60); // 60 fps
      connect (timer, &QTimer::timeout, this, [&] () {
        float val = 0.0;
        float max = 0.0;
        get_value (AudioValueFormat::Amplitude, &val, &max);
        if (val > 1e-19 || max > 1e-19)
          {
            // z_debug ("values: {}, {}", val, max);
          }

        if (!utils::math::floats_equal (
              current_amp_.load (std::memory_order_relaxed), val))
          {
            current_amp_.store (val);
            Q_EMIT currentAmplitudeChanged (val);
          }
        if (!utils::math::floats_equal (
              peak_amp_.load (std::memory_order_relaxed), max))
          {
            peak_amp_.store (max);
            Q_EMIT peakAmplitudeChanged (max);
          }
      });
      timer->start ();
    }
}

float
MeterProcessor::toDBFS (const float amp) const
{
  return utils::math::amp_to_dbfs (amp);
}

float
MeterProcessor::toFader (const float amp) const
{
  return utils::math::get_fader_val_from_amp (amp);
}

void
MeterProcessor::get_value (AudioValueFormat format, float * val, float * max)
{
  if (!port_obj_)
    {
      *val = 1e-20f;
      *max = 1e-20f;
      z_warning ("port not set");
      return;
    }

  if (!audio_engine_->activated () || !audio_engine_->running ())
    {
      // z_warning ("engine not running");
      return;
    }

  std::visit (
    [&] (auto &&port) {
      using PortT = base_type<decltype (port)>;
      /* get amplitude */
      float amp = -1.f;
      float max_amp = -1.f;
      if constexpr (std::derived_from<PortT, dsp::AudioPort>)
        {
          const RingBuffer<float> * ring{};
          if constexpr (std::derived_from<PortT, dsp::AudioPort>)
            {
              if (port->audio_ring_buffers ().empty ())
                {
                  z_debug ("no audio ring buffers available");
                  return;
                }
              ring = &port->audio_ring_buffers ().at (channel_);
            }
          else if constexpr (std::derived_from<PortT, dsp::CVPort>)
            {
              if (port->cv_ring_ == nullptr)
                {
                  if (port->audio_ring_buffers ().empty ())
                    {
                      z_debug ("no CV ring buffer available");
                      return;
                    }
                }
              ring = port->cv_ring_.get ();
            }

          size_t       read_space_avail = ring->read_space ();
          const size_t block_length = audio_engine_->get_block_length ();
          size_t       blocks_to_read =
            block_length == 0 ? 0 : read_space_avail / block_length;
          /* if no blocks available, skip */
          if (blocks_to_read == 0)
            {
              *val = 1e-20f;
              *max = 1e-20f;
              z_trace ("no blocks to read for port {}", port->get_label ());
              return;
            }

          z_return_if_fail (tmp_buf_.capacity () >= read_space_avail);
          tmp_buf_.resize (read_space_avail);
          auto samples_read =
            ring->peek_multiple (tmp_buf_.data (), read_space_avail);
          auto   blocks_read = samples_read / block_length;
          auto   num_cycles = std::min (static_cast<size_t> (4), blocks_read);
          size_t start_index = (blocks_read - num_cycles) * block_length;
          if (blocks_read == 0)
            {
              z_debug ("blocks read for port {} is 0", port->get_label ());
              *val = 1e-20f;
              *max = 1e-20f;
              return;
            }

          // const float * port_buf = port->buffers()->getReadPointer(channel_);
          const float * buf = &tmp_buf_[start_index];
          const auto    buf_sz = static_cast<int> (num_cycles * block_length);
          switch (algorithm_)
            {
            case MeterAlgorithm::METER_ALGORITHM_RMS:
              /* not used */
              z_warn_if_reached ();
              amp = utils::math::calculate_rms_amp (buf, buf_sz);
              break;
            case MeterAlgorithm::METER_ALGORITHM_TRUE_PEAK:
              true_peak_processor_->process (const_cast<float *> (buf), buf_sz);
              amp = true_peak_processor_->read_f ();
              break;
            case MeterAlgorithm::METER_ALGORITHM_K:
              kmeter_processor_->process (buf, buf_sz);
              std::tie (amp, max_amp) = kmeter_processor_->read ();
              break;
            case MeterAlgorithm::METER_ALGORITHM_DIGITAL_PEAK:
              peak_processor_->process (const_cast<float *> (buf), buf_sz);
              std::tie (amp, max_amp) = peak_processor_->read ();
              break;
            default:
              break;
            }
        }
      else if constexpr (std::derived_from<PortT, dsp::MidiPort>)
        {
          bool on = false;
          tmp_events_.clear ();
          if (!port->midi_ring_)
            {
              z_debug ("no midi ring");
              return;
            }
          const auto events_read = port->midi_ring_->peek_multiple (
            tmp_events_.data (), decltype (tmp_events_)::capacity ());
          if (events_read > 0)
            {
              tmp_events_.resize (events_read);
              for (const auto &event : tmp_events_)
                {
                  if (event.systime_ > last_midi_trigger_time_)
                    {
                      on = true;
                      last_midi_trigger_time_ = event.systime_;
                      break;
                    }
                }
            }

          amp = on ? 2.f : 0.f;
          max_amp = amp;
        }

      // z_debug (
      // "port {} amp: {}, max amp: {}", port->get_full_designation (), amp,
      // max_amp);

      /* adjust falloff */
      auto now = SteadyClock::now ();
      if (amp < last_amp_)
        {
          /* calculate new value after falloff */
          float falloff =
            (static_cast<float> (
               std::chrono::duration_cast<std::chrono::milliseconds> (
                 now - last_draw_time_)
                 .count ())
             / 1000.f)
            *
            /* rgareus says 13.3 is the standard */
            13.3f;

          /* use prev val plus falloff if higher than current val */
          float prev_val_after_falloff = utils::math::dbfs_to_amp (
            utils::math::amp_to_dbfs (last_amp_) - falloff);
          amp = std::max (prev_val_after_falloff, amp);
        }

      /* if this is a peak value, set to current falloff if peak is lower */
      max_amp = std::max (max_amp, amp);

      /* remember vals */
      last_draw_time_ = now;
      last_amp_ = amp;
      prev_max_ = max_amp;

      switch (format)
        {
        case AudioValueFormat::Amplitude:
          *val = amp;
          *max = max_amp;
          break;
        case AudioValueFormat::DBFS:
          *val = utils::math::amp_to_dbfs (amp);
          *max = utils::math::amp_to_dbfs (max_amp);
          break;
        case AudioValueFormat::Fader:
          *val = utils::math::get_fader_val_from_amp (amp);
          *max = utils::math::get_fader_val_from_amp (max_amp);
          break;
        default:
          break;
        }
    },
    convert_to_variant<MeterPortPtrVariant> (port_obj_.get ()));
}
}
