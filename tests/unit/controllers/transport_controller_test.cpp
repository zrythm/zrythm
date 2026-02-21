// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "controllers/transport_controller.h"
#include "dsp/snap_grid.h"
#include "dsp/tempo_map.h"
#include "dsp/transport.h"

#include <gtest/gtest.h>

namespace zrythm::controllers
{

class TransportControllerTest : public ::testing::Test
{
protected:
  static constexpr auto SAMPLE_RATE = units::sample_rate (44100.0);

  void SetUp () override
  {
    tempo_map_ = std::make_unique<dsp::TempoMap> (SAMPLE_RATE);
    snap_grid_ = std::make_unique<dsp::SnapGrid> (
      *tempo_map_, utils::NoteLength::Note_1_4, [] () { return 0.0; });

    // Setup config provider with default values
    config_provider_ = {
      .return_to_cue_on_pause_ = [] () { return false; },
      .metronome_countin_bars_ = [] () { return 0; },
      .recording_preroll_bars_ = [] () { return 0; }
    };

    transport_ =
      std::make_unique<dsp::Transport> (*tempo_map_, config_provider_);
    transport_controller_ =
      std::make_unique<TransportController> (*transport_, *snap_grid_);
  }

  std::unique_ptr<dsp::TempoMap>       tempo_map_;
  std::unique_ptr<dsp::SnapGrid>       snap_grid_;
  std::unique_ptr<dsp::Transport>      transport_;
  std::unique_ptr<TransportController> transport_controller_;
  dsp::Transport::ConfigProvider       config_provider_;
};

// Test backward/forward movement
TEST_F (TransportControllerTest, BackwardForwardMovement)
{
  // Move to a position first
  const double start_ticks = 3840.0; // 4 beats
  transport_->move_playhead (units::ticks (start_ticks), true);

  // Test backward movement
  const double current_pos = transport_->playhead ()->ticks ();
  transport_controller_->moveBackward ();
  EXPECT_LT (transport_->playhead ()->ticks (), current_pos);

  // Test forward movement
  transport_controller_->moveForward ();
  EXPECT_GE (transport_->playhead ()->ticks (), current_pos);
}

// Test backward movement from beginning
TEST_F (TransportControllerTest, BackwardFromBeginning)
{
  // Start at position 0
  transport_->move_playhead (units::ticks (0.0), true);

  // Should not go negative
  transport_controller_->moveBackward ();
  EXPECT_GE (transport_->playhead ()->ticks (), 0.0);
}

// Test forward movement snaps to grid
TEST_F (TransportControllerTest, ForwardSnapsToGrid)
{
  // Move to a position that's not on the snap grid
  const double off_grid_ticks = 100.0; // Not on quarter note boundary
  transport_->move_playhead (units::ticks (off_grid_ticks), true);

  // Forward movement should snap to next grid point
  transport_controller_->moveForward ();
  EXPECT_GT (transport_->playhead ()->ticks (), off_grid_ticks);
}

// Test backward movement snaps to grid
TEST_F (TransportControllerTest, BackwardSnapsToGrid)
{
  // Move to a position that's not on the snap grid
  const double off_grid_ticks = 3850.0; // Not on quarter note boundary
  transport_->move_playhead (units::ticks (off_grid_ticks), true);

  // Backward movement should snap to previous grid point
  transport_controller_->moveBackward ();
  EXPECT_LT (transport_->playhead ()->ticks (), off_grid_ticks);
}

// Test repeated forward movement
TEST_F (TransportControllerTest, RepeatedForwardMovement)
{
  transport_->move_playhead (units::ticks (0.0), true);

  double prev_pos = transport_->playhead ()->ticks ();
  for (int i = 0; i < 5; ++i)
    {
      transport_controller_->moveForward ();
      EXPECT_GT (transport_->playhead ()->ticks (), prev_pos);
      prev_pos = transport_->playhead ()->ticks ();
    }
}

// Test repeated backward movement
TEST_F (TransportControllerTest, RepeatedBackwardMovement)
{
  // Start at a higher position
  const double start_ticks = 9600.0; // 10 beats
  transport_->move_playhead (units::ticks (start_ticks), true);

  double prev_pos = transport_->playhead ()->ticks ();
  for (int i = 0; i < 5; ++i)
    {
      transport_controller_->moveBackward ();
      EXPECT_LT (transport_->playhead ()->ticks (), prev_pos);
      prev_pos = transport_->playhead ()->ticks ();
    }
}

} // namespace zrythm::controllers
