// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <map>

#include "dsp/modulator_macro_processor.h"
#include "dsp/port.h"
#include "gui/backend/modular_project_model.h"
#include "structure/project/project.h"
#include "structure/project/project_ui_state.h"
#include "structure/tracks/audio_bus_track.h"
#include "structure/tracks/channel.h"
#include "structure/tracks/modulator_track.h"
#include "structure/tracks/track_collection.h"
#include "structure/tracks/track_factory.h"
#include "structure/tracks/tracklist.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSignalSpy>

#include "helpers/project_fixture.h"

#include <gtest/gtest.h>

namespace zrythm::gui
{

class ModularProjectModelTest : public test_helpers::ProjectTestFixture
{
};

namespace
{

bool
has_valid_node_schema (const QVariantMap &node)
{
  const auto required_keys = {
    "nodeId",
    "nodeTitle",
    "nodeSubtitle",
    "nodeType",
    "nodeX",
    "nodeY",
    "accentKind",
    "inputsJson",
    "outputsJson",
    "parametersJson",
    "hasModulatableParameters",
  };
  return std::all_of (
           required_keys.begin (), required_keys.end (),
           [&node] (const char * key) { return node.contains (key); })
         && QJsonDocument::fromJson (node.value ("inputsJson").toByteArray ())
              .isArray ()
         && QJsonDocument::fromJson (node.value ("outputsJson").toByteArray ())
              .isArray ();
}

QString
selected_track_string (const structure::project::ProjectUiState &ui_state)
{
  const auto selected = ui_state.modularViewState ()->selected_track ();
  return selected.has_value ()
           ? type_safe::get (*selected).toString (QUuid::WithoutBraces)
           : QString ();
}

}

TEST_F (ModularProjectModelTest, ProjectProjectionContainsProcessorNodes)
{
  auto project = create_minimal_project ();
  project->add_default_tracks ();
  structure::project::ProjectUiState ui_state (*project, *app_settings_);
  ModularProjectModel                model (*project, ui_state);

  EXPECT_FALSE (model.nodes ().isEmpty ());
  for (const auto &node_variant : model.nodes ())
    {
      EXPECT_TRUE (has_valid_node_schema (node_variant.toMap ()));
      EXPECT_FALSE (
        node_variant.toMap ().value ("nodeType").toString ().isEmpty ());
    }
}

// The channel fader is a projection node in track scope; the passthrough
// stages live in the channel's private registry and are not projected.
TEST_F (ModularProjectModelTest, TrackScopeProjectsTheChannelFaderOnly)
{
  auto project = create_minimal_project ();
  project->add_default_tracks ();

  // The default tracks carry no instrument channel; add one so its MIDI
  // event tap is part of the check below.
  auto instrument_ref = project->track_factory_->create_empty_track<
    structure::tracks::InstrumentTrack> ();
  project->tracklist ()->collection ()->add_track (instrument_ref);

  structure::project::ProjectUiState ui_state (*project, *app_settings_);
  ModularProjectModel                model (*project, ui_state);

  bool checked_channel = false;
  for (const auto &ref : project->tracklist ()->collection ()->tracks ())
    {
      auto * track = ref.get ();
      auto * channel = track->channel ();
      if (channel == nullptr)
        continue;

      const auto fader_id =
        QStringLiteral ("processor:")
        + channel->fader ()->raw_uuid ().toString (QUuid::WithoutBraces);

      model.showTrack (track->uuidString ());
      QStringList node_ids;
      for (const auto &node : model.nodes ())
        node_ids.append (node.toMap ().value ("nodeId").toString ());
      EXPECT_TRUE (node_ids.contains (fader_id)) << fader_id.toStdString ();
      for (const auto * processor : channel->get_builtin_processors ())
        {
          if (processor == channel->fader ())
            continue;
          const auto node_id =
            QStringLiteral ("processor:")
            + processor->raw_uuid ().toString (QUuid::WithoutBraces);
          EXPECT_FALSE (node_ids.contains (node_id)) << node_id.toStdString ();
        }
      checked_channel = true;
    }
  EXPECT_TRUE (checked_channel);
}

// A processor whose output ports already feed the target must not also get a
// processor-level edge to it: the processor node feeds its own ports in the
// graph, and that hop is represented by the port-to-port edge alone.
TEST_F (ModularProjectModelTest, ProcessorLevelFeedsDoNotDuplicatePortEdges)
{
  auto project = create_minimal_project ();
  project->add_default_tracks ();

  const structure::tracks::Track * master = nullptr;
  for (const auto &ref : project->tracklist ()->collection ()->tracks ())
    {
      if (ref.get ()->is_master ())
        master = ref.get ();
    }
  ASSERT_NE (master, nullptr);

  structure::project::ProjectUiState ui_state (*project, *app_settings_);
  ModularProjectModel                model (*project, ui_state);
  model.showTrack (master->uuidString ());

  const auto *  track_processor = master->get_track_processor ();
  const QString tp_node_id =
    QStringLiteral ("processor:")
    + track_processor->raw_uuid ().toString (QUuid::WithoutBraces);
  QStringList tp_out_port_ids;
  for (const auto &port_ref : track_processor->get_attached_output_ports ())
    {
      tp_out_port_ids.append (
        port_ref.get ()->raw_uuid ().toString (QUuid::WithoutBraces));
    }
  ASSERT_FALSE (tp_out_port_ids.isEmpty ());

  QStringList source_ports_of_tp_edges;
  for (const auto &edge_variant : model.edges ())
    {
      const auto edge = edge_variant.toMap ();
      if (edge.value ("sourceNode").toString () == tp_node_id)
        source_ports_of_tp_edges.append (edge.value ("sourcePort").toString ());
    }

  ASSERT_FALSE (source_ports_of_tp_edges.isEmpty ());
  for (const auto &source_port : source_ports_of_tp_edges)
    {
      EXPECT_TRUE (tp_out_port_ids.contains (source_port))
        << source_port.toStdString ();
    }
}

// Routing between track nodes attaches at the tracks' exposed output ports
// and the target's track-processor input ports; the scheduling hop every
// track output also makes into the master track processor produces no edge,
// and processor pseudo ports never appear as edge endpoints.
TEST_F (ModularProjectModelTest, GlobalScopeTrackRoutingUsesExposedPorts)
{
  auto project = create_minimal_project ();
  project->add_default_tracks ();

  // The default tracks carry no channel output routed into master; add an
  // audio bus and route it there.
  auto bus_ref = project->track_factory_->create_empty_track<
    structure::tracks::AudioBusTrack> ();
  auto * bus_track = bus_ref.get_object_as<structure::tracks::AudioBusTrack> ();
  bus_track->setName ("Bus");
  project->tracklist ()->collection ()->add_track (bus_ref);

  const structure::tracks::Track * master = nullptr;
  std::map<QString, QString>       track_out_port_ids;
  for (const auto &ref : project->tracklist ()->collection ()->tracks ())
    {
      auto *     track = ref.get ();
      const auto node_id =
        QStringLiteral ("track:")
        + track->raw_uuid ().toString (QUuid::WithoutBraces);
      if (track->is_master ())
        {
          master = track;
          continue;
        }
      auto * channel = track->channel ();
      if (channel == nullptr)
        continue;
      const dsp::Port * out_port = channel->audioOutPort ();
      if (out_port == nullptr)
        out_port = channel->getMidiOut ();
      if (out_port != nullptr)
        track_out_port_ids.emplace (
          node_id, out_port->raw_uuid ().toString (QUuid::WithoutBraces));
    }
  ASSERT_NE (master, nullptr);
  project->tracklist ()->trackRouting ()->setOutputTrack (bus_track, master);

  structure::project::ProjectUiState ui_state (*project, *app_settings_);
  ModularProjectModel                model (*project, ui_state);
  model.showGlobal ();

  const auto master_node_id =
    QStringLiteral ("track:")
    + master->raw_uuid ().toString (QUuid::WithoutBraces);
  const auto master_in_port_id =
    master->get_track_processor ()->get_stereo_in_port ().raw_uuid ().toString (
      QUuid::WithoutBraces);

  int master_bound_edges = 0;
  for (const auto &edge_variant : model.edges ())
    {
      const auto edge = edge_variant.toMap ();
      EXPECT_NE (
        edge.value ("destinationPort").toString (),
        QStringLiteral ("processor"));

      if (edge.value ("destinationNode").toString () != master_node_id)
        continue;
      ++master_bound_edges;
      EXPECT_EQ (edge.value ("destinationPort").toString (), master_in_port_id);

      const auto out_it =
        track_out_port_ids.find (edge.value ("sourceNode").toString ());
      if (out_it != track_out_port_ids.end ())
        {
          EXPECT_EQ (edge.value ("sourcePort").toString (), out_it->second);
        }
    }
  EXPECT_GT (master_bound_edges, 0);
}

TEST_F (ModularProjectModelTest, ScopeSwitchingProjectsTracksAndKeepsIdsStable)
{
  auto project = create_minimal_project ();
  project->add_default_tracks ();
  structure::project::ProjectUiState ui_state (*project, *app_settings_);
  ModularProjectModel                model (*project, ui_state);

  const auto &tracks = project->tracklist ()->collection ()->tracks ();
  ASSERT_FALSE (tracks.empty ());
  model.showTrack (tracks.front ().get ()->uuidString ());
  ASSERT_FALSE (model.global ());
  EXPECT_FALSE (model.scopeTrackUuid ().isEmpty ());
  EXPECT_EQ (model.scopeTitle (), tracks.front ().get ()->name ());
  // The track processor and the channel fader are the minimal projection.
  EXPECT_GE (model.nodes ().size (), 2);

  QStringList global_ids;
  model.showGlobal ();
  for (const auto &node : model.nodes ())
    global_ids.append (node.toMap ().value ("nodeId").toString ());
  EXPECT_TRUE (model.global ());
  EXPECT_EQ (model.scopeTitle (), QObject::tr ("Global"));
  EXPECT_GE (model.nodes ().size (), 3);

  const auto another_track = std::find_if (
    tracks.begin (), tracks.end (), [&tracks] (const auto &track) {
      return track.get () != tracks.front ().get ()
             && track.get ()->get_track_processor () != nullptr;
    });
  if (another_track != tracks.end ())
    {
      model.showTrack (another_track->get ()->uuidString ());
      EXPECT_EQ (model.scopeTitle (), another_track->get ()->name ());
    }

  model.showGlobal ();
  QStringList rebuilt_global_ids;
  for (const auto &node : model.nodes ())
    rebuilt_global_ids.append (node.toMap ().value ("nodeId").toString ());
  EXPECT_EQ (rebuilt_global_ids, global_ids);
}

TEST_F (ModularProjectModelTest, SetScopeTrackKeepsGlobalScope)
{
  auto project = create_minimal_project ();
  project->add_default_tracks ();
  structure::project::ProjectUiState ui_state (*project, *app_settings_);
  ModularProjectModel                model (*project, ui_state);

  const auto &tracks = project->tracklist ()->collection ()->tracks ();
  const auto  another_track = std::find_if (
    tracks.begin (), tracks.end (), [&tracks] (const auto &track) {
      return track.get () != tracks.front ().get ()
             && track.get ()->get_track_processor () != nullptr;
    });
  ASSERT_NE (another_track, tracks.end ());
  const auto first_uuid = tracks.front ().get ()->uuidString ();
  const auto second_uuid = another_track->get ()->uuidString ();

  model.showGlobal ();
  ASSERT_TRUE (model.global ());
  model.setScopeTrack (second_uuid);
  EXPECT_TRUE (model.global ());
  EXPECT_EQ (model.scopeTrackUuid (), second_uuid);
  EXPECT_EQ (model.scopeTitle (), QObject::tr ("Global"));
  EXPECT_EQ (selected_track_string (ui_state), second_uuid);

  model.setScopeTrack (u8"not-a-uuid");
  EXPECT_EQ (model.scopeTrackUuid (), second_uuid);

  model.showTrack (first_uuid);
  EXPECT_FALSE (model.global ());
  EXPECT_EQ (model.scopeTrackUuid (), first_uuid);
  EXPECT_EQ (
    ui_state.modularViewState ()->scope (),
    structure::project::ModularViewState::Scope::Track);
}

TEST_F (ModularProjectModelTest, AutomaticProjectEdgesAreReadOnly)
{
  auto project = create_minimal_project ();
  project->add_default_tracks ();
  structure::project::ProjectUiState ui_state (*project, *app_settings_);
  ModularProjectModel                model (*project, ui_state);
  model.showGlobal ();

  bool has_project_edge = false;
  for (const auto &edge_variant : model.edges ())
    {
      const auto edge = edge_variant.toMap ();
      if (edge.value ("edgeId").toString ().startsWith (u8"project:"))
        {
          has_project_edge = true;
          EXPECT_TRUE (edge.value ("readOnly").toBool ());
          EXPECT_TRUE (edge.contains ("automatic"));
          // Edges between processor pseudo ports are processing order, not
          // signal routing, and are never projected.
          EXPECT_FALSE (
            edge.value ("sourcePort").toString () == u8"processor"
            && edge.value ("destinationPort").toString () == u8"processor");
        }
    }
  EXPECT_TRUE (has_project_edge);
}

TEST_F (ModularProjectModelTest, TrackInsertionSchedulesProjectionRebuild)
{
  auto project = create_minimal_project ();
  project->add_default_tracks ();
  structure::project::ProjectUiState ui_state (*project, *app_settings_);
  ModularProjectModel                model (*project, ui_state);
  model.showGlobal ();
  const int node_count_before = model.nodes ().size ();

  auto track_ref = project->track_factory_->create_empty_track<
    structure::tracks::AudioBusTrack> ();
  track_ref.get_object_as<structure::tracks::AudioBusTrack> ()->setName (
    u8"Projection Test Bus");
  project->tracklist ()->collection ()->add_track (track_ref);

  QTRY_VERIFY (model.nodes ().size () > node_count_before);
  EXPECT_EQ (model.nodes ().size (), node_count_before + 1);
}

namespace
{

QPointF
find_node_position (const ModularProjectModel &model, const QString &node_id)
{
  for (const auto &node : model.nodes ())
    {
      const auto map = node.toMap ();
      if (map.value ("nodeId").toString () == node_id)
        return QPointF (
          map.value ("nodeX").toReal (), map.value ("nodeY").toReal ());
    }
  return QPointF (-1, -1);
}

struct AudioBusPorts
{
  dsp::Port * out = nullptr;
  dsp::Port * in = nullptr;
};

dsp::Port *
find_port (structure::tracks::Track &track, dsp::PortFlow flow)
{
  auto refs =
    flow == dsp::PortFlow::Input
      ? track.get_track_processor ()->get_attached_input_ports ()
      : track.get_track_processor ()->get_attached_output_ports ();
  for (const auto &ref : refs)
    {
      auto * port = ref.get ();
      if (port != nullptr && port->type () == dsp::PortType::Audio)
        return port;
    }
  return nullptr;
}

AudioBusPorts
add_audio_bus (
  structure::project::Project &project,
  const utils::Utf8String     &name)
{
  auto track_ref = project.track_factory_->create_empty_track<
    structure::tracks::AudioBusTrack> ();
  auto * track = track_ref.get_object_as<structure::tracks::AudioBusTrack> ();
  track->setName (name);
  project.tracklist ()->collection ()->add_track (track_ref);
  return {
    find_port (*track, dsp::PortFlow::Output),
    find_port (*track, dsp::PortFlow::Input),
  };
}

QString
uuid_of (const dsp::Port &port)
{
  return port.raw_uuid ().toString (QUuid::WithoutBraces);
}

QVariantMap
find_edge_by_ports (
  const ModularProjectModel &model,
  const QString             &source_port,
  const QString             &destination_port)
{
  for (const auto &edge : model.edges ())
    {
      const auto map = edge.toMap ();
      if (
        map.value ("sourcePort").toString () == source_port
        && map.value ("destinationPort").toString () == destination_port)
        return map;
    }
  return {};
}

bool
edges_flow_left_to_right (const ModularProjectModel &model)
{
  int checked = 0;
  for (const auto &edge_variant : model.edges ())
    {
      const auto edge = edge_variant.toMap ();
      const auto source =
        find_node_position (model, edge.value ("sourceNode").toString ());
      const auto destination =
        find_node_position (model, edge.value ("destinationNode").toString ());
      if (source.x () < 0 || destination.x () < 0)
        continue;
      if (destination.x () <= source.x ())
        return false;
      ++checked;
    }
  return checked > 0;
}

}

TEST_F (ModularProjectModelTest, TrackNodesCarryTheirTypeAsNodeType)
{
  auto project = create_minimal_project ();
  project->add_default_tracks ();
  structure::project::ProjectUiState ui_state (*project, *app_settings_);
  ModularProjectModel                model (*project, ui_state);
  model.showGlobal ();

  add_audio_bus (*project, u8"Type Test Bus");

  const auto find_type_test_bus = [&model] () {
    return std::find_if (
      model.nodes ().cbegin (), model.nodes ().cend (), [] (const auto &variant) {
        return variant.toMap ().value ("nodeTitle").toString ()
               == u8"Type Test Bus";
      });
  };
  QTRY_VERIFY (find_type_test_bus () != model.nodes ().cend ());
  const auto node = find_type_test_bus ();
  EXPECT_EQ (
    node->toMap ().value ("nodeType").toString (), QObject::tr ("Audio bus"));
}

TEST_F (ModularProjectModelTest, NodePositionsSurviveRebuildAndScopeSwitch)
{
  auto project = create_minimal_project ();
  project->add_default_tracks ();
  structure::project::ProjectUiState ui_state (*project, *app_settings_);
  ModularProjectModel                model (*project, ui_state);

  QString processor_id;
  for (const auto &node : model.nodes ())
    {
      const auto id = node.toMap ().value ("nodeId").toString ();
      if (id.startsWith (u8"processor:"))
        {
          processor_id = id;
          break;
        }
    }
  ASSERT_FALSE (processor_id.isEmpty ());
  model.setNodePosition (processor_id, 555.0, 444.0);

  model.showGlobal ();
  QString track_node_id;
  for (const auto &node : model.nodes ())
    {
      const auto id = node.toMap ().value ("nodeId").toString ();
      if (id.startsWith (u8"track:"))
        {
          track_node_id = id;
          break;
        }
    }
  ASSERT_FALSE (track_node_id.isEmpty ());
  model.setNodePosition (track_node_id, 111.0, 222.0);

  const auto track_uuid = model.scopeTrackUuid ();
  ASSERT_FALSE (track_uuid.isEmpty ());
  model.showTrack (track_uuid);
  EXPECT_EQ (find_node_position (model, processor_id), QPointF (555.0, 444.0));

  model.showGlobal ();
  EXPECT_EQ (find_node_position (model, track_node_id), QPointF (111.0, 222.0));
}

TEST_F (ModularProjectModelTest, UnknownNodePositionsAreIgnored)
{
  auto project = create_minimal_project ();
  project->add_default_tracks ();
  structure::project::ProjectUiState ui_state (*project, *app_settings_);
  ModularProjectModel                model (*project, ui_state);

  model.setNodePosition (u8"unknown-node-1", 10.0, 20.0);
  EXPECT_FALSE (
    ui_state.modularViewState ()
      ->find_node_position (u8"unknown-node-1")
      .has_value ());
}

TEST_F (ModularProjectModelTest, UnlockedCustomConnectionIsEditable)
{
  auto project = create_minimal_project ();
  project->add_default_tracks ();
  structure::project::ProjectUiState ui_state (*project, *app_settings_);
  ModularProjectModel                model (*project, ui_state);
  model.showGlobal ();

  const auto bus1 = add_audio_bus (*project, u8"Bus 1");
  const auto bus2 = add_audio_bus (*project, u8"Bus 2");
  ASSERT_NE (bus1.out, nullptr);
  ASSERT_NE (bus2.in, nullptr);

  project->port_connections_manager_->add_default_connection (
    bus1.out->get_uuid (), bus2.in->get_uuid (), false);

  const auto source_uuid = uuid_of (*bus1.out);
  const auto dest_uuid = uuid_of (*bus2.in);
  QTRY_VERIFY (!find_edge_by_ports (model, source_uuid, dest_uuid).isEmpty ());

  const auto edge = find_edge_by_ports (model, source_uuid, dest_uuid);
  EXPECT_TRUE (edge.value ("edgeId").toString ().startsWith (u8"project:"));
  EXPECT_FALSE (edge.value ("readOnly").toBool ());
  EXPECT_FALSE (edge.value ("automatic").toBool ());
}

// Project edges expose the connection's multiplier and enabled state, and
// the projection picks up changes to them.
TEST_F (ModularProjectModelTest, ProjectEdgesExposeMultiplierAndEnabled)
{
  auto project = create_minimal_project ();
  project->add_default_tracks ();
  structure::project::ProjectUiState ui_state (*project, *app_settings_);
  ModularProjectModel                model (*project, ui_state);
  model.showGlobal ();

  const auto bus1 = add_audio_bus (*project, u8"Bus 1");
  const auto bus2 = add_audio_bus (*project, u8"Bus 2");
  ASSERT_NE (bus1.out, nullptr);
  ASSERT_NE (bus2.in, nullptr);

  const auto source_uuid = uuid_of (*bus1.out);
  const auto dest_uuid = uuid_of (*bus2.in);
  project->port_connections_manager_->add_default_connection (
    bus1.out->get_uuid (), bus2.in->get_uuid (), false);

  QTRY_VERIFY (!find_edge_by_ports (model, source_uuid, dest_uuid).isEmpty ());
  {
    const auto edge = find_edge_by_ports (model, source_uuid, dest_uuid);
    EXPECT_FLOAT_EQ (edge.value ("multiplier").toFloat (), 1.f);
    EXPECT_TRUE (edge.value ("connectionEnabled").toBool ());
  }

  project->port_connections_manager_->update_connection (
    bus1.out->get_uuid (), bus2.in->get_uuid (), 0.5f, false, false);
  QTRY_COMPARE (
    find_edge_by_ports (model, source_uuid, dest_uuid)
      .value ("multiplier")
      .toFloat (),
    0.5f);
  EXPECT_FALSE (
    find_edge_by_ports (model, source_uuid, dest_uuid)
      .value ("connectionEnabled")
      .toBool ());
}

TEST_F (ModularProjectModelTest, LockedCustomConnectionIsReadOnly)
{
  auto project = create_minimal_project ();
  project->add_default_tracks ();
  structure::project::ProjectUiState ui_state (*project, *app_settings_);
  ModularProjectModel                model (*project, ui_state);
  model.showGlobal ();

  const auto bus1 = add_audio_bus (*project, u8"Bus 1");
  const auto bus2 = add_audio_bus (*project, u8"Bus 2");
  ASSERT_NE (bus1.out, nullptr);
  ASSERT_NE (bus2.in, nullptr);

  project->port_connections_manager_->add_default_connection (
    bus1.out->get_uuid (), bus2.in->get_uuid (), true);

  const auto source_uuid = uuid_of (*bus1.out);
  const auto dest_uuid = uuid_of (*bus2.in);
  QTRY_VERIFY (!find_edge_by_ports (model, source_uuid, dest_uuid).isEmpty ());

  const auto edge = find_edge_by_ports (model, source_uuid, dest_uuid);
  EXPECT_TRUE (edge.value ("readOnly").toBool ());
  EXPECT_FALSE (edge.value ("automatic").toBool ());
}

TEST_F (ModularProjectModelTest, PositionsAreAppliedAfterProjectLoad)
{
  auto project = create_minimal_project ();
  project->add_default_tracks ();
  structure::project::ProjectUiState ui_state (*project, *app_settings_);
  ModularProjectModel                model (*project, ui_state);
  model.showGlobal ();

  QString track_node_id;
  for (const auto &node : model.nodes ())
    {
      const auto id = node.toMap ().value ("nodeId").toString ();
      if (id.startsWith (u8"track:"))
        {
          track_node_id = id;
          break;
        }
    }
  ASSERT_FALSE (track_node_id.isEmpty ());
  const auto before_load = find_node_position (model, track_node_id);

  // Serialize a saved state holding a different position, then load it into
  // the state the model already projects (the real load order).
  structure::project::ProjectUiState saved_state (*project, *app_settings_);
  saved_state.modularViewState ()->set_node_position (
    track_node_id, QPointF (777.0, 888.0));
  nlohmann::json j;
  to_json (j, saved_state);
  from_json (j, ui_state);

  QTRY_COMPARE (find_node_position (model, track_node_id), QPointF (777, 888));
  EXPECT_NE (find_node_position (model, track_node_id), before_load);
}

TEST_F (ModularProjectModelTest, ViewportAndScopeArePersistedAcrossLoad)
{
  auto project = create_minimal_project ();
  project->add_default_tracks ();
  structure::project::ProjectUiState ui_state (*project, *app_settings_);
  ModularProjectModel                model (*project, ui_state);

  const auto &tracks = project->tracklist ()->collection ()->tracks ();
  const auto  another_track = std::find_if (
    tracks.begin (), tracks.end (), [&tracks] (const auto &track) {
      return track.get () != tracks.front ().get ()
             && track.get ()->get_track_processor () != nullptr;
    });
  ASSERT_NE (another_track, tracks.end ());
  const auto track_uuid = another_track->get ()->uuidString ();

  model.showTrack (track_uuid);
  model.setViewport (10.5, 20.5, 1.5);
  model.showGlobal ();
  model.setViewport (-30.0, 40.0, 0.75);

  nlohmann::json j;
  to_json (j, ui_state);
  structure::project::ProjectUiState restored (*project, *app_settings_);
  from_json (j, restored);

  auto * restored_state = restored.modularViewState ();
  EXPECT_EQ (
    restored_state->scope (),
    structure::project::ModularViewState::Scope::Global);
  EXPECT_EQ (selected_track_string (restored), track_uuid);
  const auto track_vp = restored_state->find_viewport (u8"track:" + track_uuid);
  ASSERT_TRUE (track_vp.has_value ());
  EXPECT_DOUBLE_EQ (track_vp->pan_x, 10.5);
  EXPECT_DOUBLE_EQ (track_vp->pan_y, 20.5);
  EXPECT_DOUBLE_EQ (track_vp->zoom, 1.5);
  const auto global_vp = restored_state->find_viewport (u8"global");
  ASSERT_TRUE (global_vp.has_value ());
  EXPECT_DOUBLE_EQ (global_vp->pan_x, -30.0);
  EXPECT_DOUBLE_EQ (global_vp->zoom, 0.75);

  ModularProjectModel resumed (*project, restored);
  EXPECT_TRUE (resumed.global ());
  EXPECT_EQ (resumed.scopeTrackUuid (), track_uuid);
  EXPECT_DOUBLE_EQ (resumed.viewport ().value (u8"panX").toDouble (), -30.0);

  restored.modularViewState ()->set_scope (
    structure::project::ModularViewState::Scope::Track);
  ModularProjectModel resumed_track (*project, restored);
  EXPECT_FALSE (resumed_track.global ());
  EXPECT_EQ (resumed_track.scopeTrackUuid (), track_uuid);
}

// The model adopts the scope persisted with the project when the UI state is
// restored into a live session.
TEST_F (ModularProjectModelTest, ScopeIsResumedAfterProjectLoad)
{
  auto project = create_minimal_project ();
  project->add_default_tracks ();
  structure::project::ProjectUiState ui_state (*project, *app_settings_);
  ModularProjectModel                model (*project, ui_state);
  EXPECT_FALSE (model.global ());

  const auto &tracks = project->tracklist ()->collection ()->tracks ();
  const auto  another_track = std::find_if (
    tracks.begin (), tracks.end (), [&tracks] (const auto &track) {
      return track.get () != tracks.front ().get ()
             && track.get ()->get_track_processor () != nullptr;
    });
  ASSERT_NE (another_track, tracks.end ());
  const auto track_uuid = another_track->get ()->uuidString ();

  structure::project::ProjectUiState saved_state (*project, *app_settings_);
  saved_state.modularViewState ()->set_scope (
    structure::project::ModularViewState::Scope::Global);
  saved_state.modularViewState ()->set_selected_track (
    structure::project::ModularViewState::TrackUuid (QUuid (track_uuid)));
  nlohmann::json j;
  to_json (j, saved_state);
  from_json (j, ui_state);

  EXPECT_TRUE (model.global ());
  EXPECT_EQ (model.scopeTrackUuid (), track_uuid);
}

// When the scoped track is removed, the same rebuild falls back to another
// track instead of projecting an empty graph.
TEST_F (ModularProjectModelTest, RemovedScopeTrackFallsBackInSameRebuild)
{
  auto project = create_minimal_project ();
  project->add_default_tracks ();
  structure::project::ProjectUiState ui_state (*project, *app_settings_);
  ModularProjectModel                model (*project, ui_state);

  auto fallback_ref = project->track_factory_->create_empty_track<
    structure::tracks::AudioBusTrack> ();
  fallback_ref.get_object_as<structure::tracks::AudioBusTrack> ()->setName (
    u8"Fallback Bus");
  project->tracklist ()->collection ()->add_track (fallback_ref);

  const auto scope_uuid = model.scopeTrackUuid ();
  ASSERT_FALSE (scope_uuid.isEmpty ());
  project->tracklist ()->collection ()->remove_track (
    structure::tracks::TrackUuid (QUuid (scope_uuid)));

  QTRY_VERIFY (model.scopeTrackUuid () != scope_uuid);
  EXPECT_FALSE (model.scopeTrackUuid ().isEmpty ());
  EXPECT_FALSE (model.nodes ().isEmpty ());
}

TEST_F (ModularProjectModelTest, TrackScopeViewportIsStoredPerTrack)
{
  auto project = create_minimal_project ();
  project->add_default_tracks ();
  structure::project::ProjectUiState ui_state (*project, *app_settings_);
  ModularProjectModel                model (*project, ui_state);

  const auto &tracks = project->tracklist ()->collection ()->tracks ();
  const auto  second_track = std::find_if (
    tracks.begin (), tracks.end (), [&tracks] (const auto &track) {
      return track.get () != tracks.front ().get ()
             && track.get ()->get_track_processor () != nullptr;
    });
  ASSERT_NE (second_track, tracks.end ());
  const auto first_uuid = tracks.front ().get ()->uuidString ();
  const auto second_uuid = second_track->get ()->uuidString ();

  model.showTrack (first_uuid);
  model.setViewport (11.0, 21.0, 1.1);
  model.showTrack (second_uuid);
  // The second track has no stored viewport of its own yet.
  EXPECT_TRUE (model.viewport ().isEmpty ());
  model.setViewport (22.0, 42.0, 2.2);

  // Switching back restores each track's own viewport.
  model.showTrack (first_uuid);
  EXPECT_DOUBLE_EQ (model.viewport ().value (u8"panX").toDouble (), 11.0);
  EXPECT_DOUBLE_EQ (model.viewport ().value (u8"panY").toDouble (), 21.0);
  EXPECT_DOUBLE_EQ (model.viewport ().value (u8"zoom").toDouble (), 1.1);
  model.showTrack (second_uuid);
  EXPECT_DOUBLE_EQ (model.viewport ().value (u8"panX").toDouble (), 22.0);
  EXPECT_DOUBLE_EQ (model.viewport ().value (u8"zoom").toDouble (), 2.2);

  // The global viewport is independent of both tracks.
  model.showGlobal ();
  model.setViewport (99.0, 99.0, 3.0);
  model.showTrack (first_uuid);
  EXPECT_DOUBLE_EQ (model.viewport ().value (u8"panX").toDouble (), 11.0);
}

TEST_F (ModularProjectModelTest, LayoutPositionsNodesAlongSignalFlow)
{
  auto project = create_minimal_project ();
  project->add_default_tracks ();
  structure::project::ProjectUiState ui_state (*project, *app_settings_);
  ModularProjectModel                model (*project, ui_state);

  model.showGlobal ();
  EXPECT_TRUE (edges_flow_left_to_right (model));

  const auto &tracks = project->tracklist ()->collection ()->tracks ();
  ASSERT_FALSE (tracks.empty ());
  model.showTrack (tracks.front ().get ()->uuidString ());
  EXPECT_FALSE (model.global ());
  EXPECT_TRUE (edges_flow_left_to_right (model));
}

TEST_F (ModularProjectModelTest, AutoArrangeReplacesStoredPositions)
{
  auto project = create_minimal_project ();
  project->add_default_tracks ();
  structure::project::ProjectUiState ui_state (*project, *app_settings_);
  ModularProjectModel                model (*project, ui_state);

  QString processor_id;
  for (const auto &node : model.nodes ())
    {
      const auto id = node.toMap ().value ("nodeId").toString ();
      if (id.startsWith (u8"processor:"))
        {
          processor_id = id;
          break;
        }
    }
  ASSERT_FALSE (processor_id.isEmpty ());

  model.setNodePosition (processor_id, 555.0, 444.0);
  model.arrangeAutomatically ();
  EXPECT_FALSE (
    ui_state.modularViewState ()->find_node_position (processor_id).has_value ());
  EXPECT_NE (find_node_position (model, processor_id), QPointF (555.0, 444.0));
  EXPECT_TRUE (edges_flow_left_to_right (model));
}

TEST_F (ModularProjectModelTest, ModulatableParametersAreExposed)
{
  auto project = create_minimal_project ();
  project->add_default_tracks ();
  structure::project::ProjectUiState ui_state (*project, *app_settings_);
  ModularProjectModel                model (*project, ui_state);

  QString processor_id;
  for (const auto &node : model.nodes ())
    {
      const auto id = node.toMap ().value ("nodeId").toString ();
      if (id.startsWith (u8"processor:"))
        {
          processor_id = id;
          break;
        }
    }
  ASSERT_FALSE (processor_id.isEmpty ());
  const auto params = model.modulatableParameters (processor_id);
  ASSERT_FALSE (params.isEmpty ());
  for (const auto &entry_variant : params)
    {
      const auto entry = entry_variant.toMap ();
      EXPECT_FALSE (entry.value ("label").toString ().isEmpty ());
      EXPECT_FALSE (QUuid (entry.value ("portId").toString ()).isNull ());
      EXPECT_EQ (entry.value ("kind").toString (), u8"parameter");
    }

  model.showGlobal ();
  QString track_node_id;
  for (const auto &node : model.nodes ())
    {
      const auto id = node.toMap ().value ("nodeId").toString ();
      if (id.startsWith (u8"track:"))
        {
          track_node_id = id;
          break;
        }
    }
  ASSERT_FALSE (track_node_id.isEmpty ());
  EXPECT_FALSE (model.modulatableParameters (track_node_id).isEmpty ());

  EXPECT_TRUE (model.modulatableParameters (u8"nonexistent").isEmpty ());
}

TEST_F (ModularProjectModelTest, CvConnectionToParameterIsEditableEdge)
{
  auto project = create_minimal_project ();
  project->add_default_tracks ();
  structure::project::ProjectUiState ui_state (*project, *app_settings_);
  ModularProjectModel                model (*project, ui_state);
  model.showGlobal ();
  add_audio_bus (*project, u8"Bus 1");

  // CV source: the first modulator macro processor's CV output.
  dsp::CVPort * cv_out = nullptr;
  for (const auto &ref : project->tracklist ()->collection ()->tracks ())
    {
      auto * mod_track =
        qobject_cast<structure::tracks::ModulatorTrack *> (ref.get ());
      if (
        mod_track != nullptr
        && !mod_track->get_modulator_macro_processors ().empty ())
        {
          cv_out =
            &mod_track->get_modulator_macro_processors ()
               .front ()
               ->get_cv_out_port ();
          break;
        }
    }
  ASSERT_NE (cv_out, nullptr);

  // Destination: the first bus fader's first parameter modulation input.
  structure::tracks::AudioBusTrack * bus = nullptr;
  for (const auto &ref : project->tracklist ()->collection ()->tracks ())
    {
      bus = qobject_cast<structure::tracks::AudioBusTrack *> (ref.get ());
      if (bus != nullptr)
        break;
    }
  ASSERT_NE (bus, nullptr);
  auto * fader = bus->channel ()->fader ();
  ASSERT_NE (fader, nullptr);
  ASSERT_FALSE (fader->get_parameters ().empty ());
  auto * mod_port =
    fader->get_parameters ()
      .front ()
      .get ()
      ->get_modulation_input_port_ref ()
      .get ();
  ASSERT_NE (mod_port, nullptr);

  mod_port->set_detached (false);
  project->port_connections_manager_->add_default_connection (
    cv_out->get_uuid (), mod_port->get_uuid (), false);

  const auto source_uuid = uuid_of (*cv_out);
  const auto dest_uuid = uuid_of (*mod_port);
  QTRY_VERIFY (!find_edge_by_ports (model, source_uuid, dest_uuid).isEmpty ());

  const auto edge = find_edge_by_ports (model, source_uuid, dest_uuid);
  EXPECT_FALSE (edge.value ("readOnly").toBool ());
  EXPECT_EQ (edge.value ("signalKind").toString (), u8"parameter");

  // The destination is shown as a parameter slot, not as a CV input port.
  const auto track_node_id = "track:" + bus->uuidString ();
  const auto node_it = std::find_if (
    model.nodes ().cbegin (), model.nodes ().cend (),
    [&track_node_id] (const auto &node_variant) {
      return node_variant.toMap ().value ("nodeId").toString () == track_node_id;
    });
  ASSERT_NE (node_it, model.nodes ().cend ());
  const auto parameters = QJsonDocument::fromJson (
    node_it->toMap ().value ("parametersJson").toByteArray ());
  ASSERT_TRUE (parameters.isArray ());
  bool slot_found = false;
  for (const auto &slot : parameters.array ())
    {
      const auto slot_map = slot.toObject ();
      ASSERT_EQ (slot_map.value ("kind").toString (), u8"parameter");
      if (slot_map.value ("id").toString () == dest_uuid)
        {
          slot_found = true;
          EXPECT_FALSE (slot_map.value ("label").toString ().isEmpty ());
        }
    }
  EXPECT_TRUE (slot_found);
  EXPECT_TRUE (node_it->toMap ().value ("hasModulatableParameters").toBool ());

  // Parameter modulation inputs are parameter slots, never CV input ports.
  const auto inputs = QJsonDocument::fromJson (
    node_it->toMap ().value ("inputsJson").toByteArray ());
  ASSERT_TRUE (inputs.isArray ());
  EXPECT_FALSE (inputs.array ().contains (QJsonValue (dest_uuid)));
}

// Connected parameter slots expose their connections (source port and
// amount) so the node can show one amount knob per connection, and the
// slot data follows amount changes.
TEST_F (ModularProjectModelTest, ConnectedParameterSlotsExposeConnections)
{
  auto project = create_minimal_project ();
  project->add_default_tracks ();
  structure::project::ProjectUiState ui_state (*project, *app_settings_);
  ModularProjectModel                model (*project, ui_state);
  model.showGlobal ();
  add_audio_bus (*project, u8"Bus 1");

  dsp::CVPort * cv_out = nullptr;
  for (const auto &ref : project->tracklist ()->collection ()->tracks ())
    {
      auto * mod_track =
        qobject_cast<structure::tracks::ModulatorTrack *> (ref.get ());
      if (
        mod_track != nullptr
        && !mod_track->get_modulator_macro_processors ().empty ())
        {
          cv_out =
            &mod_track->get_modulator_macro_processors ()
               .front ()
               ->get_cv_out_port ();
          break;
        }
    }
  ASSERT_NE (cv_out, nullptr);

  structure::tracks::AudioBusTrack * bus = nullptr;
  for (const auto &ref : project->tracklist ()->collection ()->tracks ())
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

  mod_port->set_detached (false);
  project->port_connections_manager_->add_default_connection (
    cv_out->get_uuid (), mod_port->get_uuid (), false);
  project->port_connections_manager_->update_connection (
    cv_out->get_uuid (), mod_port->get_uuid (), 0.75f, false, true);

  const auto source_uuid = uuid_of (*cv_out);
  const auto dest_uuid = uuid_of (*mod_port);
  const auto track_node_id = "track:" + bus->uuidString ();

  const auto slot_connections = [&] {
    const auto node_it = std::find_if (
      model.nodes ().cbegin (), model.nodes ().cend (),
      [&track_node_id] (const auto &node_variant) {
        return node_variant.toMap ().value ("nodeId").toString () == track_node_id;
      });
    if (node_it == model.nodes ().cend ())
      return QJsonArray{};
    const auto parameters = QJsonDocument::fromJson (
      node_it->toMap ().value ("parametersJson").toByteArray ());
    for (const auto &slot : parameters.array ())
      {
        const auto slot_map = slot.toObject ();
        if (slot_map.value ("id").toString () == dest_uuid)
          return slot_map.value ("connections").toArray ();
      }
    return QJsonArray{};
  };

  QTRY_COMPARE (slot_connections ().size (), 1);
  const auto connections = slot_connections ();
  ASSERT_EQ (connections.size (), 1);
  EXPECT_EQ (
    connections.at (0).toObject ().value ("sourcePortId").toString (),
    source_uuid);
  EXPECT_DOUBLE_EQ (
    connections.at (0).toObject ().value ("multiplier").toDouble (), 0.75);
  EXPECT_FALSE (connections.at (0).toObject ().value ("locked").toBool ());
}

} // namespace zrythm::gui
