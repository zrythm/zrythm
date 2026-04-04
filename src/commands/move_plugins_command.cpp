// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/move_plugins_command.h"

namespace zrythm::commands
{

MovePluginsCommand::MovePluginsCommand (
  std::vector<PluginMoveInfo> plugin_infos,
  PluginLocation              target_location,
  std::optional<int>          target_start_index)
    : QUndoCommand (
        plugin_infos.size () == 1
          ? QObject::tr ("Move Plugin")
          : QObject::tr ("Move %n Plugin(s)", "", plugin_infos.size ())),
      plugin_infos_ (std::move (plugin_infos)),
      target_location_ (target_location), target_start_index_ (target_start_index)
{
  for (const auto &info : plugin_infos_)
    {
      std::vector<plugins::PluginPtrVariant> plugins;
      info.source_location.first->get_plugins (plugins);
      if (
        !std::ranges::contains (
          plugins | std::views::transform (plugins::plugin_ptr_variant_to_base),
          info.plugin_ref.id (), &plugins::Plugin::get_uuid))
        {
          throw std::invalid_argument (
            "Source group does not include the plugin");
        }
    }

  if (target_start_index.has_value ())
    {
      const auto max_index =
        static_cast<int> (target_location.first->rowCount ());
      if (target_start_index.value () > max_index)
        {
          throw std::invalid_argument ("Target start index out of bounds");
        }
    }
}

void
MovePluginsCommand::undo ()
{
  // Sort by source index ascending so reinsertions don't shift
  // later positions
  auto sorted_infos = plugin_infos_;
  std::ranges::sort (
    sorted_infos, std::ranges::less{}, &PluginMoveInfo::index_in_source);

  for (auto &info : sorted_infos)
    {
      // Remove from target
      auto plugin_ref =
        target_location_.first->remove_plugin (info.plugin_ref.id ());

      // Insert back at original index in source
      info.source_location.first->insert_plugin (
        plugin_ref, info.index_in_source);

      // Move automation back
      auto * source_atl = info.source_location.second;
      auto * target_atl = target_location_.second;
      if ((source_atl != nullptr) && (target_atl != nullptr))
        {
          move_plugin_automation (info.plugin_ref, *target_atl, *source_atl);
        }
    }
}

void
MovePluginsCommand::redo ()
{
  // Remember each plugin's index in its source group
  for (auto &info : plugin_infos_)
    {
      for (
        const auto i :
        std::views::iota (0, info.source_location.first->rowCount ()))
        {
          const auto qvar = info.source_location.first->element_at_idx (i);
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

  // Sort by source index descending so removals don't invalidate later indices
  auto sorted_infos = plugin_infos_;
  std::ranges::sort (
    sorted_infos, std::ranges::greater (), &PluginMoveInfo::index_in_source);

  // Remove all plugins from their sources (descending order)
  std::vector<plugins::PluginUuidReference> removed_plugins;
  removed_plugins.reserve (sorted_infos.size ());
  for (auto &info : sorted_infos)
    {
      auto plugin_ref =
        info.source_location.first->remove_plugin (info.plugin_ref.id ());
      removed_plugins.push_back (std::move (plugin_ref));
    }

  // Insert into target at the requested position
  auto insert_index =
    target_start_index_.value_or (target_location_.first->rowCount ());
  for (auto &plugin_ref : removed_plugins | std::views::reverse)
    {
      target_location_.first->insert_plugin (
        std::move (plugin_ref), insert_index);
      ++insert_index;
    }

  // Move automation for each plugin
  for (auto &info : plugin_infos_)
    {
      auto * source_atl = info.source_location.second;
      auto * target_atl = target_location_.second;
      if ((source_atl != nullptr) && (target_atl != nullptr))
        {
          move_plugin_automation (info.plugin_ref, *source_atl, *target_atl);
        }
    }
}

void
MovePluginsCommand::move_plugin_automation (
  const plugins::PluginUuidReference     &plugin_ref,
  structure::tracks::AutomationTracklist &from_atl,
  structure::tracks::AutomationTracklist &to_atl)
{
  if (&from_atl == &to_atl)
    return;

  auto * plugin = plugins::plugin_ptr_variant_to_base (plugin_ref.get_object ());

  // Collect matching automation tracks first to avoid iterator invalidation
  // when removing from the vector during iteration.
  std::vector<structure::tracks::AutomationTrack *> tracks_to_move;
  for (auto * at : from_atl.automation_tracks ())
    {
      auto * at_param = at->parameter ();
      for (const auto &pl_param_ref : plugin->get_parameters ())
        {
          const auto * pl_param =
            pl_param_ref.get_object_as<dsp::ProcessorParameter> ();
          if (pl_param == at_param)
            {
              tracks_to_move.push_back (at);
              break;
            }
        }
    }

  for (auto * at : tracks_to_move)
    {
      auto removed_at = from_atl.remove_automation_track (*at);
      to_atl.add_automation_track (std::move (removed_at));
    }
}

} // namespace zrythm::commands
