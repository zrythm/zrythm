// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/set_clip_loop_points_command.h"
#include "dsp/content_time_warp.h"
#include "dsp/tick_types.h"
#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/arranger_object_factory.h"
#include "utils/app_settings.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"
#include "utils/typed_uuid_reference.h"

#include "helpers/in_memory_settings_backend.h"

#include <gtest/gtest.h>

namespace zrythm::commands
{

class SetClipLoopPointsCommandTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
    tempo_map_wrapper = std::make_unique<dsp::TempoMapWrapper> (*tempo_map);

    app_settings = std::make_unique<utils::AppSettings> (
      std::make_unique<test_helpers::InMemorySettingsBackend> ());

    factory = std::make_unique<structure::arrangement::ArrangerObjectFactory> (
      structure::arrangement::ArrangerObjectFactory::Dependencies{
        .tempo_map_ = *tempo_map_wrapper,
        .registry_ = object_registry,
        .last_timeline_obj_len_provider_ = [] () { return 100.0; },
        .last_editor_obj_len_provider_ = [] () { return 50.0; },
        .automation_curve_algorithm_provider_ =
          [] () { return dsp::CurveOptions::Algorithm::Exponent; } },
      [] () { return units::sample_rate (44100); },
      [] () { return units::bpm (120.0); });

    midi_clip_ref =
      factory->get_builder<structure::arrangement::MidiClip> ()
        .build_in_registry ();
  }

  structure::arrangement::MidiClip * clip ()
  {
    return midi_clip_ref.get_object_as<structure::arrangement::MidiClip> ();
  }

  /// Build a TypedUuidReference<Clip> over the test clip for command
  /// construction.
  utils::TypedUuidReference<structure::arrangement::Clip> clip_ref ()
  {
    return utils::TypedUuidReference<structure::arrangement::Clip>{
      clip ()->get_uuid (), object_registry
    };
  }

  std::unique_ptr<dsp::TempoMap>        tempo_map;
  std::unique_ptr<dsp::TempoMapWrapper> tempo_map_wrapper;
  utils::ObjectRegistry                 object_registry;
  std::unique_ptr<utils::AppSettings>   app_settings;
  std::unique_ptr<structure::arrangement::ArrangerObjectFactory> factory;
  structure::arrangement::ArrangerObjectUuidReference            midi_clip_ref{
    object_registry
  };
};

// Basic redo applies the target positions; undo restores the originals.
TEST_F (SetClipLoopPointsCommandTest, RedoUndo)
{
  auto * c = clip ();
  c->length ()->setTicks (4000.0);
  c->set_loop_range (
    dsp::ContentTick{ units::ticks (100.0) },
    dsp::ContentTick{ units::ticks (200.0) },
    dsp::ContentTick{ units::ticks (3000.0) });
  ASSERT_FALSE (c->trackBounds ());

  SetClipLoopPointsCommand command (
    clip_ref (), dsp::ContentTick{ units::ticks (500.0) },
    dsp::ContentTick{ units::ticks (600.0) },
    dsp::ContentTick{ units::ticks (3500.0) });
  command.redo ();

  EXPECT_DOUBLE_EQ (c->clipStartPosition ()->ticks (), 500.0);
  EXPECT_DOUBLE_EQ (c->loopStartPosition ()->ticks (), 600.0);
  EXPECT_DOUBLE_EQ (c->loopEndPosition ()->ticks (), 3500.0);
  // Non-default range must leave tracking disabled.
  EXPECT_FALSE (c->trackBounds ());

  command.undo ();

  EXPECT_DOUBLE_EQ (c->clipStartPosition ()->ticks (), 100.0);
  EXPECT_DOUBLE_EQ (c->loopStartPosition ()->ticks (), 200.0);
  EXPECT_DOUBLE_EQ (c->loopEndPosition ()->ticks (), 3000.0);
  EXPECT_FALSE (c->trackBounds ());
}

// Setting the range back to the default (0, 0, length) re-engages tracking.
TEST_F (SetClipLoopPointsCommandTest, ReEngagesTrackBoundsOnDefault)
{
  auto * c = clip ();
  c->length ()->setTicks (4000.0);
  c->set_loop_range (
    dsp::ContentTick{ units::ticks (100.0) },
    dsp::ContentTick{ units::ticks (200.0) },
    dsp::ContentTick{ units::ticks (3000.0) });

  SetClipLoopPointsCommand command (
    clip_ref (), dsp::ContentTick{ units::ticks (0.0) },
    dsp::ContentTick{ units::ticks (0.0) },
    dsp::ContentTick{ units::ticks (4000.0) });
  command.redo ();

  EXPECT_DOUBLE_EQ (c->clipStartPosition ()->ticks (), 0.0);
  EXPECT_DOUBLE_EQ (c->loopStartPosition ()->ticks (), 0.0);
  EXPECT_DOUBLE_EQ (c->loopEndPosition ()->ticks (), 4000.0);
  EXPECT_TRUE (c->trackBounds ());

  // Undo restores the previous custom range and the disabled flag.
  command.undo ();

  EXPECT_DOUBLE_EQ (c->clipStartPosition ()->ticks (), 100.0);
  EXPECT_DOUBLE_EQ (c->loopStartPosition ()->ticks (), 200.0);
  EXPECT_DOUBLE_EQ (c->loopEndPosition ()->ticks (), 3000.0);
  EXPECT_FALSE (c->trackBounds ());
}

// A clip that was tracking its bounds gets tracking disabled by a non-default
// edit, and the flag is restored on undo.
TEST_F (SetClipLoopPointsCommandTest, DisablesTrackBoundsOnNonDefault)
{
  auto * c = clip ();
  c->length ()->setTicks (4000.0);
  // Freshly built clips track their bounds: positions are (0, 0, length).
  ASSERT_TRUE (c->trackBounds ());
  ASSERT_DOUBLE_EQ (c->loopEndPosition ()->ticks (), 4000.0);

  SetClipLoopPointsCommand command (
    clip_ref (), dsp::ContentTick{ units::ticks (0.0) },
    dsp::ContentTick{ units::ticks (500.0) },
    dsp::ContentTick{ units::ticks (3500.0) });
  command.redo ();

  EXPECT_DOUBLE_EQ (c->loopStartPosition ()->ticks (), 500.0);
  EXPECT_DOUBLE_EQ (c->loopEndPosition ()->ticks (), 3500.0);
  EXPECT_FALSE (c->trackBounds ());

  command.undo ();

  EXPECT_TRUE (c->trackBounds ());
  EXPECT_DOUBLE_EQ (c->loopStartPosition ()->ticks (), 0.0);
  EXPECT_DOUBLE_EQ (c->loopEndPosition ()->ticks (), 4000.0);
}

// Regression: expanding loop_start beyond the current loop_end must not be
// clamped by the per-position constraints. The command applies the target
// atomically via Clip::set_loop_range.
TEST_F (SetClipLoopPointsCommandTest, RedoDoesNotClampExpandingLoopStart)
{
  auto * c = clip ();
  c->length ()->setTicks (4000.0);
  c->set_loop_range (
    dsp::ContentTick{ units::ticks (0.0) },
    dsp::ContentTick{ units::ticks (0.0) },
    dsp::ContentTick{ units::ticks (100.0) });
  ASSERT_DOUBLE_EQ (c->loopEndPosition ()->ticks (), 100.0);

  SetClipLoopPointsCommand command (
    clip_ref (), dsp::ContentTick{ units::ticks (0.0) },
    dsp::ContentTick{ units::ticks (500.0) },
    dsp::ContentTick{ units::ticks (4000.0) });
  command.redo ();

  EXPECT_DOUBLE_EQ (c->clipStartPosition ()->ticks (), 0.0);
  EXPECT_DOUBLE_EQ (c->loopStartPosition ()->ticks (), 500.0);
  EXPECT_DOUBLE_EQ (c->loopEndPosition ()->ticks (), 4000.0);

  command.undo ();
  EXPECT_DOUBLE_EQ (c->loopStartPosition ()->ticks (), 0.0);
  EXPECT_DOUBLE_EQ (c->loopEndPosition ()->ticks (), 100.0);
}

// Command text is set for the undo stack display.
TEST_F (SetClipLoopPointsCommandTest, CommandText)
{
  SetClipLoopPointsCommand command (
    clip_ref (), dsp::ContentTick{ units::ticks (0.0) },
    dsp::ContentTick{ units::ticks (0.0) },
    dsp::ContentTick{ units::ticks (0.0) });
  EXPECT_EQ (command.text (), QString ("Change Clip Loop Points"));
}

} // namespace zrythm::commands
