// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __UNDO_ARRANGER_SELECTIONS_ACTION_H__
#define __UNDO_ARRANGER_SELECTIONS_ACTION_H__

#include <memory>
#include <vector>

#include "actions/undoable_action.h"
#include "dsp/audio_function.h"
#include "dsp/automation_function.h"
#include "dsp/lengthable_object.h"
#include "dsp/midi_function.h"
#include "dsp/midi_region.h"
#include "dsp/port_identifier.h"
#include "dsp/position.h"
#include "dsp/quantize_options.h"
#include "dsp/region.h"
#include "gui/cpp/backend/audio_selections.h"
#include "gui/cpp/backend/automation_selections.h"
#include "gui/cpp/backend/chord_selections.h"
#include "gui/cpp/backend/midi_selections.h"
#include "gui/cpp/backend/timeline_selections.h"

/**
 * @addtogroup actions
 *
 * @{
 */

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
class ArrangerSelectionsAction
    : public UndoableAction,
      public ICloneable<ArrangerSelectionsAction>,
      public ISerializable<ArrangerSelectionsAction>
{
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

  ArrangerSelectionsAction ();
  virtual ~ArrangerSelectionsAction () = default;

  class RecordAction;
  class MoveOrDuplicateAction;
  class MoveOrDuplicateTimelineAction;
  class MoveOrDuplicateMidiAction;
  class MoveOrDuplicateAutomationAction;
  class MoveOrDuplicateChordAction;
  class MoveByTicksAction;
  class MoveMidiAction;
  class MoveChordAction;
  class LinkAction;
  class AutomationFillAction;
  class SplitAction;
  class MergeAction;
  class ResizeAction;
  class QuantizeAction;

  bool contains_clip (const AudioClip &clip) const override;

  void init_after_cloning (const ArrangerSelectionsAction &other) final;

  bool needs_transport_total_bar_update (bool perform) const override
  {
    if (
      (perform && type_ == Type::Create) || (!perform && type_ == Type::Delete)
      || (perform && type_ == Type::Duplicate)
      || (perform && type_ == Type::Link))
      return false;

    return true;
  }

  bool needs_pause () const override
  {
    /* always needs a pause to update the track playback snapshots */
    return true;
  }

  bool can_contain_clip () const override { return true; }

  DECLARE_DEFINE_FIELDS_METHOD ();

protected:
  /**
   * @brief Generic constructor to be used by others
   *
   * Internally calls @ref set_before_selections().
   *
   * @param before_sel Selections before the change.
   */
  ArrangerSelectionsAction (const ArrangerSelections &before_sel, Type type);

  /**
   * @brief Sets @ref sel_ to a clone of @p src.
   *
   * @param src
   */
  void set_before_selections (const ArrangerSelections &src);

  /**
   * @brief Sets @ref sel_after_ to a clone of @p src.
   *
   * @param src
   */
  void set_after_selections (const ArrangerSelections &src);

  char * arranger_selections_action_stringize (ArrangerSelectionsAction * self);

  std::string to_string () const final;

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
    ArrangerObject &obj,
    int             tracks_diff,
    int             lanes_diff,
    bool            use_index_in_prev_lane,
    int             index_in_prev_lane);

  ArrangerSelections * get_actual_arranger_selections () const;

  void init_loaded_impl () final;
  void perform_impl () final;
  void undo_impl () final;

public:
  /** Action type. */
  Type type_ = (Type) 0;

  /** A clone of the ArrangerSelections before the change. */
  std::unique_ptr<ArrangerSelections> sel_;

  /**
   * A clone of the ArrangerSelections after the change (used in the EDIT
   * action and quantize).
   */
  std::unique_ptr<ArrangerSelections> sel_after_;

  /** Type of edit action, if an Edit action. */
  EditType edit_type_ = (EditType) 0;

  /** Ticks diff. */
  double ticks_ = 0.0;
  /** Tracks moved. */
  int delta_tracks_ = 0;
  /** Lanes moved. */
  int delta_lanes_ = 0;
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

  /** Target port (used to find corresponding automation track when
   * moving/copying automation regions to another automation track/another
   * track). */
  std::unique_ptr<PortIdentifier> target_port_;

  /** String, when changing a string. */
  std::string str_;

  /** Position, when changing a Position. */
  Position pos_;

  /** Used when splitting - these are the split ArrangerObject's. */
  std::vector<std::unique_ptr<LengthableObject>> r1_;
  std::vector<std::unique_ptr<LengthableObject>> r2_;

  /** Number of split objects. */
  size_t num_split_objs_ = 0;

  /**
   * If this is true, the first "do" call does nothing in some cases.
   *
   * Can be ignored depending on the action.
   */
  bool first_run_ = false;

  /** QuantizeOptions clone, if quantizing. */
  std::unique_ptr<QuantizeOptions> opts_;

  /** Used for automation autofill action. */
  std::unique_ptr<Region> region_before_;
  std::unique_ptr<Region> region_after_;

  /** Used by the resize action. */
  ResizeType resize_type_ = (ResizeType) 0;
};

class CreateOrDeleteArrangerSelectionsAction : public ArrangerSelectionsAction
{
public:
  virtual ~CreateOrDeleteArrangerSelectionsAction () = default;

protected:
  /**
   * Creates a new action for creating/deleting objects.
   *
   * @param create If this is true, the action will create objects. If false,
   * it will delete them.
   */
  CreateOrDeleteArrangerSelectionsAction (
    const ArrangerSelections &sel,
    bool                      create);
};

class CreateArrangerSelectionsAction
  final : public CreateOrDeleteArrangerSelectionsAction
{
public:
  CreateArrangerSelectionsAction (const ArrangerSelections &sel)
      : CreateOrDeleteArrangerSelectionsAction (sel, true) {};
};

class DeleteArrangerSelectionsAction
  final : public CreateOrDeleteArrangerSelectionsAction
{
public:
  DeleteArrangerSelectionsAction (const ArrangerSelections &sel)
      : CreateOrDeleteArrangerSelectionsAction (sel, false) {};
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
    const ArrangerSelections &sel_before,
    const ArrangerSelections &sel_after,
    bool                      already_recorded);
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
    const ArrangerSelections &sel,
    bool                      move,
    double                    ticks,
    int                       delta_chords,
    int                       delta_pitch,
    int                       delta_tracks,
    int                       delta_lanes,
    double                    delta_normalized_amount,
    const PortIdentifier *    tgt_port_id,
    bool                      already_moved);
};

class ArrangerSelectionsAction::MoveOrDuplicateTimelineAction
    : public ArrangerSelectionsAction::MoveOrDuplicateAction
{
public:
  MoveOrDuplicateTimelineAction (
    const TimelineSelections &sel,
    bool                      move,
    double                    ticks,
    int                       delta_tracks,
    int                       delta_lanes,
    const PortIdentifier *    tgt_port_id,
    bool                      already_moved)
      : MoveOrDuplicateAction (
          dynamic_cast<const ArrangerSelections &> (sel),
          move,
          ticks,
          0,
          0,
          delta_tracks,
          delta_lanes,
          0,
          tgt_port_id,
          already_moved) {};
};

class ArrangerSelectionsAction::MoveOrDuplicateMidiAction
    : public ArrangerSelectionsAction::MoveOrDuplicateAction
{
public:
  MoveOrDuplicateMidiAction (
    const MidiSelections &sel,
    bool                  move,
    double                ticks,
    int                   delta_pitch,
    bool                  already_moved)
      : MoveOrDuplicateAction (
          dynamic_cast<const ArrangerSelections &> (sel),
          move,
          ticks,
          0,
          delta_pitch,
          0,
          0,
          0,
          nullptr,
          already_moved) {};
};

class ArrangerSelectionsAction::MoveOrDuplicateChordAction
    : public ArrangerSelectionsAction::MoveOrDuplicateAction
{
public:
  MoveOrDuplicateChordAction (
    ChordSelections &sel,
    bool             move,
    double           ticks,
    int              delta_chords,
    bool             already_moved)
      : MoveOrDuplicateAction (
          dynamic_cast<ArrangerSelections &> (sel),
          move,
          ticks,
          delta_chords,
          0,
          0,
          0,
          0,
          nullptr,
          already_moved) {};
};

class ArrangerSelectionsAction::MoveOrDuplicateAutomationAction
    : public ArrangerSelectionsAction::MoveOrDuplicateAction
{
public:
  MoveOrDuplicateAutomationAction (
    AutomationSelections &sel,
    bool                  move,
    double                ticks,
    int                   delta_normalized_amount,
    bool                  already_moved)
      : MoveOrDuplicateAction (
          dynamic_cast<ArrangerSelections &> (sel),
          move,
          ticks,
          0,
          0,
          0,
          0,
          delta_normalized_amount,
          nullptr,
          already_moved) {};
};

class ArrangerSelectionsAction::MoveByTicksAction
    : public ArrangerSelectionsAction::MoveOrDuplicateAction
{
public:
  MoveByTicksAction (
    const ArrangerSelections &sel,
    double                    ticks,
    bool                      already_moved)
      : MoveOrDuplicateAction (sel, true, ticks, 0, 0, 0, 0, 0, nullptr, already_moved)
  {
  }
};

class ArrangerSelectionsAction::MoveMidiAction
    : public ArrangerSelectionsAction::MoveOrDuplicateAction
{
public:
  MoveMidiAction (
    const MidiSelections &sel,
    double                ticks,
    int                   delta_pitch,
    bool                  already_moved)
      : MoveOrDuplicateAction (sel, true, ticks, 0, delta_pitch, 0, 0, 0, nullptr, already_moved)
  {
  }
};

class ArrangerSelectionsAction::MoveChordAction
    : public ArrangerSelectionsAction::MoveOrDuplicateAction
{
public:
  MoveChordAction (
    const ChordSelections &sel,
    double                 ticks,
    int                    delta_chords,
    bool                   already_moved)
      : MoveOrDuplicateAction (sel, true, ticks, delta_chords, 0, 0, 0, 0, nullptr, already_moved)
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
    const ArrangerSelections &sel_before,
    const ArrangerSelections &sel_after,
    double                    ticks,
    int                       delta_tracks,
    int                       delta_lanes,
    bool                      already_moved);
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
    const ArrangerSelections  &sel_before,
    const ArrangerSelections * sel_after,
    EditType                   type,
    bool                       already_edited);

  /**
   * @brief Wrapper for a single object.
   */
  static std::unique_ptr<EditArrangerSelectionsAction> create (
    const ArrangerObject &obj_before,
    const ArrangerObject &obj_after,
    EditType              type,
    bool                  already_edited);

  /**
   * @brief Wrapper for MIDI functions.
   */
  static std::unique_ptr<EditArrangerSelectionsAction> create (
    const MidiSelections &sel_before,
    MidiFunctionType      midi_func_type,
    MidiFunctionOpts      opts);

  /**
   * @brief Wrapper for automation functions.
   */
  static std::unique_ptr<EditArrangerSelectionsAction> create (
    const AutomationSelections &sel_before,
    AutomationFunctionType      automation_func_type);

  /**
   * @brief Wrapper for audio functions.
   */
  static std::unique_ptr<EditArrangerSelectionsAction> create (
    const AudioSelections &sel_before,
    AudioFunctionType      audio_func_type,
    AudioFunctionOpts      opts,
    const std::string *    uri);
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
  AutomationFillAction (
    const Region &region_before,
    const Region &region_after,
    bool          already_changed);
};

class ArrangerSelectionsAction::SplitAction : public ArrangerSelectionsAction
{
public:
  /**
   * Creates a new action for splitting regions.
   *
   * @param pos Global position to split at.
   */
  SplitAction (const ArrangerSelections &sel, Position pos);
};

class ArrangerSelectionsAction::MergeAction : public ArrangerSelectionsAction
{
public:
  /**
   * Creates a new action for merging objects.
   */
  MergeAction (const ArrangerSelections &sel)
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
    const ArrangerSelections  &sel_before,
    const ArrangerSelections * sel_after,
    ResizeType                 type,
    double                     ticks);
};

class ArrangerSelectionsAction::QuantizeAction : public ArrangerSelectionsAction
{
public:
  /**
   * Creates a new action for quantizing ArrangerObject's.
   *
   * @param opts Quantize options.
   */
  QuantizeAction (const ArrangerSelections &sel, QuantizeOptions opts)
      : ArrangerSelectionsAction (sel, Type::Quantize)
  {
    opts_ = std::make_unique<QuantizeOptions> (opts);
  }
};

DEFINE_ENUM_FORMATTER (
  ArrangerSelectionsAction::ResizeType,
  ArrangerSelectionsAction_ResizeType,
  "Resize L",
  "Resize R",
  "Resize L (loop)",
  "Resize R (loop)",
  "Resize L (fade)",
  "Resize R (fade)",
  "Stretch L",
  "Stretch R");

/**
 * @}
 */

#endif
