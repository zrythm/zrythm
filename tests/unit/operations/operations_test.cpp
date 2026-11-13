// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/transport.h"
#include "structure/tracks/track.h"
#include "structure/tracks/track_collection.h"
#include "structure/tracks/track_factory.h"
#include "structure/tracks/track_routing.h"
#include "structure/tracks/tracklist.h"
#include "undo/undo_stack.h"
#include "utils/object_registry.h"
#include "utils/utf8_string.h"

#include "helpers/scoped_qcoreapplication.h"

#include "operations/builtin_operations.h"
#include "operations/operation.h"
#include "operations/operation_environment.h"
#include "operations/operation_registry.h"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

namespace zrythm::operations
{

class OperationsTest
    : public ::testing::Test,
      public test_helpers::ScopedQCoreApplication
{
protected:
  void SetUp () override
  {
    singleton_tracks_ = std::make_unique<structure::tracks::SingletonTracks> ();
    track_collection_ =
      std::make_unique<structure::tracks::TrackCollection> (registry_);
    track_routing_ =
      std::make_unique<structure::tracks::TrackRouting> (registry_);

    structure::tracks::FinalTrackDependencies factory_deps{
      tempo_map_wrapper_,
      registry_,
      soloed_tracks_exist_getter_,
      {},
    };
    track_factory_ = std::make_unique<structure::tracks::TrackFactory> (
      [factory_deps] () -> structure::tracks::FinalTrackDependencies {
        return factory_deps;
      });

    undo_stack_ = std::make_unique<undo::UndoStack> ([] (auto callback, bool) {
      callback ();
    });

    for (
      const auto type :
      { structure::tracks::Track::Type::Master,
        structure::tracks::Track::Type::Chord,
        structure::tracks::Track::Type::Marker })
      {
        auto ref = track_factory_->create_empty_track (type);
        switch (type)
          {
          case structure::tracks::Track::Type::Master:
            singleton_tracks_->setMasterTrack (
              ref.get_object_as<structure::tracks::MasterTrack> ());
            break;
          case structure::tracks::Track::Type::Chord:
            singleton_tracks_->setChordTrack (
              ref.get_object_as<structure::tracks::ChordTrack> ());
            break;
          case structure::tracks::Track::Type::Marker:
            singleton_tracks_->setMarkerTrack (
              ref.get_object_as<structure::tracks::MarkerTrack> ());
            break;
          default:
            break;
          }
        track_collection_->add_track (ref);
      }

    transport_config_ = {
      .return_to_cue_on_pause_ = [] { return false; },
      .metronome_countin_bars_ = [] { return 0; },
      .recording_preroll_bars_ = [] { return 0; },
    };
    transport_ =
      std::make_unique<dsp::Transport> (tempo_map_wrapper_, transport_config_);

    environment_ = std::make_unique<OperationEnvironment> (
      registry_, *undo_stack_, *track_collection_, *track_factory_,
      *track_routing_, *singleton_tracks_, transport_.get ());

    register_builtin_operations (operation_registry_);
  }

  /** Adds a track directly through the factory and returns it. */
  structure::tracks::Track * add_audio_track_directly ()
  {
    auto ref =
      track_factory_->create_empty_track<structure::tracks::AudioTrack> ();
    auto * track = ref.get ();
    track_collection_->add_track (ref);
    return track;
  }

  std::string uuid_of (const structure::tracks::Track &track) const
  {
    return std::string (
      utils::Utf8String::from_qstring (
        type_safe::get (track.get_uuid ()).toString (QUuid::WithoutBraces))
        .view ());
  }

  OperationReport dispatch (
    std::string_view       name,
    nlohmann::json         args = nlohmann::json::object (),
    const DispatchOptions &options = {})
  {
    return operation_registry_.dispatch (name, args, *environment_, options);
  }

  dsp::TempoMap         tempo_map_{ units::sample_rate (44100.0) };
  dsp::TempoMapWrapper  tempo_map_wrapper_{ tempo_map_ };
  utils::ObjectRegistry registry_;
  structure::tracks::SoloedTracksExistGetter soloed_tracks_exist_getter_{ [] {
    return false;
  } };

  std::unique_ptr<structure::tracks::SingletonTracks> singleton_tracks_;
  std::unique_ptr<structure::tracks::TrackCollection> track_collection_;
  std::unique_ptr<structure::tracks::TrackRouting>    track_routing_;
  std::unique_ptr<structure::tracks::TrackFactory>    track_factory_;
  std::unique_ptr<undo::UndoStack>                    undo_stack_;
  dsp::Transport::ConfigProvider                      transport_config_;
  std::unique_ptr<dsp::Transport>                     transport_;
  std::unique_ptr<OperationEnvironment>               environment_;
  OperationRegistry                                   operation_registry_;
};

// ============================================================================
// Registry shape
// ============================================================================

TEST_F (OperationsTest, BuiltinOperationsAreRegistered)
{
  const auto expected = std::vector<std::string>{
    "track.set_name", "track.set_color",          "track.add",
    "track.list",     "transport.set_play_state",
  };
  for (const auto &name : expected)
    {
      EXPECT_NE (operation_registry_.find (name), nullptr)
        << "missing operation " << name;
    }
}

TEST_F (OperationsTest, DuplicateRegistrationKeepsTheFirstOperation)
{
  const auto * original = operation_registry_.find ("track.set_name");
  ASSERT_NE (original, nullptr);
  const auto count_before = operation_registry_.operations ().size ();

  operation_registry_.register_operation (*original);

  EXPECT_EQ (operation_registry_.find ("track.set_name"), original);
  EXPECT_EQ (operation_registry_.operations ().size (), count_before);
}

TEST_F (OperationsTest, ToolDefinitionsDescribeTheirInput)
{
  for (const auto &op : operation_registry_.operations ())
    {
      const auto &def = op.tool_definition ();
      EXPECT_EQ (def.at ("name").get<std::string> (), op.name ());
      EXPECT_TRUE (def.contains ("description"));
      const auto &schema = def.at ("inputSchema");
      EXPECT_EQ (schema.at ("type").get<std::string> (), "object");
      EXPECT_TRUE (schema.contains ("properties"));
      EXPECT_TRUE (schema.contains ("required"));
    }

  const auto &set_name =
    operation_registry_.find ("track.set_name")->tool_definition ();
  const auto &props = set_name.at ("inputSchema").at ("properties");
  ASSERT_TRUE (props.contains ("track"));
  ASSERT_TRUE (props.contains ("new_name"));
  EXPECT_EQ (props.at ("new_name").at ("minLength").get<int> (), 1);
  EXPECT_EQ (props.at ("new_name").at ("maxLength").get<int> (), 128);
  const auto &required = set_name.at ("inputSchema").at ("required");
  ASSERT_EQ (required.size (), 2);

  const auto &list = operation_registry_.find ("track.list")->tool_definition ();
  EXPECT_TRUE (
    list.at ("inputSchema").at ("properties").at ("limit").contains ("default"));
  EXPECT_TRUE (list.at ("inputSchema").at ("required").empty ());
}

// ============================================================================
// track.set_name
// ============================================================================

TEST_F (OperationsTest, TrackSetNameAppliesAndUndoes)
{
  auto * track = add_audio_track_directly ();
  track->setName ("Original");

  auto report = dispatch (
    "track.set_name",
    {
      { "track",    uuid_of (*track) },
      { "new_name", "Intro"          }
  });

  ASSERT_TRUE (report.applied ()) << report.message;
  EXPECT_EQ (track->name (), QString ("Intro"));
  ASSERT_EQ (report.command_texts.size (), 1);
  EXPECT_EQ (report.command_texts.front (), "Rename Track");
  ASSERT_EQ (report.affected_objects.size (), 1);
  EXPECT_EQ (report.affected_objects.front ().kind, "track");
  EXPECT_GT (report.revision_after, report.revision_before);

  undo_stack_->undo ();
  EXPECT_EQ (track->name (), QString ("Original"));
}

TEST_F (OperationsTest, TrackSetNameRefusesMissingParameter)
{
  auto * track = add_audio_track_directly ();
  track->setName ("Original");

  auto report = dispatch (
    "track.set_name",
    {
      { "track", uuid_of (*track) }
  });

  ASSERT_TRUE (report.refused ());
  EXPECT_NE (report.message.find ("new_name"), std::string::npos);
  EXPECT_EQ (track->name (), QString ("Original"));
  EXPECT_EQ (undo_stack_->count (), 0);
  EXPECT_EQ (report.revision_after, report.revision_before);
}

TEST_F (OperationsTest, TrackSetNameRefusesEmptyName)
{
  auto * track = add_audio_track_directly ();

  auto report = dispatch (
    "track.set_name",
    {
      { "track",    uuid_of (*track) },
      { "new_name", ""               }
  });

  ASSERT_TRUE (report.refused ());
  EXPECT_NE (report.message.find ("new_name"), std::string::npos);
}

TEST_F (OperationsTest, TrackSetNameRefusesWrongValueType)
{
  auto * track = add_audio_track_directly ();

  auto report = dispatch (
    "track.set_name",
    {
      { "track",    uuid_of (*track) },
      { "new_name", 42               }
  });

  ASSERT_TRUE (report.refused ());
}

TEST_F (OperationsTest, TrackSetNameRefusesInvalidUtf8)
{
  auto * track = add_audio_track_directly ();
  track->setName ("Original");

  auto report = dispatch (
    "track.set_name",
    {
      { "track",    uuid_of (*track)                 },
      { "new_name", std::string ("Bad\xC3\x28 name") }
  });

  ASSERT_TRUE (report.refused ());
  EXPECT_NE (report.message.find ("UTF-8"), std::string::npos);
  EXPECT_EQ (track->name (), QString ("Original"));
  EXPECT_EQ (undo_stack_->count (), 0);
}

TEST_F (OperationsTest, TrackSetNameRefusesUnknownTrack)
{
  auto report = dispatch (
    "track.set_name",
    {
      { "track",    "123e4567-e89b-12d3-a456-426614174000" },
      { "new_name", "Intro"                                }
  });

  ASSERT_TRUE (report.refused ());
  EXPECT_NE (report.message.find ("no track with UUID"), std::string::npos);
}

TEST_F (OperationsTest, TrackSetNameRefusesNonUuidTrack)
{
  auto report = dispatch (
    "track.set_name",
    {
      { "track",    "not-a-uuid" },
      { "new_name", "Intro"      }
  });

  ASSERT_TRUE (report.refused ());
}

// ============================================================================
// track.set_color
// ============================================================================

TEST_F (OperationsTest, TrackSetColorAppliesAndUndoes)
{
  auto * track = add_audio_track_directly ();

  auto report = dispatch (
    "track.set_color",
    {
      { "track", uuid_of (*track) },
      { "color", "#00ff88"        }
  });

  ASSERT_TRUE (report.applied ()) << report.message;
  EXPECT_TRUE (track->useColor ());
  EXPECT_EQ (track->color (), QColor ("#00ff88"));

  undo_stack_->undo ();
  EXPECT_FALSE (track->useColor ());
}

TEST_F (OperationsTest, TrackSetColorIsIdempotentForSameColor)
{
  auto * track = add_audio_track_directly ();
  dispatch (
    "track.set_color",
    {
      { "track", uuid_of (*track) },
      { "color", "#00ff88"        }
  });
  ASSERT_EQ (undo_stack_->count (), 1);

  auto report = dispatch (
    "track.set_color",
    {
      { "track", uuid_of (*track) },
      { "color", "#00ff88"        }
  });

  ASSERT_TRUE (report.applied ());
  EXPECT_TRUE (report.command_texts.empty ());
  // no command was pushed, so the revision did not move
  EXPECT_EQ (report.revision_after, report.revision_before);
  EXPECT_EQ (undo_stack_->count (), 1);
}

TEST_F (OperationsTest, TrackSetColorRefusesInvalidColor)
{
  auto * track = add_audio_track_directly ();

  auto report = dispatch (
    "track.set_color",
    {
      { "track", uuid_of (*track) },
      { "color", "#00ff"          }
  });

  ASSERT_TRUE (report.refused ());
  EXPECT_NE (report.message.find ("color"), std::string::npos);
  EXPECT_FALSE (track->useColor ());
}

// ============================================================================
// track.add
// ============================================================================

TEST_F (OperationsTest, TrackAddCreatesAndRoutesAudioTrack)
{
  const auto count_before = track_collection_->track_count ();

  auto report = dispatch (
    "track.add",
    {
      { "type", "Audio" }
  });

  ASSERT_TRUE (report.applied ()) << report.message;
  EXPECT_EQ (track_collection_->track_count (), count_before + 1);
  ASSERT_TRUE (report.result.contains ("track"));
  const auto new_uuid = report.result.at ("track").get<std::string> ();

  const auto &tracks = track_collection_->tracks ();
  const auto  new_ref = std::ranges::find_if (tracks, [&] (const auto &ref) {
    return std::string (
             utils::Utf8String::from_qstring (
               type_safe::get (ref.id ()).toString (QUuid::WithoutBraces))
               .view ())
           == new_uuid;
  });
  ASSERT_NE (new_ref, tracks.end ());
  auto * new_track = new_ref->get ();
  EXPECT_EQ (new_track->type (), structure::tracks::Track::Type::Audio);

  const auto route = track_routing_->get_output_track (new_ref->id ());
  ASSERT_TRUE (route.has_value ());
  EXPECT_EQ (route->id (), singleton_tracks_->masterTrack ()->get_uuid ());

  undo_stack_->undo ();
  EXPECT_EQ (track_collection_->track_count (), count_before);
}

TEST_F (OperationsTest, TrackAddRefusesSingletonType)
{
  auto report = dispatch (
    "track.add",
    {
      { "type", "Master" }
  });

  ASSERT_TRUE (report.refused ());
  EXPECT_NE (report.message.find ("singleton"), std::string::npos);
  EXPECT_EQ (undo_stack_->count (), 0);
}

TEST_F (OperationsTest, TrackAddRefusesUnknownType)
{
  auto report = dispatch (
    "track.add",
    {
      { "type", "audio" }
  });

  ASSERT_TRUE (report.refused ());
  EXPECT_NE (report.message.find ("'audio'"), std::string::npos);
}

TEST_F (OperationsTest, TrackAddAcceptsIntegerEnumValue)
{
  auto report = dispatch (
    "track.add",
    {
      { "type", ENUM_VALUE_TO_INT (structure::tracks::Track::Type::Midi) }
  });

  ASSERT_TRUE (report.applied ()) << report.message;
  EXPECT_EQ (undo_stack_->count (), 1);
  undo_stack_->undo ();
}

TEST_F (OperationsTest, TrackAddRefusesWhenNoMasterTrack)
{
  // the singleton track pointers were never populated in this environment
  singleton_tracks_ = std::make_unique<structure::tracks::SingletonTracks> ();
  environment_ = std::make_unique<OperationEnvironment> (
    registry_, *undo_stack_, *track_collection_, *track_factory_,
    *track_routing_, *singleton_tracks_, transport_.get ());

  const auto count_before = track_collection_->track_count ();
  auto       report = dispatch (
    "track.add",
    {
      { "type", "Audio" }
  });

  ASSERT_TRUE (report.refused ());
  EXPECT_NE (report.message.find ("master track"), std::string::npos);
  EXPECT_EQ (track_collection_->track_count (), count_before);
  EXPECT_EQ (undo_stack_->count (), 0);
}

// ============================================================================
// track.list
// ============================================================================

TEST_F (OperationsTest, TrackListReturnsSummaries)
{
  add_audio_track_directly ()->setName ("Drums");

  auto report = dispatch ("track.list");

  ASSERT_TRUE (report.applied ());
  ASSERT_TRUE (report.result.contains ("tracks"));
  const auto &tracks = report.result.at ("tracks");
  ASSERT_EQ (tracks.size (), 4);
  EXPECT_EQ (tracks.at (3).at ("name").get<std::string> (), "Drums");
  // Enums serialize as integers in this project
  EXPECT_EQ (
    tracks.at (3).at ("type").get<int> (),
    ENUM_VALUE_TO_INT (structure::tracks::Track::Type::Audio));
  EXPECT_EQ (report.result.at ("total").get<int> (), 4);
  EXPECT_EQ (report.revision_after, report.revision_before);
}

TEST_F (OperationsTest, TrackListHonorsLimit)
{
  add_audio_track_directly ();

  auto report = dispatch (
    "track.list",
    {
      { "limit", 2 }
  });

  ASSERT_TRUE (report.applied ());
  EXPECT_EQ (report.result.at ("tracks").size (), 2);
  EXPECT_EQ (report.result.at ("total").get<int> (), 4);
}

TEST_F (OperationsTest, TrackListRefusesBadLimit)
{
  auto report = dispatch (
    "track.list",
    {
      { "limit", 0 }
  });

  ASSERT_TRUE (report.refused ());
}

// ============================================================================
// transport.set_play_state
// ============================================================================

TEST_F (OperationsTest, TransportSetPlayStateAppliesWithoutCommands)
{
  auto report = dispatch (
    "transport.set_play_state",
    {
      { "state", "Rolling" }
  });

  ASSERT_TRUE (report.applied ()) << report.message;
  EXPECT_EQ (transport_->getPlayState (), dsp::Transport::PlayState::Rolling);
  EXPECT_TRUE (report.command_texts.empty ());
  EXPECT_EQ (undo_stack_->count (), 0);
  EXPECT_EQ (report.revision_after, report.revision_before);
}

TEST_F (OperationsTest, TransportSetPlayStateRefusesUnknownState)
{
  auto report = dispatch (
    "transport.set_play_state",
    {
      { "state", "Faster" }
  });

  ASSERT_TRUE (report.refused ());
}

// ============================================================================
// Expected-revision guard
// ============================================================================

TEST_F (OperationsTest, EditRefusesStaleExpectedRevision)
{
  auto * track = add_audio_track_directly ();
  track->setName ("Original");

  auto first = dispatch (
    "track.set_name",
    {
      { "track",    uuid_of (*track) },
      { "new_name", "Intro"          }
  });
  ASSERT_TRUE (first.applied ()) << first.message;
  ASSERT_GT (first.revision_after, first.revision_before);

  auto report = dispatch (
    "track.set_name",
    {
      { "track",    uuid_of (*track) },
      { "new_name", "Drums"          }
  },
    DispatchOptions{ .expected_revision = first.revision_before });
  ASSERT_TRUE (report.refused ());
  EXPECT_NE (report.message.find ("expected revision"), std::string::npos);
  EXPECT_EQ (report.revision_before, first.revision_after);
  EXPECT_EQ (report.revision_after, report.revision_before);
  EXPECT_EQ (track->name (), QString ("Intro"));
  EXPECT_EQ (undo_stack_->count (), 1);

  // with the revision it was told about, the same edit applies
  auto retry = dispatch (
    "track.set_name",
    {
      { "track",    uuid_of (*track) },
      { "new_name", "Drums"          }
  },
    DispatchOptions{ .expected_revision = first.revision_after });
  ASSERT_TRUE (retry.applied ()) << retry.message;
  EXPECT_GT (retry.revision_after, retry.revision_before);
  EXPECT_EQ (track->name (), QString ("Drums"));
}

TEST_F (OperationsTest, QueryAndControlIgnoreExpectedRevision)
{
  auto report =
    dispatch ("track.list", {}, DispatchOptions{ .expected_revision = 99 });
  EXPECT_TRUE (report.applied ());

  report = dispatch (
    "transport.set_play_state",
    {
      { "state", "Rolling" }
  },
    DispatchOptions{ .expected_revision = 99 });
  EXPECT_TRUE (report.applied ());
}

TEST_F (OperationsTest, EditRefusesExpectedRevisionFromBeforeAnUndo)
{
  auto * track = add_audio_track_directly ();
  track->setName ("Original");

  auto edit = dispatch (
    "track.set_name",
    {
      { "track",    uuid_of (*track) },
      { "new_name", "Intro"          }
  });
  ASSERT_TRUE (edit.applied ()) << edit.message;
  undo_stack_->undo ();
  ASSERT_EQ (track->name (), QString ("Original"));

  // the undo moved the revision, so the revision from before it is stale
  auto report = dispatch (
    "track.set_name",
    {
      { "track",    uuid_of (*track) },
      { "new_name", "Drums"          }
  },
    DispatchOptions{ .expected_revision = edit.revision_after });
  ASSERT_TRUE (report.refused ());
  EXPECT_NE (report.message.find ("expected revision"), std::string::npos);
}

// ============================================================================
// Transactions over dispatched edits
// ============================================================================

TEST_F (OperationsTest, DispatchedEditsCommitAsOneUndoStep)
{
  auto * track = add_audio_track_directly ();
  track->setName ("Original");

  auto tx = undo_stack_->begin_transaction (QStringLiteral ("Batch rename"));
  auto first = dispatch (
    "track.set_name",
    {
      { "track",    uuid_of (*track) },
      { "new_name", "Intro"          }
  });
  ASSERT_TRUE (first.applied ()) << first.message;
  auto second = dispatch (
    "track.set_name",
    {
      { "track",    uuid_of (*track) },
      { "new_name", "Drums"          }
  });
  ASSERT_TRUE (second.applied ()) << second.message;
  ASSERT_TRUE (tx->commit ());

  EXPECT_EQ (track->name (), QString ("Drums"));
  EXPECT_EQ (undo_stack_->count (), 1);
  undo_stack_->undo ();
  EXPECT_EQ (track->name (), QString ("Original"));
}

TEST_F (OperationsTest, DispatchedEditsAbortLeavesProjectUnchanged)
{
  auto * track = add_audio_track_directly ();
  track->setName ("Original");

  auto tx = undo_stack_->begin_transaction (QStringLiteral ("Batch rename"));
  dispatch (
    "track.set_name",
    {
      { "track",    uuid_of (*track) },
      { "new_name", "Intro"          }
  });
  const auto revision_before_abort = undo_stack_->revision ();
  ASSERT_TRUE (tx->abort ());

  EXPECT_EQ (track->name (), QString ("Original"));
  EXPECT_EQ (undo_stack_->count (), 0);
  // the revision keeps counting forward after the abort
  EXPECT_GT (undo_stack_->revision (), revision_before_abort);
}

TEST_F (OperationsTest, TransactionAbortsAfterMidwayRefusal)
{
  auto * track = add_audio_track_directly ();
  track->setName ("Original");

  auto tx = undo_stack_->begin_transaction (QStringLiteral ("Batch rename"));
  auto applied = dispatch (
    "track.set_name",
    {
      { "track",    uuid_of (*track) },
      { "new_name", "Intro"          }
  });
  ASSERT_TRUE (applied.applied ());
  auto refused = dispatch (
    "track.set_name",
    {
      { "track",    uuid_of (*track)                 },
      { "new_name", std::string ("Bad\xC3\x28 name") }
  });
  ASSERT_TRUE (refused.refused ());
  ASSERT_TRUE (tx->abort ());

  EXPECT_EQ (track->name (), QString ("Original"));
  EXPECT_EQ (undo_stack_->count (), 0);
}

TEST_F (OperationsTest, InTransactionEditsThreadExpectedRevisionsFromReports)
{
  auto * track = add_audio_track_directly ();
  track->setName ("Original");

  auto tx = undo_stack_->begin_transaction (QStringLiteral ("Batch rename"));
  auto first = dispatch (
    "track.set_name",
    {
      { "track",    uuid_of (*track) },
      { "new_name", "Intro"          }
  });
  ASSERT_TRUE (first.applied ()) << first.message;

  // the first push moved the revision, so the revision from before the
  // transaction is stale now
  auto stale = dispatch (
    "track.set_name",
    {
      { "track",    uuid_of (*track) },
      { "new_name", "Drums"          }
  },
    DispatchOptions{ .expected_revision = first.revision_before });
  EXPECT_TRUE (stale.refused ());

  // each report's revision_after is the revision the next call must pass
  auto second = dispatch (
    "track.set_name",
    {
      { "track",    uuid_of (*track) },
      { "new_name", "Drums"          }
  },
    DispatchOptions{ .expected_revision = first.revision_after });
  ASSERT_TRUE (second.applied ()) << second.message;

  ASSERT_TRUE (tx->commit ());
  EXPECT_EQ (track->name (), QString ("Drums"));
}

// ============================================================================
// Registry-wide conformance sweep
// ============================================================================

TEST_F (OperationsTest, EveryOperationHandlesEmptyArguments)
{
  for (const auto &op : operation_registry_.operations ())
    {
      auto report = operation_registry_.dispatch (
        op.name (), nlohmann::json::object (), *environment_);
      if (report.refused ())
        {
          EXPECT_FALSE (report.message.empty ())
            << op.name () << " refused without a reason";
        }
      else
        {
          EXPECT_TRUE (report.applied ()) << op.name ();
        }
      EXPECT_TRUE (report.command_texts.empty () || report.applied ());
    }
}

TEST_F (OperationsTest, UnknownOperationFails)
{
  auto report = dispatch ("track.does_not_exist");

  ASSERT_TRUE (report.failed ());
  EXPECT_NE (report.message.find ("unknown operation"), std::string::npos);
}

} // namespace zrythm::operations
