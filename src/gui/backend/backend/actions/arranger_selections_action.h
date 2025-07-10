// SPDX-FileCopyrightText: © 2019-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "gui/backend/backend/actions/undoable_action.h"
#include "gui/dsp/quantize_options.h"
#include "structure/arrangement/arranger_object_span.h"
#include "structure/arrangement/audio_function.h"
#include "structure/arrangement/automation_function.h"
#include "structure/arrangement/midi_function.h"

namespace zrythm::gui::actions
{

/**
 * @brief An action that performs changes to the arranger selections.
 *
 * This class represents an undoable action that can be performed on the
 * arranger selections, such as creating, deleting, moving, or resizing selected
 * objects.
 *
 * The action has a Type enum that specifies the type of action being performed,
 * such as Create, Delete, Move, or Resize. For Resize actions, there is a
 * ResizeType enum that specifies the type of resize operation. For Edit
 * actions, there is an EditType enum that specifies the type of edit operation,
 * such as changing the name or position of the selected objects.
 */
class ArrangerSelectionsAction : public QObject, public UndoableAction
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_UNDOABLE_ACTION_QML_PROPERTIES (ArrangerSelectionsAction)

  using ArrangerObjectRegistry = structure::arrangement::ArrangerObjectRegistry;
  using ArrangerObjectPtrVariant =
    structure::arrangement::ArrangerObjectPtrVariant;
  using ArrangerObjectSpan = structure::arrangement::ArrangerObjectSpan;
  using ArrangerObject = structure::arrangement::ArrangerObject;
  using MidiFunction = structure::arrangement::MidiFunction;
  using AutomationFunction = structure::arrangement::AutomationFunction;

public:
  enum class Type
  {
    AutomationFill,
    Create,
    Delete,
    Duplicate,
    Edit,
    Link,
    Merge,
    Move,
    Quantize,
    Record,
    Resize,
    Split,
  };

  /**
   * Type used when the action is a RESIZE action.
   */
  enum class ResizeType
  {
    L, //< Resize the left side of the `ArrangerObject`s in the selection.
    R, //< Resize the right side of the `ArrangerObject`s in the selection.
    LLoop,
    RLoop,
    LFade,
    RFade,
    LStretch,
    RStretch,
  };

  /**
   * Type used when the action is an EDIT action.
   */
  enum class EditType
  {
    /** Edit the name of objects in the selection. */
    Name,

    /**
     * Edit a Position of objects in the selection.
     *
     * This will just set all of the positions on the object.
     */
    Position,

    /**
     * Edit a primitive (int, etc) member of objects in the selection.
     *
     * This will simply set all relevant primitive values in an ArrangerObject
     * when doing/undoing.
     */
    Primitive,

    /** For editing the MusicalScale inside `ScaleObject`s. */
    Scale,

    /** Editing fade positions or curve options. */
    Fades,

    /** Change mute status. */
    Mute,

    /** For ramping MidiNote velocities or AutomationPoint values.
     * (this is handled by EDIT_PRIMITIVE) */
    // ARRANGER_SELECTIONS_ACTION_EDIT_RAMP,

    /** MIDI function. */
    EditorFunction,
  };

  using Position = zrythm::dsp::Position;

public:
  ArrangerSelectionsAction ();
  ~ArrangerSelectionsAction () override = default;

  class RecordAction;
  class MoveOrDuplicateAction;
  class MoveOrDuplicateTimelineAction;
  class MoveOrDuplicateMidiAction;
  class MoveOrDuplicateAutomationAction;
  class MoveOrDuplicateChordAction;
  // class MoveByTicksAction;
  class MoveMidiAction;
  class MoveChordAction;
  class LinkAction;
  class AutomationFillAction;
  class SplitAction;
  class MergeAction;
  class ResizeAction;
  class QuantizeAction;

  ArrangerObjectRegistry &get_arranger_object_registry () const;

  friend void init_from (
    ArrangerSelectionsAction       &obj,
    const ArrangerSelectionsAction &other,
    utils::ObjectCloneType          clone_type);

  bool needs_transport_total_bar_update (bool perform) const override;

  bool needs_pause () const override
  {
    /* always needs a pause to update the track playback snapshots */
    return true;
  }

  [[nodiscard]] QString to_string () const final;

protected:
  /**
   * @brief Generic constructor to be used by others
   *
   * Internally calls @ref set_before_selections().
   *
   * @param before_sel Selections before the change.
   */
  ArrangerSelectionsAction (ArrangerObjectSpan before_sel, Type type)
      : UndoableAction (UndoableAction::Type::ArrangerSelections), type_ (type),
        first_run_ (true)
  {
    set_before_selections (before_sel);
  }

  /**
   * @brief Sets @ref sel_ to a clone of @p src.
   *
   * @param src
   */
  void set_before_selections (ArrangerObjectSpan src_var);

  /**
   * @brief Sets @ref sel_after_ to a clone of @p src.
   *
   * @param src
   */
  void set_after_selections (ArrangerObjectSpan src_var);

private:
  /** Common logic for perform/undo. */
  void do_or_undo (bool do_it);

  void do_or_undo_duplicate_or_link (bool link, bool do_it);
  void do_or_undo_move (bool do_it);
  void do_or_undo_create_or_delete (bool do_it, bool create);
  void do_or_undo_record (bool do_it);
  void do_or_undo_edit (bool do_it);
  void do_or_undo_automation_fill (bool do_it);
  void do_or_undo_split (bool do_it);
  void do_or_undo_merge (bool do_it);
  void do_or_undo_resize (bool do_it);
  void do_or_undo_quantize (bool do_it);

  /**
   * Finds all corresponding objects in the project and calls
   * Region.update_link_group().
   */
  void update_region_link_groups (const auto &objects);

  /**
   * @brief Moves a project object by tracks and/or labes.
   *
   * @param obj An object that already exists in the project.
   * @param tracks_diff
   * @param lanes_diff
   * @param use_index_in_prev_lane
   * @param index_in_prev_lane
   */
  void move_obj_by_tracks_and_lanes (
    ArrangerObjectPtrVariant obj_var,
    int                      tracks_diff,
    int                      lanes_diff,
    bool                     use_index_in_prev_lane,
    int                      index_in_prev_lane);

  std::vector<ArrangerObjectPtrVariant> get_project_arranger_objects () const;

  void init_loaded_impl () final;
  void perform_impl () final;
  void undo_impl () final;

public:
  /** Action type. */
  Type type_{};

  /** A clone of the ArrangerSelections before the change. */
  std::optional<std::vector<ArrangerObjectPtrVariant>> sel_;

  /**
   * A clone of the ArrangerSelections after the change (used in the EDIT
   * action and quantize).
   */
  std::optional<std::vector<ArrangerObjectPtrVariant>> sel_after_;

  /** Type of edit action, if an Edit action. */
  EditType edit_type_{};

  /** Ticks diff. */
  double ticks_{};
  /** Tracks moved. */
  int delta_tracks_{};
  /** Lanes moved. */
  int delta_lanes_{};
  /** Chords moved (up/down in the Chord editor). */
  int delta_chords_ = 0;
  /** Delta of MidiNote pitch. */
  int delta_pitch_ = 0;
  /** Delta of MidiNote velocity. */
  int delta_vel_ = 0;
  /**
   * Difference in a normalized amount, such as
   * automation point normalized value.
   */
  double delta_normalized_amount_ = 0.0;

  std::optional<std::pair<Position, Position>>
    selected_positions_in_audio_editor_;

  /** Clip editor region ID (for non-timeline actions). */
  std::optional<ArrangerObject::Uuid> clip_editor_region_id_;

  /** Target port (used to find corresponding automation track when
   * moving/copying automation regions to another automation track/another
   * track). */
  std::optional<dsp::PortUuid> target_port_;

  /** String, when changing a string. */
  utils::Utf8String str_;

  /** Position, when changing a Position. */
  Position pos_;

  /** Used when splitting - these are the split BoundedObject's. */
  std::vector<ArrangerObject::Uuid> r1_;
  std::vector<ArrangerObject::Uuid> r2_;

  /** Number of split objects. */
  // size_t num_split_objs_ = 0;

  /**
   * If this is true, the first "do" call does nothing in some cases.
   *
   * Can be ignored depending on the action.
   */
  bool first_run_ = false;

  /** QuantizeOptions clone, if quantizing. */
  std::unique_ptr<gui::old_dsp::QuantizeOptions> opts_{};

  /** Used for automation autofill action.
   * These are assumed to be inherited from Region.
   */
  std::optional<ArrangerObjectPtrVariant> region_before_;
  std::optional<ArrangerObjectPtrVariant> region_after_;

  /** Used by the resize action. */
  ResizeType resize_type_ = (ResizeType) 0;
};

class CreateOrDeleteArrangerSelectionsAction : public ArrangerSelectionsAction
{
public:
  ~CreateOrDeleteArrangerSelectionsAction () override = default;

protected:
  /**
   * Creates a new action for creating/deleting objects.
   *
   * @param create If this is true, the action will create objects. If false,
   * it will delete them.
   */
  CreateOrDeleteArrangerSelectionsAction (
    ArrangerObjectSpan sel_var,
    bool               create);
};

class CreateArrangerSelectionsAction
  final : public CreateOrDeleteArrangerSelectionsAction
{
public:
  CreateArrangerSelectionsAction (ArrangerObjectSpan sel)
      : CreateOrDeleteArrangerSelectionsAction (sel, true) { };
};

class DeleteArrangerSelectionsAction
  final : public CreateOrDeleteArrangerSelectionsAction
{
public:
  DeleteArrangerSelectionsAction (ArrangerObjectSpan sel)
      : CreateOrDeleteArrangerSelectionsAction (sel, false) { };
};

class ArrangerSelectionsAction::RecordAction : public ArrangerSelectionsAction
{
public:
  /**
   * @brief Construct a new action for recording.
   *
   * @param sel_before
   * @param sel_after
   * @param already_recorded
   */
  RecordAction (
    ArrangerObjectSpan sel_before,
    ArrangerObjectSpan sel_after,
    bool               already_recorded);
};

class ArrangerSelectionsAction::MoveOrDuplicateAction
    : public ArrangerSelectionsAction
{
public:
  /**
   * Creates a new action for moving or duplicating objects.
   *
   * @param already_moved If this is true, the first DO will do nothing.
   * @param delta_normalized_amount Difference in a normalized amount, such as
   * automation point normalized value.
   */
  MoveOrDuplicateAction (
    ArrangerObjectSpan           sel_var,
    bool                         move,
    double                       ticks,
    int                          delta_chords,
    int                          delta_pitch,
    int                          delta_tracks,
    int                          delta_lanes,
    double                       delta_normalized_amount,
    std::optional<dsp::PortUuid> tgt_port_id,
    bool                         already_moved);
};

class ArrangerSelectionsAction::MoveOrDuplicateTimelineAction
    : public ArrangerSelectionsAction::MoveOrDuplicateAction
{
public:
  MoveOrDuplicateTimelineAction (
    ArrangerObjectSpan           sel,
    bool                         move,
    double                       ticks,
    int                          delta_tracks,
    int                          delta_lanes,
    std::optional<dsp::PortUuid> tgt_port_id,
    bool                         already_moved)
      : MoveOrDuplicateAction (
          sel,
          move,
          ticks,
          0,
          0,
          delta_tracks,
          delta_lanes,
          0,
          tgt_port_id,
          already_moved) { };
};

class ArrangerSelectionsAction::MoveOrDuplicateMidiAction
    : public ArrangerSelectionsAction::MoveOrDuplicateAction
{
public:
  MoveOrDuplicateMidiAction (
    ArrangerObjectSpan sel,
    bool               move,
    double             ticks,
    int                delta_pitch,
    bool               already_moved)
      : MoveOrDuplicateAction (
          sel,
          move,
          ticks,
          0,
          delta_pitch,
          0,
          0,
          0,
          std::nullopt,
          already_moved) { };
};

class ArrangerSelectionsAction::MoveOrDuplicateChordAction
    : public ArrangerSelectionsAction::MoveOrDuplicateAction
{
public:
  MoveOrDuplicateChordAction (
    ArrangerObjectSpan sel,
    bool               move,
    double             ticks,
    int                delta_chords,
    bool               already_moved)
      : MoveOrDuplicateAction (
          sel,
          move,
          ticks,
          delta_chords,
          0,
          0,
          0,
          0,
          std::nullopt,
          already_moved) { };
};

class ArrangerSelectionsAction::MoveOrDuplicateAutomationAction
    : public ArrangerSelectionsAction::MoveOrDuplicateAction
{
public:
  MoveOrDuplicateAutomationAction (
    ArrangerObjectSpan sel,
    bool               move,
    double             ticks,
    int                delta_normalized_amount,
    bool               already_moved)
      : MoveOrDuplicateAction (
          sel,
          move,
          ticks,
          0,
          0,
          0,
          0,
          delta_normalized_amount,
          std::nullopt,
          already_moved) { };
};

#if 0
class ArrangerSelectionsAction::MoveByTicksAction
    : public ArrangerSelectionsAction::MoveOrDuplicateAction
{
public:
  template <FinalArrangerSelectionsSubclass T>
  MoveByTicksAction (const T &sel, double ticks, bool already_moved)
      : MoveOrDuplicateAction (sel, true, ticks, 0, 0, 0, 0, 0, nullptr, already_moved)
  {
  }
};
#endif

class ArrangerSelectionsAction::MoveMidiAction
    : public ArrangerSelectionsAction::MoveOrDuplicateAction
{
public:
  MoveMidiAction (
    ArrangerObjectSpan sel,
    double             ticks,
    int                delta_pitch,
    bool               already_moved)
      : MoveOrDuplicateAction (sel, true, ticks, 0, delta_pitch, 0, 0, 0, std::nullopt, already_moved)
  {
  }
};

class ArrangerSelectionsAction::MoveChordAction
    : public ArrangerSelectionsAction::MoveOrDuplicateAction
{
public:
  MoveChordAction (
    ArrangerObjectSpan sel,
    double             ticks,
    int                delta_chords,
    bool               already_moved)
      : MoveOrDuplicateAction (sel, true, ticks, delta_chords, 0, 0, 0, 0, std::nullopt, already_moved)
  {
  }
};

class ArrangerSelectionsAction::LinkAction : public ArrangerSelectionsAction
{
public:
  /**
   * Creates a new action for linking regions.
   *
   * @param already_moved If this is true, the first DO will do nothing.
   * @param sel_before Original selections.
   * @param sel_after Selections after duplication.
   */
  LinkAction (
    ArrangerObjectSpan sel_before,
    ArrangerObjectSpan sel_after,
    double             ticks,
    int                delta_tracks,
    int                delta_lanes,
    bool               already_moved);
};

class EditArrangerSelectionsAction : public ArrangerSelectionsAction
{
public:
  /**
   * Creates a new action for editing properties of an object.
   *
   * @param sel_before The selections before the change.
   * @param sel_after The selections after the change.
   * @param type Indication of which field has changed.
   */
  EditArrangerSelectionsAction (
    ArrangerObjectSpan                sel_before,
    std::optional<ArrangerObjectSpan> sel_after,
    EditType                          type,
    bool                              already_edited);

  /**
   * @brief Wrapper for a single object.
   */
  static std::unique_ptr<EditArrangerSelectionsAction> create (
    ArrangerObjectSpan sel_before,
    ArrangerObjectSpan sel_after,
    EditType           type,
    bool               already_edited);

  /**
   * @brief Wrapper for MIDI functions.
   */
  EditArrangerSelectionsAction (
    ArrangerObjectSpan    sel_before,
    MidiFunction::Type    midi_func_type,
    MidiFunction::Options opts);

  /**
   * @brief Wrapper for automation functions.
   */
  EditArrangerSelectionsAction (
    ArrangerObjectSpan       sel_before,
    AutomationFunction::Type automation_func_type);

  /**
   * @brief Wrapper for audio functions.
   */
  EditArrangerSelectionsAction (
    ArrangerObject::Uuid                      region_id,
    const dsp::Position                      &sel_start,
    const dsp::Position                      &sel_end,
    structure::arrangement::AudioFunctionType audio_func_type,
    structure::arrangement::AudioFunctionOpts opts,
    std::optional<utils::Utf8String>          uri);
};

class ArrangerSelectionsAction::AutomationFillAction
    : public ArrangerSelectionsAction
{
public:
  /**
   * Creates a new action for automation autofill.
   *
   * @param region_before The region before the change.
   * @param region_after The region after the change.
   * @param already_changed Whether the change was already made.
   */
  template <structure::arrangement::RegionObject RegionT>
  AutomationFillAction (
    const RegionT &region_before,
    const RegionT &region_after,
    bool           already_changed);
};

class ArrangerSelectionsAction::SplitAction : public ArrangerSelectionsAction
{
public:
  /**
   * Creates a new action for splitting regions.
   *
   * @param pos Global position to split at.
   */
  SplitAction (ArrangerObjectSpan sel, Position pos);
};

class ArrangerSelectionsAction::MergeAction : public ArrangerSelectionsAction
{
public:
  /**
   * Creates a new action for merging objects.
   */
  MergeAction (ArrangerObjectSpan sel)
      : ArrangerSelectionsAction (sel, Type::Merge)
  {
  }
};

class ArrangerSelectionsAction::ResizeAction : public ArrangerSelectionsAction
{
public:
  /**
   * Creates a new action for resizing ArrangerObject's.
   *
   * @param sel_after Optional selections after resizing (if already resized).
   * @param ticks How many ticks to add to the resizing edge.
   */
  ResizeAction (
    ArrangerObjectSpan                sel_before,
    std::optional<ArrangerObjectSpan> sel_after,
    ResizeType                        type,
    double                            ticks);
};

class ArrangerSelectionsAction::QuantizeAction : public ArrangerSelectionsAction
{
public:
  /**
   * Creates a new action for quantizing ArrangerObject's.
   *
   * @param opts Quantize options.
   */
  QuantizeAction (ArrangerObjectSpan sel, const old_dsp::QuantizeOptions &opts)
      : ArrangerSelectionsAction (sel, Type::Quantize)
  {
    opts_ = std::make_unique<old_dsp::QuantizeOptions> (opts);
  }
};

}; // namespace zrythm::gui::actions

DEFINE_ENUM_FORMATTER (
  zrythm::gui::actions::ArrangerSelectionsAction::ResizeType,
  ArrangerSelectionsAction_ResizeType,
  "Resize L",
  "Resize R",
  "Resize L (loop)",
  "Resize R (loop)",
  "Resize L (fade)",
  "Resize R (fade)",
  "Stretch L",
  "Stretch R");
