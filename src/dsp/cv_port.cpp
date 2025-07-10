// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/cv_port.h"
#include "utils/dsp.h"

namespace zrythm::dsp
{
CVPort::CVPort (utils::Utf8String label, PortFlow flow)
    : Port (std::move (label), PortType::CV, flow)
{
}

void
CVPort::clear_buffer (std::size_t block_length)
{
  utils::float_ranges::fill (buf_.data (), 0.f, block_length);
}

void
CVPort::process_block (const EngineProcessTimeInfo time_nfo)
{
  for (const auto &[src_port, conn] : port_sources_)
    {
      if (!conn->enabled_)
        continue;

      const float multiplier = conn->multiplier_;

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
    } /* foreach source */

  if (num_ring_buffer_readers_ > 0)
    {
      audio_ring_->force_write_multiple (
        &buf_[time_nfo.local_offset_], time_nfo.nframes_);
    }
}

void
init_from (CVPort &obj, const CVPort &other, utils::ObjectCloneType clone_type)

{
  init_from (
    static_cast<Port &> (obj), static_cast<const Port &> (other), clone_type);
}
} // namespace zrythm::dsp
