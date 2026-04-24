// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/remove_plugins_command.h"

namespace zrythm::commands
{

RemovePluginsCommand::RemovePluginsCommand (
  std::vector<PluginRemoveInfo> plugin_infos)
    : QUndoCommand (
        plugin_infos.size () == 1
          ? QObject::tr ("Remove Plugin")
          : QObject::tr ("Remove %n Plugin(s)", "", plugin_infos.size ())),
      plugin_infos_ (std::move (plugin_infos))
{
  for (const auto &info : plugin_infos_)
    {
      std::vector<plugins::PluginPtrVariant> plugins;
      info.source_group->get_plugins (plugins);
      if (
        !std::ranges::contains (
          plugins | std::views::transform (plugins::plugin_ptr_variant_to_base),
          info.plugin_ref.id (), &plugins::Plugin::get_uuid))
        {
          throw std::invalid_argument (
            "Source group does not include the plugin");
        }
    }
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

  // Close plugin windows before removal
  for (const auto &info : sorted_infos)
    {
      auto * plugin =
        plugins::plugin_ptr_variant_to_base (info.plugin_ref.get_object ());
      if (plugin->uiVisible ())
        {
          plugin->setUiVisible (false);
        }
    }

  // Remove all plugins (descending order)
  for (auto &info : sorted_infos)
    {
      info.source_group->remove_plugin (info.plugin_ref.id ());
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

  // Sort by index ascending so reinsertions don't shift later positions
  auto sorted_infos = plugin_infos_;
  std::ranges::sort (
    sorted_infos, std::ranges::less{}, &PluginRemoveInfo::index_in_source);

  // Reinsert plugins at their original positions
  for (auto &info : sorted_infos)
    {
      info.source_group->insert_plugin (info.plugin_ref, info.index_in_source);
    }
}

void
RemovePluginsCommand::remove_plugin_automation (
  const plugins::PluginUuidReference     &plugin_ref,
  structure::tracks::AutomationTracklist &atl)
{
  auto * plugin = plugins::plugin_ptr_variant_to_base (plugin_ref.get_object ());

  // Collect matching automation tracks first to avoid iterator invalidation
  std::vector<structure::tracks::AutomationTrack *> tracks_to_remove;
  for (auto * at : atl.automation_tracks ())
    {
      auto * at_param = at->parameter ();
      if (
        std::ranges::any_of (
          plugin->get_parameters (), [at_param] (const auto &pl_param_ref) {
            return pl_param_ref.template get_object_as<dsp::ProcessorParameter> ()
                   == at_param;
          }))
        {
          tracks_to_remove.push_back (at);
        }
    }

  // Store removed tracks for undo
  RemovedAutomation removed{ .atl = &atl };
  for (auto * at : tracks_to_remove)
    {
      removed.tracks.push_back (atl.remove_automation_track (*at));
    }
  removed_automation_.push_back (std::move (removed));
}

} // namespace zrythm::commands
