// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/plugin_operator.h"
#include "commands/move_plugins_command.h"

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

  // Build per-plugin move info
  std::vector<commands::MovePluginsCommand::PluginMoveInfo> infos;
  infos.reserve (plugins.size ());
  for (auto * plugin : plugins)
    {
      if (!plugin)
        continue;

      commands::MovePluginsCommand::PluginMoveInfo info{
        .plugin_ref =
          plugins::PluginUuidReference (plugin->get_uuid (), plugin_registry_),
        .source_location = PluginLocation{ source_group, source_atl }
      };

      infos.push_back (std::move (info));
    }

  auto target_idx =
    (target_start_index >= 0)
      ? std::optional<int> (target_start_index)
      : std::nullopt;

  auto cmd = std::make_unique<commands::MovePluginsCommand> (
    std::move (infos), PluginLocation{ target_group, target_atl }, target_idx);

  undo_stack_.push (cmd.release ());
}

} // namespace zrythm::actions
