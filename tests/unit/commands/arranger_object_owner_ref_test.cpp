// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/add_arranger_object_command.h"
#include "commands/arranger_object_owner_ref.h"
#include "commands/relocate_arranger_object_command.h"
#include "commands/remove_arranger_object_command.h"
#include "dsp/parameter.h"
#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/arranger_object_factory.h"
#include "structure/tracks/automation_track.h"
#include "structure/tracks/track_lane.h"
#include "structure/tracks/track_lane_list.h"
#include "utils/app_settings.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"

#include "helpers/in_memory_settings_backend.h"

#include <gtest/gtest.h>

namespace zrythm::commands
{

class ArrangerObjectOwnerRefTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    app_settings_ = std::make_unique<utils::AppSettings> (
      std::make_unique<test_helpers::InMemorySettingsBackend> ());

    factory_ = std::make_unique<structure::arrangement::ArrangerObjectFactory> (
      structure::arrangement::ArrangerObjectFactory::Dependencies{
        .tempo_map_ = tempo_map_wrapper_,
        .registry_ = registry_,
        .last_timeline_obj_len_provider_ = [] () { return 100.0; },
        .last_editor_obj_len_provider_ = [] () { return 50.0; },
        .automation_curve_algorithm_provider_ =
          [] () { return dsp::CurveOptions::Algorithm::Exponent; } },
      [] () { return units::sample_rate (44100); },
      [] () { return units::bpm (120.0); });
  }

  dsp::TempoMap                       tempo_map_{ units::sample_rate (44100) };
  dsp::TempoMapWrapper                tempo_map_wrapper_{ tempo_map_ };
  utils::ObjectRegistry               registry_;
  std::unique_ptr<utils::AppSettings> app_settings_;
  std::unique_ptr<structure::arrangement::ArrangerObjectFactory> factory_;
};

// A handle built for a lane resolves the lane's owner bases only
TEST_F (ArrangerObjectOwnerRefTest, ResolvesLaneOwnerBases)
{
  structure::tracks::TrackLaneList lane_list{ registry_, nullptr };
  auto *                           lane = lane_list.addLane ();

  const auto owner_ref = make_owner_ref (*lane, registry_);
  EXPECT_NE (owner_ref.resolve<structure::arrangement::MidiClip> (), nullptr);
  EXPECT_NE (owner_ref.resolve<structure::arrangement::AudioClip> (), nullptr);
  EXPECT_EQ (owner_ref.resolve<structure::arrangement::Marker> (), nullptr);
}

// The tempo object manager owns two object types through distinct bases;
// both resolve
TEST_F (ArrangerObjectOwnerRefTest, ResolvesTempoObjectManagerOwnerBases)
{
  const auto tom_ref = utils::create_object<
    structure::arrangement::TempoObjectManager> (registry_, registry_);

  const auto owner_ref = make_owner_ref (*tom_ref.get (), registry_);
  EXPECT_NE (owner_ref.resolve<structure::arrangement::TempoObject> (), nullptr);
  EXPECT_NE (
    owner_ref.resolve<structure::arrangement::TimeSignatureObject> (), nullptr);
  EXPECT_EQ (owner_ref.resolve<structure::arrangement::MidiClip> (), nullptr);
}

// A clip owning editor objects resolves through its arranger-object
// identity
TEST_F (ArrangerObjectOwnerRefTest, ResolvesClipOwnerBases)
{
  auto clip_ref =
    factory_->get_builder<structure::arrangement::MidiClip> ()
      .build_in_registry ();

  const auto owner_ref = make_owner_ref (*clip_ref.get (), registry_);
  EXPECT_NE (owner_ref.resolve<structure::arrangement::MidiNote> (), nullptr);
  EXPECT_NE (
    owner_ref.resolve<structure::arrangement::MidiControlEvent> (), nullptr);
  EXPECT_EQ (owner_ref.resolve<structure::arrangement::MidiClip> (), nullptr);
}

// AutomationTracks are not registry objects; their handle holds raw owner
// pointers and resolves the automation clip owner base
TEST_F (ArrangerObjectOwnerRefTest, AutomationTrackUsesRawOwnerPointers)
{
  auto param_ref = utils::create_object<dsp::ProcessorParameter> (
    registry_, registry_, dsp::ProcessorParameter::UniqueId (u8"test_param"),
    dsp::ParameterRange (dsp::ParameterRange::Type::Linear, 0.f, 1.f, 0.f, 0.5f),
    utils::Utf8String::from_utf8_encoded_string ("Test Parameter"));
  structure::tracks::AutomationTrack automation_track{
    tempo_map_wrapper_, registry_, param_ref
  };

  const auto owner_ref = make_owner_ref (automation_track);
  EXPECT_NE (
    owner_ref.resolve<structure::arrangement::AutomationClip> (), nullptr);
  EXPECT_EQ (owner_ref.resolve<structure::arrangement::TempoObject> (), nullptr);
}

// Commands refuse an owner that cannot own the command's object type
TEST_F (ArrangerObjectOwnerRefTest, WrongOwnerTypeThrows)
{
  structure::tracks::TrackLaneList lane_list{ registry_, nullptr };
  auto *                           lane = lane_list.addLane ();
  auto                             note_ref =
    factory_->get_builder<structure::arrangement::MidiNote> ()
      .build_in_registry ();

  EXPECT_THROW (
    AddArrangerObjectCommand<structure::arrangement::MidiNote> (
      make_owner_ref (*lane, registry_), note_ref),
    std::invalid_argument);
}

// Dropping the last handle to an owner deletes it from the registry
TEST_F (ArrangerObjectOwnerRefTest, LastRefReleaseDeletesOwner)
{
  auto lane_id = QUuid{};
  {
    auto lane_ref = utils::create_object<structure::tracks::TrackLane> (
      registry_,
      structure::tracks::TrackLane::TrackLaneDependencies{ registry_, nullptr });
    lane_id = type_safe::get (lane_ref.id ());
    EXPECT_TRUE (registry_.contains (lane_id));
  }
  EXPECT_FALSE (registry_.contains (lane_id));
}

// An undo command holds its owner alive: removing the lane from its list
// while the command exists leaves the lane resolvable and reattachable
TEST_F (ArrangerObjectOwnerRefTest, CommandKeepsRemovedLaneAlive)
{
  structure::tracks::TrackLaneList lane_list{ registry_, nullptr };
  lane_list.addLane ();
  auto * lane = lane_list.addLane ();
  auto   clip_ref =
    factory_->get_builder<structure::arrangement::MidiClip> ()
      .build_in_registry ();

  const auto lane_id = type_safe::get (lane->get_uuid ());
  AddArrangerObjectCommand<structure::arrangement::MidiClip> command (
    make_owner_ref (*lane, registry_), clip_ref);
  command.redo ();
  EXPECT_EQ (
    lane
      ->structure::arrangement::ArrangerObjectOwner<
        structure::arrangement::MidiClip>::get_children_vector ()
      .size (),
    1);

  lane_list.removeLane (1);
  EXPECT_TRUE (registry_.contains (lane_id));

  // The command still resolves the detached lane and detaches the clip
  command.undo ();
  EXPECT_EQ (
    lane
      ->structure::arrangement::ArrangerObjectOwner<
        structure::arrangement::MidiClip>::get_children_vector ()
      .size (),
    0);
}

// A handle survives its owner being detached from one list and attached
// to another: resolution goes through the registry, not the owner list
TEST_F (ArrangerObjectOwnerRefTest, CommandSurvivesOwnerReattachment)
{
  structure::tracks::TrackLaneList list_a{ registry_, nullptr };
  list_a.addLane ();
  auto * lane = list_a.addLane ();
  auto   clip_ref =
    factory_->get_builder<structure::arrangement::MidiClip> ()
      .build_in_registry ();

  AddArrangerObjectCommand<structure::arrangement::MidiClip> command (
    make_owner_ref (*lane, registry_), clip_ref);
  command.redo ();

  nlohmann::json list_json;
  to_json (list_json, list_a);

  list_a.removeLane (1);

  structure::tracks::TrackLaneList list_b{ registry_, nullptr };
  from_json (list_json, list_b);
  // The clip attach appended a trailing empty lane to list_a
  EXPECT_EQ (list_b.size (), 3);

  // The command resolves the lane through its registry identity and
  // removes the clip from it in its new list
  command.undo ();
  EXPECT_EQ (
    lane
      ->structure::arrangement::ArrangerObjectOwner<
        structure::arrangement::MidiClip>::get_children_vector ()
      .size (),
    0);
}

// A clip landing on the last lane keeps the list ending with an empty
// lane; undoing the add restores the previous lane count
TEST_F (ArrangerObjectOwnerRefTest, AddToLastLaneKeepsTrailingEmptyLane)
{
  structure::tracks::TrackLaneList lane_list{ registry_, nullptr };
  auto *                           lane = lane_list.addLane ();
  auto                             clip_ref =
    factory_->get_builder<structure::arrangement::MidiClip> ()
      .build_in_registry ();

  AddArrangerObjectCommand<structure::arrangement::MidiClip> command (
    make_owner_ref (*lane, registry_), clip_ref);
  command.redo ();
  EXPECT_EQ (lane_list.size (), 2);
  EXPECT_TRUE (lane_list.at (1)->is_empty ());

  command.undo ();
  EXPECT_EQ (lane_list.size (), 1);
}

// Moving a clip onto the last lane appends a trailing empty lane, and
// moving it away leaves a single trailing empty lane
TEST_F (ArrangerObjectOwnerRefTest, RelocateKeepsTrailingEmptyLane)
{
  structure::tracks::TrackLaneList lane_list{ registry_, nullptr };
  auto *                           lane_1 = lane_list.addLane ();
  auto *                           lane_2 = lane_list.addLane ();
  auto                             clip_ref =
    factory_->get_builder<structure::arrangement::MidiClip> ()
      .build_in_registry ();
  AddArrangerObjectCommand<structure::arrangement::MidiClip> attach (
    make_owner_ref (*lane_1, registry_), clip_ref);
  attach.redo ();
  // Lane 1 is not the last lane, so no spare was appended
  ASSERT_EQ (lane_list.size (), 2);

  RelocateArrangerObjectCommand<structure::arrangement::MidiClip> move_down (
    clip_ref, make_owner_ref (*lane_1, registry_),
    make_owner_ref (*lane_2, registry_));
  move_down.redo ();
  EXPECT_EQ (lane_list.size (), 3);
  EXPECT_TRUE (lane_list.at (2)->is_empty ());

  move_down.undo ();
  EXPECT_EQ (lane_list.size (), 2);
  EXPECT_TRUE (lane_list.at (1)->is_empty ());
}

// Owner pointers resolved by the selection operator convert to handles
// through the registry when the owner is a registry object
TEST_F (ArrangerObjectOwnerRefTest, ToOwnerRefUsesRegistryIdentity)
{
  structure::tracks::TrackLaneList lane_list{ registry_, nullptr };
  auto *                           lane = lane_list.addLane ();

  structure::arrangement::ArrangerObjectOwnerPtrVariant owner_ptrs{
    static_cast<structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::MidiClip> *> (lane)
  };
  const auto owner_ref = to_owner_ref (owner_ptrs, registry_);
  EXPECT_NE (owner_ref.resolve<structure::arrangement::MidiClip> (), nullptr);
}

TEST_F (ArrangerObjectOwnerRefTest, ToOwnerRefThrowsOnNullOwner)
{
  structure::arrangement::ArrangerObjectOwnerPtrVariant owner_ptrs{
    static_cast<structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::MidiClip> *> (nullptr)
  };
  EXPECT_THROW (to_owner_ref (owner_ptrs, registry_), std::invalid_argument);
}

} // namespace zrythm::commands
