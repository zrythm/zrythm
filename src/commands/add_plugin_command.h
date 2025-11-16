// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <utility>

#include "plugins/plugin_group.h"

#include <QUndoCommand>

namespace zrythm::commands
{

class AddPluginCommand : public QUndoCommand
{
public:
  static constexpr auto CommandId = 1763165707;

  AddPluginCommand (
    plugins::PluginGroup        &plugin_group,
    plugins::PluginUuidReference plugin_ref,
    std::optional<int>           index = std::nullopt)
      : QUndoCommand (QObject::tr ("Add Plugin")), plugin_group_ (plugin_group),
        plugin_ref_ (std::move (plugin_ref)), index_ (index)
  {
  }

  int id () const override { return CommandId; }

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
