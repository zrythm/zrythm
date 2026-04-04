// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <utility>

#include "plugins/plugin_group.h"
#include "structure/tracks/automation_tracklist.h"

#include <QUndoCommand>

namespace zrythm::commands
{

/**
 * @brief Command to move one or more plugins between PluginGroups.
 *
 * Supports moving multiple plugins at once (e.g., when multiple plugins are
 * selected and dragged together).
 */
class MovePluginsCommand : public QUndoCommand
{
public:
  static constexpr int CommandId = 1763227783;

  using PluginLocation =
    std::pair<plugins::PluginGroup *, structure::tracks::AutomationTracklist *>;

  /** Per-plugin info for the move. */
  struct PluginMoveInfo
  {
    /** Reference to the plugin being moved. */
    plugins::PluginUuidReference plugin_ref;
    /** Where the plugin currently lives. */
    PluginLocation source_location;
    /** Index of the plugin in its source group (saved during redo). */
    int index_in_source{};
  };

  /**
   * @brief Constructs a command to move plugins to a target location.
   *
   * @param plugin_infos Plugins to move with their source locations.
   * @param target_location Target PluginGroup and automation tracklist.
   * @param target_start_index Position to insert at in the target group.
   * std::nullopt means append to the end.
   */
  MovePluginsCommand (
    std::vector<PluginMoveInfo> plugin_infos,
    PluginLocation              target_location,
    std::optional<int>          target_start_index = std::nullopt);

  void undo () override;
  void redo () override;

  int id () const override { return CommandId; }

private:
  void move_plugin_automation (
    const plugins::PluginUuidReference     &plugin_ref,
    structure::tracks::AutomationTracklist &from_atl,
    structure::tracks::AutomationTracklist &to_atl);

private:
  std::vector<PluginMoveInfo> plugin_infos_;
  PluginLocation              target_location_;
  std::optional<int>          target_start_index_;
};

} // namespace zrythm::commands
