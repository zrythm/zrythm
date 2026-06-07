// SPDX-FileCopyrightText: © 2020-2022, 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>

#include "dsp/kmeter_dsp.h"
#include "dsp/midi_event.h"
#include "dsp/peak_dsp.h"
#include "dsp/port.h"
#include "dsp/port_observation_token.h"
#include "dsp/true_peak_dsp.h"
#include "gui/qquick/meter_processor.h"
#include "utils/math_utils.h"
#include "utils/qt.h"
#include "utils/types.h"

namespace zrythm::gui::qquick
{

struct MeterProcessor::Impl
{
  QPointer<dsp::Port> port_;

  std::optional<dsp::ObservationToken> observation_token_;

  std::unique_ptr<zrythm::dsp::TruePeakDsp> true_peak_processor_;
  std::unique_ptr<zrythm::dsp::TruePeakDsp> true_peak_max_processor_;

  float true_peak_ = 0.f;
  float true_peak_max_ = 0.f;

  std::unique_ptr<zrythm::dsp::KMeterDsp> kmeter_processor_;
  std::unique_ptr<zrythm::dsp::PeakDsp>   peak_processor_;

  MeterAlgorithm algorithm_ = MeterAlgorithm::Auto;

  float prev_max_ = 0.f;
  float last_amp_ = 0.f;

  SteadyTimePoint last_draw_time_;

  std::optional<dsp::RealtimeMidiEvent> last_seen_midi_event_;

  float current_amp_ = 0.f;
  float peak_amp_ = 0.f;

  int channel_{};
  int sample_rate_{};

  QPointer<dsp::PortObservationManager> observation_manager_;

  utils::QObjectUniquePtr<QTimer> timer_;
};

MeterProcessor::MeterProcessor (QObject * parent)
    : QObject (parent), impl_ (std::make_unique<Impl> ())
{
}

MeterProcessor::~MeterProcessor () = default;

dsp::Port *
MeterProcessor::port () const
{
  return impl_->port_;
}

int
MeterProcessor::channel () const
{
  return impl_->channel_;
}

int
MeterProcessor::sampleRate () const
{
  return impl_->sample_rate_;
}

dsp::PortObservationManager *
MeterProcessor::portObservationManager () const
{
  return impl_->observation_manager_;
}

MeterProcessor::MeterAlgorithm
MeterProcessor::algorithm () const
{
  return impl_->algorithm_;
}

void
MeterProcessor::setAlgorithm (MeterAlgorithm algo)
{
  if (impl_->algorithm_ == algo)
    return;
  impl_->algorithm_ = algo;

  impl_->kmeter_processor_.reset ();
  impl_->peak_processor_.reset ();
  impl_->true_peak_processor_.reset ();
  impl_->true_peak_max_processor_.reset ();

  switch (impl_->algorithm_)
    {
    case MeterAlgorithm::K:
      impl_->kmeter_processor_ = std::make_unique<zrythm::dsp::KMeterDsp> ();
      impl_->kmeter_processor_->init (impl_->sample_rate_);
      break;
    case MeterAlgorithm::TruePeak:
      impl_->true_peak_processor_ =
        std::make_unique<zrythm::dsp::TruePeakDsp> ();
      impl_->true_peak_processor_->init (impl_->sample_rate_);
      break;
    case MeterAlgorithm::RMS:
    case MeterAlgorithm::DigitalPeak:
      impl_->peak_processor_ = std::make_unique<zrythm::dsp::PeakDsp> ();
      impl_->peak_processor_->init (impl_->sample_rate_);
      break;
    case MeterAlgorithm::Auto:
      break;
    }

  Q_EMIT algorithmChanged ();
}

float
MeterProcessor::currentAmplitude () const
{
  return impl_->current_amp_;
}

float
MeterProcessor::peakAmplitude () const
{
  return impl_->peak_amp_;
}

void
MeterProcessor::setChannel (int channel)
{
  impl_->channel_ = channel;
}

void
MeterProcessor::setSampleRate (int rate)
{
  if (impl_->sample_rate_ == rate)
    return;
  impl_->sample_rate_ = rate;
  if (impl_->kmeter_processor_)
    impl_->kmeter_processor_->init (impl_->sample_rate_);
  if (impl_->peak_processor_)
    impl_->peak_processor_->init (impl_->sample_rate_);
  if (impl_->true_peak_processor_)
    impl_->true_peak_processor_->init (impl_->sample_rate_);
  Q_EMIT sampleRateChanged ();
}

void
MeterProcessor::setPortObservationManager (dsp::PortObservationManager * manager)
{
  impl_->observation_manager_ = manager;
  try_create_token ();
}

void
MeterProcessor::try_create_token ()
{
  if (impl_->port_ == nullptr || impl_->observation_manager_ == nullptr)
    return;
  impl_->observation_token_.emplace (
    *impl_->observation_manager_, *impl_->port_);
}

void
MeterProcessor::setPort (dsp::Port * port)
{
  if (impl_->port_ != nullptr)
    {
      impl_->observation_token_.reset ();
      QObject::disconnect (impl_->port_, &QObject::destroyed, this, nullptr);
      impl_->port_.clear ();
    }
  impl_->port_ = port;
  if (impl_->port_ != nullptr)
    {
      QObject::connect (impl_->port_, &QObject::destroyed, this, [this] () {
        impl_->observation_token_.reset ();
        impl_->port_.clear ();
      });

      z_trace ("setting port to {}", impl_->port_->get_label ());

      try_create_token ();

      if (impl_->algorithm_ == MeterAlgorithm::Auto)
        {
          if (impl_->port_->is_audio () || impl_->port_->is_cv ())
            setAlgorithm (MeterAlgorithm::DigitalPeak);
        }

      impl_->timer_ = utils::make_qobject_unique<QTimer> (this);
      impl_->timer_->setInterval (1000 / 60);
      connect (impl_->timer_.get (), &QTimer::timeout, this, [&] () {
        float val = 0.0;
        float max = 0.0;
        get_value (AudioValueFormat::Amplitude, val, max);

        if (!utils::math::floats_equal (impl_->current_amp_, val))
          {
            impl_->current_amp_ = val;
            Q_EMIT currentAmplitudeChanged (val);
          }
        if (!utils::math::floats_equal (impl_->peak_amp_, max))
          {
            impl_->peak_amp_ = max;
            Q_EMIT peakAmplitudeChanged (max);
          }
      });
      impl_->timer_->start ();
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
MeterProcessor::get_value (AudioValueFormat format, float &val, float &max)
{
  if (impl_->port_ == nullptr)
    {
      val = 1e-20f;
      max = 1e-20f;
      z_warning ("port not set");
      return;
    }

  float amp = -1.f;
  float max_amp = -1.f;

  if (!impl_->observation_token_)
    {
      val = 1e-20f;
      max = 1e-20f;
      return;
    }

  auto &cache = impl_->observation_token_->cache ();

  if (impl_->port_->is_audio ())
    {
      auto &channel_data = cache.audio[impl_->channel_];
      if (channel_data.empty ())
        {
          val = 1e-20f;
          max = 1e-20f;
          return;
        }

      auto   buf_sz = static_cast<int> (channel_data.size ());
      auto * buf = channel_data.data ();
      switch (impl_->algorithm_)
        {
        case MeterAlgorithm::RMS:
          z_warn_if_reached ();
          amp = utils::math::calculate_rms_amp (buf, buf_sz);
          break;
        case MeterAlgorithm::TruePeak:
          impl_->true_peak_processor_->process (buf, buf_sz);
          amp = impl_->true_peak_processor_->read_f ();
          break;
        case MeterAlgorithm::K:
          impl_->kmeter_processor_->process (buf, buf_sz);
          std::tie (amp, max_amp) = impl_->kmeter_processor_->read ();
          break;
        case MeterAlgorithm::DigitalPeak:
          impl_->peak_processor_->process (buf, buf_sz);
          std::tie (amp, max_amp) = impl_->peak_processor_->read ();
          break;
        default:
          break;
        }
      cache.clear_audio ();
    }
  else if (impl_->port_->is_cv ())
    {
      auto &channel_data = cache.audio[0];
      if (channel_data.empty ())
        {
          val = 1e-20f;
          max = 1e-20f;
          return;
        }

      auto buf_sz = static_cast<int> (channel_data.size ());
      impl_->peak_processor_->process (channel_data.data (), buf_sz);
      std::tie (amp, max_amp) = impl_->peak_processor_->read ();
      cache.clear_audio ();
    }
  else if (impl_->port_->is_midi ())
    {
      bool on = false;
      if (!cache.midi.empty ())
        {
          const auto &latest = cache.midi.back ();
          if (
            !impl_->last_seen_midi_event_.has_value ()
            || latest != *impl_->last_seen_midi_event_)
            {
              on = true;
              impl_->last_seen_midi_event_ = latest;
            }
        }

      amp = on ? 2.f : 0.f;
      max_amp = amp;
      cache.clear_midi ();
    }

  /* adjust falloff */
  auto now = SteadyClock::now ();
  if (amp < impl_->last_amp_)
    {
      float falloff =
        (static_cast<float> (
           std::chrono::duration_cast<std::chrono::milliseconds> (
             now - impl_->last_draw_time_)
             .count ())
         / 1000.f)
        *
        /* rgareus says 13.3 is the standard */
        13.3f;

      /* use prev val plus falloff if higher than current val */

      float prev_val_after_falloff = utils::math::dbfs_to_amp (
        utils::math::amp_to_dbfs (impl_->last_amp_) - falloff);
      amp = std::max (prev_val_after_falloff, amp);
    }

  /* if this is a peak value, set to current falloff if peak is lower */
  max_amp = std::max (max_amp, amp);

  /* remember vals */
  impl_->last_draw_time_ = now;
  impl_->last_amp_ = amp;
  impl_->prev_max_ = max_amp;

  switch (format)
    {
    case AudioValueFormat::Amplitude:
      val = amp;
      max = max_amp;
      break;
    case AudioValueFormat::DBFS:
      val = utils::math::amp_to_dbfs (amp);
      max = utils::math::amp_to_dbfs (max_amp);
      break;
    case AudioValueFormat::Fader:
      val = utils::math::get_fader_val_from_amp (amp);
      max = utils::math::get_fader_val_from_amp (max_amp);
      break;
    default:
      break;
    }
}
}
