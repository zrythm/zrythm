// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/cv_port.h"
#include "dsp/midi_port.h"
#include "dsp/parameter.h"
#include "dsp/transport.h"
#include "structure/project/port_connections_manager.h"
#include "structure/tracks/audio_group_track.h"
#include "structure/tracks/channel_send_routing.h"
#include "structure/tracks/midi_group_track.h"
#include "structure/tracks/track.h"
#include "structure/tracks/track_collection.h"
#include "structure/tracks/track_factory.h"
#include "structure/tracks/track_routing.h"
#include "structure/tracks/tracklist.h"
#include "undo/undo_stack.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"
#include "utils/utf8_string.h"
#include "utils/uuid_reference.h"

#include "helpers/scoped_qcoreapplication.h"
#include "helpers/sidechain_test_plugin.h"

#include "operations/builtin_operations.h"
#include "operations/operation.h"
#include "operations/operation_dispatcher.h"
#include "operations/operation_environment.h"
#include "operations/operation_registry.h"
#include "operations/port_connection_operations.h"
#include "operations/track_operations.h"
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
    channel_send_routing_ = std::make_unique<
      structure::tracks::ChannelSendRouting> (*track_collection_);

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

    undo_stack_ =
      std::make_unique<undo::UndoStack> ([this] (auto callback, bool) {
        ++engine_request_count_;
        if (
          drop_push_requests_from_ == 0
          || engine_request_count_ < drop_push_requests_from_)
          {
            callback ();
          }
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

    port_connections_manager_ =
      std::make_unique<structure::project::PortConnectionsManager> ();
    environment_ = std::make_unique<OperationEnvironment> (
      registry_, *undo_stack_, *track_collection_, *track_factory_,
      *track_routing_, *channel_send_routing_, *singleton_tracks_,
      *port_connections_manager_, transport_.get ());

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

  /** Registers a standalone CV port owned by the fixture registry. */
  dsp::CVPort * add_cv_port (utils::Utf8String label, dsp::PortFlow flow)
  {
    auto ref =
      utils::create_object<dsp::CVPort> (registry_, std::move (label), flow);
    // the fixture holds the registry reference so the port outlives the
    // test body
    port_refs_.push_back (ref.as_untyped ());
    return ref.get ();
  }

  /** Registers a processor parameter owned by the fixture registry. */
  dsp::ProcessorParameter * add_parameter (const char8_t * unique_id)
  {
    auto ref = utils::create_object<dsp::ProcessorParameter> (
      registry_, registry_, dsp::ProcessorParameter::UniqueId (unique_id),
      dsp::ParameterRange (
        dsp::ParameterRange::Type::Linear, 0.f, 1.f, 0.f, 0.5f),
      utils::Utf8String{ unique_id });
    parameter_refs_.push_back (ref.as_untyped ());
    return ref.get ();
  }

  std::string uuid_of (const structure::tracks::Track &track) const
  {
    return std::string (
      utils::Utf8String::from_qstring (
        type_safe::get (track.get_uuid ()).toString (QUuid::WithoutBraces))
        .view ());
  }

  std::string uuid_of (const dsp::Port &port) const
  {
    return std::string (
      utils::Utf8String::from_qstring (
        port.raw_uuid ().toString (QUuid::WithoutBraces))
        .view ());
  }

  std::string uuid_of (const dsp::ProcessorParameter &parameter) const
  {
    return std::string (
      utils::Utf8String::from_qstring (
        type_safe::get (parameter.get_uuid ()).toString (QUuid::WithoutBraces))
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

  std::unique_ptr<structure::tracks::SingletonTracks>    singleton_tracks_;
  std::unique_ptr<structure::tracks::TrackCollection>    track_collection_;
  std::unique_ptr<structure::tracks::TrackRouting>       track_routing_;
  std::unique_ptr<structure::tracks::ChannelSendRouting> channel_send_routing_;
  std::unique_ptr<structure::tracks::TrackFactory>       track_factory_;
  std::unique_ptr<structure::project::PortConnectionsManager>
                                        port_connections_manager_;
  std::unique_ptr<undo::UndoStack>      undo_stack_;
  dsp::Transport::ConfigProvider        transport_config_;
  std::unique_ptr<dsp::Transport>       transport_;
  std::unique_ptr<OperationEnvironment> environment_;
  OperationRegistry                     operation_registry_;
  std::vector<utils::UuidReference>     port_refs_;
  std::vector<utils::UuidReference>     parameter_refs_;

  /**
   * The fixture's undo stack drops engine requests from this request
   * number onward (1 drops every request); 0 accepts every request.
   */
  int drop_push_requests_from_ = 0;

  /** Number of engine requests the fixture's undo stack has received. */
  int engine_request_count_ = 0;
};

// ============================================================================
// Registry shape
// ============================================================================

TEST_F (OperationsTest, BuiltinOperationsAreRegistered)
{
  const auto expected = std::vector<std::string>{
    "track.set_name",
    "track.set_color",
    "track.unset_color",
    "track.set_comment",
    "track.set_timebase",
    "track.route",
    "track.unroute",
    "track.add_send",
    "track.remove_send",
    "track.set_send_prefader",
    "track.set_send_target_track",
    "track.set_send_target_port",
    "track.clear_send_target",
    "track.add",
    "track.list",
    "transport.set_play_state",
    "port.connect",
    "port.disconnect",
    "port.set_connection_multiplier",
    "port.set_connection_enabled",
    "parameter.set_value",
    "parameter.set_value_live",
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
// track.unset_color
// ============================================================================

TEST_F (OperationsTest, TrackUnsetColorAppliesAndUndoes)
{
  auto * track = add_audio_track_directly ();
  dispatch (
    "track.set_color",
    {
      { "track", uuid_of (*track) },
      { "color", "#00ff88"        }
  });
  ASSERT_TRUE (track->useColor ());

  auto report = dispatch (
    "track.unset_color",
    {
      { "track", uuid_of (*track) }
  });

  ASSERT_TRUE (report.applied ()) << report.message;
  EXPECT_FALSE (track->useColor ());

  undo_stack_->undo ();
  EXPECT_TRUE (track->useColor ());
  EXPECT_EQ (track->color (), QColor ("#00ff88"));
}

TEST_F (OperationsTest, TrackUnsetColorIsIdempotentForDefaultColor)
{
  auto * track = add_audio_track_directly ();
  ASSERT_FALSE (track->useColor ());

  auto report = dispatch (
    "track.unset_color",
    {
      { "track", uuid_of (*track) }
  });

  ASSERT_TRUE (report.applied ());
  EXPECT_TRUE (report.command_texts.empty ());
  // no command was pushed, so the revision did not move
  EXPECT_EQ (report.revision_after, report.revision_before);
  EXPECT_EQ (undo_stack_->count (), 0);
}

// ============================================================================
// track.set_comment
// ============================================================================

TEST_F (OperationsTest, TrackSetCommentAppliesAndUndoes)
{
  auto * track = add_audio_track_directly ();

  auto report = dispatch (
    "track.set_comment",
    {
      { "track",   uuid_of (*track) },
      { "comment", "hello world"    }
  });

  ASSERT_TRUE (report.applied ()) << report.message;
  EXPECT_EQ (track->comment (), QString ("hello world"));

  undo_stack_->undo ();
  EXPECT_TRUE (track->comment ().isEmpty ());
}

TEST_F (OperationsTest, TrackSetCommentClearsWithEmptyString)
{
  auto * track = add_audio_track_directly ();
  dispatch (
    "track.set_comment",
    {
      { "track",   uuid_of (*track) },
      { "comment", "hello world"    }
  });
  ASSERT_EQ (track->comment (), QString ("hello world"));

  auto report = dispatch (
    "track.set_comment",
    {
      { "track",   uuid_of (*track) },
      { "comment", ""               }
  });

  ASSERT_TRUE (report.applied ());
  EXPECT_TRUE (track->comment ().isEmpty ());
}

// ============================================================================
// track.set_timebase
// ============================================================================

TEST_F (OperationsTest, TrackSetTimebaseAppliesAndUndoes)
{
  auto * track = add_audio_track_directly ();
  ASSERT_EQ (
    track->timebaseProvider ()->effectiveTimebase (), dsp::Timebase::Absolute);

  auto report = dispatch (
    "track.set_timebase",
    {
      { "track",    uuid_of (*track) },
      { "timebase", "Musical"        }
  });

  ASSERT_TRUE (report.applied ()) << report.message;
  EXPECT_EQ (
    track->timebaseProvider ()->effectiveTimebase (), dsp::Timebase::Musical);
  EXPECT_EQ (
    track->timebaseProvider ()->overrideValue (),
    std::optional{ dsp::Timebase::Musical });

  undo_stack_->undo ();
  EXPECT_EQ (
    track->timebaseProvider ()->effectiveTimebase (), dsp::Timebase::Absolute);
  EXPECT_EQ (
    track->timebaseProvider ()->overrideValue (),
    std::optional{ dsp::Timebase::Absolute });
}

// ============================================================================
// track.route / track.unroute
// ============================================================================

TEST_F (OperationsTest, TrackRouteAppliesAndUndoes)
{
  auto * track = add_audio_track_directly ();
  auto   group_ref =
    track_factory_->create_empty_track<structure::tracks::AudioGroupTrack> ();
  track_collection_->add_track (group_ref);

  auto report = dispatch (
    "track.route",
    {
      { "track",  uuid_of (*track)            },
      { "target", uuid_of (*group_ref.get ()) }
  });

  ASSERT_TRUE (report.applied ()) << report.message;
  const auto output = track_routing_->get_output_track (track->get_uuid ());
  ASSERT_TRUE (output.has_value ());
  EXPECT_EQ (output->id (), group_ref.id ());
  EXPECT_EQ (undo_stack_->count (), 1);

  undo_stack_->undo ();
  EXPECT_FALSE (
    track_routing_->get_output_track (track->get_uuid ()).has_value ());
}

TEST_F (OperationsTest, TrackRouteRefusesCycleWithReason)
{
  auto * track = add_audio_track_directly ();
  auto   group1_ref =
    track_factory_->create_empty_track<structure::tracks::AudioGroupTrack> ();
  auto group2_ref =
    track_factory_->create_empty_track<structure::tracks::AudioGroupTrack> ();
  track_collection_->add_track (group1_ref);
  track_collection_->add_track (group2_ref);
  dispatch (
    "track.route",
    {
      { "track",  uuid_of (*track)             },
      { "target", uuid_of (*group1_ref.get ()) }
  });
  dispatch (
    "track.route",
    {
      { "track",  uuid_of (*group1_ref.get ()) },
      { "target", uuid_of (*group2_ref.get ()) }
  });

  auto report = dispatch (
    "track.route",
    {
      { "track",  uuid_of (*group2_ref.get ()) },
      { "target", uuid_of (*group1_ref.get ()) }
  });

  ASSERT_TRUE (report.refused ());
  EXPECT_EQ (report.refusal_kind, utils::RefusalKind::CyclicRoute);
  EXPECT_NE (report.message.find ("cycle"), std::string::npos);
  EXPECT_EQ (undo_stack_->count (), 2);
}

TEST_F (OperationsTest, TrackRouteRefusesSignalTypeMismatchWithReason)
{
  auto * track = add_audio_track_directly ();
  auto   midi_group_ref =
    track_factory_->create_empty_track<structure::tracks::MidiGroupTrack> ();
  track_collection_->add_track (midi_group_ref);

  auto report = dispatch (
    "track.route",
    {
      { "track",  uuid_of (*track)                 },
      { "target", uuid_of (*midi_group_ref.get ()) }
  });

  ASSERT_TRUE (report.refused ());
  EXPECT_EQ (report.refusal_kind, utils::RefusalKind::TypeMismatch);
  EXPECT_NE (report.message.find ("outputs"), std::string::npos);
  EXPECT_EQ (undo_stack_->count (), 0);
}

TEST_F (OperationsTest, TrackRouteRefusesMasterSourceWithReason)
{
  auto * master = singleton_tracks_->masterTrack ();
  ASSERT_NE (master, nullptr);
  auto group_ref =
    track_factory_->create_empty_track<structure::tracks::AudioGroupTrack> ();
  track_collection_->add_track (group_ref);

  auto report = dispatch (
    "track.route",
    {
      { "track",  uuid_of (*master)           },
      { "target", uuid_of (*group_ref.get ()) }
  });

  ASSERT_TRUE (report.refused ());
  EXPECT_EQ (report.refusal_kind, utils::RefusalKind::Unsupported);
  EXPECT_NE (report.message.find ("master"), std::string::npos);
}

TEST_F (OperationsTest, TrackUnrouteRemovesRouteAndUndoes)
{
  auto * track = add_audio_track_directly ();
  auto   group_ref =
    track_factory_->create_empty_track<structure::tracks::AudioGroupTrack> ();
  track_collection_->add_track (group_ref);
  dispatch (
    "track.route",
    {
      { "track",  uuid_of (*track)            },
      { "target", uuid_of (*group_ref.get ()) }
  });
  ASSERT_TRUE (
    track_routing_->get_output_track (track->get_uuid ()).has_value ());

  auto report = dispatch (
    "track.unroute",
    {
      { "track", uuid_of (*track) }
  });

  ASSERT_TRUE (report.applied ()) << report.message;
  EXPECT_FALSE (
    track_routing_->get_output_track (track->get_uuid ()).has_value ());

  undo_stack_->undo ();
  const auto output = track_routing_->get_output_track (track->get_uuid ());
  ASSERT_TRUE (output.has_value ());
  EXPECT_EQ (output->id (), group_ref.id ());
}

// ============================================================================
// track.add_send / track.remove_send / track.set_send_prefader
// ============================================================================

TEST_F (OperationsTest, TrackAddSendReturnsSendUuidAndUndoes)
{
  auto * track = add_audio_track_directly ();

  auto report = dispatch (
    "track.add_send",
    {
      { "track",       uuid_of (*track) },
      { "is_prefader", false            }
  });

  ASSERT_TRUE (report.applied ()) << report.message;
  EXPECT_EQ (track->channel ()->sends ()->get_sends ().size (), 1);
  EXPECT_FALSE (report.result.at ("send").get<std::string> ().empty ());

  undo_stack_->undo ();
  EXPECT_EQ (track->channel ()->sends ()->get_sends ().size (), 0);
}

TEST_F (OperationsTest, TrackAddSendRefusesTrackWithoutChannel)
{
  auto * marker = singleton_tracks_->markerTrack ();
  ASSERT_NE (marker, nullptr);

  auto report = dispatch (
    "track.add_send",
    {
      { "track",       uuid_of (*marker) },
      { "is_prefader", false             }
  });

  ASSERT_TRUE (report.refused ());
  EXPECT_NE (report.message.find ("channel"), std::string::npos);
}

TEST_F (OperationsTest, TrackSetSendPrefaderAppliesAndUndoes)
{
  auto * track = add_audio_track_directly ();
  dispatch (
    "track.add_send",
    {
      { "track",       uuid_of (*track) },
      { "is_prefader", false            }
  });
  const auto send_uuid = uuid_to_string (
    type_safe::get (track->channel ()->sends ()->get_sends ().front ().id ()));
  auto * send = track->channel ()->sends ()->get_sends ().front ().get ();
  ASSERT_FALSE (send->isPrefader ());

  auto report = dispatch (
    "track.set_send_prefader",
    {
      { "send",        send_uuid },
      { "is_prefader", true      }
  });

  ASSERT_TRUE (report.applied ()) << report.message;
  EXPECT_TRUE (send->isPrefader ());

  undo_stack_->undo ();
  EXPECT_FALSE (send->isPrefader ());
}

TEST_F (OperationsTest, TrackSetSendPrefaderElidesSameTapPoint)
{
  auto * track = add_audio_track_directly ();
  dispatch (
    "track.add_send",
    {
      { "track",       uuid_of (*track) },
      { "is_prefader", false            }
  });
  ASSERT_EQ (undo_stack_->count (), 1);
  const auto send_uuid = uuid_to_string (
    type_safe::get (track->channel ()->sends ()->get_sends ().front ().id ()));

  auto report = dispatch (
    "track.set_send_prefader",
    {
      { "send",        send_uuid },
      { "is_prefader", false     }
  });

  ASSERT_TRUE (report.applied ());
  EXPECT_TRUE (report.command_texts.empty ());
  EXPECT_EQ (report.revision_after, report.revision_before);
}

TEST_F (OperationsTest, TrackRemoveSendRefusesForeignSend)
{
  auto * track_a = add_audio_track_directly ();
  auto * track_b = add_audio_track_directly ();
  dispatch (
    "track.add_send",
    {
      { "track",       uuid_of (*track_a) },
      { "is_prefader", false              }
  });
  const auto send_uuid = uuid_to_string (
    type_safe::get (track_a->channel ()->sends ()->get_sends ().front ().id ()));

  auto report = dispatch (
    "track.remove_send",
    {
      { "track", uuid_of (*track_b) },
      { "send",  send_uuid          }
  });

  ASSERT_TRUE (report.refused ());
  EXPECT_NE (report.message.find ("belong"), std::string::npos);
  EXPECT_EQ (track_a->channel ()->sends ()->get_sends ().size (), 1);
}

// ============================================================================
// track.set_send_target_track / track.set_send_target_port /
// track.clear_send_target
// ============================================================================

TEST_F (OperationsTest, TrackSetSendTargetTrackRoutesAndUndoes)
{
  auto * track = add_audio_track_directly ();
  auto   group_ref =
    track_factory_->create_empty_track<structure::tracks::AudioGroupTrack> ();
  auto * group = group_ref.get ();
  track_collection_->add_track (group_ref);
  dispatch (
    "track.add_send",
    {
      { "track",       uuid_of (*track) },
      { "is_prefader", false            }
  });
  const auto send_uuid = uuid_to_string (
    type_safe::get (track->channel ()->sends ()->get_sends ().front ().id ()));
  auto * send = track->channel ()->sends ()->get_sends ().front ().get ();

  auto report = dispatch (
    "track.set_send_target_track",
    {
      { "track",        uuid_of (*track) },
      { "send",         send_uuid        },
      { "target_track", uuid_of (*group) }
  });

  ASSERT_TRUE (report.applied ()) << report.message;
  ASSERT_TRUE (send->destination_port ().has_value ());
  EXPECT_EQ (
    send->destination_port ()->id (),
    group->get_track_processor ()->get_stereo_in_port ().get_uuid ());

  undo_stack_->undo ();
  EXPECT_FALSE (send->has_destination ());
}

TEST_F (OperationsTest, TrackSetSendTargetTrackRefusesSignalMismatch)
{
  auto * track = add_audio_track_directly ();
  auto   midi_group_ref =
    track_factory_->create_empty_track<structure::tracks::MidiGroupTrack> ();
  track_collection_->add_track (midi_group_ref);
  dispatch (
    "track.add_send",
    {
      { "track",       uuid_of (*track) },
      { "is_prefader", false            }
  });
  const auto send_uuid = uuid_to_string (
    type_safe::get (track->channel ()->sends ()->get_sends ().front ().id ()));

  auto report = dispatch (
    "track.set_send_target_track",
    {
      { "track",        uuid_of (*track)                 },
      { "send",         send_uuid                        },
      { "target_track", uuid_of (*midi_group_ref.get ()) }
  });

  ASSERT_TRUE (report.refused ());
  EXPECT_NE (report.message.find ("outputs Audio but"), std::string::npos);
}

TEST_F (OperationsTest, TrackSetSendTargetTrackElidesSameDestination)
{
  auto * track = add_audio_track_directly ();
  auto   group_ref =
    track_factory_->create_empty_track<structure::tracks::AudioGroupTrack> ();
  track_collection_->add_track (group_ref);
  dispatch (
    "track.add_send",
    {
      { "track",       uuid_of (*track) },
      { "is_prefader", false            }
  });
  const auto send_uuid = uuid_to_string (
    type_safe::get (track->channel ()->sends ()->get_sends ().front ().id ()));
  dispatch (
    "track.set_send_target_track",
    {
      { "track",        uuid_of (*track)            },
      { "send",         send_uuid                   },
      { "target_track", uuid_of (*group_ref.get ()) }
  });
  ASSERT_EQ (undo_stack_->count (), 2);

  auto report = dispatch (
    "track.set_send_target_track",
    {
      { "track",        uuid_of (*track)            },
      { "send",         send_uuid                   },
      { "target_track", uuid_of (*group_ref.get ()) }
  });

  ASSERT_TRUE (report.applied ());
  EXPECT_TRUE (report.command_texts.empty ());
  EXPECT_EQ (report.revision_after, report.revision_before);
}

TEST_F (OperationsTest, TrackSetSendTargetTrackRefusesForeignSend)
{
  auto * track_a = add_audio_track_directly ();
  auto * track_b = add_audio_track_directly ();
  auto   group_ref =
    track_factory_->create_empty_track<structure::tracks::AudioGroupTrack> ();
  track_collection_->add_track (group_ref);
  dispatch (
    "track.add_send",
    {
      { "track",       uuid_of (*track_b) },
      { "is_prefader", false              }
  });
  const auto send_uuid = uuid_to_string (
    type_safe::get (track_b->channel ()->sends ()->get_sends ().front ().id ()));

  auto report = dispatch (
    "track.set_send_target_track",
    {
      { "track",        uuid_of (*track_a)          },
      { "send",         send_uuid                   },
      { "target_track", uuid_of (*group_ref.get ()) }
  });

  ASSERT_TRUE (report.refused ());
  EXPECT_NE (report.message.find ("belong"), std::string::npos);
}

// A connectivity query's reason reaches the caller verbatim
TEST_F (OperationsTest, TrackSetSendTargetTrackPassesQueryRefusalThrough)
{
  auto * track = add_audio_track_directly ();
  auto   group_ref =
    track_factory_->create_empty_track<structure::tracks::AudioGroupTrack> ();
  track_collection_->add_track (group_ref);
  dispatch (
    "track.add_send",
    {
      { "track",       uuid_of (*track) },
      { "is_prefader", false            }
  });
  const auto send_uuid = uuid_to_string (
    type_safe::get (track->channel ()->sends ()->get_sends ().front ().id ()));

  environment_ = std::make_unique<OperationEnvironment> (
    registry_, *undo_stack_, *track_collection_, *track_factory_,
    *track_routing_, *channel_send_routing_, *singleton_tracks_,
    *port_connections_manager_, transport_.get (),
    [] (const structure::tracks::ChannelSend &, const dsp::Port &) {
      return std::optional<utils::RefusalReason>{
        utils::RefusalReason{
                             utils::RefusalKind::CyclicRoute,
                             utils::Utf8String::from_utf8_encoded_string (
            "the send cannot be wired to this destination") }
      };
    });

  const auto undo_count_before = undo_stack_->count ();
  auto       report = dispatch (
    "track.set_send_target_track",
    {
      { "track",        uuid_of (*track)            },
      { "send",         send_uuid                   },
      { "target_track", uuid_of (*group_ref.get ()) }
  });

  ASSERT_TRUE (report.refused ());
  EXPECT_EQ (report.refusal_kind, utils::RefusalKind::CyclicRoute);
  EXPECT_EQ (report.message, "the send cannot be wired to this destination");
  EXPECT_EQ (undo_stack_->count (), undo_count_before);
}

TEST_F (OperationsTest, TrackClearSendTargetClearsAndUndoes)
{
  auto * track = add_audio_track_directly ();
  auto   group_ref =
    track_factory_->create_empty_track<structure::tracks::AudioGroupTrack> ();
  track_collection_->add_track (group_ref);
  dispatch (
    "track.add_send",
    {
      { "track",       uuid_of (*track) },
      { "is_prefader", false            }
  });
  const auto send_uuid = uuid_to_string (
    type_safe::get (track->channel ()->sends ()->get_sends ().front ().id ()));
  auto * send = track->channel ()->sends ()->get_sends ().front ().get ();
  dispatch (
    "track.set_send_target_track",
    {
      { "track",        uuid_of (*track)            },
      { "send",         send_uuid                   },
      { "target_track", uuid_of (*group_ref.get ()) }
  });
  ASSERT_TRUE (send->has_destination ());

  auto report = dispatch (
    "track.clear_send_target",
    {
      { "track", uuid_of (*track) },
      { "send",  send_uuid        }
  });

  ASSERT_TRUE (report.applied ()) << report.message;
  EXPECT_FALSE (send->has_destination ());

  // clearing a send with no destination elides
  report = dispatch (
    "track.clear_send_target",
    {
      { "track", uuid_of (*track) },
      { "send",  send_uuid        }
  });
  ASSERT_TRUE (report.applied ());
  EXPECT_TRUE (report.command_texts.empty ());
  EXPECT_EQ (report.revision_after, report.revision_before);

  undo_stack_->undo ();
  EXPECT_TRUE (send->has_destination ());
}

TEST_F (OperationsTest, TrackSetSendTargetPortRoutesSidechainAndUndoes)
{
  auto * source = add_audio_track_directly ();
  auto   group_ref =
    track_factory_->create_empty_track<structure::tracks::AudioGroupTrack> ();
  auto * group = group_ref.get ();
  track_collection_->add_track (group_ref);
  auto plugin_ref =
    utils::create_object<plugins::SidechainTestPlugin> (registry_, registry_);
  group->channel ()->inserts ()->append_plugin (plugin_ref);
  auto * sidechain_in =
    plugin_ref->get_all_input_ports ().at (1).get_object_as<dsp::AudioPort> ();
  dispatch (
    "track.add_send",
    {
      { "track",       uuid_of (*source) },
      { "is_prefader", false             }
  });
  const auto send_uuid = uuid_to_string (
    type_safe::get (source->channel ()->sends ()->get_sends ().front ().id ()));
  auto * send = source->channel ()->sends ()->get_sends ().front ().get ();

  auto report = dispatch (
    "track.set_send_target_port",
    {
      { "track",       uuid_of (*source)                           },
      { "send",        send_uuid                                   },
      { "target_port",
       uuid_to_string (type_safe::get (sidechain_in->get_uuid ())) }
  });

  ASSERT_TRUE (report.applied ()) << report.message;
  ASSERT_TRUE (send->destination_port ().has_value ());
  EXPECT_EQ (send->destination_port ()->id (), sidechain_in->get_uuid ());

  undo_stack_->undo ();
  EXPECT_FALSE (send->has_destination ());
}

TEST_F (OperationsTest, TrackSetSendTargetPortRefusesNonSidechainPort)
{
  auto * source = add_audio_track_directly ();
  auto   group_ref =
    track_factory_->create_empty_track<structure::tracks::AudioGroupTrack> ();
  auto * group = group_ref.get ();
  track_collection_->add_track (group_ref);
  dispatch (
    "track.add_send",
    {
      { "track",       uuid_of (*source) },
      { "is_prefader", false             }
  });
  const auto send_uuid = uuid_to_string (
    type_safe::get (source->channel ()->sends ()->get_sends ().front ().id ()));

  // the group's own input is an audio input port, but not a sidechain
  auto report = dispatch (
    "track.set_send_target_port",
    {
      { "track",       uuid_of (*source)                                       },
      { "send",        send_uuid                                               },
      { "target_port",
       uuid_to_string (
          type_safe::get (
            group->get_track_processor ()->get_stereo_in_port ().get_uuid ())) }
  });

  ASSERT_TRUE (report.refused ());
  EXPECT_NE (report.message.find ("not a sidechain input"), std::string::npos);
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

TEST_F (OperationsTest, TrackAddRefusesNumericType)
{
  auto report = dispatch (
    "track.add",
    {
      { "type", ENUM_VALUE_TO_INT (structure::tracks::Track::Type::Midi) }
  });

  ASSERT_TRUE (report.refused ());
  EXPECT_NE (report.message.find ("expected a name"), std::string::npos);
  EXPECT_EQ (undo_stack_->count (), 0);
}

TEST_F (OperationsTest, TrackAddRefusesWhenNoMasterTrack)
{
  // the singleton track pointers were never populated in this environment
  singleton_tracks_ = std::make_unique<structure::tracks::SingletonTracks> ();
  environment_ = std::make_unique<OperationEnvironment> (
    registry_, *undo_stack_, *track_collection_, *track_factory_,
    *track_routing_, *channel_send_routing_, *singleton_tracks_,
    *port_connections_manager_, transport_.get ());

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
  // Enums serialize as their enumerator names
  EXPECT_EQ (
    tracks.at (3).at ("type").get<std::string> (),
    std::string (ENUM_NAME (structure::tracks::Track::Type::Audio)));
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
  EXPECT_EQ (report.refusal_kind, utils::RefusalKind::StaleRevision);
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
// Engine-refused pushes
// ============================================================================

TEST_F (OperationsTest, EditReportsErrorWhenTheEngineRefusesTheCommands)
{
  const auto count_before = track_collection_->track_count ();

  drop_push_requests_from_ = 1;
  auto report = dispatch (
    "track.add",
    {
      { "type", "Audio" }
  });
  ASSERT_TRUE (report.failed ());
  EXPECT_NE (report.message.find ("refused"), std::string::npos);
  EXPECT_TRUE (report.command_texts.empty ());
  EXPECT_TRUE (report.result.is_null ());
  EXPECT_TRUE (report.affected_objects.empty ());
  EXPECT_EQ (track_collection_->track_count (), count_before);
  EXPECT_EQ (undo_stack_->count (), 0);
  EXPECT_EQ (report.revision_after, report.revision_before);

  // the same edit applies once the engine cooperates again
  drop_push_requests_from_ = 0;
  auto retry = dispatch (
    "track.add",
    {
      { "type", "Audio" }
  });
  ASSERT_TRUE (retry.applied ()) << retry.message;
  EXPECT_EQ (track_collection_->track_count (), count_before + 1);
  EXPECT_GT (retry.revision_after, retry.revision_before);
}

TEST_F (OperationsTest, PartiallyRefusedEditReportsErrorWithTheCommandsThatRan)
{
  const auto count_before = track_collection_->track_count ();

  // the track is added, its master routing is refused
  drop_push_requests_from_ = 2;
  auto report = dispatch (
    "track.add",
    {
      { "type", "Audio" }
  });

  ASSERT_TRUE (report.failed ());
  EXPECT_NE (report.message.find ("refused 1"), std::string::npos);
  ASSERT_EQ (report.command_texts.size (), 1);
  EXPECT_EQ (report.command_texts.front (), "Add Track");
  EXPECT_TRUE (report.result.is_null ());
  EXPECT_FALSE (report.affected_objects.empty ());
  EXPECT_EQ (track_collection_->track_count (), count_before + 1);
  EXPECT_EQ (undo_stack_->count (), 1);
  EXPECT_GT (report.revision_after, report.revision_before);

  // the commands that ran still undo
  drop_push_requests_from_ = 0;
  undo_stack_->undo ();
  EXPECT_EQ (track_collection_->track_count (), count_before);
}

// ============================================================================
// port.connect
// ============================================================================

TEST_F (OperationsTest, PortConnectAddsUndoableConnection)
{
  auto * src = add_cv_port (u8"CV out", dsp::PortFlow::Output);
  auto * dest = add_cv_port (u8"CV in", dsp::PortFlow::Input);

  auto report = dispatch (
    "port.connect",
    {
      { "source",      uuid_of (*src)  },
      { "destination", uuid_of (*dest) }
  });

  ASSERT_TRUE (report.applied ()) << report.message;
  EXPECT_TRUE (port_connections_manager_->connection_exists (
    src->get_uuid (), dest->get_uuid ()));
  EXPECT_EQ (undo_stack_->count (), 1);
  EXPECT_GT (report.revision_after, report.revision_before);
  ASSERT_EQ (report.affected_objects.size (), 2);
  EXPECT_EQ (report.affected_objects[0].kind, "port");

  undo_stack_->undo ();
  EXPECT_FALSE (port_connections_manager_->connection_exists (
    src->get_uuid (), dest->get_uuid ()));
}

TEST_F (OperationsTest, PortConnectRefusesInvalidPairings)
{
  auto * src = add_cv_port (u8"CV out", dsp::PortFlow::Output);
  auto * dest = add_cv_port (u8"CV in", dsp::PortFlow::Input);

  const auto self = dispatch (
    "port.connect",
    {
      { "source",      uuid_of (*src) },
      { "destination", uuid_of (*src) }
  });
  ASSERT_TRUE (self.refused ());
  EXPECT_NE (self.message.find ("itself"), std::string::npos);

  const auto direction = dispatch (
    "port.connect",
    {
      { "source",      uuid_of (*dest) },
      { "destination", uuid_of (*src)  }
  });
  ASSERT_TRUE (direction.refused ());
  EXPECT_NE (direction.message.find ("input port"), std::string::npos);

  const auto unknown_port = dispatch (
    "port.connect",
    {
      { "source",      uuid_of (*src)                                      },
      { "destination",
       QUuid::createUuid ().toString (QUuid::WithoutBraces).toStdString () }
  });
  ASSERT_TRUE (unknown_port.refused ());
  EXPECT_NE (unknown_port.message.find ("no port with UUID"), std::string::npos);

  port_connections_manager_->add_default_connection (
    src->get_uuid (), dest->get_uuid (), false);
  const auto duplicate = dispatch (
    "port.connect",
    {
      { "source",      uuid_of (*src)  },
      { "destination", uuid_of (*dest) }
  });
  ASSERT_TRUE (duplicate.refused ());
  EXPECT_NE (duplicate.message.find ("already connected"), std::string::npos);

  EXPECT_EQ (undo_stack_->count (), 0);
}

TEST_F (OperationsTest, PortConnectRefusesWhenGraphRejects)
{
  auto * src = add_cv_port (u8"CV out", dsp::PortFlow::Output);
  auto * dest = add_cv_port (u8"CV in", dsp::PortFlow::Input);

  environment_ = std::make_unique<OperationEnvironment> (
    registry_, *undo_stack_, *track_collection_, *track_factory_,
    *track_routing_, *channel_send_routing_, *singleton_tracks_,
    *port_connections_manager_, transport_.get (), SendConnectivityQuery{},
    [] (const dsp::Port &, const dsp::Port &) {
      return std::optional<utils::RefusalReason>{
        utils::RefusalReason{
                             utils::RefusalKind::CyclicRoute,
                             utils::Utf8String::from_utf8_encoded_string (
            "the wiring would close a signal loop") }
      };
    });

  auto report = dispatch (
    "port.connect",
    {
      { "source",      uuid_of (*src)  },
      { "destination", uuid_of (*dest) }
  });

  ASSERT_TRUE (report.refused ());
  EXPECT_NE (report.message.find ("signal loop"), std::string::npos);
  EXPECT_FALSE (port_connections_manager_->connection_exists (
    src->get_uuid (), dest->get_uuid ()));
  EXPECT_EQ (undo_stack_->count (), 0);
}

// ============================================================================
// port.disconnect
// ============================================================================

TEST_F (OperationsTest, PortDisconnectRemovesAndUndoRestores)
{
  auto * src = add_cv_port (u8"CV out", dsp::PortFlow::Output);
  auto * dest = add_cv_port (u8"CV in", dsp::PortFlow::Input);
  ASSERT_TRUE (
    dispatch (
      "port.connect",
      {
        { "source",      uuid_of (*src)  },
        { "destination", uuid_of (*dest) }
  })
      .applied ());

  auto report = dispatch (
    "port.disconnect",
    {
      { "source",      uuid_of (*src)  },
      { "destination", uuid_of (*dest) }
  });

  ASSERT_TRUE (report.applied ()) << report.message;
  EXPECT_FALSE (port_connections_manager_->connection_exists (
    src->get_uuid (), dest->get_uuid ()));
  EXPECT_EQ (undo_stack_->count (), 2);

  undo_stack_->undo ();
  EXPECT_TRUE (port_connections_manager_->connection_exists (
    src->get_uuid (), dest->get_uuid ()));
}

TEST_F (OperationsTest, PortDisconnectRefusesMissingAndLocked)
{
  auto * src = add_cv_port (u8"CV out", dsp::PortFlow::Output);
  auto * dest = add_cv_port (u8"CV in", dsp::PortFlow::Input);
  auto * locked_dest = add_cv_port (u8"Locked in", dsp::PortFlow::Input);
  port_connections_manager_->add_default_connection (
    src->get_uuid (), locked_dest->get_uuid (), true);

  const auto missing = dispatch (
    "port.disconnect",
    {
      { "source",      uuid_of (*src)  },
      { "destination", uuid_of (*dest) }
  });
  ASSERT_TRUE (missing.refused ());
  EXPECT_NE (missing.message.find ("no connection"), std::string::npos);

  const auto locked = dispatch (
    "port.disconnect",
    {
      { "source",      uuid_of (*src)         },
      { "destination", uuid_of (*locked_dest) }
  });
  ASSERT_TRUE (locked.refused ());
  EXPECT_NE (locked.message.find ("locked"), std::string::npos);
  EXPECT_TRUE (port_connections_manager_->connection_exists (
    src->get_uuid (), locked_dest->get_uuid ()));

  EXPECT_EQ (undo_stack_->count (), 0);
}

// ============================================================================
// port.set_connection_multiplier
// ============================================================================

TEST_F (OperationsTest, PortMultiplierUpdatesMergeAndElide)
{
  auto * src = add_cv_port (u8"CV out", dsp::PortFlow::Output);
  auto * dest = add_cv_port (u8"CV in", dsp::PortFlow::Input);
  ASSERT_TRUE (
    dispatch (
      "port.connect",
      {
        { "source",      uuid_of (*src)  },
        { "destination", uuid_of (*dest) }
  })
      .applied ());
  const auto * conn = port_connections_manager_->get_connection (
    src->get_uuid (), dest->get_uuid ());
  ASSERT_NE (conn, nullptr);

  const auto set_multiplier = [&] (double value) {
    return dispatch (
      "port.set_connection_multiplier",
      {
        { "source",      uuid_of (*src)  },
        { "destination", uuid_of (*dest) },
        { "multiplier",  value           }
    });
  };

  auto first = set_multiplier (0.5);
  ASSERT_TRUE (first.applied ()) << first.message;
  EXPECT_FLOAT_EQ (conn->multiplier_, 0.5f);
  EXPECT_EQ (undo_stack_->count (), 2);

  // consecutive changes within the merge window undo as one step
  auto second = set_multiplier (0.7);
  ASSERT_TRUE (second.applied ()) << second.message;
  EXPECT_FLOAT_EQ (conn->multiplier_, 0.7f);
  EXPECT_EQ (undo_stack_->count (), 2);

  undo_stack_->undo ();
  EXPECT_FLOAT_EQ (conn->multiplier_, 1.f);
  undo_stack_->redo ();
  EXPECT_FLOAT_EQ (conn->multiplier_, 0.7f);

  auto elided = set_multiplier (0.7);
  ASSERT_TRUE (elided.applied ());
  EXPECT_EQ (undo_stack_->count (), 2);
  EXPECT_EQ (elided.revision_after, elided.revision_before);
}

TEST_F (OperationsTest, PortMultiplierRefusesInvalidInput)
{
  auto * src = add_cv_port (u8"CV out", dsp::PortFlow::Output);
  auto * dest = add_cv_port (u8"CV in", dsp::PortFlow::Input);
  auto   midi_dest_ref = utils::create_object<dsp::MidiPort> (
    registry_, u8"MIDI in", dsp::PortFlow::Input);
  port_refs_.push_back (midi_dest_ref.as_untyped ());
  auto * midi_dest = midi_dest_ref.get ();
  ASSERT_TRUE (
    dispatch (
      "port.connect",
      {
        { "source",      uuid_of (*src)  },
        { "destination", uuid_of (*dest) }
  })
      .applied ());
  port_connections_manager_->add_default_connection (
    src->get_uuid (), midi_dest->get_uuid (), false);

  const auto set_multiplier = [&] (const dsp::Port &port, double value) {
    return dispatch (
      "port.set_connection_multiplier",
      {
        { "source",      uuid_of (*src) },
        { "destination", uuid_of (port) },
        { "multiplier",  value          }
    });
  };

  const auto wire_range = set_multiplier (*dest, 2.5);
  ASSERT_TRUE (wire_range.refused ());
  EXPECT_NE (wire_range.message.find ("between -2 and 2"), std::string::npos);

  const auto cv_range = set_multiplier (*dest, 1.5);
  ASSERT_TRUE (cv_range.refused ());
  EXPECT_NE (cv_range.message.find ("between -1 and 1"), std::string::npos);

  const auto midi = set_multiplier (*midi_dest, 0.5);
  ASSERT_TRUE (midi.refused ());
  EXPECT_NE (midi.message.find ("MIDI"), std::string::npos);

  const auto missing =
    set_multiplier (*add_cv_port (u8"Other in", dsp::PortFlow::Input), 0.5);
  ASSERT_TRUE (missing.refused ());
  EXPECT_NE (missing.message.find ("no connection"), std::string::npos);

  EXPECT_EQ (undo_stack_->count (), 1);
}

// ============================================================================
// port.set_connection_enabled
// ============================================================================

TEST_F (OperationsTest, PortEnabledTogglesAndElides)
{
  auto * src = add_cv_port (u8"CV out", dsp::PortFlow::Output);
  auto * dest = add_cv_port (u8"CV in", dsp::PortFlow::Input);
  ASSERT_TRUE (
    dispatch (
      "port.connect",
      {
        { "source",      uuid_of (*src)  },
        { "destination", uuid_of (*dest) }
  })
      .applied ());
  const auto * conn = port_connections_manager_->get_connection (
    src->get_uuid (), dest->get_uuid ());
  ASSERT_NE (conn, nullptr);
  EXPECT_TRUE (conn->enabled_);

  const auto set_enabled = [&] (bool enabled) {
    return dispatch (
      "port.set_connection_enabled",
      {
        { "source",      uuid_of (*src)  },
        { "destination", uuid_of (*dest) },
        { "enabled",     enabled         }
    });
  };

  auto disabled = set_enabled (false);
  ASSERT_TRUE (disabled.applied ()) << disabled.message;
  EXPECT_FALSE (conn->enabled_);
  EXPECT_EQ (undo_stack_->count (), 2);

  undo_stack_->undo ();
  EXPECT_TRUE (conn->enabled_);
  undo_stack_->redo ();
  EXPECT_FALSE (conn->enabled_);

  auto elided = set_enabled (false);
  ASSERT_TRUE (elided.applied ());
  EXPECT_EQ (undo_stack_->count (), 2);
  EXPECT_EQ (elided.revision_after, elided.revision_before);

  auto * locked_dest = add_cv_port (u8"Locked in", dsp::PortFlow::Input);
  port_connections_manager_->add_default_connection (
    src->get_uuid (), locked_dest->get_uuid (), true);
  const auto locked = dispatch (
    "port.set_connection_enabled",
    {
      { "source",      uuid_of (*src)         },
      { "destination", uuid_of (*locked_dest) },
      { "enabled",     false                  }
  });
  ASSERT_TRUE (locked.refused ());
  EXPECT_NE (locked.message.find ("locked"), std::string::npos);
}

// ============================================================================
// parameter.set_value
// ============================================================================

TEST_F (OperationsTest, ParameterSetValueAppliesAndUndoes)
{
  auto * param = add_parameter (u8"gain");
  param->setBaseValue (0.5f);

  auto report = dispatch (
    "parameter.set_value",
    {
      { "parameter", uuid_of (*param) },
      { "value",     0.75             }
  });
  ASSERT_TRUE (report.applied ()) << report.message;
  EXPECT_FLOAT_EQ (param->baseValue (), 0.75f);
  EXPECT_GT (report.revision_after, report.revision_before);
  EXPECT_EQ (undo_stack_->count (), 1);

  undo_stack_->undo ();
  EXPECT_FLOAT_EQ (param->baseValue (), 0.5f);
  undo_stack_->redo ();
  EXPECT_FLOAT_EQ (param->baseValue (), 0.75f);
}

TEST_F (OperationsTest, ParameterSetValueElidesCurrentValue)
{
  auto * param = add_parameter (u8"gain");
  param->setBaseValue (0.5f);

  const auto report = dispatch (
    "parameter.set_value",
    {
      { "parameter", uuid_of (*param) },
      { "value",     0.5              }
  });
  ASSERT_TRUE (report.applied ());
  EXPECT_FLOAT_EQ (param->baseValue (), 0.5f);
  EXPECT_EQ (undo_stack_->count (), 0);
  EXPECT_EQ (report.revision_after, report.revision_before);
}

TEST_F (OperationsTest, ParameterSetValueRefusesOutOfRangeValue)
{
  auto * param = add_parameter (u8"gain");
  param->setBaseValue (0.5f);

  const auto report = dispatch (
    "parameter.set_value",
    {
      { "parameter", uuid_of (*param) },
      { "value",     1.5              }
  });
  ASSERT_TRUE (report.refused ());
  EXPECT_EQ (report.refusal_kind, utils::RefusalKind::OutOfRange);
  EXPECT_FLOAT_EQ (param->baseValue (), 0.5f);
  EXPECT_EQ (undo_stack_->count (), 0);
}

TEST_F (OperationsTest, ParameterSetValueRefusesUnknownParameter)
{
  const auto report = dispatch (
    "parameter.set_value",
    {
      { "parameter", "11111111-2222-3333-4444-555555555555" },
      { "value",     0.5                                    }
  });
  ASSERT_TRUE (report.refused ());
  EXPECT_EQ (report.refusal_kind, utils::RefusalKind::NotFound);
}

TEST_F (OperationsTest, ParameterSetValueRefusesMissingValue)
{
  auto * param = add_parameter (u8"gain");

  const auto report = dispatch (
    "parameter.set_value",
    {
      { "parameter", uuid_of (*param) }
  });
  ASSERT_TRUE (report.refused ());
  EXPECT_EQ (report.refusal_kind, utils::RefusalKind::MissingParameter);
  EXPECT_EQ (undo_stack_->count (), 0);
}

// ============================================================================
// parameter.set_value_live
// ============================================================================

TEST_F (OperationsTest, ParameterSetValueLiveAppliesWithoutCommands)
{
  auto * param = add_parameter (u8"mute");
  param->setBaseValue (0.f);

  const auto report = dispatch (
    "parameter.set_value_live",
    {
      { "parameter", uuid_of (*param) },
      { "value",     1.0              }
  });
  ASSERT_TRUE (report.applied ()) << report.message;
  EXPECT_FLOAT_EQ (param->baseValue (), 1.f);
  EXPECT_EQ (undo_stack_->count (), 0);
  EXPECT_EQ (report.revision_before, report.revision_after);
}

TEST_F (OperationsTest, ParameterSetValueLiveElidesCurrentValue)
{
  auto * param = add_parameter (u8"mute");
  param->setBaseValue (0.5f);

  const auto report = dispatch (
    "parameter.set_value_live",
    {
      { "parameter", uuid_of (*param) },
      { "value",     0.5              }
  });
  ASSERT_TRUE (report.applied ());
  EXPECT_FLOAT_EQ (param->baseValue (), 0.5f);
  EXPECT_TRUE (report.affected_objects.empty ());
}

TEST_F (OperationsTest, ParameterSetValueLiveRefusesOutOfRangeValue)
{
  auto * param = add_parameter (u8"mute");
  param->setBaseValue (0.f);

  const auto report = dispatch (
    "parameter.set_value_live",
    {
      { "parameter", uuid_of (*param) },
      { "value",     -0.5             }
  });
  ASSERT_TRUE (report.refused ());
  EXPECT_EQ (report.refusal_kind, utils::RefusalKind::OutOfRange);
  EXPECT_FLOAT_EQ (param->baseValue (), 0.f);
}

TEST_F (OperationsTest, ParameterSetValueLiveRefusesUnknownParameter)
{
  const auto report = dispatch (
    "parameter.set_value_live",
    {
      { "parameter", "11111111-2222-3333-4444-555555555555" },
      { "value",     0.5                                    }
  });
  ASSERT_TRUE (report.refused ());
  EXPECT_EQ (report.refusal_kind, utils::RefusalKind::NotFound);
}

// ============================================================================
// Typed dispatch (C++ callers)
// ============================================================================

TEST_F (OperationsTest, TypedDispatchAppliesAndRefuses)
{
  auto * track = add_audio_track_directly ();
  track->setName ("Original");

  operations::OperationDispatcher dispatcher (
    operation_registry_, *environment_);
  auto applied = dispatcher.invoke (
    operations::track_set_name_operation (), track,
    utils::Utf8String (u8"Intro"));
  ASSERT_TRUE (applied.applied ()) << applied.message;
  EXPECT_EQ (track->name (), QString ("Intro"));
  EXPECT_GT (applied.revision_after, applied.revision_before);

  // the typed path validates like the JSON path
  auto refused = dispatcher.invoke (
    operations::track_set_name_operation (), track, utils::Utf8String (u8""));
  EXPECT_TRUE (refused.refused ());
  EXPECT_NE (refused.message.find ("new_name"), std::string::npos);
  EXPECT_EQ (track->name (), QString ("Intro"));
  EXPECT_EQ (undo_stack_->count (), 1);

  undo_stack_->undo ();
  EXPECT_EQ (track->name (), QString ("Original"));
}

TEST_F (OperationsTest, TypedPortDispatchAppliesAndRefuses)
{
  dsp::Port * src = add_cv_port (u8"CV out", dsp::PortFlow::Output);
  dsp::Port * dest = add_cv_port (u8"CV in", dsp::PortFlow::Input);

  operations::OperationDispatcher dispatcher (
    operation_registry_, *environment_);
  auto applied =
    dispatcher.invoke (operations::port_connect_operation (), src, dest);
  ASSERT_TRUE (applied.applied ()) << applied.message;
  EXPECT_TRUE (port_connections_manager_->connection_exists (
    src->get_uuid (), dest->get_uuid ()));

  // the typed path validates like the JSON path
  auto refused =
    dispatcher.invoke (operations::port_connect_operation (), src, src);
  EXPECT_TRUE (refused.refused ());
  EXPECT_NE (refused.message.find ("itself"), std::string::npos);
  EXPECT_EQ (undo_stack_->count (), 1);
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
