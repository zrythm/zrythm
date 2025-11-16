// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "actions/track_creator.h"
#include "plugins/plugin_factory.h"
#include "structure/tracks/track.h"
#include "undo/undo_stack.h"

#include <QObject>

namespace zrythm::actions
{

class PluginImporter : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  explicit PluginImporter (
    undo::UndoStack        &undo_stack,
    plugins::PluginFactory &plugin_factory,
    const TrackCreator     &track_creator,
    plugins::PluginFactory::InstantiationFinishedHandler
              instantiation_finished_handler,
    QObject * parent = nullptr);

  /**
   * @brief Imports a plugin instance from a descriptor.
   *
   * This method creates a plugin instance from the given descriptor and
   * adds it to the appropriate track's plugin group using the AddPluginCommand.
   *
   * @param descriptor The plugin descriptor to import.
   */
  Q_INVOKABLE void importPlugin (const plugins::PluginDescriptor * descriptor);

private:
  undo::UndoStack        &undo_stack_;
  plugins::PluginFactory &plugin_factory_;
  const TrackCreator     &track_creator_;
  plugins::PluginFactory::InstantiationFinishedHandler
    instantiation_finished_handler_;
};

} // namespace zrythm::actions
