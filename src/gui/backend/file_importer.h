// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "actions/arranger_object_creator.h"
#include "actions/track_creator.h"

namespace zrythm::gui::backend
{
class FileImporter : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("One instance per project")

public:
  explicit FileImporter (
    undo::UndoStack                          &undo_stack,
    ::zrythm::actions::ArrangerObjectCreator &arranger_object_creator,
    ::zrythm::actions::TrackCreator          &track_creator,
    QObject *                                 parent = nullptr)
      : QObject (parent), track_creator_ (track_creator),
        arranger_object_creator_ (arranger_object_creator),
        undo_stack_ (undo_stack)
  {
  }

  /**
   * @brief Imports the given file.
   *
   * @param path File to import.
   * @param startTicks Start ticks to place file contents at, or 0.
   * @param track Track to import in (optional).
   */
  Q_INVOKABLE void importFiles (
    const QStringList         &filePaths,
    double                     startTicks,
    structure::tracks::Track * track) const;

private:
  ::zrythm::actions::TrackCreator          &track_creator_;
  ::zrythm::actions::ArrangerObjectCreator &arranger_object_creator_;
  undo::UndoStack                          &undo_stack_;
};
}
