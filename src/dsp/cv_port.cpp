// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/cv_port.h"
#include "dsp/engine.h"
#include "project.h"
#include "utils/dsp.h"

void
CVPort::allocate_bufs ()
{
  size_t max = std::max (AUDIO_ENGINE->block_length_, 1u);
  buf_.resize (max);
  last_buf_sz_ = max;
}

void
CVPort::clear_buffer (AudioEngine &engine)
{
  dsp_fill (
    buf_.data (), DENORMAL_PREVENTION_VAL (&engine), engine.block_length_);
}

void
CVPort::process (const EngineProcessTimeInfo time_nfo, const bool noroll)
{
  if (noroll)
    {
      dsp_fill (
        &buf_.data ()[time_nfo.local_offset_],
        DENORMAL_PREVENTION_VAL (AUDIO_ENGINE), time_nfo.nframes_);
      return;
    }

  const auto &id = id_;
  const auto  owner_type = id.owner_type_;
  auto        track = [&] () -> Track * {
    if (owner_type == PortIdentifier::OwnerType::TrackProcessor
        || owner_type == PortIdentifier::OwnerType::Track
        || owner_type == PortIdentifier::OwnerType::Channel
        || (owner_type == PortIdentifier::OwnerType::Fader
            && (ENUM_BITSET_TEST (PortIdentifier::Flags2, id_.flags2_, PortIdentifier::Flags2::Prefader)
                || ENUM_BITSET_TEST (PortIdentifier::Flags2, id_.flags2_, PortIdentifier::Flags2::Postfader)))
        || (owner_type == PortIdentifier::OwnerType::Plugin && id_.plugin_id_.slot_type_ == PluginSlotType::Instrument))
      {
        return ZRYTHM_TESTING ? get_track (true) : track_;
      }
    return nullptr;
  }();
  z_return_if_fail (
    track || owner_type != PortIdentifier::OwnerType::TrackProcessor);

  for (size_t k = 0; k < srcs_.size (); k++)
    {
      auto        src_port = srcs_[k];
      const auto &conn = src_connections_[k];
      if (!conn->enabled_)
        continue;

      const float depth_range = (maxf_ - minf_) * 0.5f;
      const float multiplier = depth_range * conn->multiplier_;

      /* sum the signals */
      if (math_floats_equal_epsilon (multiplier, 1.f, 0.00001f)) [[likely]]
        {
          dsp_add2 (
            &buf_.data ()[time_nfo.local_offset_],
            &src_port->buf_.data ()[time_nfo.local_offset_], time_nfo.nframes_);
        }
      else
        {
          dsp_mix2 (
            &buf_.data ()[time_nfo.local_offset_],
            &src_port->buf_.data ()[time_nfo.local_offset_], 1.f, multiplier,
            time_nfo.nframes_);
        }

      float abs_peak =
        dsp_abs_max (&buf_.data ()[time_nfo.local_offset_], time_nfo.nframes_);
      if (abs_peak > maxf_)
        {
          /* this limiting wastes around 50% of port processing so only
           * do it on CV connections and faders if they exceed maxf */
          dsp_limit1 (
            &buf_.data ()[time_nfo.local_offset_], minf_, maxf_,
            time_nfo.nframes_);
        }
    } /* foreach source */

  if (time_nfo.local_offset_ + time_nfo.nframes_ == AUDIO_ENGINE->block_length_)
    {
      audio_ring_->force_write_multiple (
        &buf_.data ()[0], AUDIO_ENGINE->block_length_);
    }
}

bool
CVPort::has_sound () const
{
  g_return_val_if_fail (buf_.size () >= AUDIO_ENGINE->block_length_, false);
  for (nframes_t i = 0; i < AUDIO_ENGINE->block_length_; i++)
    {
      if (fabsf (buf_[i]) > 0.0000001f)
        {
          return true;
        }
    }
  return false;
}

void
CVPort::init_after_cloning (const CVPort &other)
{
  Port::copy_members_from (other);
}