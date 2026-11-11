// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <ranges>
#include <unordered_set>

#include "commands/remove_plugins_command.h"
#include "dsp/parameter.h"
#include "dsp/port.h"
#include "plugins/plugin.h"
#include "utils/logger.h"
#include "utils/registry_utils.h"

#include <type_safe/strong_typedef.hpp>

namespace zrythm::commands
{

RemovePluginsCommand::RemovePluginsCommand (
  std::vector<PluginRemoveInfo>               plugin_infos,
  structure::project::PortConnectionsManager &connections_manager,
  utils::IObjectRegistry                     &registry)
    : Command (
        plugin_infos.size () == 1
          ? QObject::tr ("Remove Plugin")
          : QObject::tr ("Remove %n Plugin(s)", "", plugin_infos.size ()),
        EngineRequirement::RecalculateGraph),
      plugin_infos_ (std::move (plugin_infos)),
      connections_manager_ (connections_manager), registry_ (registry)
{
  for (const auto &info : plugin_infos_)
    {
      const auto plugins = info.source_group->get_plugins ();
      if (!std::ranges::contains (
            plugins, info.plugin_ref.id (), &plugins::PluginUuidReference::id))
        {
          throw std::invalid_argument (
            "Source group does not include the plugin");
        }
    }

  snapshot_plugin_connections ();
  snapshot_sends_targeting_plugins ();
}

void
RemovePluginsCommand::snapshot_plugin_connections ()
{
  const auto snapshot_port_connections = [this] (const dsp::PortUuid &port_id) {
    structure::project::PortConnectionsManager::ConnectionsVector connections;
    connections_manager_.get_sources (&connections, port_id);
    connections_manager_.get_dests (&connections, port_id);
    for (const auto * conn : connections)
      {
        const auto already_snapshotted = std::ranges::any_of (
          removed_connections_, [conn] (const RemovedConnection &removed) {
            return removed.src == conn->src_id_ && removed.dest == conn->dest_id_;
          });
        if (already_snapshotted)
          continue;
        removed_connections_.push_back (
          RemovedConnection{
            .src = conn->src_id_,
            .dest = conn->dest_id_,
            .multiplier = conn->multiplier_,
            .locked = conn->locked_,
            .enabled = conn->enabled_,
            .bipolar = conn->bipolar_,
          });
      }
  };

  for (const auto &info : plugin_infos_)
    {
      auto * plugin = info.plugin_ref.get ();
      if (plugin == nullptr)
        continue;

      for (const auto &port_ref : plugin->get_all_input_ports ())
        {
          if (port_ref.get () != nullptr)
            snapshot_port_connections (port_ref.get ()->get_uuid ());
        }
      for (const auto &port_ref : plugin->get_all_output_ports ())
        {
          if (port_ref.get () != nullptr)
            snapshot_port_connections (port_ref.get ()->get_uuid ());
        }
      for (const auto &param_ref : plugin->get_parameters ())
        {
          const auto * param = param_ref.get ();
          if (param == nullptr)
            continue;
          auto * mod_port = param->get_modulation_input_port_ref ().get ();
          if (mod_port != nullptr)
            snapshot_port_connections (mod_port->get_uuid ());
        }
    }
}

dsp::Port *
RemovePluginsCommand::find_port (const dsp::PortUuid &uuid) const
{
  return utils::try_get_typed<dsp::Port> (registry_, uuid);
}

void
RemovePluginsCommand::snapshot_sends_targeting_plugins ()
{
  std::unordered_set<dsp::PortUuid> removed_port_ids;
  for (const auto &info : plugin_infos_)
    {
      const auto * plugin = info.plugin_ref.get ();
      if (plugin == nullptr)
        continue;
      removed_port_ids.merge (plugin->get_all_port_ids ());
    }

  registry_.for_each_matching<structure::tracks::ChannelSend> (
    [this, &removed_port_ids] (structure::tracks::ChannelSend &send) {
      const auto destination = send.destination_port ();
      if (
        !destination.has_value ()
        || !removed_port_ids.contains (destination->id ()))
        return;
      severed_sends_.push_back (
        SeveredSend{
          structure::tracks::ChannelSendUuidReference{
                                                      send.get_uuid (), registry_ },
          destination->id ()
      });
    });
}

void
RemovePluginsCommand::redo ()
{
  // Remember each plugin's index in its group
  for (auto &info : plugin_infos_)
    {
      for (const auto i : std::views::iota (0, info.source_group->rowCount ()))
        {
          const auto qvar = info.source_group->element_at_idx (i);
          if (!qvar.canConvert<plugins::Plugin *> ())
            continue;

          const auto * pl = qvar.value<plugins::Plugin *> ();
          if (pl->get_uuid () == info.plugin_ref.id ())
            {
              info.index_in_source = i;
              break;
            }
        }
    }

  // Sort by index descending so removals don't invalidate later indices
  auto sorted_infos = plugin_infos_;
  std::ranges::sort (
    sorted_infos, std::ranges::greater (), &PluginRemoveInfo::index_in_source);

  // Remove all plugins (descending order)
  for (const auto &info : sorted_infos)
    {
      info.source_group->remove_plugin (info.plugin_ref.id ());
    }

  // Remove the connections that reference the removed plugins' ports
  for (const auto &removed : removed_connections_)
    {
      connections_manager_.remove_connection (removed.src, removed.dest);
    }

  // Sever sends whose destination is one of the removed plugins' ports
  for (const auto &severed : severed_sends_)
    {
      if (auto * send = severed.send.get ())
        {
          send->clear_destination_port ();
        }
    }

  // Destination ports that lost their last source return to the detached
  // state of unconnected modulation inputs
  for (const auto &removed : removed_connections_)
    {
      auto * destination = find_port (removed.dest);
      if (
        destination == nullptr || destination->detached ()
        || !destination->attachment_follows_connections ()
        || connections_manager_.get_num_sources (removed.dest) > 0)
        continue;
      destination->set_detached (true);
      detached_destinations_.push_back (removed.dest);
    }

  // Remove automation tracks for each plugin
  for (const auto &info : plugin_infos_)
    {
      if (info.source_atl != nullptr)
        {
          remove_plugin_automation (info.plugin_ref, *info.source_atl);
        }
    }
}

void
RemovePluginsCommand::undo ()
{
  // Re-add automation tracks first
  for (auto &removed : removed_automation_)
    {
      for (auto &track : removed.tracks)
        {
          removed.atl->add_automation_track (std::move (track));
        }
      removed.tracks.clear ();
    }
  removed_automation_.clear ();

  // Restore the removed connections
  for (const auto &removed : removed_connections_)
    {
      connections_manager_.add_connection (
        removed.src, removed.dest, removed.multiplier, removed.locked,
        removed.enabled);
      if (
        auto * conn =
          connections_manager_.get_connection (removed.src, removed.dest))
        {
          conn->bipolar_ = removed.bipolar;
        }
    }

  // Re-attach the destinations detached during redo
  for (const auto &dest_uuid : detached_destinations_)
    {
      if (auto * destination = find_port (dest_uuid))
        destination->set_detached (false);
    }
  detached_destinations_.clear ();

  // Restore the severed send destinations
  for (const auto &severed : severed_sends_)
    {
      if (auto * send = severed.send.get ())
        {
          send->set_destination_port (
            dsp::PortUuidReference{ severed.destination, send->registry () });
        }
    }

  // Sort by index ascending so reinsertions don't shift later positions
  auto sorted_infos = plugin_infos_;
  std::ranges::sort (
    sorted_infos, std::ranges::less{}, &PluginRemoveInfo::index_in_source);

  // Reinsert plugins at their original positions
  for (const auto &info : sorted_infos)
    {
      info.source_group->insert_plugin (info.plugin_ref, info.index_in_source);
    }
}

void
RemovePluginsCommand::remove_plugin_automation (
  const plugins::PluginUuidReference     &plugin_ref,
  structure::tracks::AutomationTracklist &atl)
{
  auto * plugin = plugin_ref.get ();

  // Collect matching automation tracks first to avoid iterator invalidation
  std::vector<structure::tracks::AutomationTrack *> tracks_to_remove;
  for (auto * at : atl.automation_tracks ())
    {
      auto * at_param = at->parameter ();
      if (
        std::ranges::any_of (
          plugin->get_parameters (), [at_param] (const auto &pl_param_ref) {
            return pl_param_ref.get () == at_param;
          }))
        {
          tracks_to_remove.push_back (at);
        }
    }

  // Store removed tracks for undo
  RemovedAutomation removed{ .atl = &atl, .tracks = {} };
  for (auto * at : tracks_to_remove)
    {
      removed.tracks.push_back (atl.remove_automation_track (*at));
    }
  removed_automation_.push_back (std::move (removed));
}

} // namespace zrythm::commands
