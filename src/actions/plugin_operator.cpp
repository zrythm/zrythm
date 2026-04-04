// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/plugin_operator.h"
#include "commands/move_plugins_command.h"
#include "commands/remove_plugins_command.h"

namespace zrythm::actions
{

void
PluginOperator::movePlugins (
  QList<plugins::Plugin *>   plugins,
  plugins::PluginGroup *     source_group,
  structure::tracks::Track * source_track,
  plugins::PluginGroup *     target_group,
  structure::tracks::Track * target_track,
  int                        target_start_index)
{
  if (plugins.isEmpty () || !source_group || !target_group)
    {
      z_warning ("movePlugins: invalid arguments");
      return;
    }

  auto source_atl =
    source_track ? source_track->automationTracklist () : nullptr;
  auto target_atl =
    target_track ? target_track->automationTracklist () : nullptr;

  using PluginLocation = commands::MovePluginsCommand::PluginLocation;

  auto cmd = std::make_unique<commands::MovePluginsCommand> (
    plugins | std::views::filter ([] (auto * p) {
      return p != nullptr;
    }) | std::views::transform ([this, source_group, source_atl] (auto * plugin) {
      return commands::MovePluginsCommand::PluginMoveInfo{
        .plugin_ref =
          plugins::PluginUuidReference (plugin->get_uuid (), plugin_registry_),
        .source_location = PluginLocation{ source_group, source_atl }
      };
    }) | std::ranges::to<std::vector> (),
    PluginLocation{ target_group, target_atl },
    (target_start_index >= 0)
      ? std::optional<int> (target_start_index)
      : std::nullopt);

  undo_stack_.push (cmd.release ());
}

void
PluginOperator::removePlugins (
  QList<plugins::Plugin *>   plugins,
  plugins::PluginGroup *     group,
  structure::tracks::Track * track)
{
  if (plugins.isEmpty () || !group)
    {
      z_warning ("removePlugins: invalid arguments");
      return;
    }

  auto atl = track ? track->automationTracklist () : nullptr;

  auto cmd = std::make_unique<commands::RemovePluginsCommand> (
    plugins | std::views::filter ([] (auto * p) { return p != nullptr; })
    | std::views::transform ([this, group, atl] (auto * plugin) {
        return commands::RemovePluginsCommand::PluginRemoveInfo{
          .plugin_ref = plugins::PluginUuidReference (
            plugin->get_uuid (), plugin_registry_),
          .source_group = group,
          .source_atl = atl
        };
      })
    | std::ranges::to<std::vector> ());

  undo_stack_.push (cmd.release ());
}

} // namespace zrythm::actions
