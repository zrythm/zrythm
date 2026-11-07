// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <utility>

#include "commands/command.h"
#include "plugins/plugin_group.h"

namespace zrythm::commands
{

class AddPluginCommand : public Command
{
public:
  AddPluginCommand (
    plugins::PluginGroup        &plugin_group,
    plugins::PluginUuidReference plugin_ref,
    std::optional<int>           index = std::nullopt)
      : Command (QObject::tr ("Add Plugin"), EngineRequirement::RecalculateGraph),
        plugin_group_ (plugin_group), plugin_ref_ (std::move (plugin_ref)),
        index_ (index)
  {
  }

  void undo () override { plugin_group_.remove_plugin (plugin_ref_.id ()); }

  void redo () override
  {
    if (index_.has_value ())
      {
        plugin_group_.insert_plugin (plugin_ref_, index_.value ());
      }
    else
      {
        plugin_group_.append_plugin (plugin_ref_);
      }
  }

private:
  plugins::PluginGroup        &plugin_group_;
  plugins::PluginUuidReference plugin_ref_;
  std::optional<int>           index_;
};

} // namespace zrythm::commands
