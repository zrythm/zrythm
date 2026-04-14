// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <ranges>
#include <unordered_set>

#include "structure/tracks/track_collection.h"
#include "undo/undo_stack.h"
#include "utils/traits.h"

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

  /**
   * @brief Deletes the given tracks (and their descendants if foldable).
   *
   * Pushes a single DeleteTracksCommand onto the undo stack.
   * @throw std::invalid_argument if any track is non-deletable.
   */
  Q_INVOKABLE void
  deleteTracks (const QList<zrythm::structure::tracks::Track *> &tracks);

private:
  /**
   * @brief Expands track refs to include descendants of foldable tracks.
   *
   * When a folder track is in the range, all its descendants are added
   * (deduplicating against the initial set). Preserves list order.
   */
  [[nodiscard]] std::vector<structure::tracks::TrackUuidReference>
  expand_with_descendants (
    RangeOf<structure::tracks::TrackUuidReference> auto &&track_refs) const
  {
    assert (collection_ != nullptr);

    auto &registry = collection_->get_track_registry ();

    auto seen =
      track_refs
      | std::views::transform ([] (const auto &ref) { return ref.id (); })
      | std::ranges::to<std::unordered_set> ();

    auto expanded =
      track_refs
      | std::ranges::to<std::vector<structure::tracks::TrackUuidReference>> ();

    for (const auto &ref : track_refs)
      {
        if (collection_->is_track_foldable (ref.id ()))
          {
            for (
              const auto &desc_id : collection_->get_all_descendants (ref.id ()))
              {
                if (!seen.contains (desc_id))
                  {
                    expanded.emplace_back (desc_id, registry);
                    seen.insert (desc_id);
                  }
              }
          }
      }

    return expanded;
  }

  structure::tracks::TrackCollection * collection_{};
  undo::UndoStack *                    undo_stack_{};
};

} // namespace zrythm::actions
