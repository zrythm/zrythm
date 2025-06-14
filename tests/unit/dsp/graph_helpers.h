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
    (EngineProcessTimeInfo, std::span<const graph::IProcessable * const>),
    (override));
  MOCK_METHOD (void, release_resources, (), (override));
};

class MockTransport : public zrythm::dsp::ITransport
{
public:
  MOCK_METHOD (
    void,
    position_add_frames,
    (Position &, signed_frame_t),
    (const, override));
  MOCK_METHOD (
    (std::pair<Position, Position>),
    get_loop_range_positions,
    (),
    (const, override));
  MOCK_METHOD (PlayState, get_play_state, (), (const, override));
  MOCK_METHOD (Position, get_playhead_position, (), (const, override));
  MOCK_METHOD (bool, get_loop_enabled, (), (const, override));
  MOCK_METHOD (
    nframes_t,
    is_loop_point_met,
    (signed_frame_t g_start_frames, nframes_t nframes),
    (const, override));
};
}
