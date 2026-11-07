// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <utility>
#include <vector>

#include "commands/command.h"
#include "dsp/port_fwd.h"
#include "plugins/plugin_group.h"
#include "structure/project/port_connections_manager.h"
#include "structure/tracks/automation_tracklist.h"
#include "utils/iobject_registry.h"
#include "utils/qt.h"

namespace zrythm::commands
{

/**
 * @brief Command to remove one or more plugins from a PluginGroup.
 *
 * Port connections referencing the removed plugins' ports are removed with
 * them; on undo, plugins are reinserted at their original positions, their
 * automation tracks are restored and the connections are re-added. Ports
 * are identified by UUID and resolved through the project registry when the
 * command runs, so the command holds no port pointers.
 */
class RemovePluginsCommand : public Command
{
public:
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
   * @param connections_manager Project connection manager owning the
   * connections to remove.
   * @param registry Registry the connection ports live in.
   */
  RemovePluginsCommand (
    std::vector<PluginRemoveInfo>               plugin_infos,
    structure::project::PortConnectionsManager &connections_manager,
    utils::IObjectRegistry                     &registry);

  void undo () override;
  void redo () override;

private:
  void remove_plugin_automation (
    const plugins::PluginUuidReference     &plugin_ref,
    structure::tracks::AutomationTracklist &atl);

  /** Snapshots the connections that reference the plugins' ports. */
  void snapshot_plugin_connections ();

  /** Resolves a port UUID through the registry, or null. */
  [[nodiscard]] dsp::Port * find_port (const dsp::PortUuid &uuid) const;

  std::vector<PluginRemoveInfo> plugin_infos_;

  structure::project::PortConnectionsManager &connections_manager_;
  utils::IObjectRegistry                     &registry_;

  /** Connection values removed during redo, stored for undo restoration. */
  struct RemovedConnection
  {
    dsp::PortUuid src;
    dsp::PortUuid dest;
    float         multiplier{};
    bool          locked{};
    bool          enabled{};
    bool          bipolar{};
  };
  std::vector<RemovedConnection> removed_connections_;

  /** Destinations detached during redo because their last source went
   * away, stored by UUID. */
  std::vector<dsp::PortUuid> detached_destinations_;

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
