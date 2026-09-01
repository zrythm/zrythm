// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>

#include "plugins/plugin_preset_list_model.h"

#include <QHash>
#include <QVariantMap>

namespace zrythm::plugins
{

PluginPresetListModel::PluginPresetListModel (QObject * parent)
    : QAbstractListModel (parent)
{
}

QHash<int, QByteArray>
PluginPresetListModel::roleNames () const
{
  static const QHash<int, QByteArray> roles = {
    { Qt::DisplayRole, "display" },
    { NameRole,        "name"    },
    { GroupRole,       "group"   },
  };
  return roles;
}

int
PluginPresetListModel::rowCount (const QModelIndex &parent) const
{
  if (parent.isValid ())
    return 0;
  return static_cast<int> (cached_entries_.size ());
}

QVariant
PluginPresetListModel::data (const QModelIndex &index, int role) const
{
  if (
    !index.isValid ()
    || index.row () >= static_cast<int> (cached_entries_.size ()))
    return {};

  const auto &entry = cached_entries_.at (static_cast<size_t> (index.row ()));

  switch (role)
    {
    case NameRole:
    case Qt::DisplayRole:
      return entry.name;
    case GroupRole:
      return entry.group;
    default:
      return {};
    }
}

QString
PluginPresetListModel::nameAt (int row) const
{
  if (row < 0 || row >= static_cast<int> (cached_entries_.size ()))
    return {};
  return cached_entries_.at (static_cast<size_t> (row)).name;
}

QString
PluginPresetListModel::groupAt (int row) const
{
  if (row < 0 || row >= static_cast<int> (cached_entries_.size ()))
    return {};
  return cached_entries_.at (static_cast<size_t> (row)).group;
}

void
PluginPresetListModel::setPlugin (Plugin * plugin)
{
  if (plugin_ == plugin)
    return;

  disconnect (destroyed_connection_);
  disconnect (presets_changed_connection_);

  plugin_ = plugin;
  if (plugin_ != nullptr)
    {
      destroyed_connection_ =
        connect (plugin_, &QObject::destroyed, this, [this] () {
          plugin_ = nullptr;
          disconnect (presets_changed_connection_);
          Q_EMIT pluginChanged ();
          // Queued: destroyed() is emitted before QML clears its references
          // to the plugin, so a synchronous reset here would let views call
          // back into the partially destroyed plugin
          QMetaObject::invokeMethod (
            this, &PluginPresetListModel::reload, Qt::QueuedConnection);
        });
      presets_changed_connection_ =
        connect (plugin_, &Plugin::presetsChanged, this, [this] { reload (); });
    }

  reload ();
  Q_EMIT pluginChanged ();
}

void
PluginPresetListModel::reload ()
{
  std::vector<Plugin::PresetEntry> new_entries;
  if (plugin_ != nullptr)
    {
      const auto entries = plugin_->presetEntries ();
      new_entries.assign (entries.begin (), entries.end ());
    }
  const auto has_groups = std::ranges::any_of (
    new_entries, [] (const auto &entry) { return !entry.group.isEmpty (); });

  // Named groups in order of first appearance, with per-group counts
  QHash<QString, qsizetype> group_indices;
  QVariantList              new_groups;
  for (const auto &entry : new_entries)
    {
      if (entry.group.isEmpty ())
        continue;
      const auto it = group_indices.find (entry.group);
      if (it == group_indices.end ())
        {
          group_indices.insert (entry.group, new_groups.size ());
          new_groups.append (
            QVariantMap{
              { QStringLiteral ("name"),  entry.group },
              { QStringLiteral ("count"), 1           },
          });
        }
      else
        {
          auto map = new_groups[*it].toMap ();
          map[QStringLiteral ("count")] =
            map[QStringLiteral ("count")].toInt () + 1;
          new_groups[*it] = map;
        }
    }

  const auto old_size = cached_entries_.size ();
  beginResetModel ();
  cached_entries_ = std::move (new_entries);
  // Group-related state must be updated before endResetModel(): views
  // react to the reset synchronously and would read the stale value
  if (has_groups != has_groups_)
    {
      has_groups_ = has_groups;
      Q_EMIT hasGroupsChanged ();
    }
  if (new_groups != groups_)
    {
      groups_ = std::move (new_groups);
      Q_EMIT groupsChanged ();
    }
  endResetModel ();
  if (cached_entries_.size () != old_size)
    Q_EMIT countChanged ();
}

} // namespace zrythm::plugins
