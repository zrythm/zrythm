// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <functional>
#include <ranges>
#include <unordered_set>

#include "controllers/clipboard.h"
#include "structure/project/project_registry.h"
#include "structure/tracks/singleton_tracks.h"
#include "structure/tracks/track_collection.h"
#include "structure/tracks/track_routing.h"
#include "undo/undo_stack.h"
#include "utils/traits.h"

#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::actions
{

/**
 * @brief QML-exposed operator for track collection operations.
 *
 * This class provides QML bindings for operations that affect multiple
 * tracks or the track collection as a whole, such as reordering,
 * deleting and clipboard operations.
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
  Q_PROPERTY (
    bool canPasteTracks READ canPasteTracks NOTIFY canPasteTracksChanged FINAL)
  QML_ELEMENT
  QML_UNCREATABLE ("Owned by the project session")

public:
  /**
   * @brief Constructs an operator with full dependencies for clipboard
   * operations.
   *
   * @param track_routing Per-tracklist output routing (captured on copy,
   * restored on paste).
   * @param singleton_tracks Source of the master track for the default
   * route of pasted audio tracks.
   * @param project_id_provider Returns the current project id (regenerated
   * by Save As), identifying the payload's source project.
   */
  explicit TrackCollectionOperator (
    undo::UndoStack                     &undo_stack,
    structure::project::ProjectRegistry &project_registry,
    controllers::Clipboard              &clipboard,
    structure::tracks::TrackCollection  &collection,
    structure::tracks::TrackRouting     &track_routing,
    structure::tracks::SingletonTracks  &singleton_tracks,
    std::function<QString ()>            project_id_provider,
    QObject *                            parent = nullptr);

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

  undo::UndoStack * undoStack () const { return undo_stack_; }
  void              setUndoStack (undo::UndoStack * undoStack)
  {
    if (undo_stack_ != undoStack)
      {
        undo_stack_ = undoStack;
        Q_EMIT undoStackChanged ();
      }
  }

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

  /**
   * @brief Copies the closure of @p tracks (each track with its lanes,
   * clips, automation, channel and plugins) to the clipboard.
   *
   * Folder tracks are expanded to their descendants. Fails if any
   * selected track is of a non-copyable type.
   *
   * @return false if the selection is empty or the copy was refused, in
   * which case the clipboard keeps its previous payload.
   */
  Q_INVOKABLE bool
  copyTracks (const QList<zrythm::structure::tracks::Track *> &tracks);

  /**
   * @brief Copies @p tracks to the clipboard, then deletes them.
   *
   * @return false if the copy step refused the operation; in that case
   * nothing is deleted.
   */
  Q_INVOKABLE bool
  cutTracks (const QList<zrythm::structure::tracks::Track *> &tracks);

  /**
   * @brief Pastes the clipboard's tracks into the collection.
   *
   * The pasted tracks are named uniquely against the collection, their
   * in-set folder nesting and routing are restored, and tracks routed
   * outside the pasted set keep that route in the same project (audio
   * tracks fall back to the master track otherwise, MIDI tracks get no
   * route). Plugins the tracks carry must finish instantiating before
   * anything is attached; a failed or timed-out instantiation refuses
   * the paste.
   *
   * @param target_position Position the first pasted track ends up at
   * (-1 appends to the end of the collection).
   * @return The pasted tracks' UUID strings (brace-less), empty if the
   * paste was refused.
   */
  Q_INVOKABLE QVariantList pasteTracks (int target_position);

  /**
   * @brief Duplicates @p tracks, inserting after the last of them.
   *
   * The duplicates are cloned directly without touching the clipboard
   * contents.
   *
   * @return The duplicated tracks' UUID strings (brace-less), empty if
   * the operation was refused.
   */
  Q_INVOKABLE QVariantList
  duplicateTracks (const QList<zrythm::structure::tracks::Track *> &tracks);

  /** Whether the clipboard holds a pasteable track payload. */
  bool canPasteTracks () const
  {
    return clipboard_ != nullptr && clipboard_->hasTracks ();
  }

Q_SIGNALS:
  /**
   * @brief Emitted when an operation is refused (e.g. a non-copyable
   * track is selected), with the reason.
   */
  void operationRefused (const QString &reason);

  /**
   * @brief Emitted after a paste whose content was modified (dropped
   * audio or severed references), with a summary.
   */
  void pasteContentModified (const QString &summary);

  void collectionChanged ();
  void undoStackChanged ();
  void canPasteTracksChanged ();

private:
  /** A clipboard payload with the IDs of its copied roots. */
  struct PreparedTrackCopy
  {
    structure::project::ClipboardPayload payload;
    std::vector<QUuid>                   root_ids;
  };

  /** A paste-ready payload with the IDs its import registered. */
  struct PreparedTrackPaste
  {
    structure::project::ClipboardPayload payload;
    std::vector<QUuid>                   imported_ids;
    /** Pool-bound objects dropped while filtering (their audio data is
     * unresolvable in the target project). */
    std::size_t dropped_audio_objects = 0;
    /** External references that did not resolve in the target and were
     * severed while filtering. */
    std::size_t severed_references = 0;
  };

  /**
   * @brief Serializes the closure of @p tracks into a track payload.
   *
   * Folder tracks are expanded to their descendants, and the routing and
   * folder nesting of the roots are captured as metadata.
   *
   * @return The payload, or std::nullopt if nothing is copyable, a
   * non-copyable track is selected, or serialization fails.
   */
  [[nodiscard]] std::optional<PreparedTrackCopy>
  build_tracks_payload (const QList<structure::tracks::Track *> &tracks) const;

  /**
   * @brief Filters, regenerates the UUIDs of, and imports @p payload's
   * tracks into the project registry.
   */
  [[nodiscard]] std::optional<PreparedTrackPaste>
  import_payload_for_paste (const structure::project::ClipboardPayload &payload);

  /**
   * @brief Attaches the imported roots of @p paste to the collection.
   *
   * Names each root uniquely against the collection (and the roots added
   * before it), waits for the tracks' plugins to finish instantiating
   * (refusing the paste on failure or timeout), moves the tracks to @p
   * target_position, restores their folder nesting and routing, and
   * discards the imports the attached roots do not need.
   *
   * @param label_pattern Translated undo-macro label with a %1
   * placeholder for the track count.
   * @param explicit_target_folder When set, the pasted block's topmost
   * tracks become children of this folder regardless of the target
   * position (duplicating keeps the source's folder this way).
   * @return The attached tracks' UUID strings (brace-less), empty if
   * the paste was refused.
   */
  [[nodiscard]] QVariantList attach_paste (
    const PreparedTrackPaste                     &paste,
    int                                           target_position,
    const QString                                &label_pattern,
    std::optional<structure::tracks::Track::Uuid> explicit_target_folder =
      std::nullopt);

  /** Logs @p reason and emits operationRefused(). */
  void refuse_operation (const QString &reason);

  /** Whether the dependencies needed by clipboard operations are set. */
  bool clipboard_dependencies_ready () const;

  /**
   * @brief Expands track refs to include descendants of foldable tracks.
   *
   * When a folder track is in the range, all its descendants are added
   * (deduplicating against the initial set). Preserves list order.
   */
  [[nodiscard]] std::vector<structure::tracks::TrackUuidReference>
  expand_with_descendants (
    utils::RangeOf<structure::tracks::TrackUuidReference> auto &&track_refs) const
  {
    assert (collection_ != nullptr);

    auto &registry = collection_->get_registry ();

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

  /** Clipboard-operation dependencies (set by the full constructor). */
  structure::project::ProjectRegistry * project_registry_{};
  controllers::Clipboard *              clipboard_{};
  structure::tracks::TrackRouting *     track_routing_{};
  structure::tracks::SingletonTracks *  singleton_tracks_{};
  std::function<QString ()>             project_id_provider_;
};

} // namespace zrythm::actions
