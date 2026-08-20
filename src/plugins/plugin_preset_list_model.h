// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "plugins/plugin.h"

#include <QAbstractListModel>
#include <QPointer>
#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::plugins
{

/**
 * @brief List model exposing a plugin's presets to QML.
 *
 * Assign the @ref plugin property and the model follows the plugin's
 * preset entries, resetting whenever the list content changes.
 */
class PluginPresetListModel : public QAbstractListModel
{
  Q_OBJECT
  Q_PROPERTY (
    zrythm::plugins::Plugin * plugin READ plugin WRITE setPlugin NOTIFY
      pluginChanged)
  Q_PROPERTY (int count READ rowCount NOTIFY countChanged)
  // True when at least one preset entry has a non-empty group; views use
  // this to decide whether to show group section headers
  Q_PROPERTY (bool hasGroups READ hasGroups NOTIFY hasGroupsChanged)
  QML_ELEMENT

public:
  enum Roles
  {
    NameRole = Qt::UserRole + 1,
    GroupRole,
  };
  Q_ENUM (Roles)

  explicit PluginPresetListModel (QObject * parent = nullptr);

  QHash<int, QByteArray> roleNames () const override;
  int      rowCount (const QModelIndex &parent = QModelIndex ()) const override;
  QVariant data (const QModelIndex &index, int role) const override;

  /**
   * @brief Returns the preset name at @p row, or an empty string if out of
   * range.
   *
   * For resolving a preset's display name outside of delegates. Bindings
   * do not track invokables, so callers must re-query this on @ref
   * Plugin::presetIndexChanged and model resets.
   */
  Q_INVOKABLE QString nameAt (int row) const;

  Plugin *      plugin () const { return plugin_.get (); }
  bool          hasGroups () const { return has_groups_; }
  void          setPlugin (Plugin * plugin);
  Q_SIGNAL void pluginChanged ();
  Q_SIGNAL void countChanged ();
  Q_SIGNAL void hasGroupsChanged ();

private:
  /** Resets the model from the current plugin's preset entries. */
  void reload ();

  QPointer<Plugin>        plugin_;
  QMetaObject::Connection destroyed_connection_;
  QMetaObject::Connection presets_changed_connection_;
  bool                    has_groups_ = false;

  std::vector<Plugin::PresetEntry> cached_entries_;
};

} // namespace zrythm::plugins
