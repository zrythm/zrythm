// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/port_connection_operator.h"
#include "commands/port_connection_commands.h"
#include "dsp/graph.h"
#include "dsp/midi_port.h"
#include "dsp/port.h"
#include "dsp/processor_base.h"
#include "plugins/modulators/engines/macro_engine.h"
#include "plugins/modulators/modulator.h"
#include "plugins/plugin_configuration.h"
#include "structure/project/port_connections_manager.h"
#include "structure/tracks/audio_bus_track.h"
#include "structure/tracks/track_collection.h"
#include "structure/tracks/tracklist.h"
#include "utils/registry_utils.h"

#include "helpers/project_fixture.h"

#include "unit/actions/mock_undo_stack.h"
#include <gtest/gtest.h>

namespace zrythm::actions
{

class PortConnectionOperatorTest : public test_helpers::ProjectTestFixture
{
protected:
  struct BusPorts
  {
    dsp::Port * out = nullptr;
    dsp::Port * in = nullptr;
  };

  dsp::Port * find_port (
    structure::tracks::Track &track,
    dsp::PortFlow             flow,
    dsp::PortType             type)
  {
    auto * processor = track.get_track_processor ();
    if (processor == nullptr)
      return nullptr;
    auto refs =
      flow == dsp::PortFlow::Input
        ? processor->get_attached_input_ports ()
        : processor->get_attached_output_ports ();
    for (const auto &ref : refs)
      {
        auto * port = ref.get ();
        if (port != nullptr && port->type () == type)
          return port;
      }
    return nullptr;
  }

  BusPorts add_audio_bus (const utils::Utf8String &name)
  {
    auto track_ref = project_->track_factory_->create_empty_track<
      structure::tracks::AudioBusTrack> ();
    auto * track = track_ref.get_object_as<structure::tracks::AudioBusTrack> ();
    track->setName (name);
    project_->tracklist ()->collection ()->add_track (track_ref);
    return {
      find_port (*track, dsp::PortFlow::Output, dsp::PortType::Audio),
      find_port (*track, dsp::PortFlow::Input, dsp::PortType::Audio),
    };
  }

  plugins::modulators::Modulator * add_macro ()
  {
    const auto descriptor = plugins::modulators::Modulator::make_descriptor (
      plugins::modulators::MacroEngine::kDescriptorId);
    auto config =
      plugins::PluginConfiguration::create_new_for_descriptor (*descriptor);
    auto plugin_ref = project_->plugin_factory_->create_plugin_from_setting (
      *config,
      plugins::PluginFactory::InstantiationFinishOptions{
        .handler_ = [] (plugins::PluginUuidReference, bool, const QString &) { },
        .handler_context_ = nullptr });
    project_->modulators ()->append_plugin (plugin_ref);
    return plugin_ref.get_object_as<plugins::modulators::Modulator> ();
  }

  dsp::CVPort * macro_cv_out (plugins::modulators::Modulator &macro)
  {
    return qobject_cast<dsp::CVPort *> (
      macro.get_all_output_ports ().front ().get ());
  }

  void SetUp () override
  {
    ProjectTestFixture::SetUp ();
    project_ = create_minimal_project ();
    project_->add_default_tracks ();
    undo_stack_ = create_mock_undo_stack ();
    operator_ =
      std::make_unique<PortConnectionOperator> (*undo_stack_, *project_);
    bus1_ = add_audio_bus (u8"Bus 1");
    bus2_ = add_audio_bus (u8"Bus 2");
    ASSERT_NE (bus1_.out, nullptr);
    ASSERT_NE (bus1_.in, nullptr);
    ASSERT_NE (bus2_.out, nullptr);
    ASSERT_NE (bus2_.in, nullptr);
  }

  void TearDown () override
  {
    operator_.reset ();
    undo_stack_.reset ();
    project_.reset ();
  }

  static QString uuid_of (const dsp::Port &port)
  {
    return port.raw_uuid ().toString (QUuid::WithoutBraces);
  }

  bool connection_exists (const dsp::Port &src, const dsp::Port &dest) const
  {
    return project_->port_connections_manager_->connection_exists (
      src.get_uuid (), dest.get_uuid ());
  }

  std::unique_ptr<structure::project::Project> project_;
  std::unique_ptr<undo::UndoStack>             undo_stack_;
  std::unique_ptr<PortConnectionOperator>      operator_;
  BusPorts                                     bus1_;
  BusPorts                                     bus2_;
};

TEST_F (PortConnectionOperatorTest, ConnectDisconnectUndoRedo)
{
  auto &manager = *project_->port_connections_manager_;

  EXPECT_EQ (
    operator_->connectPorts (uuid_of (*bus1_.out), uuid_of (*bus2_.in)),
    QString ());
  EXPECT_TRUE (connection_exists (*bus1_.out, *bus2_.in));
  EXPECT_EQ (undo_stack_->count (), 1);

  undo_stack_->undo ();
  EXPECT_FALSE (connection_exists (*bus1_.out, *bus2_.in));
  undo_stack_->redo ();
  EXPECT_TRUE (connection_exists (*bus1_.out, *bus2_.in));

  EXPECT_EQ (
    operator_->disconnectPorts (uuid_of (*bus1_.out), uuid_of (*bus2_.in)),
    QString ());
  EXPECT_FALSE (connection_exists (*bus1_.out, *bus2_.in));

  undo_stack_->undo ();
  ASSERT_TRUE (connection_exists (*bus1_.out, *bus2_.in));
  const auto * restored =
    manager.get_connection (bus1_.out->get_uuid (), bus2_.in->get_uuid ());
  ASSERT_NE (restored, nullptr);
  EXPECT_FLOAT_EQ (restored->multiplier_, 1.0f);
  EXPECT_FALSE (restored->locked_);
  EXPECT_TRUE (restored->enabled_);

  undo_stack_->redo ();
  EXPECT_FALSE (connection_exists (*bus1_.out, *bus2_.in));
}

TEST_F (
  PortConnectionOperatorTest,
  DisconnectingRegularDestinationKeepsItAttached)
{
  // Regular processor inputs are statically attached: connecting to and
  // disconnecting from them must not change their attachment state.
  ASSERT_FALSE (bus2_.in->detached ());

  ASSERT_EQ (
    operator_->connectPorts (uuid_of (*bus1_.out), uuid_of (*bus2_.in)),
    QString ());
  EXPECT_FALSE (bus2_.in->detached ());

  ASSERT_EQ (
    operator_->disconnectPorts (uuid_of (*bus1_.out), uuid_of (*bus2_.in)),
    QString ());
  EXPECT_FALSE (connection_exists (*bus1_.out, *bus2_.in));
  EXPECT_FALSE (bus2_.in->detached ());

  undo_stack_->undo ();
  EXPECT_TRUE (connection_exists (*bus1_.out, *bus2_.in));
  EXPECT_FALSE (bus2_.in->detached ());

  undo_stack_->redo ();
  EXPECT_FALSE (connection_exists (*bus1_.out, *bus2_.in));
  EXPECT_FALSE (bus2_.in->detached ());
}

TEST_F (PortConnectionOperatorTest, ValidationErrors)
{
  auto &manager = *project_->port_connections_manager_;

  EXPECT_NE (
    operator_->connectPorts (u8"not-a-uuid", uuid_of (*bus2_.in)), QString ());
  EXPECT_NE (
    operator_->connectPorts (
      QUuid::createUuid ().toString (QUuid::WithoutBraces), uuid_of (*bus2_.in)),
    QString ());
  EXPECT_NE (
    operator_->connectPorts (uuid_of (*bus1_.out), uuid_of (*bus1_.out)),
    QString ());
  EXPECT_NE (
    operator_->connectPorts (uuid_of (*bus1_.in), uuid_of (*bus2_.out)),
    QString ());
  EXPECT_NE (
    operator_->disconnectPorts (uuid_of (*bus1_.out), uuid_of (*bus2_.in)),
    QString ());

  // Signal type mismatch: audio output to a MIDI input.
  dsp::Port * midi_input = nullptr;
  for (const auto &ref : project_->tracklist ()->collection ()->tracks ())
    {
      midi_input =
        find_port (*ref.get (), dsp::PortFlow::Input, dsp::PortType::Midi);
      if (midi_input != nullptr)
        break;
    }
  ASSERT_NE (midi_input, nullptr);
  EXPECT_NE (
    operator_->connectPorts (uuid_of (*bus1_.out), uuid_of (*midi_input)),
    QString ());

  // Duplicate connection.
  ASSERT_EQ (
    operator_->connectPorts (uuid_of (*bus1_.out), uuid_of (*bus2_.in)),
    QString ());
  EXPECT_NE (
    operator_->connectPorts (uuid_of (*bus1_.out), uuid_of (*bus2_.in)),
    QString ());
  EXPECT_EQ (undo_stack_->count (), 1);

  // Locked connections cannot be disconnected.
  manager.add_default_connection (
    bus2_.out->get_uuid (), bus1_.in->get_uuid (), true);
  EXPECT_NE (
    operator_->disconnectPorts (uuid_of (*bus2_.out), uuid_of (*bus1_.in)),
    QString ());
  EXPECT_TRUE (connection_exists (*bus2_.out, *bus1_.in));
}

TEST_F (PortConnectionOperatorTest, CyclicConnectionIsRejected)
{
  ASSERT_EQ (
    operator_->connectPorts (uuid_of (*bus1_.out), uuid_of (*bus2_.in)),
    QString ());
  EXPECT_EQ (undo_stack_->count (), 1);

  EXPECT_NE (
    operator_->connectPorts (uuid_of (*bus2_.out), uuid_of (*bus1_.in)),
    QString ());
  EXPECT_FALSE (connection_exists (*bus2_.out, *bus1_.in));
  EXPECT_EQ (undo_stack_->count (), 1);
}

TEST_F (PortConnectionOperatorTest, CvToParameterConnection)
{
  // CV source: a Macro modulator plugin in the global rack.
  auto * macro = add_macro ();
  ASSERT_NE (macro, nullptr);
  auto * cv_out = macro_cv_out (*macro);
  ASSERT_NE (cv_out, nullptr);

  // Destination: the first bus fader's first parameter modulation input.
  structure::tracks::AudioBusTrack * bus1 = nullptr;
  for (const auto &ref : project_->tracklist ()->collection ()->tracks ())
    {
      bus1 = qobject_cast<structure::tracks::AudioBusTrack *> (ref.get ());
      if (bus1 != nullptr)
        break;
    }
  ASSERT_NE (bus1, nullptr);
  auto * fader = bus1->channel ()->fader ();
  ASSERT_NE (fader, nullptr);
  ASSERT_FALSE (fader->get_parameters ().empty ());
  auto * param = fader->get_parameters ().front ().get ();
  auto * mod_port = param->get_modulation_input_port_ref ().get ();
  ASSERT_NE (mod_port, nullptr);
  EXPECT_TRUE (mod_port->detached ());

  EXPECT_EQ (
    operator_->connectPorts (uuid_of (*cv_out), uuid_of (*mod_port)),
    QString ());
  EXPECT_TRUE (connection_exists (*cv_out, *mod_port));
  EXPECT_FALSE (mod_port->detached ());
  EXPECT_EQ (undo_stack_->count (), 1);

  const auto modulation_input_has_graph_node = [&fader, mod_port] {
    dsp::graph::Graph graph;
    dsp::ProcessorGraphBuilder::add_nodes (graph, *fader);
    return graph.get_nodes ().find_node_for_processable (*mod_port) != nullptr;
  };
  EXPECT_TRUE (modulation_input_has_graph_node ());

  undo_stack_->undo ();
  EXPECT_FALSE (connection_exists (*cv_out, *mod_port));
  EXPECT_TRUE (mod_port->detached ());
  EXPECT_FALSE (modulation_input_has_graph_node ());
  undo_stack_->redo ();
  EXPECT_TRUE (connection_exists (*cv_out, *mod_port));
  EXPECT_FALSE (mod_port->detached ());

  EXPECT_EQ (
    operator_->disconnectPorts (uuid_of (*cv_out), uuid_of (*mod_port)),
    QString ());
  EXPECT_FALSE (connection_exists (*cv_out, *mod_port));
  EXPECT_TRUE (mod_port->detached ());
  EXPECT_FALSE (modulation_input_has_graph_node ());

  undo_stack_->undo ();
  EXPECT_TRUE (connection_exists (*cv_out, *mod_port));
  EXPECT_FALSE (mod_port->detached ());
  undo_stack_->redo ();
  EXPECT_FALSE (connection_exists (*cv_out, *mod_port));
  EXPECT_TRUE (mod_port->detached ());

  auto unrouted_source_ref = utils::create_object<dsp::CVPort> (
    project_->get_registry (), u8"Unrouted CV", dsp::PortFlow::Output);
  auto * unrouted_source = unrouted_source_ref.get_object_as<dsp::CVPort> ();
  ASSERT_NE (unrouted_source, nullptr);
  EXPECT_NE (
    operator_->connectPorts (uuid_of (*unrouted_source), uuid_of (*mod_port)),
    QString ());
  EXPECT_FALSE (connection_exists (*unrouted_source, *mod_port));
  EXPECT_TRUE (mod_port->detached ());
  EXPECT_EQ (undo_stack_->count (), 2);
}

TEST_F (PortConnectionOperatorTest, LastParameterModInputConnectionDetaches)
{
  auto * macro0 = add_macro ();
  auto * macro1 = add_macro ();
  ASSERT_NE (macro0, nullptr);
  ASSERT_NE (macro1, nullptr);
  std::vector<dsp::CVPort *> cv_outputs{
    macro_cv_out (*macro0), macro_cv_out (*macro1)
  };
  ASSERT_NE (cv_outputs[0], nullptr);
  ASSERT_NE (cv_outputs[1], nullptr);

  structure::tracks::AudioBusTrack * bus = nullptr;
  for (const auto &ref : project_->tracklist ()->collection ()->tracks ())
    {
      bus = qobject_cast<structure::tracks::AudioBusTrack *> (ref.get ());
      if (bus != nullptr)
        break;
    }
  ASSERT_NE (bus, nullptr);
  auto * fader = bus->channel ()->fader ();
  ASSERT_NE (fader, nullptr);
  auto * mod_port =
    fader->get_parameters ()
      .front ()
      .get ()
      ->get_modulation_input_port_ref ()
      .get ();
  ASSERT_NE (mod_port, nullptr);

  ASSERT_EQ (
    operator_->connectPorts (uuid_of (*cv_outputs[0]), uuid_of (*mod_port)),
    QString ());
  ASSERT_EQ (
    operator_->connectPorts (uuid_of (*cv_outputs[1]), uuid_of (*mod_port)),
    QString ());
  EXPECT_FALSE (mod_port->detached ());

  ASSERT_EQ (
    operator_->disconnectPorts (uuid_of (*cv_outputs[0]), uuid_of (*mod_port)),
    QString ());
  EXPECT_TRUE (connection_exists (*cv_outputs[1], *mod_port));
  EXPECT_FALSE (mod_port->detached ());

  undo_stack_->undo ();
  EXPECT_TRUE (connection_exists (*cv_outputs[0], *mod_port));
  EXPECT_FALSE (mod_port->detached ());
  undo_stack_->redo ();
  EXPECT_FALSE (connection_exists (*cv_outputs[0], *mod_port));
  EXPECT_FALSE (mod_port->detached ());

  ASSERT_EQ (
    operator_->disconnectPorts (uuid_of (*cv_outputs[1]), uuid_of (*mod_port)),
    QString ());
  EXPECT_TRUE (mod_port->detached ());

  undo_stack_->undo ();
  EXPECT_TRUE (connection_exists (*cv_outputs[1], *mod_port));
  EXPECT_FALSE (mod_port->detached ());
  undo_stack_->redo ();
  EXPECT_FALSE (connection_exists (*cv_outputs[1], *mod_port));
  EXPECT_TRUE (mod_port->detached ());
}

// Multiplier changes apply to the connection, restore on undo and replay
// on redo; consecutive changes to one connection undo as a single step.
TEST_F (PortConnectionOperatorTest, SetConnectionMultiplierUpdatesAndUndoRestores)
{
  ASSERT_EQ (
    operator_->connectPorts (uuid_of (*bus1_.out), uuid_of (*bus2_.in)),
    QString ());

  auto  &manager = *project_->port_connections_manager_;
  auto * conn =
    manager.get_connection (bus1_.out->get_uuid (), bus2_.in->get_uuid ());
  ASSERT_NE (conn, nullptr);
  EXPECT_FLOAT_EQ (conn->multiplier_, 1.f);

  EXPECT_EQ (
    operator_->setConnectionMultiplier (
      uuid_of (*bus1_.out), uuid_of (*bus2_.in), 0.5),
    QString ());
  EXPECT_FLOAT_EQ (conn->multiplier_, 0.5f);
  // a drag gesture pushes more updates on the same connection
  EXPECT_EQ (
    operator_->setConnectionMultiplier (
      uuid_of (*bus1_.out), uuid_of (*bus2_.in), 0.7),
    QString ());
  EXPECT_FLOAT_EQ (conn->multiplier_, 0.7f);
  EXPECT_EQ (undo_stack_->count (), 2);

  undo_stack_->undo ();
  EXPECT_FLOAT_EQ (conn->multiplier_, 1.f);
  undo_stack_->redo ();
  EXPECT_FLOAT_EQ (conn->multiplier_, 0.7f);

  // an unchanged multiplier pushes nothing
  EXPECT_EQ (
    operator_->setConnectionMultiplier (
      uuid_of (*bus1_.out), uuid_of (*bus2_.in), 0.7),
    QString ());
  EXPECT_EQ (undo_stack_->count (), 2);
}

TEST_F (PortConnectionOperatorTest, MultiplierUpdatesAfterMergeWindowDoNotMerge)
{
  ASSERT_EQ (
    operator_->connectPorts (uuid_of (*bus1_.out), uuid_of (*bus2_.in)),
    QString ());

  auto &manager = *project_->port_connections_manager_;
  // A command that has not redone yet has its last redo timestamp at the
  // clock's epoch, which is beyond the merge window.
  commands::UpdatePortConnectionCommand outside_window (
    manager, bus1_.out->get_uuid (), bus2_.in->get_uuid (), 0.4f, true, true);
  commands::UpdatePortConnectionCommand next_update (
    manager, bus1_.out->get_uuid (), bus2_.in->get_uuid (), 0.6f, true, true);

  EXPECT_FALSE (outside_window.mergeWith (&next_update));
}

// Strength is rejected for locked connections, missing connections, MIDI
// destinations and out-of-range values.
TEST_F (PortConnectionOperatorTest, SetConnectionMultiplierRejectsInvalidInput)
{
  auto &manager = *project_->port_connections_manager_;
  ASSERT_EQ (
    operator_->connectPorts (uuid_of (*bus1_.out), uuid_of (*bus2_.in)),
    QString ());

  // audio range is -2.0…2.0
  EXPECT_NE (
    operator_->setConnectionMultiplier (
      uuid_of (*bus1_.out), uuid_of (*bus2_.in), 2.5),
    QString ());
  EXPECT_FLOAT_EQ (
    manager.get_connection (bus1_.out->get_uuid (), bus2_.in->get_uuid ())
      ->multiplier_,
    1.f);
  EXPECT_EQ (undo_stack_->count (), 1);

  // locked connections are read-only
  manager.add_default_connection (
    bus2_.out->get_uuid (), bus1_.in->get_uuid (), true);
  EXPECT_NE (
    operator_->setConnectionMultiplier (
      uuid_of (*bus2_.out), uuid_of (*bus1_.in), 0.5),
    QString ());
  EXPECT_EQ (undo_stack_->count (), 1);

  // missing connection
  EXPECT_NE (
    operator_->setConnectionMultiplier (
      uuid_of (*bus1_.out),
      QUuid::createUuid ().toString (QUuid::WithoutBraces), 0.5),
    QString ());

  // MIDI destinations carry no strength
  auto midi_out_ref = utils::create_object<dsp::MidiPort> (
    project_->get_registry (), u8"Standalone MIDI out", dsp::PortFlow::Output);
  auto midi_in_ref = utils::create_object<dsp::MidiPort> (
    project_->get_registry (), u8"Standalone MIDI in", dsp::PortFlow::Input);
  manager.add_default_connection (
    midi_out_ref.get_object_as<dsp::MidiPort> ()->get_uuid (),
    midi_in_ref.get_object_as<dsp::MidiPort> ()->get_uuid (), false);
  EXPECT_NE (
    operator_->setConnectionMultiplier (
      uuid_of (*midi_out_ref.get_object_as<dsp::MidiPort> ()),
      uuid_of (*midi_in_ref.get_object_as<dsp::MidiPort> ()), 0.5),
    QString ());
}

// Enabled toggling is undoable and idempotent; locked connections reject.
TEST_F (PortConnectionOperatorTest, SetConnectionEnabledTogglesAndUndoRestores)
{
  ASSERT_EQ (
    operator_->connectPorts (uuid_of (*bus1_.out), uuid_of (*bus2_.in)),
    QString ());

  auto  &manager = *project_->port_connections_manager_;
  auto * conn =
    manager.get_connection (bus1_.out->get_uuid (), bus2_.in->get_uuid ());
  ASSERT_NE (conn, nullptr);
  EXPECT_TRUE (conn->enabled_);

  EXPECT_EQ (
    operator_->setConnectionEnabled (
      uuid_of (*bus1_.out), uuid_of (*bus2_.in), false),
    QString ());
  EXPECT_FALSE (conn->enabled_);
  EXPECT_EQ (undo_stack_->count (), 2);

  undo_stack_->undo ();
  EXPECT_TRUE (conn->enabled_);
  undo_stack_->redo ();
  EXPECT_FALSE (conn->enabled_);

  // unchanged state pushes nothing
  EXPECT_EQ (
    operator_->setConnectionEnabled (
      uuid_of (*bus1_.out), uuid_of (*bus2_.in), false),
    QString ());
  EXPECT_EQ (undo_stack_->count (), 2);

  manager.add_default_connection (
    bus2_.out->get_uuid (), bus1_.in->get_uuid (), true);
  EXPECT_NE (
    operator_->setConnectionEnabled (
      uuid_of (*bus2_.out), uuid_of (*bus1_.in), false),
    QString ());
}

} // namespace zrythm::actions
