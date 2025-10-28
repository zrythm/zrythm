// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <utility>

#include "plugins/plugin_group.h"
#include "structure/tracks/automation_tracklist.h"

#include <QPointer>
#include <QUndoCommand>

namespace zrythm::commands
{
class MovePluginCommand : public QUndoCommand
{
public:
  using PluginLocation =
    std::pair<plugins::PluginGroup *, structure::tracks::AutomationTracklist *>;

  MovePluginCommand (
    plugins::PluginUuidReference plugin_ref,
    PluginLocation               source_locaction,
    PluginLocation               target_locaction,
    std::optional<int>           index = std::nullopt);

  void undo () override;
  void redo () override;

private:
  void move_plugin_automation (
    structure::tracks::AutomationTracklist &from_atl,
    structure::tracks::AutomationTracklist &to_atl);

private:
  plugins::PluginUuidReference plugin_ref_;
  PluginLocation               target_location_;
  PluginLocation               source_location_;
  int                          index_in_previous_list_{};
  std::optional<int>           index_;
};

} // namespace zrythm::commands
