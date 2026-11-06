// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <map>
#include <optional>
#include <ranges>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "dsp/engine.h"
#include "dsp/midi_input_processor.h"
#include "dsp/parameter.h"
#include "dsp/port.h"
#include "dsp/processor_base.h"
#include "gui/backend/modular_project_model.h"
#include "plugins/modulators/engines/macro_engine.h"
#include "plugins/modulators/modulator.h"
#include "plugins/plugin.h"
#include "plugins/plugin_group.h"
#include "structure/project/modular_view_state.h"
#include "structure/project/project.h"
#include "structure/project/project_graph_builder.h"
#include "structure/project/project_ui_state.h"
#include "structure/tracks/channel.h"
#include "structure/tracks/track.h"
#include "structure/tracks/track_collection.h"
#include "structure/tracks/tracklist.h"
#include "utils/logger.h"
#include "utils/views.h"

#include <QAbstractItemModel>
#include <QJsonArray>
#include <QJsonDocument>
#include <QPointF>
#include <QVariant>

namespace zrythm::gui
{

using structure::project::Project;
using structure::tracks::Channel;
using structure::tracks::Track;
using structure::tracks::TrackCollection;

namespace
{

constexpr auto               kProcessorPortId = "processor"sv;
constexpr std::u8string_view kProcessorIdPrefix = u8"processor:";

QString
uuid_string (const utils::UuidIdentifiableBase &object)
{
  return object.raw_uuid ().toString (QUuid::WithoutBraces);
}

QString
port_kind (const dsp::Port &port)
{
  switch (port.type ())
    {
    case dsp::PortType::Midi:
      return QStringLiteral ("midi");
    case dsp::PortType::CV:
      return QStringLiteral ("cv");
    case dsp::PortType::Audio:
      return QStringLiteral ("audio");
    }
  return QStringLiteral ("audio");
}

QString
kind_from_port_list (const QVariantList &ports, const QString &fallback)
{
  for (const auto &port_variant : ports)
    {
      const auto port = port_variant.toMap ();
      if (port.value ("kind").toString () == u8"cv")
        return QStringLiteral ("cv");
    }
  if (!ports.isEmpty ())
    return ports.first ().toMap ().value ("kind").toString ();
  return fallback;
}

QVariantList
attached_ports (dsp::ProcessorBase &processor, dsp::PortFlow flow)
{
  QVariantList ports;
  auto         refs =
    flow == dsp::PortFlow::Input
      ? processor.get_attached_input_ports ()
      : processor.get_attached_output_ports ();
  for (const auto &ref : refs)
    {
      const auto * port = ref.get ();
      if (port == nullptr || port->detached ())
        continue;

      QString label = port->label ();
      if (label.isEmpty ())
        label = port->get_full_designation ().to_qstring ();

      ports.append (
        QVariantMap{
          { "id",    uuid_string (*port) },
          { "label", label               },
          { "kind",  port_kind (*port)   },
      });
    }
  return ports;
}

QString
ports_json (const QVariantList &ports)
{
  return QString::fromUtf8 (
    QJsonDocument (QJsonArray::fromVariantList (ports))
      .toJson (QJsonDocument::Compact));
}

QString
track_type_label (const Track &track)
{
  switch (track.type ())
    {
    case Track::Type::Instrument:
      return QObject::tr ("Instrument track");
    case Track::Type::Audio:
      return QObject::tr ("Audio track");
    case Track::Type::Master:
      return QObject::tr ("Master track");
    case Track::Type::Chord:
      return QObject::tr ("Chord track");
    case Track::Type::Marker:
      return QObject::tr ("Marker track");
    case Track::Type::AudioBus:
      return QObject::tr ("Audio bus");
    case Track::Type::AudioGroup:
      return QObject::tr ("Audio group");
    case Track::Type::Midi:
      return QObject::tr ("MIDI track");
    case Track::Type::MidiBus:
      return QObject::tr ("MIDI bus");
    case Track::Type::MidiGroup:
      return QObject::tr ("MIDI group");
    case Track::Type::Folder:
      return QObject::tr ("Folder track");
    }
  return QObject::tr ("Track");
}

QString
processor_type_label (const dsp::ProcessorBase &processor)
{
  if (
    qobject_cast<const zrythm::plugins::modulators::Modulator *> (&processor)
    != nullptr)
    {
      return QObject::tr ("Modulator");
    }
  if (qobject_cast<const zrythm::plugins::Plugin *> (&processor) != nullptr)
    {
      return QObject::tr ("Plugin");
    }
  return QObject::tr ("Processor");
}

/** Slot label of a parameter modulation input, optionally prefixed with the
 * owning processor's name. */
QString
parameter_slot_label (
  const dsp::ProcessorBase      &processor,
  const dsp::ProcessorParameter &param,
  bool                           prefix_with_processor)
{
  return prefix_with_processor
           ? processor.get_node_name ().to_qstring () + u8": " + param.label ()
           : param.label ();
}

/**
 * @brief Storage key of the canvas viewport for the current scope.
 *
 * Track scope keeps a viewport per track (the projection is a different graph
 * for each track); global scope has a single viewport.
 */
QString
viewport_key (bool global, const std::optional<Track::Uuid> &scope_track)
{
  using State = structure::project::ModularViewState;
  if (global)
    return QString::fromUtf8 (State::kScopeGlobalString.data ());
  if (!scope_track.has_value ())
    return QString::fromUtf8 (State::kScopeTrackString.data ());
  return u8"track:"
         + type_safe::get (*scope_track).toString (QUuid::WithoutBraces);
}

/**
 * @brief Whether one of the processor's ports or parameters is registered in
 * the project registry.
 *
 * This marks processors that can participate in project connections and
 * parameter modulation. Processors whose ports live in a private registry
 * (the channel's passthrough stages) are internal to their owner and are not
 * projected.
 */
bool
processor_is_project_addressable (
  const dsp::ProcessorBase     &processor,
  const utils::IObjectRegistry &registry)
{
  const auto has_registered_port = [&registry] (auto port_refs) {
    return std::ranges::any_of (port_refs, [&registry] (const auto &ref) {
      const auto * port = ref.get ();
      return port != nullptr && registry.contains (port->raw_uuid ());
    });
  };
  if (has_registered_port (processor.get_attached_input_ports ()))
    return true;
  if (has_registered_port (processor.get_attached_output_ports ()))
    return true;
  for (const auto &param_ref : processor.get_parameters ())
    {
      const auto * param = param_ref.get ();
      if (param == nullptr)
        continue;
      const auto   mod_ref = param->get_modulation_input_port_ref ();
      const auto * mod_port = mod_ref.get ();
      if (mod_port != nullptr && registry.contains (mod_port->raw_uuid ()))
        return true;
    }
  return false;
}

std::vector<dsp::ProcessorBase *>
processors_for_track (Track &track, const utils::IObjectRegistry &registry)
{
  std::vector<dsp::ProcessorBase *> processors;
  if (auto * processor = track.get_track_processor ())
    processors.push_back (processor);
  for (auto * group : track.plugin_groups ())
    {
      for (auto &ref : group->get_plugins ())
        {
          if (auto * plugin = ref.get ())
            processors.push_back (plugin);
        }
    }
  if (auto * channel = track.channel ())
    {
      // Only channel processors with project-registered ports are projected;
      // the passthrough stages live in the channel's private registry.
      for (auto * processor : channel->get_builtin_processors ())
        {
          if (processor_is_project_addressable (*processor, registry))
            processors.push_back (processor);
        }
    }
  return processors;
}

/**
 * @brief Live processor with the given UUID, or null.
 *
 * Walks the same object graph the projection is built from: processors are
 * registered in different registries (e.g. per-channel ones), so no single
 * registry can answer this.
 */
dsp::ProcessorBase *
find_processor_by_uuid (Project &project, QStringView processor_uuid)
{
  const QUuid uuid (QUuid::fromString (processor_uuid.toString ()));
  if (uuid.isNull ())
    return nullptr;
  const auto matches = [uuid] (const dsp::ProcessorBase * processor) {
    return processor->raw_uuid () == uuid;
  };
  for (const auto &ref : project.tracklist ()->collection ()->tracks ())
    {
      auto processors =
        processors_for_track (*ref.get (), project.get_registry ());
      const auto found = std::ranges::find_if (processors, matches);
      if (found != processors.end ())
        return *found;
    }
  if (auto * audio_input = project.engine ()->audio_input_processor ())
    {
      if (matches (audio_input))
        return audio_input;
    }
  for (
    const auto &[id, midi_input] : project.engine ()->midi_input_processors ())
    {
      if (matches (midi_input.get ()))
        return midi_input.get ();
    }
  if (matches (&project.monitor_fader ()))
    return &project.monitor_fader ();
  for (const auto &pl_ref : project.modulators ()->get_plugins ())
    {
      if (auto * plugin = pl_ref.get (); plugin != nullptr && matches (plugin))
        return plugin;
    }
  return nullptr;
}

using ConnectedDestinationPorts = std::unordered_set<QString>;

bool
has_modulatable_parameters (const dsp::ProcessorBase &processor)
{
  return std::ranges::any_of (processor.get_parameters (), [] (const auto &ref) {
    return ref.get () != nullptr
           && ref.get ()->get_modulation_input_port_ref ().get () != nullptr;
  });
}

/**
 * @brief Node-map entries of a native modulator: its descriptor ID, which
 * selects the node's compact editor.
 *
 * Non-modulator processors get an empty entry so that every node carries the
 * same key. Editors resolve their parameters live through nodeParameter()
 * instead of pointers stored in the projection.
 */
QVariantMap
modulator_node_extras (const dsp::ProcessorBase &processor)
{
  const auto * modulator =
    qobject_cast<const zrythm::plugins::modulators::Modulator *> (&processor);
  QString descriptor_id;
  if (modulator != nullptr)
    {
      const auto * configuration = modulator->configuration ();
      const auto * descriptor =
        configuration != nullptr ? configuration->descriptor () : nullptr;
      if (descriptor != nullptr)
        {
          if (
            const auto * id =
              std::get_if<utils::Utf8String> (&descriptor->path_or_id_))
            {
              descriptor_id = id->to_qstring ();
            }
        }
    }
  return QVariantMap{
    { "modulatorDescriptorId", descriptor_id },
  };
}

QVariantList
connected_parameter_slots (
  const dsp::ProcessorBase                          &processor,
  bool                                               prefix_with_processor,
  const ConnectedDestinationPorts                   &connected_destinations,
  const structure::project::PortConnectionsManager * manager)
{
  QVariantList slots;
  for (const auto &param_ref : processor.get_parameters ())
    {
      const auto * param = param_ref.get ();
      if (param == nullptr)
        continue;
      auto         mod_ref = param->get_modulation_input_port_ref ();
      const auto * mod_port = mod_ref.get ();
      if (mod_port == nullptr)
        continue;
      const auto id = uuid_string (*mod_port);
      if (!connected_destinations.contains (id))
        continue;
      const auto label =
        parameter_slot_label (processor, *param, prefix_with_processor);
      QVariantList connections;
      if (manager != nullptr)
        {
          structure::project::PortConnectionsManager::ConnectionsVector sources;
          manager->get_sources (&sources, mod_port->get_uuid ());
          for (const auto * conn : sources)
            {
              connections.append (
                QVariantMap{
                  { "sourcePortId",
                   type_safe::get (conn->src_id_)
                      .toString (QUuid::WithoutBraces) },
                  { "multiplier",   conn->multiplier_  },
                  { "locked",       conn->locked_      },
              });
            }
        }
      slots.append (
        QVariantMap{
          { "id",          id                           },
          { "label",       label                        },
          { "kind",        QStringLiteral ("parameter") },
          { "connections", connections                  },
      });
    }
  return slots;
}

/** Layout metrics shared with the QML node delegates. */
constexpr int kLayoutNodeWidth = 236;
constexpr int kLayoutHeaderHeight = 40;
constexpr int kLayoutPortHeight = 26;
// Height of the compact modulator editor body shown below the ports of
// native modulator nodes.
constexpr int kLayoutEditorBodyHeight = 56;
constexpr int kLayoutColumnGap = 140;
constexpr int kLayoutRowGap = 48;
constexpr int kLayoutMarginX = 90;
constexpr int kLayoutMarginY = 80;

int
layout_node_height (const QVariantMap &node)
{
  const auto count_slots = [] (const QVariant &json) {
    const auto document = QJsonDocument::fromJson (json.toByteArray ());
    return document.isArray () ? document.array ().size () : 0;
  };
  const int input_slots =
    static_cast<int> (count_slots (node.value ("inputsJson")))
    + static_cast<int> (count_slots (node.value ("parametersJson")));
  const int output_slots =
    static_cast<int> (count_slots (node.value ("outputsJson")));
  const bool has_modulator_editor =
    !node.value ("modulatorDescriptorId").toString ().isEmpty ();
  return kLayoutHeaderHeight
         + (std::max) (input_slots, output_slots) * kLayoutPortHeight + 8
         + (has_modulator_editor ? kLayoutEditorBodyHeight : 0);
}

/**
 * Positions nodes without stored positions in left-to-right signal-flow
 * columns: one column per longest-path rank, stacked and vertically centered
 * against the tallest column.
 */
void
apply_layered_layout (
  QVariantList            &nodes,
  const QVariantList      &edges,
  const std::set<QString> &stored)
{
  std::map<QString, std::set<QString>> successors;
  for (const auto &edge_variant : edges)
    {
      const auto edge = edge_variant.toMap ();
      const auto source = edge.value ("sourceNode").toString ();
      const auto destination = edge.value ("destinationNode").toString ();
      if (source != destination)
        successors[source].insert (destination);
    }

  std::map<QString, int> in_degree;
  for (const auto &node_variant : nodes)
    in_degree.emplace (node_variant.toMap ().value ("nodeId").toString (), 0);
  for (const auto &[source, targets] : successors)
    {
      for (const auto &target : targets)
        ++in_degree[target];
    }

  std::map<QString, int> rank;
  std::vector<QString>   ready;
  for (const auto &[id, degree] : in_degree)
    {
      if (degree == 0)
        ready.push_back (id);
    }
  while (!ready.empty ())
    {
      const auto current = ready.back ();
      ready.pop_back ();
      const auto successor_it = successors.find (current);
      if (successor_it == successors.end ())
        continue;
      for (const auto &target : successor_it->second)
        {
          const int candidate_rank = rank[current] + 1;
          if (candidate_rank > rank[target])
            rank[target] = candidate_rank;
          if (--in_degree[target] == 0)
            ready.push_back (target);
        }
    }

  // Heights are computed once; the layout below reads them several times.
  const auto node_heights =
    nodes | std::views::transform ([] (const auto &node_variant) {
      return layout_node_height (node_variant.toMap ());
    })
    | std::ranges::to<std::vector> ();
  std::map<int, std::vector<qsizetype>> columns;
  for (const auto [index, node_variant] : nodes | utils::views::enumerate)
    {
      const auto id = node_variant.toMap ().value ("nodeId").toString ();
      columns[rank.contains (id) ? rank.at (id) : 0].push_back (index);
    }

  int                max_column_height = 0;
  std::map<int, int> column_heights;
  for (const auto &[column, indices] : columns)
    {
      int height = 0;
      for (const auto index : indices)
        height += node_heights[index] + kLayoutRowGap;
      height -= kLayoutRowGap;
      column_heights[column] = height;
      max_column_height = std::max (max_column_height, height);
    }

  for (const auto &[column, indices] : columns)
    {
      qreal y =
        kLayoutMarginY
        + static_cast<qreal> (max_column_height - column_heights[column]) / 2;
      for (const auto index : indices)
        {
          auto       node = nodes[index].toMap ();
          const auto id = node.value ("nodeId").toString ();
          if (stored.count (id) == 0)
            {
              node.insert (
                "nodeX",
                static_cast<qreal> (
                  kLayoutMarginX
                  + column * (kLayoutNodeWidth + kLayoutColumnGap)));
              node.insert ("nodeY", y);
              nodes.replace (index, node);
            }
          y += node_heights[index] + kLayoutRowGap;
        }
    }
}

} // namespace

struct ModularProjectModel::Impl
{
  struct Endpoint
  {
    QString node_id;
    QString port_id;
    bool    parameter{};
  };

  std::unordered_map<const dsp::graph::IProcessable *, Endpoint> endpoints;
  std::unordered_map<QString, QString>                           node_accents;
  std::unordered_set<QString>                                    edge_ids;
  std::set<QString>          nodes_with_stored_positions;
  QVariantList               nodes;
  QVariantList               edges;
  std::optional<Track::Uuid> scope_track;
  bool                       rebuild_pending = false;

  void clear ()
  {
    endpoints.clear ();
    node_accents.clear ();
    edge_ids.clear ();
    nodes_with_stored_positions.clear ();
    nodes.clear ();
    edges.clear ();
  }
};

ModularProjectModel::ModularProjectModel (
  Project                            &project,
  structure::project::ProjectUiState &ui_state,
  QObject *                           parent)
    : QObject (parent), project_ (project), ui_state_ (ui_state),
      impl_ (std::make_unique<Impl> ())
{
  const auto * collection = project_.tracklist ()->collection ();
  QObject::connect (
    collection, &QAbstractItemModel::rowsInserted, this,
    &ModularProjectModel::schedule_rebuild, Qt::UniqueConnection);
  QObject::connect (
    collection, &QAbstractItemModel::rowsRemoved, this,
    &ModularProjectModel::schedule_rebuild, Qt::UniqueConnection);
  QObject::connect (
    collection, &QAbstractItemModel::dataChanged, this,
    &ModularProjectModel::schedule_rebuild, Qt::UniqueConnection);
  QObject::connect (
    collection, &QAbstractItemModel::modelReset, this,
    &ModularProjectModel::schedule_rebuild, Qt::UniqueConnection);
  QObject::connect (
    project_.tracklist ()->trackRouting (),
    &structure::tracks::TrackRouting::routingChanged, this,
    &ModularProjectModel::schedule_rebuild, Qt::UniqueConnection);
  if (const auto * manager = project_.port_connections_manager_.get ())
    {
      QObject::connect (
        manager, &structure::project::PortConnectionsManager::connectionsChanged,
        this, &ModularProjectModel::schedule_rebuild, Qt::UniqueConnection);
    }
  QObject::connect (
    &ui_state_, &structure::project::ProjectUiState::restored, this,
    &ModularProjectModel::on_ui_state_restored, Qt::UniqueConnection);

  resume_saved_scope ();

  rebuild_projection ();
}

void
ModularProjectModel::resume_saved_scope ()
{
  const bool previous_global = global_;
  const auto previous_scope_track = impl_->scope_track;

  // Resume the scope and scope track saved with the project. The scope track
  // is also restored in global scope, where it names the linked track.
  const auto &saved_state = *ui_state_.modularViewState ();
  if (const auto saved_track = saved_state.selected_track ())
    {
      if (project_.tracklist ()->collection ()->contains (*saved_track))
        impl_->scope_track = *saved_track;
    }
  global_ =
    saved_state.scope () == structure::project::ModularViewState::Scope::Global;

  if (global_ != previous_global || impl_->scope_track != previous_scope_track)
    Q_EMIT scopeChanged ();
}

void
ModularProjectModel::on_ui_state_restored ()
{
  resume_saved_scope ();
  schedule_rebuild ();
}

ModularProjectModel::~ModularProjectModel () = default;

const QVariantList &
ModularProjectModel::nodes () const
{
  return nodes_;
}

const QVariantList &
ModularProjectModel::edges () const
{
  return edges_;
}

bool
ModularProjectModel::global () const
{
  return global_;
}

QString
ModularProjectModel::scopeTrackUuid () const
{
  if (!impl_->scope_track.has_value ())
    return {};
  return type_safe::get (*impl_->scope_track).toString (QUuid::WithoutBraces);
}

QString
ModularProjectModel::scopeTitle () const
{
  if (global_)
    return QObject::tr ("Global");
  if (!impl_->scope_track.has_value ())
    return QObject::tr ("No track");
  const auto * collection = project_.tracklist ()->collection ();
  for (const auto &ref : collection->tracks ())
    {
      auto * track = ref.get ();
      if (track->get_uuid () == *impl_->scope_track)
        return track->name ();
    }
  return QObject::tr ("No track");
}

void
ModularProjectModel::setScopeTrack (const QString &track_uuid)
{
  const QUuid raw_uuid (track_uuid);
  if (raw_uuid.isNull ())
    return;

  const Track::Uuid track_id (raw_uuid);
  if (!project_.tracklist ()->collection ()->contains (track_id))
    return;

  ui_state_.modularViewState ()->set_selected_track (track_id);
  if (impl_->scope_track == track_id)
    {
      return;
    }

  impl_->scope_track = track_id;
  Q_EMIT scopeChanged ();
  if (!global_)
    rebuild_projection ();
}

void
ModularProjectModel::showTrack (const QString &track_uuid)
{
  const QUuid raw_uuid (track_uuid);
  if (raw_uuid.isNull ())
    return;

  const Track::Uuid track_id (raw_uuid);
  if (!project_.tracklist ()->collection ()->contains (track_id))
    return;

  if (!global_ && impl_->scope_track == track_id)
    return;

  auto * const modular_state = ui_state_.modularViewState ();
  modular_state->set_scope (structure::project::ModularViewState::Scope::Track);
  modular_state->set_selected_track (track_id);

  global_ = false;
  impl_->scope_track = track_id;
  Q_EMIT scopeChanged ();
  rebuild_projection ();
}

void
ModularProjectModel::showGlobal ()
{
  if (global_)
    return;
  global_ = true;
  ui_state_.modularViewState ()->set_scope (
    structure::project::ModularViewState::Scope::Global);
  Q_EMIT scopeChanged ();
  rebuild_projection ();
}

QVariantMap
ModularProjectModel::viewport () const
{
  QVariantMap result;
  const auto  saved = ui_state_.modularViewState ()->find_viewport (
    viewport_key (global_, impl_->scope_track));
  if (!saved.has_value ())
    return result;
  result.insert (u8"panX", saved->pan_x);
  result.insert (u8"panY", saved->pan_y);
  result.insert (u8"zoom", saved->zoom);
  return result;
}

void
ModularProjectModel::setViewport (double pan_x, double pan_y, double zoom)
{
  if (zoom <= 0.0)
    return;
  ui_state_.modularViewState ()->set_viewport (
    viewport_key (global_, impl_->scope_track),
    structure::project::ModularViewState::Viewport{ pan_x, pan_y, zoom });
}

void
ModularProjectModel::arrangeAutomatically ()
{
  for (const auto &node : impl_->nodes)
    ui_state_.modularViewState ()->clear_node_position (
      node.toMap ().value ("nodeId").toString ());
  impl_->nodes_with_stored_positions.clear ();
  rebuild_projection ();
}

QVariantList
ModularProjectModel::modulatableParameters (const QString &node_id) const
{
  QVariantList result;
  const auto   append_params =
    [&result] (const dsp::ProcessorBase &processor, bool prefix_with_processor) {
      for (const auto &param_ref : processor.get_parameters ())
        {
          const auto * param = param_ref.get ();
          if (param == nullptr)
            continue;
          auto         mod_ref = param->get_modulation_input_port_ref ();
          const auto * mod_port = mod_ref.get ();
          if (mod_port == nullptr)
            continue;
          result.append (
            QVariantMap{
              { "label",
               parameter_slot_label (processor, *param, prefix_with_processor) },
              { "portId", uuid_string (*mod_port) },
              { "kind", QStringLiteral ("parameter") },
          });
        }
    };

  if (node_id.startsWith (kProcessorIdPrefix.data ()))
    {
      // Resolved through the live object graph rather than the projection's
      // endpoint map: the map holds raw pointers that go stale between an
      // object's removal and the next rebuild.
      const auto * processor = find_processor_by_uuid (
        project_,
        node_id.sliced (static_cast<qsizetype> (kProcessorIdPrefix.size ())));
      if (processor != nullptr)
        append_params (*processor, false);
    }
  else if (node_id.startsWith (u8"track:"))
    {
      for (const auto &ref : project_.tracklist ()->collection ()->tracks ())
        {
          auto * track = ref.get ();
          if ("track:" + uuid_string (*track) != node_id)
            continue;
          for (
            auto * processor :
            processors_for_track (*track, project_.get_registry ()))
            append_params (*processor, true);
          break;
        }
    }
  return result;
}

QVariantMap
ModularProjectModel::rackMemberInfo (const QString &node_id) const
{
  QVariantMap result;
  if (!node_id.startsWith (kProcessorIdPrefix.data ()))
    return result;
  auto * processor = find_processor_by_uuid (
    project_,
    node_id.sliced (static_cast<qsizetype> (kProcessorIdPrefix.size ())));
  auto * plugin =
    processor != nullptr ? qobject_cast<plugins::Plugin *> (processor) : nullptr;
  if (plugin == nullptr)
    return result;

  const auto group_contains_plugin =
    [plugin] (const plugins::PluginGroup * group) {
      return group != nullptr
             && std::ranges::any_of (
               group->get_plugins (), [plugin] (const auto &ref) {
                 return ref.id () == plugin->get_uuid ();
               });
    };
  if (group_contains_plugin (project_.modulators ()))
    {
      result.insert ("plugin", QVariant::fromValue (plugin));
      result.insert ("group", QVariant::fromValue (project_.modulators ()));
      return result;
    }
  for (const auto &ref : project_.tracklist ()->collection ()->tracks ())
    {
      auto * rack = ref.get ()->modulators ();
      if (group_contains_plugin (rack))
        {
          result.insert ("plugin", QVariant::fromValue (plugin));
          result.insert ("group", QVariant::fromValue (rack));
          // Needed for automation track cleanup when the plugin is removed
          result.insert ("track", QVariant::fromValue (ref.get ()));
          break;
        }
    }
  return result;
}

QObject *
ModularProjectModel::nodeParameter (
  const QString &node_id,
  const QString &parameter_id) const
{
  if (!node_id.startsWith (kProcessorIdPrefix.data ()))
    return nullptr;
  auto * processor = find_processor_by_uuid (
    project_,
    node_id.sliced (static_cast<qsizetype> (kProcessorIdPrefix.size ())));
  if (processor == nullptr)
    return nullptr;
  const auto wanted_id = dsp::ProcessorParameter::UniqueId (
    utils::Utf8String::from_qstring (parameter_id));
  for (const auto &param_ref : processor->get_parameters ())
    {
      auto * param = param_ref.get ();
      if (param != nullptr && param->get_unique_id () == wanted_id)
        return param;
    }
  return nullptr;
}

QObject *
ModularProjectModel::scopeRack () const
{
  if (!global_ && impl_->scope_track.has_value ())
    {
      const auto &tracks = project_.tracklist ()->collection ()->tracks ();
      const auto  found = std::ranges::find_if (
        tracks, [scope = *impl_->scope_track] (const auto &ref) {
          return ref.get ()->get_uuid () == scope;
        });
      if (found != tracks.end ())
        {
          auto * rack = found->get ()->modulators ();
          if (rack != nullptr)
            return rack;
        }
    }
  return project_.modulators ();
}

QString
ModularProjectModel::macroDescriptorId () const
{
  return QString::fromUtf8 (plugins::modulators::MacroEngine::kDescriptorId);
}

QString
ModularProjectModel::macroValueParameterId () const
{
  return QString::fromUtf8 (plugins::modulators::MacroEngine::kParameterId);
}

void
ModularProjectModel::setNodePosition (const QString &node_id, double x, double y)
{
  // Only nodes in the current projection have persistable positions.
  if (!impl_->node_accents.contains (node_id))
    return;
  ui_state_.modularViewState ()->set_node_position (node_id, QPointF (x, y));
}

void
ModularProjectModel::schedule_rebuild ()
{
  if (impl_->rebuild_pending)
    return;
  impl_->rebuild_pending = true;
  QMetaObject::invokeMethod (
    this,
    [this] () {
      impl_->rebuild_pending = false;
      rebuild_projection ();
    },
    Qt::QueuedConnection);
}

void
ModularProjectModel::rebuild_projection ()
{
  impl_->clear ();

  const auto * collection = project_.tracklist ()->collection ();
  const auto  &tracks = collection->tracks ();

  // Resolve the effective track scope before projecting: a scope track that
  // no longer exists falls back to the first track with a track processor in
  // the same pass, and scope changes notify consumers.
  if (!global_)
    {
      const auto previous_scope_track = impl_->scope_track;
      const auto scope_track_exists = [&tracks] (const Track::Uuid &id) {
        return std::ranges::any_of (tracks, [&id] (const auto &ref) {
          return ref.get ()->get_uuid () == id;
        });
      };
      if (
        !impl_->scope_track.has_value ()
        || !scope_track_exists (*impl_->scope_track))
        {
          impl_->scope_track.reset ();
          const auto first_track =
            std::ranges::find_if (tracks, [] (const auto &ref) {
              return ref.get ()->get_track_processor () != nullptr;
            });
          if (first_track != tracks.end ())
            impl_->scope_track = first_track->id ();
        }
      if (impl_->scope_track != previous_scope_track)
        Q_EMIT scopeChanged ();
    }

  Track * scope_track = nullptr;
  if (!global_ && impl_->scope_track.has_value ())
    {
      const auto found = std::ranges::find_if (
        tracks, [scope = *impl_->scope_track] (const auto &ref) {
          return ref.get ()->get_uuid () == scope;
        });
      if (found != tracks.end ())
        scope_track = found->get ();
    }

  ConnectedDestinationPorts connected_destination_ports;
  if (const auto * manager = project_.port_connections_manager_.get ())
    {
      for (const auto &connection : manager->connections ())
        connected_destination_ports.insert (
          type_safe::get (connection->dest_id_).toString (QUuid::WithoutBraces));
    }

  const auto watch_model = [this] (const QAbstractItemModel * model) {
    if (model == nullptr)
      return;
    QObject::connect (
      model, &QAbstractItemModel::rowsInserted, this,
      &ModularProjectModel::schedule_rebuild, Qt::UniqueConnection);
    QObject::connect (
      model, &QAbstractItemModel::rowsRemoved, this,
      &ModularProjectModel::schedule_rebuild, Qt::UniqueConnection);
    QObject::connect (
      model, &QAbstractItemModel::dataChanged, this,
      &ModularProjectModel::schedule_rebuild, Qt::UniqueConnection);
    QObject::connect (
      model, &QAbstractItemModel::modelReset, this,
      &ModularProjectModel::schedule_rebuild, Qt::UniqueConnection);
  };
  // Watches everything of a track that the projection renders: its name
  // and the plugin groups its nodes are built from.
  const auto watch_track = [this, watch_model] (const Track &track) {
    QObject::connect (
      &track, &Track::nameChanged, this, &ModularProjectModel::schedule_rebuild,
      Qt::UniqueConnection);
    for (auto * group : track.plugin_groups ())
      {
        watch_model (group);
      }
  };
  const auto register_port = [this] (const dsp::Port * port) {
    if (port == nullptr)
      return;
    QObject::connect (
      port, &dsp::Port::detachedChanged, this,
      &ModularProjectModel::schedule_rebuild, Qt::UniqueConnection);
    QObject::connect (
      port, &dsp::Port::labelChanged, this,
      &ModularProjectModel::schedule_rebuild, Qt::UniqueConnection);
  };
  const auto register_processor =
    [this, register_port] (
      const QString &node_id, const dsp::ProcessorBase &processor) {
      impl_->endpoints.emplace (
        &processor,
        Impl::Endpoint{ node_id, QString::fromUtf8 (kProcessorPortId) });
      const auto register_endpoint =
        [this, node_id] (const dsp::Port &port, bool parameter) {
          impl_->endpoints.emplace (
            &port, Impl::Endpoint{ node_id, uuid_string (port), parameter });
        };
      const auto register_ports = [&] (auto refs) {
        for (const auto &ref : refs)
          {
            auto * port = ref.get ();
            if (port == nullptr)
              continue;
            register_port (port);
            register_endpoint (*port, false);
          }
      };
      register_ports (processor.get_attached_input_ports ());
      register_ports (processor.get_attached_output_ports ());
      for (const auto &param_ref : processor.get_parameters ())
        {
          const auto * param = param_ref.get ();
          if (param == nullptr)
            continue;
          auto   mod_ref = param->get_modulation_input_port_ref ();
          auto * mod_port = mod_ref.get ();
          if (mod_port == nullptr)
            continue;
          register_port (mod_port);
          register_endpoint (*mod_port, true);
        }
    };
  // A track node renders the channel's exposed output ports as its own
  // outputs; those ports live on the post-fader stage, which is not a
  // projected processor, so they are registered directly.
  const auto register_exposed_port =
    [this, register_port] (const QString &node_id, const dsp::Port &port) {
      register_port (&port);
      impl_->endpoints.emplace (
        &port, Impl::Endpoint{ node_id, uuid_string (port), false });
    };
  const auto append_node =
    [&] (
      const QString &id, const QString &title, const QString &subtitle,
      const QString &type, const QVariantList &inputs,
      const QVariantList &outputs, const QVariantList &parameters,
      bool has_parameters, const QString &accent,
      const QVariantMap &extras = QVariantMap{}) {
      const auto stored_position =
        ui_state_.modularViewState ()->find_node_position (id);
      if (stored_position.has_value ())
        impl_->nodes_with_stored_positions.insert (id);
      const auto position = stored_position.value_or (QPointF (0, 0));
      impl_->node_accents.emplace (id, accent);
      QVariantMap node_map{
        { "nodeId",                   id                      },
        { "nodeTitle",                title                   },
        { "nodeSubtitle",             subtitle                },
        { "nodeType",                 type                    },
        { "nodeX",                    position.x ()           },
        { "nodeY",                    position.y ()           },
        { "accentKind",               accent                  },
        { "inputsJson",               ports_json (inputs)     },
        { "outputsJson",              ports_json (outputs)    },
        { "parametersJson",           ports_json (parameters) },
        { "hasModulatableParameters", has_parameters          },
        // Present on every node so the QML ListModel role always exists;
        // modulator nodes override it through the extras
        { "modulatorDescriptorId",    QString ()              },
      };
      for (auto it = extras.begin (); it != extras.end (); ++it)
        {
          node_map.insert (it.key (), it.value ());
        }
      impl_->nodes.append (node_map);
    };
  // Global-rack members are projected in every scope: Track scope needs
  // their modulation sources at hand alongside the scope track's nodes.
  const auto append_global_rack_nodes = [&] {
    for (const auto &pl_ref : project_.modulators ()->get_plugins ())
      {
        auto * global_plugin = pl_ref.get ();
        if (global_plugin == nullptr)
          continue;
        const auto inputs =
          attached_ports (*global_plugin, dsp::PortFlow::Input);
        const auto outputs =
          attached_ports (*global_plugin, dsp::PortFlow::Output);
        const QString node_id = "processor:" + uuid_string (*global_plugin);
        append_node (
          node_id, global_plugin->get_node_name ().to_qstring (), QString (),
          QObject::tr ("Global"), inputs, outputs,
          connected_parameter_slots (
            *global_plugin, false, connected_destination_ports,
            project_.port_connections_manager_.get ()),
          has_modulatable_parameters (*global_plugin),
          kind_from_port_list (outputs, QStringLiteral ("cv")),
          modulator_node_extras (*global_plugin));
        register_processor (node_id, *global_plugin);
      }
  };
  // Rack membership edits rebuild the projection in every scope.
  watch_model (project_.modulators ());

  if (global_)
    {
      for (const auto &ref : tracks)
        {
          auto * track = ref.get ();
          watch_track (*track);

          const auto processors =
            processors_for_track (*track, project_.get_registry ());
          if (processors.empty ())
            continue;

          QVariantList parameters;
          bool         has_parameters = false;
          for (auto * processor : processors)
            {
              auto processor_slots = connected_parameter_slots (
                *processor, true, connected_destination_ports,
                project_.port_connections_manager_.get ());
              parameters.append (processor_slots);
              has_parameters =
                has_parameters || has_modulatable_parameters (*processor);
            }

          QVariantList inputs =
            track->get_track_processor () != nullptr
              ? attached_ports (
                  *track->get_track_processor (), dsp::PortFlow::Input)
              : QVariantList{};
          QVariantList outputs;
          if (auto * channel = track->channel ())
            {
              if (auto * audio_output = channel->audioOutPort ())
                outputs.append (
                  QVariantMap{
                    { "id",    uuid_string (*audio_output) },
                    { "label", QObject::tr ("Audio out")   },
                    { "kind",  QStringLiteral ("audio")    },
                });
              if (auto * midi_output = channel->getMidiOut ())
                outputs.append (
                  QVariantMap{
                    { "id",    uuid_string (*midi_output) },
                    { "label", QObject::tr ("MIDI out")   },
                    { "kind",  QStringLiteral ("midi")    },
                });
            }
          if (outputs.isEmpty () && track->get_track_processor () != nullptr)
            outputs = attached_ports (
              *track->get_track_processor (), dsp::PortFlow::Output);

          const QString node_id = "track:" + uuid_string (*track);
          const QString accent =
            kind_from_port_list (outputs, QStringLiteral ("neutral"));
          append_node (
            node_id, track->name (), QString (), track_type_label (*track),
            inputs, outputs, parameters, has_parameters, accent);
          for (auto * processor : processors)
            register_processor (node_id, *processor);
          if (auto * channel = track->channel ())
            {
              if (auto * audio_output = channel->audioOutPort ())
                register_exposed_port (node_id, *audio_output);
              if (auto * midi_output = channel->getMidiOut ())
                register_exposed_port (node_id, *midi_output);
            }
        }

      if (auto * audio_input = project_.engine ()->audio_input_processor ())
        {
          const auto inputs =
            attached_ports (*audio_input, dsp::PortFlow::Input);
          const auto outputs =
            attached_ports (*audio_input, dsp::PortFlow::Output);
          const QString node_id = "processor:" + uuid_string (*audio_input);
          append_node (
            node_id, QObject::tr ("Audio input"), QString (),
            QObject::tr ("Hardware"), inputs, outputs,
            connected_parameter_slots (
              *audio_input, false, connected_destination_ports,
              project_.port_connections_manager_.get ()),
            has_modulatable_parameters (*audio_input),
            kind_from_port_list (outputs, QStringLiteral ("audio")));
          register_processor (node_id, *audio_input);
        }
      for (
        const auto &[_, midi_input] :
        project_.engine ()->midi_input_processors ())
        {
          const auto inputs = attached_ports (*midi_input, dsp::PortFlow::Input);
          const auto outputs =
            attached_ports (*midi_input, dsp::PortFlow::Output);
          const QString node_id = "processor:" + uuid_string (*midi_input);
          append_node (
            node_id, midi_input->get_node_name ().to_qstring (), QString (),
            QObject::tr ("Hardware"), inputs, outputs,
            connected_parameter_slots (
              *midi_input, false, connected_destination_ports,
              project_.port_connections_manager_.get ()),
            has_modulatable_parameters (*midi_input),
            kind_from_port_list (outputs, QStringLiteral ("midi")));
          register_processor (node_id, *midi_input);
        }
      {
        auto &monitor_fader = project_.monitor_fader ();
        const auto inputs = attached_ports (monitor_fader, dsp::PortFlow::Input);
        const auto outputs =
          attached_ports (monitor_fader, dsp::PortFlow::Output);
        const QString node_id = "processor:" + uuid_string (monitor_fader);
        append_node (
          node_id, QObject::tr ("Monitor output"), QString (),
          QObject::tr ("Hardware"), inputs, outputs,
          connected_parameter_slots (
            monitor_fader, false, connected_destination_ports,
            project_.port_connections_manager_.get ()),
          has_modulatable_parameters (monitor_fader),
          kind_from_port_list (outputs, QStringLiteral ("neutral")));
        register_processor (node_id, monitor_fader);
      }
      append_global_rack_nodes ();
    }
  else if (scope_track != nullptr)
    {
      auto &track = *scope_track;
      watch_track (track);
      for (
        auto * processor :
        processors_for_track (track, project_.get_registry ()))
        {
          const auto inputs = attached_ports (*processor, dsp::PortFlow::Input);
          const auto outputs =
            attached_ports (*processor, dsp::PortFlow::Output);
          const QString node_id = "processor:" + uuid_string (*processor);
          append_node (
            node_id, processor->get_node_name ().to_qstring (),
            track_type_label (track), processor_type_label (*processor), inputs,
            outputs,
            connected_parameter_slots (
              *processor, false, connected_destination_ports,
              project_.port_connections_manager_.get ()),
            has_modulatable_parameters (*processor),
            kind_from_port_list (outputs, QStringLiteral ("neutral")),
            modulator_node_extras (*processor));
          register_processor (node_id, *processor);
        }
      append_global_rack_nodes ();
    }
  try
    {
      dsp::graph::Graph                       graph;
      structure::project::ProjectGraphBuilder builder (
        project_, project_.metronome (), project_.monitor_fader ());
      builder.build_graph (graph);

      // Edges bridge over graph nodes without a real-port endpoint: the
      // channel's passthrough stages are not projected at all, and
      // processor-level links are processing order rather than signal
      // routing, so neither produces an edge of its own. A processor-level
      // hop is walked through only when the processor has no ports at all
      // (a portless insert passes the chain on); a processor with ports has
      // its ports as its signal boundary, so hops into its processor node
      // are scheduling (e.g. every track's output also connects to the
      // master track processor for solo/mute checks and metering).
      const QString               processor_port_id (kProcessorPortId.data ());
      std::unordered_set<QString> nodes_with_ports;
      for (const auto &[endpoint_processable, endpoint] : impl_->endpoints)
        {
          if (endpoint.port_id != processor_port_id)
            nodes_with_ports.insert (endpoint.node_id);
        }
      const auto collect_exposed_targets =
        [this, processor_port_id, &nodes_with_ports] (
          const Impl::Endpoint                       &source_endpoint,
          const dsp::graph::GraphNode                &node,
          std::vector<const dsp::graph::GraphNode *> &visited,
          std::vector<const dsp::graph::GraphNode *> &targets) {
          // Returns whether the walk continues through the reached node.
          // A real port of another projected node ends a path as an edge
          // target; endpoints of the source node itself end it too, since
          // each endpoint's own walk covers the paths leaving it (a
          // processor-level hop returns to the processor's ports before
          // reaching any other node).
          const auto visit =
            [this, processor_port_id, &nodes_with_ports, &source_endpoint,
             &visited, &targets] (const dsp::graph::GraphNode &target) {
              if (std::ranges::find (visited, &target) != visited.end ())
                return false;
              visited.push_back (&target);
              const auto target_it =
                impl_->endpoints.find (&target.get_processable ());
              if (target_it == impl_->endpoints.end ())
                return true;
              if (target_it->second.node_id == source_endpoint.node_id)
                return false;
              if (target_it->second.port_id == processor_port_id)
                return nodes_with_ports.count (target_it->second.node_id) == 0;
              targets.push_back (&target);
              return false;
            };
          std::vector<const dsp::graph::GraphNode *> frontier;
          for (const auto &target_ref : node.feeds ())
            {
              auto &target = target_ref.get ();
              if (visit (target))
                frontier.push_back (&target);
            }
          while (!frontier.empty ())
            {
              const auto * current = frontier.back ();
              frontier.pop_back ();
              for (const auto &target_ref : current->feeds ())
                {
                  auto &target = target_ref.get ();
                  if (visit (target))
                    frontier.push_back (&target);
                }
            }
        };

      for (const auto &source_owner : graph.get_nodes ().graph_nodes_)
        {
          const auto  &source_node = *source_owner;
          const auto * source_processable = &source_node.get_processable ();
          const auto   source_it = impl_->endpoints.find (source_processable);
          if (source_it == impl_->endpoints.end ())
            continue;

          const auto                                &source = source_it->second;
          std::vector<const dsp::graph::GraphNode *> visited{ &source_node };
          std::vector<const dsp::graph::GraphNode *> exposed_targets;
          collect_exposed_targets (
            source, source_node, visited, exposed_targets);
          for (const auto * exposed_target : exposed_targets)
            {
              const auto  &target_node = *exposed_target;
              const auto * target_processable = &target_node.get_processable ();
              const auto target_it = impl_->endpoints.find (target_processable);
              if (target_it == impl_->endpoints.end ())
                continue;

              const auto &target = target_it->second;

              const QString edge_id =
                source.node_id + "/" + source.port_id + "->" + target.node_id
                + "/" + target.port_id;
              if (!impl_->edge_ids.insert (edge_id).second)
                continue;

              const auto * source_port =
                dynamic_cast<const dsp::Port *> (source_processable);
              const auto * target_port =
                dynamic_cast<const dsp::Port *> (target_processable);
              QString signal_kind = QStringLiteral ("audio");
              if (target.parameter)
                signal_kind = QStringLiteral ("parameter");
              else if (source_port != nullptr)
                signal_kind = port_kind (*source_port);
              else if (target_port != nullptr)
                signal_kind = port_kind (*target_port);
              else
                signal_kind =
                  impl_->node_accents.count (source.node_id) != 0
                    ? impl_->node_accents.at (source.node_id)
                    : QStringLiteral ("audio");

              const auto connection_config =
                target_node.connection_for_dependency (source_node);
              const bool custom_connection = connection_config.has_value ();

              // Automatic edges and locked connections are read-only;
              // unlocked user connections can be disconnected.
              const auto * manager = project_.port_connections_manager_.get ();
              const auto * conn =
                custom_connection
                  ? manager->get_connection (
                      dsp::PortUuid (QUuid (source.port_id)),
                      dsp::PortUuid (QUuid (target.port_id)))
                  : nullptr;
              const bool read_only =
                !custom_connection || conn == nullptr || conn->locked_;
              impl_->edges.append (
                QVariantMap{
                  { "automatic",         !custom_connection                         },
                  { "edgeId",            "project:" + edge_id                       },
                  { "sourceNode",        source.node_id                             },
                  { "sourcePort",        source.port_id                             },
                  { "destinationNode",   target.node_id                             },
                  { "destinationPort",   target.port_id                             },
                  { "signalKind",        signal_kind                                },
                  { "readOnly",          read_only                                  },
                  { "multiplier",        conn != nullptr ? conn->multiplier_ : 1.0f },
                  { "connectionEnabled", conn != nullptr ? conn->enabled_ : true    },
              });
            }
        }
    }
  catch (const std::exception &error)
    {
      z_warning (
        "Failed to build the Modular graph projection: {}", error.what ());
    }

  apply_layered_layout (
    impl_->nodes, impl_->edges, impl_->nodes_with_stored_positions);

  if (nodes_ != impl_->nodes)
    {
      nodes_ = impl_->nodes;
      Q_EMIT nodesChanged ();
    }
  if (edges_ != impl_->edges)
    {
      edges_ = impl_->edges;
      Q_EMIT edgesChanged ();
    }
}

} // namespace zrythm::gui
