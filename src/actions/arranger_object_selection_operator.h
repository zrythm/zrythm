// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "commands/change_qobject_property_command.h"
#include "commands/resize_arranger_objects_command.h"
#include "structure/arrangement/arranger_object_factory.h"
#include "undo/undo_stack.h"
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
   * @brief Enumerates every arranger object owned by the tracklist (all
   * clips, markers and scale objects across all tracks, lanes and automation
   * lanes), without any filtering.
   *
   * Used by cutAllObjectsAt().
   */
  using TimelineObjectsEnumerator = std::function<void (ArrangerObjectVisitor)>;

  explicit ArrangerObjectSelectionOperator (
    undo::UndoStack                               &undoStack,
    QItemSelectionModel                           &selectionModel,
    ObjectOwnerProvider                            objectOwnerProvider,
    structure::arrangement::ArrangerObjectFactory &objectFactory,
    TimelineObjectsEnumerator timelineObjectsEnumerator = {},
    QObject *                 parent = nullptr);

  Q_INVOKABLE bool moveByTicks (double tick_delta);

  Q_INVOKABLE bool moveNotesByPitch (int pitch_delta);

  Q_INVOKABLE bool changeVelocities (int velocity_delta);

  Q_INVOKABLE bool moveAutomationPointsByDelta (double delta);

  Q_INVOKABLE bool resizeObjects (
    commands::ResizeType      type,
    commands::ResizeDirection direction,
    double                    delta);

  Q_INVOKABLE bool deleteObjects ();

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
  Q_INVOKABLE bool cutObjectsAt (double ticks);

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

  Q_INVOKABLE bool cloneObjects ();

  Q_INVOKABLE bool toggleMute ();

  /// Sets the timestretch algorithm on all selected AudioClips.
  Q_INVOKABLE bool
  setStretchAlgorithm (dsp::StretchOptions::Algorithm algorithm);

  /// Sets a per-clip timebase override on all selected clips.
  Q_INVOKABLE bool setTimebaseOverride (dsp::Timebase timebase);

  /// Clears the per-clip timebase override on all selected clips
  /// (inherit from parent provider).
  Q_INVOKABLE bool clearTimebaseOverride ();

  /// Returns true if any selected object has a timebase provider (i.e., is a
  /// clip).
  Q_INVOKABLE bool selectionHasTimebaseProviders () const;

private:
  auto extractSelectedObjects () const -> SelectedObjectsVector;

  static bool validateHorizontalMovement (
    const SelectedObjectsVector &objects,
    double                       tick_delta);
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

  bool process_vertical_move (double delta);

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

private:
  undo::UndoStack                               &undo_stack_;
  QItemSelectionModel                           &selection_model_;
  ObjectOwnerProvider                            object_owner_provider_;
  structure::arrangement::ArrangerObjectFactory &object_factory_;
  TimelineObjectsEnumerator                      timeline_objects_enumerator_;
};

} // namespace zrythm::actions
