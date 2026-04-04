// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <utility>

#include "plugins/plugin_group.h"
#include "structure/tracks/automation_tracklist.h"
#include "utils/qt.h"

#include <QUndoCommand>

namespace zrythm::commands
{

/**
 * @brief Command to remove one or more plugins from a PluginGroup.
 *
 * On undo, plugins are reinserted at their original positions and
 * their automation tracks are restored.
 */
class RemovePluginsCommand : public QUndoCommand
{
public:
  static constexpr int CommandId = 1775263310;

  /** Per-plugin info for the removal. */
  struct PluginRemoveInfo
  {
    /** Reference to the plugin being removed. */
    plugins::PluginUuidReference plugin_ref;
    /** Group the plugin belongs to. */
    plugins::PluginGroup * source_group{};
    /** Automation tracklist for the owning track (may be null). */
    structure::tracks::AutomationTracklist * source_atl{};
    /** Index of the plugin in its group (saved during redo). */
    int index_in_source{};
  };

  /**
   * @brief Constructs a command to remove plugins.
   *
   * @param plugin_infos Plugins to remove with their source locations.
   */
  explicit RemovePluginsCommand (std::vector<PluginRemoveInfo> plugin_infos);

  void undo () override;
  void redo () override;

  int id () const override { return CommandId; }

private:
  void remove_plugin_automation (
    const plugins::PluginUuidReference     &plugin_ref,
    structure::tracks::AutomationTracklist &atl);

  std::vector<PluginRemoveInfo> plugin_infos_;

  /** Automation tracks removed during redo, stored for undo restoration. */
  struct RemovedAutomation
  {
    structure::tracks::AutomationTracklist * atl{};
    std::vector<utils::QObjectUniquePtr<structure::tracks::AutomationTrackHolder>>
      tracks;
  };
  std::vector<RemovedAutomation> removed_automation_;
};

} // namespace zrythm::commands
