// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <ranges>

#include "dsp/fader.h"
#include "dsp/parameter.h"
#include "engine/session/control_room.h"

#include "helpers/scoped_juce_qapplication.h"

#include "unit/dsp/graph_helpers.h"
#include <gtest/gtest.h>

namespace zrythm::engine::session
{

class ControlRoomTest
    : public ::testing::Test,
      public test_helpers::ScopedJuceQApplication
{
protected:
  void SetUp () override
  {
    control_room_ = std::make_unique<ControlRoom> (
      [] () -> ControlRoom::RealtimeTracks { return {}; });
    mock_transport_ = std::make_unique<dsp::graph_test::MockTransport> ();
    tempo_map_ = std::make_unique<dsp::TempoMap> (units::sample_rate (48000.0));

    monitor_fader ().prepare_for_processing (
      nullptr, sample_rate_, max_block_length_);
    monitor_fader ().gain ()->setBaseValue (
      monitor_fader ().gain ()->range ().convertTo0To1 (1.0f));
  }

  void TearDown () override
  {
    if (control_room_ != nullptr)
      {
        monitor_fader ().release_resources ();
      }
  }

  dsp::Fader &monitor_fader () const { return *control_room_->monitorFader (); }

  /// Sets a constant input level, processes enough blocks for the gain
  /// smoother to settle, and returns the first output sample of the left
  /// channel.
  float settled_output_for_constant_input (float input_level)
  {
    auto &stereo_in = monitor_fader ().get_stereo_in_port ();
    auto &stereo_out = monitor_fader ().get_stereo_out_port ();

    const auto time_nfo = dsp::graph::ProcessBlockInfo::
      from_position_and_nframes (units::samples (0), block_length_);

    for (const auto _ : std::views::iota (0, 10))
      {
        for (
          const auto sample_index :
          std::views::iota (0, block_length_.in<int> (units::samples)))
          {
            stereo_in.buffers ()->setSample (0, sample_index, input_level);
            stereo_in.buffers ()->setSample (1, sample_index, input_level);
          }
        monitor_fader ().process_block (time_nfo, *mock_transport_, *tempo_map_);
      }

    return stereo_out.buffers ()->getSample (0, 0);
  }

  std::unique_ptr<ControlRoom>                    control_room_;
  std::unique_ptr<dsp::graph_test::MockTransport> mock_transport_;
  std::unique_ptr<dsp::TempoMap>                  tempo_map_;
  units::sample_rate_t sample_rate_{ units::sample_rate (48000) };
  units::sample_u32_t  max_block_length_{ units::samples (1024) };
  units::sample_u32_t  block_length_{ units::samples (512) };
};

TEST_F (ControlRoomTest, DimLevelAppliesConfiguredAmplitude)
{
  auto * dim_volume = control_room_->dimVolume ();
  dim_volume->setBaseValue (dim_volume->range ().convertTo0To1 (0.5f));
  control_room_->setDimOutput (true);

  EXPECT_NEAR (settled_output_for_constant_input (1.0f), 0.5f, 0.05f);
}

TEST_F (ControlRoomTest, MuteLevelAppliesConfiguredAmplitude)
{
  auto * mute_volume = control_room_->muteVolume ();
  mute_volume->setBaseValue (mute_volume->range ().convertTo0To1 (0.25f));
  monitor_fader ().mute ()->setBaseValue (1.0f);

  EXPECT_NEAR (settled_output_for_constant_input (1.0f), 0.25f, 0.05f);
}

TEST_F (ControlRoomTest, MonoToggleSumsBothChannels)
{
  monitor_fader ().monoToggle ()->setBaseValue (1.0f);

  auto &stereo_in = monitor_fader ().get_stereo_in_port ();
  auto &stereo_out = monitor_fader ().get_stereo_out_port ();

  const auto time_nfo = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), block_length_);

  for (const auto _ : std::views::iota (0, 10))
    {
      for (
        const auto sample_index :
        std::views::iota (0, block_length_.in<int> (units::samples)))
        {
          stereo_in.buffers ()->setSample (0, sample_index, 0.9f);
          stereo_in.buffers ()->setSample (1, sample_index, 0.1f);
        }
      monitor_fader ().process_block (time_nfo, *mock_transport_, *tempo_map_);
    }

  const auto left = stereo_out.buffers ()->getSample (0, 0);
  const auto right = stereo_out.buffers ()->getSample (1, 0);
  EXPECT_NEAR (left, right, 0.01f);
}

} // namespace zrythm::engine::session
