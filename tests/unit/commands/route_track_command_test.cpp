// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/route_track_command.h"
#include "structure/tracks/track_factory.h"
#include "structure/tracks/track_routing.h"

#include "unit/dsp/graph_helpers.h"
#include <gtest/gtest.h>

namespace zrythm::commands
{

class RouteTrackCommandTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    // Create track routing
    track_routing_ =
      std::make_unique<structure::tracks::TrackRouting> (track_registry_);

    // Create track factory with minimal dependencies
    structure::tracks::FinalTrackDependencies factory_deps{
      tempo_map_wrapper_,   file_audio_source_registry_,
      plugin_registry_,     port_registry_,
      param_registry_,      obj_registry_,
      track_registry_,      transport_,
      [] { return false; },
    };
    track_factory_ =
      std::make_unique<structure::tracks::TrackFactory> (factory_deps);

    // Create test tracks
    source_track_ref_ = track_factory_->create_empty_track (
      structure::tracks::Track::Type::Audio);
    target_track_ref_ = track_factory_->create_empty_track (
      structure::tracks::Track::Type::Audio);
  }

  // Create minimal dependencies for track creation
  dsp::TempoMap                   tempo_map_{ 44100.0 * mp_units::si::hertz };
  dsp::TempoMapWrapper            tempo_map_wrapper_{ tempo_map_ };
  dsp::FileAudioSourceRegistry    file_audio_source_registry_;
  plugins::PluginRegistry         plugin_registry_;
  dsp::PortRegistry               port_registry_;
  dsp::ProcessorParameterRegistry param_registry_{ port_registry_ };
  structure::arrangement::ArrangerObjectRegistry obj_registry_;
  dsp::graph_test::MockTransport                 transport_;

  structure::tracks::TrackRegistry                 track_registry_;
  std::unique_ptr<structure::tracks::TrackRouting> track_routing_;
  std::unique_ptr<structure::tracks::TrackFactory> track_factory_;
  structure::tracks::TrackUuidReference source_track_ref_{ track_registry_ };
  structure::tracks::TrackUuidReference target_track_ref_{ track_registry_ };
};

// Test initial state after construction
TEST_F (RouteTrackCommandTest, InitialState)
{
  RouteTrackCommand command (
    *track_routing_, source_track_ref_.id (), target_track_ref_.id ());

  // Initially, there should be no route between the tracks
  auto route = track_routing_->get_output_track (source_track_ref_.id ());
  EXPECT_FALSE (route.has_value ());
}

// Test redo operation adds route
TEST_F (RouteTrackCommandTest, RedoAddsRoute)
{
  RouteTrackCommand command (
    *track_routing_, source_track_ref_.id (), target_track_ref_.id ());

  command.redo ();

  // After redo, the route should be established
  auto route = track_routing_->get_output_track (source_track_ref_.id ());
  EXPECT_TRUE (route.has_value ());
  EXPECT_EQ (route->id (), target_track_ref_.id ());
}

// Test undo operation removes route
TEST_F (RouteTrackCommandTest, UndoRemovesRoute)
{
  RouteTrackCommand command (
    *track_routing_, source_track_ref_.id (), target_track_ref_.id ());

  // First add the route
  command.redo ();
  auto route = track_routing_->get_output_track (source_track_ref_.id ());
  EXPECT_TRUE (route.has_value ());
  EXPECT_EQ (route->id (), target_track_ref_.id ());

  // Then undo should remove it
  command.undo ();
  route = track_routing_->get_output_track (source_track_ref_.id ());
  EXPECT_FALSE (route.has_value ());
}

// Test multiple undo/redo cycles
TEST_F (RouteTrackCommandTest, MultipleUndoRedoCycles)
{
  RouteTrackCommand command (
    *track_routing_, source_track_ref_.id (), target_track_ref_.id ());

  // First cycle
  command.redo ();
  auto route = track_routing_->get_output_track (source_track_ref_.id ());
  EXPECT_TRUE (route.has_value ());
  EXPECT_EQ (route->id (), target_track_ref_.id ());

  command.undo ();
  route = track_routing_->get_output_track (source_track_ref_.id ());
  EXPECT_FALSE (route.has_value ());

  // Second cycle
  command.redo ();
  route = track_routing_->get_output_track (source_track_ref_.id ());
  EXPECT_TRUE (route.has_value ());
  EXPECT_EQ (route->id (), target_track_ref_.id ());

  command.undo ();
  route = track_routing_->get_output_track (source_track_ref_.id ());
  EXPECT_FALSE (route.has_value ());
}

// Test command text
TEST_F (RouteTrackCommandTest, CommandText)
{
  RouteTrackCommand command (
    *track_routing_, source_track_ref_.id (), target_track_ref_.id ());

  // The command should have the default text "Route Track"
  EXPECT_EQ (command.text (), QString ("Route Track"));
}

// Test replacing existing route
TEST_F (RouteTrackCommandTest, ReplaceExistingRoute)
{
  // Create a third track
  auto third_track_ref =
    track_factory_->create_empty_track (structure::tracks::Track::Type::Audio);

  // Add initial route
  RouteTrackCommand command1 (
    *track_routing_, source_track_ref_.id (), target_track_ref_.id ());
  command1.redo ();

  // Verify initial route exists
  auto route = track_routing_->get_output_track (source_track_ref_.id ());
  EXPECT_TRUE (route.has_value ());
  EXPECT_EQ (route->id (), target_track_ref_.id ());

  // Replace with new route to third track
  RouteTrackCommand command2 (
    *track_routing_, source_track_ref_.id (), third_track_ref.id ());
  command2.redo ();

  // Verify route was replaced
  route = track_routing_->get_output_track (source_track_ref_.id ());
  EXPECT_TRUE (route.has_value ());
  EXPECT_EQ (route->id (), third_track_ref.id ());

  // Undo second command should restore first route
  command2.undo ();
  route = track_routing_->get_output_track (source_track_ref_.id ());
  EXPECT_TRUE (route.has_value ());
  EXPECT_EQ (route->id (), target_track_ref_.id ());

  // Undo first command should remove route completely
  command1.undo ();
  route = track_routing_->get_output_track (source_track_ref_.id ());
  EXPECT_FALSE (route.has_value ());
}

// Test with different track types
TEST_F (RouteTrackCommandTest, DifferentTrackTypes)
{
  // Test with MIDI track as source
  auto midi_track_ref =
    track_factory_->create_empty_track (structure::tracks::Track::Type::Midi);

  RouteTrackCommand midi_command (
    *track_routing_, midi_track_ref.id (), target_track_ref_.id ());

  midi_command.redo ();
  auto route = track_routing_->get_output_track (midi_track_ref.id ());
  EXPECT_TRUE (route.has_value ());
  EXPECT_EQ (route->id (), target_track_ref_.id ());

  midi_command.undo ();
  route = track_routing_->get_output_track (midi_track_ref.id ());
  EXPECT_FALSE (route.has_value ());

  // Test with Instrument track as target
  auto instrument_track_ref = track_factory_->create_empty_track (
    structure::tracks::Track::Type::Instrument);

  RouteTrackCommand instrument_command (
    *track_routing_, source_track_ref_.id (), instrument_track_ref.id ());

  instrument_command.redo ();
  route = track_routing_->get_output_track (source_track_ref_.id ());
  EXPECT_TRUE (route.has_value ());
  EXPECT_EQ (route->id (), instrument_track_ref.id ());

  instrument_command.undo ();
  route = track_routing_->get_output_track (source_track_ref_.id ());
  EXPECT_FALSE (route.has_value ());
}

} // namespace zrythm::commands
