// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <vector>

#include "utils/float_ranges.h"
#include "utils/units.h"

namespace zrythm::controllers
{

struct RecordingAudioPacket
{
  units::sample_t     timeline_position;
  bool                transport_recording{};
  units::sample_u32_t nframes;
  std::vector<float>  l_frames;
  std::vector<float>  r_frames;

  static void write_to_slot (
    RecordingAudioPacket  &slot,
    units::sample_t        timeline_position,
    bool                   transport_recording,
    std::span<const float> l_data,
    std::span<const float> r_data) noexcept [[clang::nonblocking]]
  {
    slot.timeline_position = timeline_position;
    slot.transport_recording = transport_recording;
    slot.nframes = units::samples (l_data.size ());
    utils::float_ranges::copy (
      std::span (slot.l_frames).subspan (0, l_data.size ()), l_data);
    utils::float_ranges::copy (
      std::span (slot.r_frames).subspan (0, r_data.size ()), r_data);
  }

  static void
  copy_from (RecordingAudioPacket &slot, const RecordingAudioPacket &source)
    [[clang::blocking]]
  {
    slot.timeline_position = source.timeline_position;
    slot.transport_recording = source.transport_recording;
    slot.nframes = source.nframes;
    const auto n = source.nframes.in<size_t> (units::samples);
    slot.l_frames.assign (
      source.l_frames.begin (), source.l_frames.begin () + n);
    slot.r_frames.assign (
      source.r_frames.begin (), source.r_frames.begin () + n);
  }

  static void
  resize (RecordingAudioPacket &slot, units::sample_u32_t block_length) noexcept
  {
    const auto new_size = block_length.in (units::samples);
    slot.l_frames.resize (new_size);
    slot.r_frames.resize (new_size);
  }
};

}
