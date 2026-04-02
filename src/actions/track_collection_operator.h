// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/tracks/track_collection.h"
#include "undo/undo_stack.h"

#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::actions
{

/**
 * @brief QML-exposed operator for track collection operations.
 *
 * This class provides QML bindings for operations that affect multiple
 * tracks or the track collection as a whole, such as reordering tracks.
 */
class TrackCollectionOperator : public QObject
{
  Q_OBJECT
  Q_PROPERTY (
    zrythm::structure::tracks::TrackCollection * collection READ collection
      WRITE setCollection NOTIFY collectionChanged)
  Q_PROPERTY (
    zrythm::undo::UndoStack * undoStack READ undoStack WRITE setUndoStack NOTIFY
      undoStackChanged)
  QML_ELEMENT

public:
  explicit TrackCollectionOperator (QObject * parent = nullptr)
      : QObject (parent)
  {
  }

  structure::tracks::TrackCollection * collection () const
  {
    return collection_;
  }
  void setCollection (structure::tracks::TrackCollection * collection)
  {
    if (collection_ != collection)
      {
        collection_ = collection;
        Q_EMIT collectionChanged ();
      }
  }
  Q_SIGNAL void collectionChanged ();

  undo::UndoStack * undoStack () const { return undo_stack_; }
  void              setUndoStack (undo::UndoStack * undoStack)
  {
    if (undo_stack_ != undoStack)
      {
        undo_stack_ = undoStack;
        Q_EMIT undoStackChanged ();
      }
  }
  Q_SIGNAL void undoStackChanged ();

  /**
   * @brief Moves tracks to a new position in the collection.
   *
   * @param tracks List of tracks to move (in their current order).
   * @param targetPosition The position where the first track should end up.
   * @param targetFolder If non-null, the foldable track to make the moved
   *   tracks children of. If null and the target position is inside an
   *   expanded folder, the enclosing folder is inferred automatically.
   */
  Q_INVOKABLE void moveTracks (
    const QList<zrythm::structure::tracks::Track *> &tracks,
    int                                              targetPosition,
    zrythm::structure::tracks::Track *               targetFolder);

  /**
   * @brief Convenience overload for QML - moves without specifying a folder.
   *
   * QML cannot resolve C++ default parameters on Q_INVOKABLE methods.
   * Use this overload when no target folder is needed.
   */
  Q_INVOKABLE void moveTracks (
    const QList<zrythm::structure::tracks::Track *> &tracks,
    int                                              targetPosition)
  {
    moveTracks (tracks, targetPosition, nullptr);
  }

private:
  structure::tracks::TrackCollection * collection_{};
  undo::UndoStack *                    undo_stack_{};
};

} // namespace zrythm::actions
