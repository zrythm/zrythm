// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "engine/device_io/engine.h"
#include "gui/backend/backend/project.h"
#include "gui/dsp/cv_port.h"
#include "utils/dsp.h"
#include "utils/gtest_wrapper.h"

CVPort::CVPort () : CVPort ({}, {}) { }

CVPort::CVPort (utils::Utf8String label, PortFlow flow)
    : Port (label, PortType::CV, flow, -1.f, 1.f, 0.f)
{
}

void
CVPort::allocate_bufs ()
{
  audio_ring_ = std::make_unique<RingBuffer<float>> (AudioPort::AUDIO_RING_SIZE);

  size_t max = std::max (AUDIO_ENGINE->block_length_, 1u);
  buf_.resize (max);
  last_buf_sz_ = max;
}

void
CVPort::clear_buffer (std::size_t block_length)
{
  utils::float_ranges::fill (buf_.data (), 0.f, block_length);
}

void
CVPort::process_block (const EngineProcessTimeInfo time_nfo)
{
  for (const auto &[_src_port, conn] : std::views::zip (srcs_, src_connections_))
    {
      if (!conn->enabled_)
        continue;

      const auto * src_port = dynamic_cast<const CVPort *> (_src_port);
      const float  depth_range = (range_.maxf_ - range_.minf_) * 0.5f;
      const float  multiplier = depth_range * conn->multiplier_;

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

      float abs_peak = utils::float_ranges::abs_max (
        &buf_[time_nfo.local_offset_], time_nfo.nframes_);
      if (abs_peak > range_.maxf_)
        {
          /* this limiting wastes around 50% of port processing so only
           * do it on CV connections and faders if they exceed maxf */
          utils::float_ranges::clip (
            &buf_[time_nfo.local_offset_], range_.minf_, range_.maxf_,
            time_nfo.nframes_);
        }
    } /* foreach source */

  if (time_nfo.local_offset_ + time_nfo.nframes_ == AUDIO_ENGINE->block_length_)
    {
      audio_ring_->force_write_multiple (
        buf_.data (), AUDIO_ENGINE->block_length_);
    }
}

bool
CVPort::has_sound () const
{
  z_return_val_if_fail (buf_.size () >= AUDIO_ENGINE->block_length_, false);
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
CVPort::init_after_cloning (const CVPort &other, ObjectCloneType clone_type)

{
  Port::copy_members_from (other, clone_type);
}
