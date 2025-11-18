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
   * @param descriptor The plugin descriptor to import.
   */
  Q_INVOKABLE void
  importPluginToNewTrack (const plugins::PluginDescriptor * descriptor);

  Q_INVOKABLE void importPluginToGroup (
    const plugins::PluginDescriptor * descriptor,
    plugins::PluginGroup *            group);

  Q_INVOKABLE void importPluginToTrack (
    const plugins::PluginDescriptor * descriptor,
    structure::tracks::Track *        track);

private:
  /**
   * @brief Imports the plugin into the given plugin group, or a plugin group in
   * the given track based on the plugin type, or creates a new track based on
   * @p descriptor if @p track_or_group is empty.
   *
   * This method creates a plugin instance from the given descriptor and
   * adds it to the appropriate track's plugin group using the AddPluginCommand.
   *
   * @param descriptor
   * @param track_or_group
   */
  void import (
    const plugins::PluginDescriptor * descriptor,
    std::optional<std::variant<plugins::PluginGroup *, structure::tracks::Track *>>
      track_or_group);

private:
  undo::UndoStack        &undo_stack_;
  plugins::PluginFactory &plugin_factory_;
  const TrackCreator     &track_creator_;
  plugins::PluginFactory::InstantiationFinishedHandler
    instantiation_finished_handler_;
};

} // namespace zrythm::actions
