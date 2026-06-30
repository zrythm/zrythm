// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/arranger_object_all.h"
#include "utils/object_registry.h"

#include <gtest/gtest.h>

namespace zrythm::structure::arrangement
{

class ArrangerObjectAllTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map_ = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
    tempo_map_->add_tempo_event (
      units::ticks (0), units::bpm (120.0), dsp::TempoMap::CurveType::Constant);
    tempo_map_wrapper_ = std::make_unique<dsp::TempoMapWrapper> (*tempo_map_);

    clip_ = std::make_unique<MidiClip> (*tempo_map_wrapper_, registry_, nullptr);
    clip_->position ()->setTicks (0.0);
    clip_->length ()->setTicks (960.0);

    marker_ = std::make_unique<Marker> (
      *tempo_map_wrapper_, Marker::MarkerType::Custom, nullptr);
    marker_->position ()->setTicks (480.0);
  }

  MidiNote &add_note (double position_ticks, double length_ticks)
  {
    auto ref = utils::create_object<MidiNote> (registry_, *tempo_map_wrapper_);
    auto * note = ref.get_object_as<MidiNote> ();
    note->position ()->setTicks (position_ticks);
    note->length ()->setTicks (length_ticks);
    clip_->ArrangerObjectOwner<MidiNote>::add_object (ref);
    return *note;
  }

  std::unique_ptr<dsp::TempoMap>        tempo_map_;
  std::unique_ptr<dsp::TempoMapWrapper> tempo_map_wrapper_;
  utils::ObjectRegistry                 registry_;
  std::unique_ptr<MidiClip>             clip_;
  std::unique_ptr<Marker>               marker_;
};

// ========================================================================
// is_clip
// ========================================================================

TEST_F (ArrangerObjectAllTest, IsClip_TrueForClips)
{
  EXPECT_TRUE (is_clip (*clip_));

  auto audio_clip =
    std::make_unique<AudioClip> (*tempo_map_wrapper_, registry_, nullptr);
  EXPECT_TRUE (is_clip (*audio_clip));

  auto chord_clip =
    std::make_unique<ChordClip> (*tempo_map_wrapper_, registry_, nullptr);
  EXPECT_TRUE (is_clip (*chord_clip));

  auto automation_clip =
    std::make_unique<AutomationClip> (*tempo_map_wrapper_, registry_, nullptr);
  EXPECT_TRUE (is_clip (*automation_clip));
}

TEST_F (ArrangerObjectAllTest, IsClip_FalseForNonClips)
{
  EXPECT_FALSE (is_clip (*marker_));
  auto &note = add_note (0, 100);
  EXPECT_FALSE (is_clip (note));
}

// ========================================================================
// timeline_ticks
// ========================================================================

TEST_F (ArrangerObjectAllTest, TimelineTicks_TimelineObject)
{
  EXPECT_DOUBLE_EQ (timeline_ticks (*marker_).asDouble (), 480.0);
}

TEST_F (ArrangerObjectAllTest, TimelineTicks_Clip)
{
  clip_->position ()->setTicks (320.0);
  EXPECT_DOUBLE_EQ (timeline_ticks (*clip_).asDouble (), 320.0);
}

TEST_F (ArrangerObjectAllTest, TimelineTicks_ChildIdentityWarp)
{
  auto &note = add_note (240.0, 480.0);
  // Clip at 0, identity warp → content 240 maps to timeline 240
  EXPECT_NEAR (timeline_ticks (note).asDouble (), 240.0, 0.5);
}

TEST_F (ArrangerObjectAllTest, TimelineTicks_ChildWithWarp)
{
  // Project tempo 240 BPM, source tempo 120 BPM → content doubled on timeline
  auto fast_tempo_map =
    std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
  fast_tempo_map->add_tempo_event (
    units::ticks (0), units::bpm (240.0), dsp::TempoMap::CurveType::Constant);
  auto fast_wrapper = std::make_unique<dsp::TempoMapWrapper> (*fast_tempo_map);

  auto warped_clip =
    std::make_unique<MidiClip> (*fast_wrapper, registry_, nullptr);
  warped_clip->position ()->setTicks (0.0);
  warped_clip->length ()->setTicks (7680.0);
  warped_clip->contentWarp ()->configure_as_source (units::bpm (120.0));

  auto   note_ref = utils::create_object<MidiNote> (registry_, *fast_wrapper);
  auto * note = note_ref.get_object_as<MidiNote> ();
  note->position ()->setTicks (3840.0);
  note->length ()->setTicks (3840.0);
  warped_clip->ArrangerObjectOwner<MidiNote>::add_object (note_ref);

  // Content 3840 with 2× stretch → timeline ~7680
  EXPECT_NEAR (timeline_ticks (*note).asDouble (), 7680.0, 5.0);
}

// ========================================================================
// timeline_end_ticks
// ========================================================================

TEST_F (ArrangerObjectAllTest, TimelineEndTicks_ClipIdentityWarp)
{
  // Clip at 0, length 960 → end 960
  EXPECT_NEAR (timeline_end_ticks (*clip_).asDouble (), 960.0, 0.5);
}

TEST_F (ArrangerObjectAllTest, TimelineEndTicks_ChildIdentityWarp)
{
  auto &note = add_note (240.0, 480.0);
  // Content pos 240 + len 480 = 720, identity warp → timeline 720
  EXPECT_NEAR (timeline_end_ticks (note).asDouble (), 720.0, 0.5);
}

TEST_F (ArrangerObjectAllTest, TimelineEndTicks_UnboundedObject)
{
  // Marker has no length → end == start
  EXPECT_DOUBLE_EQ (
    timeline_end_ticks (*marker_).asDouble (),
    timeline_ticks (*marker_).asDouble ());
}

TEST_F (ArrangerObjectAllTest, TimelineEndTicks_ClipWithWarp)
{
  // Project tempo 240 BPM, source tempo 120 BPM → content doubled on timeline
  auto fast_tempo_map =
    std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
  fast_tempo_map->add_tempo_event (
    units::ticks (0), units::bpm (240.0), dsp::TempoMap::CurveType::Constant);
  auto fast_wrapper = std::make_unique<dsp::TempoMapWrapper> (*fast_tempo_map);

  auto warped_clip =
    std::make_unique<MidiClip> (*fast_wrapper, registry_, nullptr);
  warped_clip->position ()->setTicks (0.0);
  warped_clip->length ()->setTicks (7680.0);
  warped_clip->contentWarp ()->configure_as_source (units::bpm (120.0));

  // Content length 7680 with 2× stretch → timeline end ~15360
  EXPECT_NEAR (timeline_end_ticks (*warped_clip).asDouble (), 15360.0, 5.0);
}

TEST_F (ArrangerObjectAllTest, TimelineEndTicks_ChildWithWarp)
{
  // Project tempo 240 BPM, source tempo 120 BPM → content doubled on timeline
  auto fast_tempo_map =
    std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
  fast_tempo_map->add_tempo_event (
    units::ticks (0), units::bpm (240.0), dsp::TempoMap::CurveType::Constant);
  auto fast_wrapper = std::make_unique<dsp::TempoMapWrapper> (*fast_tempo_map);

  auto warped_clip =
    std::make_unique<MidiClip> (*fast_wrapper, registry_, nullptr);
  warped_clip->position ()->setTicks (0.0);
  warped_clip->length ()->setTicks (7680.0);
  warped_clip->contentWarp ()->configure_as_source (units::bpm (120.0));

  auto   note_ref = utils::create_object<MidiNote> (registry_, *fast_wrapper);
  auto * note = note_ref.get_object_as<MidiNote> ();
  note->position ()->setTicks (3840.0);
  note->length ()->setTicks (3840.0);
  warped_clip->ArrangerObjectOwner<MidiNote>::add_object (note_ref);

  // Content pos 3840 + len 3840 = 7680, with 2× stretch → timeline ~15360
  EXPECT_NEAR (timeline_end_ticks (*note).asDouble (), 15360.0, 5.0);
}

} // namespace zrythm::structure::arrangement
