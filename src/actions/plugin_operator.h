// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "plugins/plugin_all.h"
#include "structure/tracks/track_all.h"
#include "undo/undo_stack.h"

#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::actions
{

class PluginOperator : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("Needs plugin registry")

public:
  explicit PluginOperator (
    undo::UndoStack         &undo_stack,
    plugins::PluginRegistry &plugin_registry,
    QObject *                parent = nullptr)
      : QObject (parent), plugin_registry_ (plugin_registry),
        undo_stack_ (undo_stack)
  {
  }

  /**
   * @brief Moves one or more plugins between PluginGroups.
   *
   * @param plugins Plugins to move.
   * @param source_group Group the plugins currently belong to.
   * @param source_track Track owning the source group (for automation moves).
   * @param target_group Group to move the plugins into.
   * @param target_track Track owning the target group (for automation moves).
   * @param target_start_index Starting position in the target group.
   * -1 means append to the end.
   */
  Q_INVOKABLE void movePlugins (
    QList<plugins::Plugin *>   plugins,
    plugins::PluginGroup *     source_group,
    structure::tracks::Track * source_track,
    plugins::PluginGroup *     target_group,
    structure::tracks::Track * target_track,
    int                        target_start_index = -1);

private:
  plugins::PluginRegistry &plugin_registry_;
  undo::UndoStack         &undo_stack_;
};

} // namespace zrythm::actions
