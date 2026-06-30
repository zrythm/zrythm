// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/tempo_map.h"
#include "gui/qquick/timeline_position_tracker.h"
#include "structure/arrangement/arranger_object_factory.h"
#include "structure/arrangement/midi_clip.h"
#include "structure/arrangement/midi_note.h"
#include "utils/app_settings.h"
#include "utils/object_registry.h"

#include <QSignalSpy>

#include "helpers/in_memory_settings_backend.h"
#include "helpers/scoped_qcoreapplication.h"

#include <gtest/gtest.h>

namespace zrythm::gui::qquick::tests
{

class TimelinePositionTrackerTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    app_ = std::make_unique<test_helpers::ScopedQCoreApplication> ();
    registry_ = std::make_unique<utils::ObjectRegistry> ();
    tempo_map_ = std::make_unique<dsp::TempoMap> (units::sample_rate (44100));
    tempo_map_wrapper_ = std::make_unique<dsp::TempoMapWrapper> (*tempo_map_);
    sample_rate_provider_ = [] () { return units::sample_rate (44100); };
    bpm_provider_ = [] () { return units::bpm (120.0); };
    app_settings_ = std::make_unique<utils::AppSettings> (
      std::make_unique<test_helpers::InMemorySettingsBackend> ());
    factory_ = std::make_unique<structure::arrangement::ArrangerObjectFactory> (
      structure::arrangement::ArrangerObjectFactory::Dependencies{
        .tempo_map_ = *tempo_map_wrapper_,
        .registry_ = *registry_,
        .last_timeline_obj_len_provider_ = [] () { return 100.0; },
        .last_editor_obj_len_provider_ = [] () { return 50.0; },
        .automation_curve_algorithm_provider_ =
          [] () { return dsp::CurveOptions::Algorithm::Exponent; },
      },
      sample_rate_provider_, bpm_provider_);
  }

  structure::arrangement::MidiClip *
  make_midi_clip (double start_ticks, double end_ticks)
  {
    auto ref =
      factory_->get_builder<structure::arrangement::MidiClip> ()
        .with_start_ticks (units::ticks (start_ticks))
        .with_end_ticks (units::ticks (end_ticks))
        .build_in_registry ();
    auto * clip = ref.get_object_as<structure::arrangement::MidiClip> ();
    clips_.push_back (std::move (ref));
    return clip;
  }

  structure::arrangement::MidiNote *
  add_note (structure::arrangement::MidiClip &clip, double start_ticks, int pitch)
  {
    auto ref =
      factory_->get_builder<structure::arrangement::MidiNote> ()
        .with_start_ticks (units::ticks (start_ticks))
        .with_pitch (pitch)
        .build_in_registry ();
    auto * obj = ref.get_object_as<structure::arrangement::MidiNote> ();
    static_cast<structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::MidiNote> &> (clip)
      .add_object (ref);
    note_refs_.push_back (std::move (ref));
    return obj;
  }

  std::unique_ptr<test_helpers::ScopedQCoreApplication> app_;
  std::unique_ptr<utils::ObjectRegistry>                registry_;
  std::unique_ptr<dsp::TempoMap>                        tempo_map_;
  std::unique_ptr<dsp::TempoMapWrapper>                 tempo_map_wrapper_;
  std::function<units::sample_rate_t ()>                sample_rate_provider_;
  std::function<units::bpm_t ()>                        bpm_provider_;
  std::unique_ptr<utils::AppSettings>                   app_settings_;
  std::unique_ptr<structure::arrangement::ArrangerObjectFactory>   factory_;
  std::vector<structure::arrangement::ArrangerObjectUuidReference> clips_;
  std::vector<structure::arrangement::ArrangerObjectUuidReference> note_refs_;
};

TEST_F (TimelinePositionTrackerTest, NullObjectReturnsZero)
{
  TimelinePositionTracker tracker;
  EXPECT_DOUBLE_EQ (tracker.timelineTicks (), 0.0);
  EXPECT_DOUBLE_EQ (tracker.timelineEndTicks (), 0.0);
}

TEST_F (TimelinePositionTrackerTest, TopLevelClipPosition)
{
  auto * clip = make_midi_clip (1920.0, 3840.0);

  TimelinePositionTracker tracker;
  tracker.setArrangerObject (clip);

  // Clip at tick 1920 → timelineTicks should be 1920
  EXPECT_DOUBLE_EQ (tracker.timelineTicks (), 1920.0);
  // End = position + timelineLengthTicks = 1920 + 1920 = 3840
  EXPECT_DOUBLE_EQ (tracker.timelineEndTicks (), 3840.0);
}

TEST_F (TimelinePositionTrackerTest, PositionChangeEmitsSignal)
{
  auto * clip = make_midi_clip (0.0, 1920.0);

  TimelinePositionTracker tracker;
  tracker.setArrangerObject (clip);

  QSignalSpy spy (&tracker, &TimelinePositionTracker::timelineTicksChanged);
  clip->position ()->setTicks (960.0);

  EXPECT_EQ (spy.count (), 1);
  EXPECT_DOUBLE_EQ (tracker.timelineTicks (), 960.0);
}

TEST_F (TimelinePositionTrackerTest, LengthChangeEmitsEndSignal)
{
  auto * clip = make_midi_clip (0.0, 1920.0);

  TimelinePositionTracker tracker;
  tracker.setArrangerObject (clip);

  QSignalSpy startSpy (&tracker, &TimelinePositionTracker::timelineTicksChanged);
  QSignalSpy endSpy (
    &tracker, &TimelinePositionTracker::timelineEndTicksChanged);

  clip->length ()->setTicks (3840.0);

  EXPECT_EQ (startSpy.count (), 0);
  EXPECT_EQ (endSpy.count (), 1);
  EXPECT_DOUBLE_EQ (tracker.timelineEndTicks (), 3840.0);
}

TEST_F (TimelinePositionTrackerTest, ChildObjectPositionInMusicalMode)
{
  // Parent clip at tick 1920, length 1920
  auto * clip = make_midi_clip (1920.0, 3840.0);
  // Note at content tick 480 inside the clip
  auto * note = add_note (*clip, 480.0, 60);

  TimelinePositionTracker tracker;
  tracker.setArrangerObject (note);

  // In musical mode (default, identity warp): timeline = parent + child
  EXPECT_DOUBLE_EQ (tracker.timelineTicks (), 1920.0 + 480.0);
}

TEST_F (TimelinePositionTrackerTest, WarpChangeEmitsSignalForChild)
{
  auto * clip = make_midi_clip (0.0, 1920.0);
  auto * note = add_note (*clip, 480.0, 60);

  TimelinePositionTracker tracker;
  tracker.setArrangerObject (note);

  // In musical mode: timeline = 0 + 480 = 480
  EXPECT_DOUBLE_EQ (tracker.timelineTicks (), 480.0);

  QSignalSpy spy (&tracker, &TimelinePositionTracker::timelineTicksChanged);

  // Switch to source mode (absolute) with BPM 240 (project is 120).
  // Content tick 480 at 240 BPM = 0.5 beats = 0.125 seconds.
  // At project 120 BPM, 0.125 sec = 0.25 beats = 240 ticks.
  clip->contentWarp ()->configure_as_source (units::bpm (240.0));

  ASSERT_EQ (spy.count (), 1);
  // Warped: content plays faster, so takes less timeline space.
  EXPECT_NE (tracker.timelineTicks (), 480.0);
  EXPECT_DOUBLE_EQ (tracker.timelineTicks (), 240.0);
}

TEST_F (TimelinePositionTrackerTest, ObjectChangeDisconnectsOld)
{
  auto * clip1 = make_midi_clip (960.0, 1920.0);
  auto * clip2 = make_midi_clip (3840.0, 5760.0);

  TimelinePositionTracker tracker;
  tracker.setArrangerObject (clip1);
  EXPECT_DOUBLE_EQ (tracker.timelineTicks (), 960.0);

  // Switch to different object
  QSignalSpy spy (&tracker, &TimelinePositionTracker::timelineTicksChanged);
  tracker.setArrangerObject (clip2);

  EXPECT_EQ (spy.count (), 1);
  EXPECT_DOUBLE_EQ (tracker.timelineTicks (), 3840.0);

  // Changing clip1's position should NOT trigger signals anymore
  clip1->position ()->setTicks (0.0);
  EXPECT_EQ (spy.count (), 1);
}

// A child object's timeline position depends on its parent clip's position
// (contentToTimeline adds the clip position). Moving the parent must emit.
TEST_F (TimelinePositionTrackerTest, ParentClipPositionMoveEmitsForChild)
{
  auto * clip = make_midi_clip (1920.0, 3840.0);
  auto * note = add_note (*clip, 480.0, 60);

  TimelinePositionTracker tracker;
  tracker.setArrangerObject (note);

  QSignalSpy tickSpy (&tracker, &TimelinePositionTracker::timelineTicksChanged);
  QSignalSpy endSpy (
    &tracker, &TimelinePositionTracker::timelineEndTicksChanged);

  clip->position ()->setTicks (960.0);

  EXPECT_EQ (tickSpy.count (), 1);
  EXPECT_EQ (endSpy.count (), 1);
}

// A clip's own end depends on its warp map. Changing the warp must emit the
// end signal even when the tracked object is the clip itself.
TEST_F (TimelinePositionTrackerTest, SelfWarpChangeEmitsEndForClip)
{
  auto * clip = make_midi_clip (0.0, 7680.0);

  TimelinePositionTracker tracker;
  tracker.setArrangerObject (clip);

  QSignalSpy endSpy (
    &tracker, &TimelinePositionTracker::timelineEndTicksChanged);

  clip->contentWarp ()->configure_as_source (units::bpm (240.0));

  EXPECT_EQ (endSpy.count (), 1);
}

} // namespace zrythm::gui::qquick::tests
