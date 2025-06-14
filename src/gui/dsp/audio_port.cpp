// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "engine/device_io/engine.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/dsp/audio_port.h"
#include "structure/tracks/channel_track.h"
#include "structure/tracks/master_track.h"
#include "structure/tracks/tracklist.h"
#include "utils/dsp.h"

#include <fmt/format.h>

AudioPort::AudioPort () : AudioPort ({}, PortFlow::Input) { }

AudioPort::AudioPort (utils::Utf8String label, PortFlow flow)
    : Port (label, PortType::Audio, flow, -1.f, 1.f, 0.f)
{
}

void
init_from (
  AudioPort             &obj,
  const AudioPort       &other,
  utils::ObjectCloneType clone_type)
{
  init_from (
    static_cast<Port &> (obj), static_cast<const Port &> (other), clone_type);
}

void
AudioPort::clear_buffer (std::size_t block_length)
{
  utils::float_ranges::fill (buf_.data (), 0.f, block_length);
}

bool
AudioPort::has_sound () const
{
  z_return_val_if_fail (
    this->buf_.size () >= AUDIO_ENGINE->get_block_length (), false);
  for (nframes_t i = 0; i < AUDIO_ENGINE->get_block_length (); i++)
    {
      if (fabsf (this->buf_[i]) > 0.0000001f)
        {
          return true;
        }
    }
  return false;
}

void
AudioPort::process_block (const EngineProcessTimeInfo time_nfo)
{
  if (id_->is_monitor_fader_stereo_in_or_out_port () && AUDIO_ENGINE->exporting_)
    {
      /* if exporting and the port is not a project port, skip
       * processing */
      return;
    }

  const auto &id = id_;
  const auto  owner_type = id->owner_type_;

  const bool is_stereo = is_stereo_port ();

  if (is_input () && owner_->should_sum_data_from_backend ())
    {
      // TODO
    }

  for (const auto &[_src_port, conn] : std::views::zip (srcs_, src_connections_))
    {
      if (!conn->enabled_)
        continue;

      const auto * src_port = dynamic_cast<const AudioPort *> (_src_port);
      const float  multiplier = conn->multiplier_;

      /* sum the signals */
      if (utils::math::floats_near (multiplier, 1.f, 0.00001f)) [[likely]]
        {
          utils::float_ranges::add2 (
            &buf_[time_nfo.local_offset_],
            &src_port->buf_[time_nfo.local_offset_], time_nfo.nframes_);
        }
      else
        {
          utils::float_ranges::mix_product (
            &buf_[time_nfo.local_offset_],
            &src_port->buf_[time_nfo.local_offset_], multiplier,
            time_nfo.nframes_);
        }

      if (owner_type == PortIdentifier::OwnerType::Fader)
        {
          constexpr float minf = -2.f;
          constexpr float maxf = 2.f;
          float           abs_peak = utils::float_ranges::abs_max (
            &buf_[time_nfo.local_offset_], time_nfo.nframes_);
          if (abs_peak > maxf)
            {
              /* this limiting wastes around 50% of port processing so only
               * do it on CV connections and faders if they exceed maxf */
              utils::float_ranges::clip (
                &buf_[time_nfo.local_offset_], minf, maxf, time_nfo.nframes_);
            }
        }
    }

  if (
    time_nfo.local_offset_ + time_nfo.nframes_
    == AUDIO_ENGINE->get_block_length ())
    {
      // z_debug ("writing to ring for {}", get_label ());
      audio_ring_->force_write_multiple (
        buf_.data (), AUDIO_ENGINE->get_block_length ());
    }

  /* if track output (to be shown on mixer) */
  if (
    owner_type == PortIdentifier::OwnerType::Channel && is_stereo
    && is_output ())
    {
      /* calculate meter values */
      /* reset peak if needed */
      auto time_now = Zrythm::getInstance ()->get_monotonic_time_usecs ();
      if (time_now - peak_timestamp_ > TIME_TO_RESET_PEAK)
        peak_ = -1.f;

      bool changed = utils::float_ranges::abs_max_with_existing_peak (
        &buf_[time_nfo.local_offset_], &peak_, time_nfo.nframes_);
      if (changed)
        {
          peak_timestamp_ = Zrythm::getInstance ()->get_monotonic_time_usecs ();
        }
    }

  /* if bouncing tracks directly to master (e.g., when bouncing the track on
   * its own without parents), clear master input */
  const auto master_processor_stereo_ins = std::pair (
    P_MASTER_TRACK->processor_->stereo_in_left_id_->id (),
    P_MASTER_TRACK->processor_->stereo_in_right_id_->id ());
  if (AUDIO_ENGINE->bounce_mode_ > engine::device_io::BounceMode::Off && !AUDIO_ENGINE->bounce_with_parents_ &&
    (get_uuid() == master_processor_stereo_ins.first
    || get_uuid() == master_processor_stereo_ins.second)) [[unlikely]]
    {
      utils::float_ranges::fill (
        &buf_[time_nfo.local_offset_], AUDIO_ENGINE->denormal_prevention_val_,
        time_nfo.nframes_);
    }

  /* if bouncing directly to master (e.g., when bouncing a track on
   * its own without parents), add the buffer to master output */
  if (
    AUDIO_ENGINE->bounce_mode_ > engine::device_io::BounceMode::Off && is_stereo
    && is_output ()
    && owner_->should_bounce_to_master (AUDIO_ENGINE->bounce_step_)) [[unlikely]]
    {
      auto &dest =
        ENUM_BITSET_TEST (id_->flags_, PortIdentifier::Flags::StereoL)
          ? P_MASTER_TRACK->channel_->get_stereo_out_ports ().first
          : P_MASTER_TRACK->channel_->get_stereo_out_ports ().second;
      utils::float_ranges::add2 (
        &dest.buf_[time_nfo.local_offset_], &this->buf_[time_nfo.local_offset_],
        time_nfo.nframes_);
    }
}

void
AudioPort::apply_pan (
  float                     pan,
  zrythm::dsp::PanLaw       pan_law,
  zrythm::dsp::PanAlgorithm pan_algo,
  nframes_t                 start_frame,
  const nframes_t           nframes)
{
  auto [calc_r, calc_l] = dsp::calculate_panning (pan_law, pan_algo, pan);

  /* if stereo R */
  if (ENUM_BITSET_TEST (id_->flags_, PortIdentifier::Flags::StereoR))
    {
      utils::float_ranges::mul_k2 (&buf_[start_frame], calc_r, nframes);
    }
  /* else if stereo L */
  else
    {
      utils::float_ranges::mul_k2 (&buf_[start_frame], calc_l, nframes);
    }
}

void
AudioPort::apply_fader (float amp, nframes_t start_frame, const nframes_t nframes)
{
  utils::float_ranges::mul_k2 (&buf_[start_frame], amp, nframes);
}

std::pair<PortUuidReference, PortUuidReference>
StereoPorts::create_stereo_ports (
  PortRegistry     &port_registry,
  bool              input,
  utils::Utf8String name,
  utils::Utf8String symbol)
{
  auto l_names = get_name_and_symbols (true, name, symbol);
  auto r_names = get_name_and_symbols (false, name, symbol);
  auto l_port_ref = port_registry.create_object<AudioPort> (
    l_names.first,
    input ? structure::tracks::PortFlow::Input
          : structure::tracks::PortFlow::Output);
  auto r_port_ref = port_registry.create_object<AudioPort> (
    r_names.first,
    input ? structure::tracks::PortFlow::Input
          : structure::tracks::PortFlow::Output);
  {
    auto * l_port = std::get<AudioPort *> (l_port_ref.get_object ());
    auto * r_port = std::get<AudioPort *> (r_port_ref.get_object ());
    l_port->id_->flags_ |= structure::tracks::PortIdentifier::Flags::StereoL;
    l_port->id_->sym_ = l_names.second;
    r_port->id_->flags_ |= structure::tracks::PortIdentifier::Flags::StereoR;
    r_port->id_->sym_ = r_names.second;
  }
  return std::make_pair (l_port_ref, r_port_ref);
}
