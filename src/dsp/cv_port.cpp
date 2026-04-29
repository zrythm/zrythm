// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/cv_port.h"
#include "utils/float_ranges.h"
#include "utils/math_utils.h"

namespace zrythm::dsp
{
CVPort::CVPort (utils::Utf8String label, PortFlow flow)
    : Port (std::move (label), PortType::CV, flow)
{
}

void
CVPort::clear_buffer (std::size_t offset, std::size_t nframes)
{
  utils::float_ranges::fill (std::span (buf_).subspan (offset, nframes), 0.f);
}

void
CVPort::prepare_for_processing (
  const graph::GraphNode * node,
  units::sample_rate_t     sample_rate,
  units::sample_u32_t      max_block_length)
{
  if (node != nullptr)
    {
      auto source_ports =
        node->depends () | std::views::transform ([] (const auto &child_node) {
          return dynamic_cast<CVPort *> (&child_node.get ().get_processable ());
        })
        | std::views::filter ([] (const auto * port) { return port != nullptr; });
      set_port_sources (source_ports);
    }

  size_t max = std::max (max_block_length.in (units::samples), 1u);
  buf_.resize (max);

  // 8 cycles
  cv_ring_ = std::make_unique<RingBuffer<float>> (max * 8);
}

void
CVPort::release_resources ()
{
  buf_.clear ();
  cv_ring_.reset ();
}

void
CVPort::process_block (
  dsp::graph::EngineProcessTimeInfo time_nfo,
  const dsp::ITransport            &transport,
  const dsp::TempoMap              &tempo_map) noexcept
{
  const auto sub_offset = time_nfo.local_offset_.in (units::samples);
  const auto sub_nframes = time_nfo.nframes_.in (units::samples);
  for (const auto &[src_port, conn] : port_sources ())
    {
      if (!conn->enabled_)
        continue;

      const float multiplier = conn->multiplier_;

      /* sum the signals */
      if (utils::math::floats_near (multiplier, 1.f, 0.00001f)) [[likely]]
        {
          utils::float_ranges::add2 (
            std::span (buf_).subspan (sub_offset, sub_nframes),
            std::span (src_port->buf_).subspan (sub_offset, sub_nframes));
        }
      else
        {
          utils::float_ranges::mix_product (
            std::span (buf_).subspan (sub_offset, sub_nframes),
            std::span (src_port->buf_).subspan (sub_offset, sub_nframes),
            multiplier);
        }
    } /* foreach source */

  if (num_ring_buffer_readers_ > 0)
    {
      cv_ring_->force_write_multiple (
        &buf_[time_nfo.local_offset_.in (units::samples)],
        time_nfo.nframes_.in (units::samples));
    }
}

void
init_from (CVPort &obj, const CVPort &other, utils::ObjectCloneType clone_type)

{
  init_from (
    static_cast<Port &> (obj), static_cast<const Port &> (other), clone_type);
}
} // namespace zrythm::dsp
