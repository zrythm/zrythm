// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/clip_operator.h"
#include "dsp/tick_types.h"
#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/arranger_object_factory.h"
#include "undo/undo_stack.h"
#include "utils/app_settings.h"
#include "utils/object_registry.h"

#include "helpers/in_memory_settings_backend.h"

#include "unit/actions/mock_undo_stack.h"
#include <gtest/gtest.h>

namespace zrythm::actions
{

class ClipOperatorTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
    tempo_map_wrapper = std::make_unique<dsp::TempoMapWrapper> (*tempo_map);

    app_settings = std::make_unique<utils::AppSettings> (
      std::make_unique<test_helpers::InMemorySettingsBackend> (),
      utils::UserDirectories{});

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

    undo_stack = create_mock_undo_stack ();
    clip_operator =
      std::make_unique<ClipOperator> (object_registry, *undo_stack, nullptr);
    clip_operator->setClip (clip ());
  }

  structure::arrangement::MidiClip * clip ()
  {
    return midi_clip_ref.get_object_as<structure::arrangement::MidiClip> ();
  }

  std::unique_ptr<dsp::TempoMap>        tempo_map;
  std::unique_ptr<dsp::TempoMapWrapper> tempo_map_wrapper;
  utils::ObjectRegistry                 object_registry;
  std::unique_ptr<utils::AppSettings>   app_settings;
  std::unique_ptr<structure::arrangement::ArrangerObjectFactory> factory;
  structure::arrangement::ArrangerObjectUuidReference            midi_clip_ref{
    object_registry
  };
  std::unique_ptr<undo::UndoStack> undo_stack;
  std::unique_ptr<ClipOperator>    clip_operator;
};

// A press-without-drag (begin, no update, end) pushes nothing.
TEST_F (ClipOperatorTest, BeginWithoutUpdateEndPushesNothing)
{
  clip ()->length ()->setTicks (4000.0);
  ASSERT_EQ (undo_stack->count (), 0);

  clip_operator->beginClipLoopPointsDrag ();
  clip_operator->endClipLoopPointsDrag ();

  EXPECT_EQ (undo_stack->count (), 0);
}

// Live updates mutate the clip directly during the gesture (visual feedback),
// and a whole drag collapses to a single undo entry.
TEST_F (ClipOperatorTest, LivePreviewAndSingleUndoEntry)
{
  auto * c = clip ();
  c->length ()->setTicks (4000.0);
  ASSERT_TRUE (c->trackBounds ()); // before: (0, 0, 4000), tracking on

  clip_operator->beginClipLoopPointsDrag ();

  clip_operator->updateClipLoopPointsDrag (100.0, 0.0, 4000.0);
  // Live preview: the clip already reflects the target before end().
  EXPECT_DOUBLE_EQ (c->clipStartPosition ()->ticks (), 100.0);

  clip_operator->updateClipLoopPointsDrag (200.0, 0.0, 4000.0);
  EXPECT_DOUBLE_EQ (c->clipStartPosition ()->ticks (), 200.0);

  EXPECT_EQ (undo_stack->count (), 0);
  clip_operator->endClipLoopPointsDrag ();

  EXPECT_EQ (undo_stack->count (), 1);
  EXPECT_DOUBLE_EQ (c->clipStartPosition ()->ticks (), 200.0);

  // Undo through the stack restores the pre-drag state.
  undo_stack->undo ();
  EXPECT_DOUBLE_EQ (c->clipStartPosition ()->ticks (), 0.0);
  EXPECT_TRUE (c->trackBounds ());
}

// A drag that nets no change pushes nothing (no empty undo entry).
TEST_F (ClipOperatorTest, NoOpGesturePushesNothing)
{
  auto * c = clip ();
  c->length ()->setTicks (4000.0);

  clip_operator->beginClipLoopPointsDrag ();
  // Echo the current (default) values — no net change.
  clip_operator->updateClipLoopPointsDrag (0.0, 0.0, 4000.0);
  clip_operator->endClipLoopPointsDrag ();

  EXPECT_EQ (undo_stack->count (), 0);
}

// Aborting reverts the live preview to the pre-drag state without pushing.
TEST_F (ClipOperatorTest, AbortRevertsLivePreviewWithoutPushing)
{
  auto * c = clip ();
  c->length ()->setTicks (4000.0);

  clip_operator->beginClipLoopPointsDrag ();
  clip_operator->updateClipLoopPointsDrag (300.0, 0.0, 4000.0);
  ASSERT_DOUBLE_EQ (c->clipStartPosition ()->ticks (), 300.0);

  clip_operator->abortClipLoopPointsDrag ();

  EXPECT_EQ (undo_stack->count (), 0);
  EXPECT_DOUBLE_EQ (c->clipStartPosition ()->ticks (), 0.0);
  EXPECT_TRUE (c->trackBounds ());
}

// A drag ending on the default range re-engages trackBounds.
TEST_F (ClipOperatorTest, DefaultRangeReEngagesTrackBounds)
{
  auto * c = clip ();
  c->length ()->setTicks (4000.0);
  // Start from a custom (non-default) state.
  c->set_loop_range (
    dsp::ContentTick{ units::ticks (100.0) },
    dsp::ContentTick{ units::ticks (0.0) },
    dsp::ContentTick{ units::ticks (4000.0) });
  ASSERT_FALSE (c->trackBounds ());

  clip_operator->beginClipLoopPointsDrag ();
  clip_operator->updateClipLoopPointsDrag (0.0, 0.0, 4000.0);
  clip_operator->endClipLoopPointsDrag ();

  EXPECT_TRUE (c->trackBounds ());
}

// Setting a new clip mid-drag finalizes the old clip's gesture before
// switching (the old clip is still valid at that point).
TEST_F (ClipOperatorTest, SetClipMidDragFinalizesOldGesture)
{
  auto * c = clip ();
  c->length ()->setTicks (4000.0);

  clip_operator->beginClipLoopPointsDrag ();
  clip_operator->updateClipLoopPointsDrag (250.0, 0.0, 4000.0);
  ASSERT_EQ (undo_stack->count (), 0);

  // Switch away mid-drag.
  clip_operator->setClip (nullptr);

  EXPECT_EQ (undo_stack->count (), 1);
  EXPECT_DOUBLE_EQ (c->clipStartPosition ()->ticks (), 250.0);
  EXPECT_EQ (clip_operator->clip (), nullptr);
}

} // namespace zrythm::actions
