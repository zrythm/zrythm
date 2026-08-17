// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "plugins/plugin_transport_context.h"

#include "unit/dsp/graph_helpers.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace zrythm::plugins
{

class PluginTransportContextTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    ON_CALL (transport_, get_play_state ())
      .WillByDefault (::testing::Return (dsp::ITransport::PlayState::Paused));
    ON_CALL (transport_, recording_enabled ())
      .WillByDefault (::testing::Return (false));
    ON_CALL (transport_, loop_enabled ())
      .WillByDefault (::testing::Return (false));
    ON_CALL (transport_, get_loop_range_positions ())
      .WillByDefault (
        ::testing::Return (
          std::make_pair (units::samples (0), units::samples (0))));
    ON_CALL (transport_, recording_preroll_frames_remaining ())
      .WillByDefault (::testing::Return (units::samples (0)));
  }

  PluginTransportContext build (units::sample_t position)
  {
    return build_plugin_transport_context (transport_, tempo_map_, position);
  }

  dsp::graph_test::MockTransport transport_;
  dsp::TempoMap                  tempo_map_{ units::sample_rate (48000) };
};

TEST_F (PluginTransportContextTest, PlayStateRecordingAndPrerollFlags)
{
  ON_CALL (transport_, get_play_state ())
    .WillByDefault (::testing::Return (dsp::ITransport::PlayState::Rolling));
  ON_CALL (transport_, recording_enabled ())
    .WillByDefault (::testing::Return (true));
  ON_CALL (transport_, recording_preroll_frames_remaining ())
    .WillByDefault (::testing::Return (units::samples (128)));

  const auto context = build (units::samples (0));
  EXPECT_TRUE (context.playing_);
  EXPECT_TRUE (context.recording_);
  EXPECT_TRUE (context.within_preroll_);

  ON_CALL (transport_, get_play_state ())
    .WillByDefault (::testing::Return (dsp::ITransport::PlayState::Paused));
  ON_CALL (transport_, recording_enabled ())
    .WillByDefault (::testing::Return (false));
  ON_CALL (transport_, recording_preroll_frames_remaining ())
    .WillByDefault (::testing::Return (units::samples (0)));

  const auto paused_context = build (units::samples (0));
  EXPECT_FALSE (paused_context.playing_);
  EXPECT_FALSE (paused_context.recording_);
  EXPECT_FALSE (paused_context.within_preroll_);
}

TEST_F (PluginTransportContextTest, MusicalPositionBarAndSeconds)
{
  // Default tempo map: 120 BPM, 4/4. 240000 samples at 48kHz = 5s = 10
  // quarter notes; bar 3 (0-based 2) starts at quarter 8
  const auto context = build (units::samples (240000));
  EXPECT_DOUBLE_EQ (context.position_.in (units::quarter_notes), 10.0);
  EXPECT_DOUBLE_EQ (context.bar_start_.in (units::quarter_notes), 8.0);
  EXPECT_EQ (context.bar_number_, 2);
  EXPECT_DOUBLE_EQ (context.tempo_.in (units::bpm), 120.0);
  EXPECT_EQ (context.time_sig_numerator_, 4);
  EXPECT_EQ (context.time_sig_denominator_, 4);
  EXPECT_DOUBLE_EQ (context.position_seconds_.in (units::seconds), 5.0);

  const auto start = build (units::samples (0));
  EXPECT_DOUBLE_EQ (start.position_.in (units::quarter_notes), 0.0);
  EXPECT_DOUBLE_EQ (start.bar_start_.in (units::quarter_notes), 0.0);
  EXPECT_EQ (start.bar_number_, 0);
}

TEST_F (PluginTransportContextTest, LoopRangeInQuartersAndSeconds)
{
  ON_CALL (transport_, loop_enabled ()).WillByDefault (::testing::Return (true));
  ON_CALL (transport_, get_loop_range_positions ())
    .WillByDefault (
      ::testing::Return (
        std::make_pair (units::samples (96000), units::samples (192000))));

  // 120 BPM: 96000..192000 samples at 48kHz = 2..4s = quarters 4..8
  const auto context = build (units::samples (0));
  EXPECT_TRUE (context.loop_enabled_);
  EXPECT_DOUBLE_EQ (context.loop_start_.in (units::quarter_notes), 4.0);
  EXPECT_DOUBLE_EQ (context.loop_end_.in (units::quarter_notes), 8.0);
  EXPECT_DOUBLE_EQ (context.loop_start_seconds_.in (units::seconds), 2.0);
  EXPECT_DOUBLE_EQ (context.loop_end_seconds_.in (units::seconds), 4.0);
}

TEST_F (PluginTransportContextTest, TempoChangeIsReflected)
{
  // 60 BPM starting at quarter 2 (1 second in at the initial 120 BPM)
  tempo_map_.add_tempo_event (
    units::ticks (1920), units::bpm (60.0), dsp::TempoMap::CurveType::Constant);

  EXPECT_DOUBLE_EQ (build (units::samples (0)).tempo_.in (units::bpm), 120.0);
  EXPECT_DOUBLE_EQ (build (units::samples (48000)).tempo_.in (units::bpm), 60.0);
}

TEST_F (PluginTransportContextTest, TimeSignatureChangeAffectsBars)
{
  // 3/8 starting at tick 3840 (bar 2 at the default 4/4). A 3/8 bar spans
  // 1.5 quarter notes, so bar 3 starts at tick 5280 = quarter 5.5 = 132000
  // samples at 120 BPM / 48kHz
  tempo_map_.add_time_signature_event (units::ticks (3840), 3, 8);

  const auto context = build (units::samples (132000));
  EXPECT_EQ (context.time_sig_numerator_, 3);
  EXPECT_EQ (context.time_sig_denominator_, 8);
  EXPECT_EQ (context.bar_number_, 2);
  EXPECT_DOUBLE_EQ (context.bar_start_.in (units::quarter_notes), 5.5);
}

} // namespace zrythm::plugins
