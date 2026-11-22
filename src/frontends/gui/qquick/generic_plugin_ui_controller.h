// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <unordered_set>
#include <vector>

#include "plugins/plugin.h"

#include <QAbstractListModel>
#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::frontends::gui::qquick
{

/**
 * @brief Model of plugins that currently need a generic (non-native) UI
 * window.
 *
 * Plugins are registered via trackPluginUiVisibility(); the model then reacts
 * to UI visibility / native-UI / instantiation changes, exposing one row per
 * plugin that needs a generic UI window. Rows are added and removed
 * incrementally so that existing windows are never recreated.
 */
class GenericPluginUiController : public QAbstractListModel
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  enum Roles
  {
    PluginRole = Qt::UserRole + 1,
  };
  Q_ENUM (Roles)

  explicit GenericPluginUiController (QObject * parent = nullptr);

  /**
   * @brief Starts tracking the given plugin's UI visibility (idempotent).
   *
   * To be called when a plugin is added via the UI, and after project load
   * for each existing plugin.
   */
  void trackPluginUiVisibility (plugins::Plugin * plugin);

  /**
   * @brief Returns the plugin at the given row, or nullptr if out of range.
   */
  Q_INVOKABLE plugins::Plugin * pluginAt (int row) const;

  QHash<int, QByteArray> roleNames () const override;
  int      rowCount (const QModelIndex &parent = QModelIndex ()) const override;
  QVariant data (const QModelIndex &index, int role) const override;

private:
  void evaluate (plugins::Plugin * plugin);
  void remove_plugin (plugins::Plugin * plugin);

  std::vector<plugins::Plugin *>        shown_plugins_;
  std::unordered_set<plugins::Plugin *> tracked_plugins_;
};

} // namespace zrythm::frontends::gui::qquick
