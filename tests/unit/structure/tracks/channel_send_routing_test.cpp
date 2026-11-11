// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "plugins/internal_plugin_base.h"
#include "structure/tracks/audio_bus_track.h"
#include "structure/tracks/audio_group_track.h"
#include "structure/tracks/audio_track.h"
#include "structure/tracks/channel_send_routing.h"
#include "structure/tracks/marker_track.h"
#include "structure/tracks/master_track.h"
#include "structure/tracks/midi_group_track.h"
#include "structure/tracks/midi_track.h"
#include "structure/tracks/track_collection.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"

#include "helpers/sidechain_test_plugin.h"

#include "unit/dsp/graph_helpers.h"
#include <gtest/gtest.h>

namespace zrythm::structure::tracks
{

class ChannelSendRoutingTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    collection_ = std::make_unique<TrackCollection> (registry_);
    routing_ = std::make_unique<ChannelSendRouting> (*collection_);
  }

  dsp::TempoMap                  tempo_map_{ units::sample_rate (44100.0) };
  dsp::TempoMapWrapper           tempo_map_wrapper_{ tempo_map_ };
  dsp::graph_test::MockTransport transport_;
  SoloedTracksExistGetter soloed_tracks_exist_getter_{ [] { return false; } };

  utils::ObjectRegistry registry_;

  FinalTrackDependencies dependencies_{
    tempo_map_wrapper_,
    registry_,
    soloed_tracks_exist_getter_,
    {},
  };

  std::unique_ptr<TrackCollection>    collection_;
  std::unique_ptr<ChannelSendRouting> routing_;

  TrackUuidReference add_track (Track::Type type)
  {
    auto ref = [this, type] () -> TrackUuidReference {
      switch (type)
        {
        case Track::Type::Audio:
          return utils::create_object<AudioTrack> (registry_, dependencies_);
        case Track::Type::Midi:
          return utils::create_object<MidiTrack> (registry_, dependencies_);
        case Track::Type::AudioGroup:
          return utils::create_object<AudioGroupTrack> (
            registry_, dependencies_);
        case Track::Type::MidiGroup:
          return utils::create_object<MidiGroupTrack> (registry_, dependencies_);
        case Track::Type::AudioBus:
          return utils::create_object<AudioBusTrack> (registry_, dependencies_);
        case Track::Type::Marker:
          return utils::create_object<MarkerTrack> (registry_, dependencies_);
        case Track::Type::Master:
          return utils::create_object<MasterTrack> (registry_, dependencies_);
        default:
          throw std::invalid_argument ("unsupported track type in fixture");
        }
    }();
    collection_->add_track (ref);
    return ref;
  }
};

TEST_F (ChannelSendRoutingTest, AudioChannelSendsToAudioTargets)
{
  auto source = add_track (Track::Type::Audio);
  auto group = add_track (Track::Type::AudioGroup);
  auto bus = add_track (Track::Type::AudioBus);

  EXPECT_TRUE (routing_->can_send_to_track (*source.get (), *group.get ()));
  EXPECT_TRUE (routing_->can_send_to_track (*source.get (), *bus.get ()));
}

TEST_F (ChannelSendRoutingTest, SignalTypesMustMatch)
{
  auto audio_source = add_track (Track::Type::Audio);
  auto midi_group = add_track (Track::Type::MidiGroup);
  EXPECT_FALSE (
    routing_->can_send_to_track (*audio_source.get (), *midi_group.get ()));

  auto midi_source = add_track (Track::Type::Midi);
  auto audio_group = add_track (Track::Type::AudioGroup);
  EXPECT_FALSE (
    routing_->can_send_to_track (*midi_source.get (), *audio_group.get ()));
  EXPECT_TRUE (
    routing_->can_send_to_track (*midi_source.get (), *midi_group.get ()));
}

TEST_F (ChannelSendRoutingTest, SelfAndChannellessTargetsRejected)
{
  auto source = add_track (Track::Type::Audio);
  auto marker = add_track (Track::Type::Marker);

  EXPECT_FALSE (routing_->can_send_to_track (*source.get (), *source.get ()));
  EXPECT_FALSE (routing_->can_send_to_track (*source.get (), *marker.get ()));
}

TEST_F (ChannelSendRoutingTest, BusTracksAreSendTargetsButNotRouteTargets)
{
  auto source = add_track (Track::Type::Audio);
  auto bus = add_track (Track::Type::AudioBus);

  EXPECT_TRUE (routing_->can_send_to_track (*source.get (), *bus.get ()));
  EXPECT_FALSE (Track::type_can_be_group_target (bus->type ()));
}

TEST_F (ChannelSendRoutingTest, DestinationPortForTrackResolvesProcessorInput)
{
  auto group = add_track (Track::Type::AudioGroup);
  auto port = routing_->destination_port_for_track (*group);
  ASSERT_TRUE (port.has_value ());
  EXPECT_EQ (
    port->id (),
    group->get_track_processor ()->get_stereo_in_port ().get_uuid ());
}

TEST_F (ChannelSendRoutingTest, ResolvePortOwnerFindsTrackAndPluginPorts)
{
  auto source = add_track (Track::Type::Audio);
  auto group = add_track (Track::Type::AudioGroup);

  // Track processor input resolves to the track, without a plugin
  const auto &group_in = group->get_track_processor ()->get_stereo_in_port ();
  auto        track_owner = routing_->resolve_port_owner (group_in.get_uuid ());
  ASSERT_TRUE (track_owner.has_value ());
  EXPECT_EQ (track_owner->track, group.get ());
  EXPECT_EQ (track_owner->plugin, nullptr);

  // A plugin's ports resolve to the track and the plugin; only the
  // sidechain input is a valid send destination
  auto plugin_ref =
    utils::create_object<plugins::SidechainTestPlugin> (registry_, registry_);
  group->channel ()->inserts ()->append_plugin (plugin_ref);
  const auto * plugin = plugin_ref.get ();
  ASSERT_EQ (plugin->get_all_input_ports ().size (), 2u);
  const auto * main_in =
    plugin->get_all_input_ports ().front ().get_object_as<dsp::AudioPort> ();
  const auto * sidechain_in =
    plugin->get_all_input_ports ().at (1).get_object_as<dsp::AudioPort> ();

  auto plugin_owner = routing_->resolve_port_owner (main_in->get_uuid ());
  ASSERT_TRUE (plugin_owner.has_value ());
  EXPECT_EQ (plugin_owner->track, group.get ());
  EXPECT_EQ (plugin_owner->plugin, plugin);

  EXPECT_FALSE (routing_->can_send_to_port (*source, *main_in));
  EXPECT_TRUE (routing_->can_send_to_port (*source, *sidechain_in));

  // A track cannot send to its own plugins
  EXPECT_FALSE (routing_->can_send_to_port (*group, *sidechain_in));
}

TEST_F (ChannelSendRoutingTest, MasterTrackCannotSourceSends)
{
  auto master = add_track (Track::Type::Master);
  auto group = add_track (Track::Type::AudioGroup);

  auto plugin_ref =
    utils::create_object<plugins::SidechainTestPlugin> (registry_, registry_);
  group->channel ()->inserts ()->append_plugin (plugin_ref);
  const auto * sidechain_in =
    plugin_ref->get_all_input_ports ().at (1).get_object_as<dsp::AudioPort> ();

  EXPECT_FALSE (routing_->can_send_to_track (*master.get (), *group.get ()));
  EXPECT_FALSE (routing_->can_send_to_port (*master.get (), *sidechain_in));
}

TEST_F (ChannelSendRoutingTest, SendTargetEntriesIdentifyTrackAndPortDestinations)
{
  auto source = add_track (Track::Type::Audio);
  auto group = add_track (Track::Type::AudioGroup);

  // Track entries set the track and leave the port unset (invalid QVariant)
  const auto targets = routing_->sendTargetsFor (source.get ());
  ASSERT_EQ (targets.size (), 1);
  const auto track_entry = targets.at (0).toMap ();
  EXPECT_EQ (
    track_entry.value (QStringLiteral ("display")).toString (), group->name ());
  EXPECT_EQ (
    track_entry.value (QStringLiteral ("track")).value<Track *> (),
    group.get ());
  EXPECT_FALSE (track_entry.value (QStringLiteral ("port")).isValid ());

  // Sidechain entries set the routing port and the owning track
  auto plugin_ref =
    utils::create_object<plugins::SidechainTestPlugin> (registry_, registry_);
  group->channel ()->inserts ()->append_plugin (plugin_ref);
  const auto * sidechain_in =
    plugin_ref->get_all_input_ports ().at (1).get_object_as<dsp::AudioPort> ();

  const auto targets_with_sidechain = routing_->sendTargetsFor (source.get ());
  ASSERT_EQ (targets_with_sidechain.size (), 2);
  const auto port_entry = targets_with_sidechain.at (1).toMap ();
  EXPECT_EQ (
    port_entry.value (QStringLiteral ("track")).value<Track *> (), group.get ());
  EXPECT_EQ (
    port_entry.value (QStringLiteral ("port")).value<dsp::AudioPort *> (),
    sidechain_in);
}

TEST_F (ChannelSendRoutingTest, SendDestinationKeyTracksTrackTargets)
{
  auto source = add_track (Track::Type::Audio);
  auto group = add_track (Track::Type::AudioGroup);

  auto * send = source->channel ()->sends ()->append_send (false).get ();
  EXPECT_TRUE (routing_->sendDestinationKey (send).isEmpty ());

  auto port = routing_->destination_port_for_track (*group.get ());
  send->set_destination_port (*port);
  EXPECT_EQ (
    routing_->sendDestinationKey (send),
    type_safe::get (group.id ()).toString (QUuid::WithoutBraces));
}

} // namespace zrythm::structure::tracks
