// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/graph_node.h"
#include "dsp/itransport.h"

#include <gmock/gmock.h>

namespace zrythm::dsp::graph_test
{
class MockProcessable : public zrythm::dsp::graph::IProcessable
{
public:
  MOCK_METHOD (utils::Utf8String, get_node_name, (), (const, override));
  MOCK_METHOD (nframes_t, get_single_playback_latency, (), (const, override));
  MOCK_METHOD (
    void,
    prepare_for_processing,
    (sample_rate_t, nframes_t),
    (override));
  MOCK_METHOD (
    void,
    process_block,
    (EngineProcessTimeInfo, const dsp::ITransport &),
    (noexcept, override));
  MOCK_METHOD (void, release_resources, (), (override));
};

class MockTransport : public zrythm::dsp::ITransport
{
public:
  MOCK_METHOD (
    (std::pair<units::sample_t, units::sample_t>),
    get_loop_range_positions,
    (),
    (const, noexcept, override));
  MOCK_METHOD (
    (std::pair<units::sample_t, units::sample_t>),
    get_punch_range_positions,
    (),
    (const, noexcept, override));
  MOCK_METHOD (PlayState, get_play_state, (), (const, noexcept, override));
  MOCK_METHOD (
    units::sample_t,
    get_playhead_position_in_audio_thread,
    (),
    (const, noexcept, override));
  MOCK_METHOD (bool, loop_enabled, (), (const, noexcept, override));
  MOCK_METHOD (bool, punch_enabled, (), (const, noexcept, override));
  MOCK_METHOD (bool, recording_enabled, (), (const, noexcept, override));
  MOCK_METHOD (
    units::sample_t,
    recording_preroll_frames_remaining,
    (),
    (const, noexcept, override));
  MOCK_METHOD (
    units::sample_t,
    metronome_countin_frames_remaining,
    (),
    (const, noexcept, override));
};
}
