// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "plugins/plugin_all.h"
#include "utils/views.h"

namespace zrythm::plugins
{

class PluginList : public QAbstractListModel
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  PluginList (
    plugins::PluginRegistry &plugin_registry,
    QObject *                parent = nullptr)
      : QAbstractListModel (parent), plugin_registry_ (plugin_registry)
  {
  }

  // ========================================================================
  // QML Interface
  // ========================================================================

  enum Roles
  {
    PluginVariantRole = Qt::UserRole + 1,
  };

  QHash<int, QByteArray> roleNames () const override
  {
    static QHash<int, QByteArray> roles = {
      { PluginVariantRole, "plugin" },
    };
    return roles;
  }
  int rowCount (const QModelIndex &parent = QModelIndex ()) const override
  {
    if (parent.isValid ())
      return 0;
    return static_cast<int> (plugins_.size ());
  }
  QVariant
  data (const QModelIndex &index, int role = Qt::DisplayRole) const override
  {
    if (!index.isValid () || index.row () >= static_cast<int> (plugins_.size ()))
      return {};

    switch (role)
      {
      case PluginVariantRole:
        return QVariant::fromStdVariant (
          plugins_.at (index.row ()).get_object ());
      default:
        return {};
      }
  }

  // ========================================================================

  // Note: plugin add/remove API does not concern itself with automation tracks
  // or graph rebuilding.
  // When plugins are                                   added or removed,
  // automation tracks should be generated/moved accordingly and the DSP graph
  // should be regenerated.

  void insert_plugin (plugins::PluginUuidReference plugin_ref, int index = -1)
  {
    beginInsertRows (
      {}, static_cast<int> (plugins_.size ()),
      static_cast<int> (plugins_.size ()));
    if (index == -1)
      {
        plugins_.push_back (plugin_ref);
      }
    else
      {
        plugins_.insert (
          std::ranges::next (plugins_.begin (), index), plugin_ref);
      }
    endInsertRows ();
  }
  void append_plugin (plugins::PluginUuidReference plugin_ref)
  {
    insert_plugin (plugin_ref, -1);
  }
  plugins::PluginUuidReference
  remove_plugin (const plugins::Plugin::Uuid &plugin_id)
  {
    auto it = std::ranges::find (
      plugins_, plugin_id, &plugins::PluginUuidReference::id);
    if (it == plugins_.end ())
      throw std::invalid_argument ("Plugin ID not found");

    const auto index = std::ranges::distance (plugins_.begin (), it);
    beginRemoveRows ({}, static_cast<int> (index), static_cast<int> (index));
    auto erased_plugin_it = plugins_.erase (it);
    endRemoveRows ();

    return *erased_plugin_it;
  }

  auto &plugins () const { return plugins_; }

private:
  static constexpr auto kPluginsKey = "plugins"sv;
  friend void           to_json (nlohmann::json &j, const PluginList &l)
  {
    j[kPluginsKey] = l.plugins_;
  }
  friend void from_json (const nlohmann::json &j, PluginList &l)
  {
    for (
      const auto &[index, pl_json] :
      utils::views::enumerate (j.at (kPluginsKey)))
      {
        plugins::PluginUuidReference ref{ l.plugin_registry_ };
        from_json (pl_json, ref);
        l.append_plugin (ref);
      }
  }

private:
  plugins::PluginRegistry                  &plugin_registry_;
  std::vector<plugins::PluginUuidReference> plugins_;
};
} // namespace zrythm::plugins
