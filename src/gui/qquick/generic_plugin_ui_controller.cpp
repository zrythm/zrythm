// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>

#include "gui/qquick/generic_plugin_ui_controller.h"

namespace zrythm::gui::qquick
{

GenericPluginUiController::GenericPluginUiController (QObject * parent)
    : QAbstractListModel (parent)
{
}

void
GenericPluginUiController::trackPluginUiVisibility (plugins::Plugin * plugin)
{
  if (plugin == nullptr)
    return;

  if (tracked_plugins_.contains (plugin))
    {
      evaluate (plugin);
      return;
    }
  tracked_plugins_.insert (plugin);

  const auto reevaluate = [this, plugin] { evaluate (plugin); };
  connect (plugin, &plugins::Plugin::uiVisibleChanged, this, reevaluate);
  connect (plugin, &plugins::Plugin::hasNativeUiChanged, this, reevaluate);
  connect (plugin, &plugins::Plugin::instantiationFinished, this, reevaluate);
  connect (plugin, &QObject::destroyed, this, [this, plugin] {
    remove_plugin (plugin);
  });

  evaluate (plugin);
}

void
GenericPluginUiController::evaluate (plugins::Plugin * plugin)
{
  const bool should_show =
    plugin->uiVisible () && !plugin->hasPresentableNativeUi ()
    && plugin->instantiationStatus ()
         == plugins::Plugin::InstantiationStatus::Successful;

  const auto it = std::ranges::find (shown_plugins_, plugin);
  const bool is_shown = it != shown_plugins_.end ();

  if (should_show && !is_shown)
    {
      const int row = static_cast<int> (shown_plugins_.size ());
      beginInsertRows ({}, row, row);
      shown_plugins_.push_back (plugin);
      endInsertRows ();
    }
  else if (!should_show && is_shown)
    {
      const int row =
        static_cast<int> (std::ranges::distance (shown_plugins_.begin (), it));
      beginRemoveRows ({}, row, row);
      shown_plugins_.erase (it);
      endRemoveRows ();
    }
}

void
GenericPluginUiController::remove_plugin (plugins::Plugin * plugin)
{
  tracked_plugins_.erase (plugin);

  const auto it = std::ranges::find (shown_plugins_, plugin);
  if (it != shown_plugins_.end ())
    {
      const int row =
        static_cast<int> (std::ranges::distance (shown_plugins_.begin (), it));
      beginRemoveRows ({}, row, row);
      shown_plugins_.erase (it);
      endRemoveRows ();
    }
}

plugins::Plugin *
GenericPluginUiController::pluginAt (int row) const
{
  if (row < 0 || row >= static_cast<int> (shown_plugins_.size ()))
    return nullptr;

  return shown_plugins_.at (static_cast<size_t> (row));
}

QHash<int, QByteArray>
GenericPluginUiController::roleNames () const
{
  static const QHash<int, QByteArray> roles = {
    { PluginRole, "plugin" },
  };
  return roles;
}

int
GenericPluginUiController::rowCount (const QModelIndex &parent) const
{
  if (parent.isValid ())
    return 0;
  return static_cast<int> (shown_plugins_.size ());
}

QVariant
GenericPluginUiController::data (const QModelIndex &index, int role) const
{
  if (
    !index.isValid ()
    || index.row () >= static_cast<int> (shown_plugins_.size ()))
    return {};

  auto * plugin = shown_plugins_.at (index.row ());
  if (role == PluginRole)
    return QVariant::fromValue (plugin);

  return {};
}

} // namespace zrythm::gui::qquick
