// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <functional>
#include <unordered_map>

#include "commands/change_qobject_property_command.h"
#include "commands/resize_arranger_objects_command.h"
#include "controllers/clipboard.h"
#include "structure/arrangement/arranger_object_factory.h"
#include "structure/arrangement/arranger_object_fwd.h"
#include "structure/arrangement/tempo_object_manager.h"
#include "structure/tracks/track_fwd.h"
#include "undo/undo_stack.h"
#include "utils/units.h"
#include "utils/variant_helpers.h"

#include <QItemSelectionModel>
#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::actions
{
class ArrangerObjectSelectionOperator : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")
  QML_EXTENDED_NAMESPACE (zrythm::commands)

public:
  using SelectedObjectsVector =
    std::vector<structure::arrangement::ArrangerObjectUuidReference>;
  using ArrangerObjectOwnerPtrVariant =
    utils::to_pointer_variant<utils::wrap_variant_t<
      structure::arrangement::ArrangerObjectVariant,
      structure::arrangement::ArrangerObjectOwner>>;
  using ObjectOwnerProvider = std::function<ArrangerObjectOwnerPtrVariant (
    structure::arrangement::ArrangerObjectPtrVariant)>;

  /// Visitor invoked by TimelineObjectsEnumerator for each object.
  using ArrangerObjectVisitor =
    std::function<void (structure::arrangement::ArrangerObjectUuidReference)>;

  /**
   * @brief Returns the persistent identity of the project the selection
   * operator acts on (Project::project_id(), a UUID that survives reloads
   * and is regenerated on Save As).
   *
   * Copied payloads record it as their source project identity. Paste
   * decisions about unresolvable content are registry-membership based
   * (see ClipboardPayload::filtered_for_target()); project identity is
   * a routing input for track and plugin pastes.
   */
  using ProjectIdProvider = std::function<QString ()>;

  /**
   * @brief Enumerates every arranger object owned by the tracklist (all
   * clips, markers and scale objects across all tracks, lanes and automation
   * lanes), without any filtering.
   *
   * Used by cutAllObjectsAt().
   */
  using TimelineObjectsEnumerator = std::function<void (ArrangerObjectVisitor)>;

  /**
   * @brief Constructs a session-scoped selection operator.
   *
   * The selection model is passed per operation, so this object holds no
   * view state. Parent it to an object that outlives everything the
   * callbacks capture (in the application, the owning session).
   */
  explicit ArrangerObjectSelectionOperator (
    undo::UndoStack                               &undoStack,
    ObjectOwnerProvider                            objectOwnerProvider,
    structure::arrangement::ArrangerObjectFactory &objectFactory,
    structure::project::ProjectRegistry           &projectRegistry,
    controllers::Clipboard                        &clipboard,
    ProjectIdProvider                              projectIdProvider,
    TimelineObjectsEnumerator                      timelineObjectsEnumerator,
    QObject *                                      parent = nullptr);

Q_SIGNALS:
  /**
   * @brief Emitted when an operation is refused (e.g. it would place a
   * time signature off a bar boundary).
   *
   * The UI is expected to surface the reason to the user.
   */
  void operationRefused (const QString &reason);

  /**
   * @brief Emitted after a paste whose content had to be modified (e.g.
   * audio content dropped because it lives in another project's pool).
   *
   * The UI is expected to surface the summary to the user.
   */
  void pasteContentModified (const QString &summary);

public:
  Q_INVOKABLE bool
  moveByTicks (QItemSelectionModel * selectionModel, double tick_delta);

  Q_INVOKABLE bool
  moveNotesByPitch (QItemSelectionModel * selectionModel, int pitch_delta);

  Q_INVOKABLE bool
  changeVelocities (QItemSelectionModel * selectionModel, int velocity_delta);

  /**
   * @brief Ramps the velocities of MIDI notes linearly.
   *
   * Each affected note is set to the velocity of the line from
   * (@p start_ticks, @p start_value) to (@p end_ticks, @p end_value),
   * evaluated at the note's timeline position and clamped to [0, 127].
   *
   * When at least one MIDI note is selected, only the selected notes are
   * affected; selected notes outside the line's tick span get the nearest
   * endpoint's value. Otherwise, all notes of @p clip whose position is
   * inside the line's tick span are affected. All inside a single undo
   * macro.
   *
   * @return false if no notes are affected.
   */
  Q_INVOKABLE bool rampVelocities (
    QItemSelectionModel *              selectionModel,
    structure::arrangement::MidiClip * clip,
    double                             start_ticks,
    double                             start_value,
    double                             end_ticks,
    double                             end_value);

  Q_INVOKABLE bool moveAutomationPointsByDelta (
    QItemSelectionModel * selectionModel,
    double                delta);

  Q_INVOKABLE bool resizeObjects (
    QItemSelectionModel *     selectionModel,
    commands::ResizeType      type,
    commands::ResizeDirection direction,
    double                    delta);

  Q_INVOKABLE bool deleteObjects (QItemSelectionModel * selectionModel);

  /**
   * @brief Deletes the given object (eraser tool).
   *
   * Unlike deleteObjects(), this does not operate on the selection. Pushes a
   * single remove command; wrap calls in an undo macro to erase multiple
   * objects in one undo step.
   *
   * @return false if the object is null, not deletable, or not found in its
   * owner.
   */
  Q_INVOKABLE bool
  deleteObject (structure::arrangement::ArrangerObject * object);

  /**
   * @brief Cuts the selected cuttable objects at the given timeline position.
   *
   * Only objects that strictly span @p ticks are cut (see is_cuttable_at()):
   * bounded objects (clips, notes) and chord objects. For bounded objects,
   * each cut resizes the original object to end at @p ticks and adds a clone
   * starting at @p ticks that plays back identically to the original. For
   * chord objects (unbounded — each chord plays until the next one), the cut
   * adds a clone at @p ticks, which automatically ends the original's
   * effective span. All inside a single undo macro.
   */
  Q_INVOKABLE bool
  cutObjectsAt (QItemSelectionModel * selectionModel, double ticks);

  /**
   * @brief Cuts every cuttable object spanning the given timeline position.
   *
   * If @p clip is non-null, cuts the cuttable children of that clip (editor
   * context, e.g. MIDI notes in the piano roll); otherwise cuts across the
   * whole timeline using the @ref TimelineObjectsEnumerator given at
   * construction.
   */
  Q_INVOKABLE bool
  cutAllObjectsAt (double ticks, structure::arrangement::Clip * clip);

  Q_INVOKABLE bool cloneObjects (QItemSelectionModel * selectionModel);

  /**
   * @brief Copies the selected objects to the clipboard.
   *
   * Non-copyable objects (e.g. the start/end markers) are skipped with a
   * warning. The payload records the earliest selected position as the
   * paste anchor and, for lane clips, the source lane index.
   *
   * @return false if nothing copyable is selected.
   */
  Q_INVOKABLE bool copyObjects (QItemSelectionModel * selectionModel);

  /**
   * @brief Copies the selected objects to the clipboard and deletes them,
   * in a single undo step.
   *
   * @return false if nothing selected or some object is not deletable (in
   * which case nothing is copied or deleted).
   */
  Q_INVOKABLE bool cutObjects (QItemSelectionModel * selectionModel);

  /**
   * @brief Duplicates the selected objects: each is cloned and shifted by
   * the selection's tick span, in a single undo step, without touching the
   * clipboard.
   *
   * @return The new objects' UUIDs, or an empty list on failure.
   */
  Q_INVOKABLE QVariantList
  duplicateObjects (QItemSelectionModel * selectionModel);

  /**
   * @brief Pastes the clipboard's arranger objects onto the timeline so
   * that the earliest pasted object lands at @p playheadTicks.
   *
   * Owners for the object types that live on singleton tracks are passed
   * by the caller. Incompatible objects are skipped with a warning (lane
   * clips pasted only onto tracks with matching lanes, automation clips
   * and editor objects are not pastable on the timeline); the imports of
   * skipped roots are rolled back. If nothing can be pasted, all imported
   * payload content is discarded and an empty list returned; otherwise
   * each pasted root is attached with one undoable command inside a
   * single undo macro.
   *
   * @return The pasted objects' UUIDs (for selection), or an empty list on
   * failure.
   */
  Q_INVOKABLE QVariantList pasteObjectsOnTimeline (
    structure::tracks::Track *                   targetTrack,
    structure::tracks::MarkerTrack *             markerTrack,
    structure::tracks::ChordTrack *              chordTrack,
    structure::arrangement::TempoObjectManager * tempoObjectManager,
    double                                       playheadTicks);

  /**
   * @brief Pastes the clipboard's arranger objects into @p clip (editor
   * context), so that the earliest pasted object lands at @p
   * positionTicks in the clip's content space.
   *
   * Objects that the clip type does not own are skipped with a warning.
   *
   * @return The pasted objects' UUIDs (for selection), or an empty list on
   * failure.
   */
  Q_INVOKABLE QVariantList pasteObjectsIntoClip (
    structure::arrangement::Clip * clip,
    double                         positionTicks);

  Q_INVOKABLE bool toggleMute (QItemSelectionModel * selectionModel);

  /// Sets the timestretch algorithm on all selected AudioClips.
  Q_INVOKABLE bool setStretchAlgorithm (
    QItemSelectionModel *          selectionModel,
    dsp::StretchOptions::Algorithm algorithm);

  /// Sets a per-clip timebase override on all selected clips.
  Q_INVOKABLE bool setTimebaseOverride (
    QItemSelectionModel * selectionModel,
    dsp::Timebase         timebase);

  /// Clears the per-clip timebase override on all selected clips
  /// (inherit from parent provider).
  Q_INVOKABLE bool clearTimebaseOverride (QItemSelectionModel * selectionModel);

  /// Returns true if any selected object has a timebase provider (i.e., is a
  /// clip).
  Q_INVOKABLE bool
  selectionHasTimebaseProviders (QItemSelectionModel * selectionModel) const;

private:
  static auto
  extractSelectedObjects (const QItemSelectionModel * selectionModel)
    -> SelectedObjectsVector;

  static bool validateHorizontalMovement (
    const SelectedObjectsVector &objects,
    double                       tick_delta);

  /**
   * @brief Returns whether @p obj_var, when shifted by @p shift, still
   * satisfies its position constraints.
   *
   * Time-signature objects are only allowed at bar boundaries; every other
   * object type always satisfies the constraints.
   */
  [[nodiscard]] static bool time_signature_lands_on_bar (
    structure::arrangement::ArrangerObjectPtrVariant obj_var,
    dsp::TimelineTick                                shift);
  static bool
  validateVerticalMovement (const SelectedObjectsVector &objects, double delta);
  static bool validateResize (
    const SelectedObjectsVector &objects,
    commands::ResizeType         type,
    commands::ResizeDirection    direction,
    double                       delta);
  static bool validateBoundsResize (
    structure::arrangement::ArrangerObjectPtrVariant obj_var,
    commands::ResizeDirection                        direction,
    double                                           delta);
  static bool validateFadesResize (
    structure::arrangement::ArrangerObjectPtrVariant obj_var,
    commands::ResizeDirection                        direction,
    double                                           delta);

  bool
  process_vertical_move (QItemSelectionModel * selectionModel, double delta);

  /**
   * @brief Cuts the given objects at @p ticks inside a single undo macro.
   *
   * @pre @p objects only contains objects that strictly span @p ticks (see
   * is_cuttable_at()).
   */
  bool cut_objects (const SelectedObjectsVector &objects, double ticks);

  /**
   * @brief Returns whether the object can be cut at the given timeline
   * position.
   *
   * Bounded objects (clips, notes) are cuttable strictly inside their span.
   * Chord objects, though unbounded, effectively span until the next chord
   * or the clip's end and are cuttable strictly inside that span.
   */
  static bool is_cuttable_at (
    const structure::arrangement::ArrangerObjectUuidReference &obj_ref,
    double                                                     ticks);

  static bool all_objects_deletable (const SelectedObjectsVector &objects);

  /**
   * @brief Returns whether is_arranger_object_copyable() holds for every
   * object.
   */
  static bool all_objects_copyable (const SelectedObjectsVector &objects);

  /**
   * @brief A payload prepared for pasting: identity-regenerated, filtered
   * for this project and imported into the registry.
   */
  struct PreparedPaste
  {
    structure::project::ClipboardPayload payload;
    /** Source lane index per pasted lane-clip root. */
    std::unordered_map<structure::arrangement::ArrangerObjectUuid, std::size_t>
      lane_indices;
    /** Shift applied to pasted positions: paste position - anchor. */
    units::precise_tick_t delta;
    /** IDs of the objects newly registered by import_into(). */
    std::vector<QUuid> imported_ids;
    /** Pool-bound audio objects the target project cannot resolve (dropped
     * from a cross-project payload). */
    std::size_t dropped_audio_objects = 0;
    /** External references that did not resolve in the target project and
     * were severed. */
    std::size_t severed_references = 0;
  };

  /**
   * @brief Resolves the owner for one pasted root, or std::nullopt if the
   * root cannot be pasted into the target context (with a warning logged).
   *
   * @param paste The prepared paste (lane indices for lane clips).
   * @param root_uuid UUID of the root being resolved.
   * @param obj_var Pointer variant of the imported root object.
   */
  using PasteOwnerResolver = std::function<std::optional<
    ArrangerObjectOwnerPtrVariant> (
    const PreparedPaste                              &paste,
    const structure::arrangement::ArrangerObjectUuid &root_uuid,
    structure::arrangement::ArrangerObjectPtrVariant  obj_var)>;

  /**
   * @brief Memoizes object_owner_provider_ results for one operation.
   *
   * The provider scans all tracks and lanes per call; an operation that
   * resolves the same object's owner more than once resolves it once
   * instead.
   */
  class OwnerResolver
  {
  public:
    explicit OwnerResolver (const ObjectOwnerProvider &provider)
        : provider_ (provider)
    {
    }

    ArrangerObjectOwnerPtrVariant
    resolve (const structure::arrangement::ArrangerObjectPtrVariant &obj_var);

  private:
    const ObjectOwnerProvider &provider_;
    std::unordered_map<const QObject *, ArrangerObjectOwnerPtrVariant> cache_;
  };

  /**
   * @brief Owner-resolving overloads used inside one operation: @p resolver
   * is shared across the operation's phases.
   */
  [[nodiscard]] bool
  delete_objects (const SelectedObjectsVector &objects, OwnerResolver &resolver);

  [[nodiscard]] bool can_delete_objects (
    const SelectedObjectsVector &objects,
    OwnerResolver               &resolver) const;

  /**
   * @brief Logs @p reason and emits operationRefused().
   */
  void refuse_operation (const QString &reason);

  /**
   * @brief Returns the shift duplicates of @p objects are placed at: the
   * selection's span, or one bar for a zero-span selection, or
   * std::nullopt if no positive shift can be determined.
   */
  [[nodiscard]] static std::optional<units::precise_tick_t>
  duplicate_shift (const SelectedObjectsVector &objects);

  /**
   * @brief Returns the owner resolver pasting on the timeline: lane clips
   * into @p targetTrack's matching lane, markers into @p markerTrack,
   * scales/chord clips into @p chordTrack, tempo objects into @p
   * tempoObjectManager.
   */
  [[nodiscard]] PasteOwnerResolver timeline_paste_owner_resolver (
    structure::tracks::Track *                   targetTrack,
    structure::tracks::MarkerTrack *             markerTrack,
    structure::tracks::ChordTrack *              chordTrack,
    structure::arrangement::TempoObjectManager * tempoObjectManager) const;

  /**
   * @brief Returns the owner resolver pasting editor objects into @p clip
   * (notes/CC into MIDI clips, automation points into automation clips,
   * chords into chord clips, audio sources into audio clips).
   */
  [[nodiscard]] static PasteOwnerResolver
  clip_paste_owner_resolver (structure::arrangement::Clip * clip);

  /**
   * @brief Filters @p objects down to the copyable ones, logging a warning
   * per skipped object (e.g. the start/end markers).
   */
  [[nodiscard]] static SelectedObjectsVector
  copyable_objects (const SelectedObjectsVector &objects);

  /**
   * @brief Copies @p objects (already filtered by copyable_objects()) to
   * the clipboard.
   *
   * @return Whether a payload was stored on the clipboard.
   */
  [[nodiscard]] bool
  copy_objects (const SelectedObjectsVector &objects, OwnerResolver &resolver);

  /**
   * @brief Returns the clipboard's arranger-objects payload prepared for
   * pasting at @p position, or std::nullopt if the clipboard holds nothing
   * pasteable or the preparation failed.
   */
  [[nodiscard]] std::optional<PreparedPaste>
  prepare_paste (units::precise_tick_t position);

  /**
   * @brief Attaches the prepared paste's roots to the owners given by @p
   * resolve_owner, pushing one add command per attached root inside a
   * single undo macro.
   *
   * Imports of roots that cannot be attached are rolled back (all of them
   * if none attach).
   *
   * @return The new objects' UUIDs, or an empty list on failure.
   */
  [[nodiscard]] QVariantList attach_paste_targets (
    const PreparedPaste      &paste,
    const PasteOwnerResolver &resolve_owner);

  /**
   * @brief Clones @p obj_var with a fresh identity, applies @p mutate and
   * pushes an add command for the clone on its owner.
   *
   * If the object has no owner, the warning is logged, no command is
   * pushed and the clone is destroyed automatically once its last
   * reference goes away.
   *
   * @return The clone's UUID, or std::nullopt if it could not be attached.
   */
  std::optional<QUuid> clone_and_attach (
    structure::arrangement::ArrangerObjectPtrVariant obj_var,
    const std::function<void (structure::arrangement::ArrangerObject &)>
      &mutate = {});

  /**
   * @brief Deletes the imported objects of @p paste from the registry
   * (paste rollback), keeping the objects listed in @p ids_to_keep and
   * everything they still need.
   * @param ids_to_keep IDs that must stay registered (e.g. the roots that
   *   were pasted successfully).
   */
  void discard_imported_objects (
    const PreparedPaste      &paste,
    const std::vector<QUuid> &ids_to_keep = {});

  /**
   * @brief Returns the positions of @p objects in their own position
   * space, which is uniform per arranger pane (timeline or content).
   */
  static std::vector<units::precise_tick_t>
  object_positions (const SelectedObjectsVector &objects);

  /**
   * @brief Returns whether @p objects all live in the same position space
   * (all timeline or all clip-content objects).
   *
   * Anchor and span computations are meaningless across mixed spaces, so
   * callers refuse such selections.
   */
  [[nodiscard]] static bool
  selection_in_single_position_space (const SelectedObjectsVector &objects);

  /**
   * @brief Returns the end position of @p obj_var in its own position
   * space (loop-aware for clips, position + length for other bounded
   * objects, position for unbounded objects).
   */
  static units::precise_tick_t
  object_end_ticks (structure::arrangement::ArrangerObjectPtrVariant obj_var);

private:
  undo::UndoStack                               &undo_stack_;
  ObjectOwnerProvider                            object_owner_provider_;
  structure::arrangement::ArrangerObjectFactory &object_factory_;
  structure::project::ProjectRegistry           &project_registry_;
  controllers::Clipboard                        &clipboard_;
  ProjectIdProvider                              project_id_provider_;
  TimelineObjectsEnumerator                      timeline_objects_enumerator_;
};

} // namespace zrythm::actions
