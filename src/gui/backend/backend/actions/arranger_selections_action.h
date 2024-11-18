// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __UNDO_ARRANGER_SELECTIONS_ACTION_H__
#define __UNDO_ARRANGER_SELECTIONS_ACTION_H__

#include <memory>
#include <vector>

#include "common/dsp/audio_function.h"
#include "common/dsp/automation_function.h"
#include "common/dsp/lengthable_object.h"
#include "common/dsp/midi_function.h"
#include "common/dsp/midi_region.h"
#include "common/dsp/port_identifier.h"
#include "common/dsp/position.h"
#include "common/dsp/quantize_options.h"
#include "common/dsp/region.h"
#include "gui/backend/backend/actions/undoable_action.h"
#include "gui/backend/backend/audio_selections.h"
#include "gui/backend/backend/automation_selections.h"
#include "gui/backend/backend/chord_selections.h"
#include "gui/backend/backend/midi_selections.h"
#include "gui/backend/backend/timeline_selections.h"

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
class ArrangerSelectionsAction
    : public QObject,
      public UndoableAction,
      public ICloneable<ArrangerSelectionsAction>,
      public ISerializable<ArrangerSelectionsAction>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_UNDOABLE_ACTION_QML_PROPERTIES (ArrangerSelectionsAction)

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

  bool contains_clip (const AudioClip &clip) const override;

  void init_after_cloning (const ArrangerSelectionsAction &other) final;

  bool needs_transport_total_bar_update (bool perform) const override;

  bool needs_pause () const override
  {
    /* always needs a pause to update the track playback snapshots */
    return true;
  }

  bool can_contain_clip () const override { return true; }

  [[nodiscard]] QString to_string () const final;

  DECLARE_DEFINE_FIELDS_METHOD ();

protected:
  /**
   * @brief Generic constructor to be used by others
   *
   * Internally calls @ref set_before_selections().
   *
   * @param before_sel Selections before the change.
   */
  template <FinalArrangerSelectionsSubclass T>
  ArrangerSelectionsAction (const T &before_sel, Type type)
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
  template <FinalArrangerSelectionsSubclass T>
  void set_before_selections (const T &src);

  /**
   * @brief Sets @ref sel_after_ to a clone of @p src.
   *
   * @param src
   */
  template <FinalArrangerSelectionsSubclass T>
  void set_after_selections (const T &src);

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
    ArrangerObjectWithoutVelocityPtrVariant obj_var,
    int                                     tracks_diff,
    int                                     lanes_diff,
    bool                                    use_index_in_prev_lane,
    int                                     index_in_prev_lane);

  ArrangerSelectionsPtrVariant get_actual_arranger_selections () const;

  void init_loaded_impl () final;
  void perform_impl () final;
  void undo_impl () final;

public:
  /** Action type. */
  Type type_ = (Type) 0;

  /** A clone of the ArrangerSelections before the change. */
  std::optional<ArrangerSelectionsPtrVariant> sel_;

  /**
   * A clone of the ArrangerSelections after the change (used in the EDIT
   * action and quantize).
   */
  std::optional<ArrangerSelectionsPtrVariant> sel_after_;

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
  PortIdentifier * target_port_ = nullptr;

  /** String, when changing a string. */
  std::string str_;

  /** Position, when changing a Position. */
  Position pos_;

  /** Used when splitting - these are the split ArrangerObject's. */
  std::vector<LengthableObjectPtrVariant> r1_;
  std::vector<LengthableObjectPtrVariant> r2_;

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
  std::optional<RegionPtrVariant> region_before_;
  std::optional<RegionPtrVariant> region_after_;

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
  template <FinalArrangerSelectionsSubclass T>
  CreateOrDeleteArrangerSelectionsAction (const T &sel, bool create);
};

class CreateArrangerSelectionsAction
  final : public CreateOrDeleteArrangerSelectionsAction
{
public:
  template <FinalArrangerSelectionsSubclass T>
  CreateArrangerSelectionsAction (const T &sel)
      : CreateOrDeleteArrangerSelectionsAction (sel, true){};
};

class DeleteArrangerSelectionsAction
  final : public CreateOrDeleteArrangerSelectionsAction
{
public:
  template <FinalArrangerSelectionsSubclass T>
  DeleteArrangerSelectionsAction (const T &sel)
      : CreateOrDeleteArrangerSelectionsAction (sel, false){};
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
  template <FinalArrangerSelectionsSubclass T>
  RecordAction (const T &sel_before, const T &sel_after, bool already_recorded);
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
  template <FinalArrangerSelectionsSubclass T>
  MoveOrDuplicateAction (
    const T               &sel,
    bool                   move,
    double                 ticks,
    int                    delta_chords,
    int                    delta_pitch,
    int                    delta_tracks,
    int                    delta_lanes,
    double                 delta_normalized_amount,
    const PortIdentifier * tgt_port_id,
    bool                   already_moved);
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
          sel,
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
      : MoveOrDuplicateAction (sel, move, ticks, 0, delta_pitch, 0, 0, 0, nullptr, already_moved) {
        };
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
      : MoveOrDuplicateAction (sel, move, ticks, delta_chords, 0, 0, 0, 0, nullptr, already_moved) {
        };
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
          sel,
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
  template <FinalArrangerSelectionsSubclass T>
  LinkAction (
    const T &sel_before,
    const T &sel_after,
    double   ticks,
    int      delta_tracks,
    int      delta_lanes,
    bool     already_moved);
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
  template <FinalArrangerSelectionsSubclass T>
  EditArrangerSelectionsAction (
    const T  &sel_before,
    const T * sel_after,
    EditType  type,
    bool      already_edited);

  /**
   * @brief Wrapper for a single object.
   */
  template <FinalArrangerObjectSubclass T>
  static std::unique_ptr<EditArrangerSelectionsAction> create (
    const T &obj_before,
    const T &obj_after,
    EditType type,
    bool     already_edited);

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
    const AudioSelections     &sel_before,
    AudioFunctionType          audio_func_type,
    AudioFunctionOpts          opts,
    std::optional<std::string> uri);
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
  template <FinalRegionSubclass RegionT>
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
  template <FinalArrangerSelectionsSubclass T>
  SplitAction (const T &sel, Position pos);
};

class ArrangerSelectionsAction::MergeAction : public ArrangerSelectionsAction
{
public:
  /**
   * Creates a new action for merging objects.
   */
  template <FinalArrangerSelectionsSubclass T>
  MergeAction (const T &sel) : ArrangerSelectionsAction (sel, Type::Merge)
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
  template <FinalArrangerSelectionsSubclass T>
  ResizeAction (
    const T   &sel_before,
    const T *  sel_after,
    ResizeType type,
    double     ticks);
};

class ArrangerSelectionsAction::QuantizeAction : public ArrangerSelectionsAction
{
public:
  /**
   * Creates a new action for quantizing ArrangerObject's.
   *
   * @param opts Quantize options.
   */
  template <FinalArrangerSelectionsSubclass T>
  QuantizeAction (const T &sel, const QuantizeOptions &opts)
      : ArrangerSelectionsAction (sel, Type::Quantize)
  {
    opts_ = std::make_unique<QuantizeOptions> (opts);
  }
};

extern template CreateArrangerSelectionsAction::CreateArrangerSelectionsAction (
  const TimelineSelections &sel);
extern template CreateArrangerSelectionsAction::CreateArrangerSelectionsAction (
  const MidiSelections &sel);
extern template CreateArrangerSelectionsAction::CreateArrangerSelectionsAction (
  const AutomationSelections &sel);
extern template CreateArrangerSelectionsAction::CreateArrangerSelectionsAction (
  const ChordSelections &sel);
extern template CreateArrangerSelectionsAction::CreateArrangerSelectionsAction (
  const AudioSelections &sel);

extern template DeleteArrangerSelectionsAction::DeleteArrangerSelectionsAction (
  const TimelineSelections &sel);
extern template DeleteArrangerSelectionsAction::DeleteArrangerSelectionsAction (
  const MidiSelections &sel);
extern template DeleteArrangerSelectionsAction::DeleteArrangerSelectionsAction (
  const AutomationSelections &sel);
extern template DeleteArrangerSelectionsAction::DeleteArrangerSelectionsAction (
  const ChordSelections &sel);
extern template DeleteArrangerSelectionsAction::DeleteArrangerSelectionsAction (
  const AudioSelections &sel);

extern template ArrangerSelectionsAction::RecordAction::RecordAction (
  const TimelineSelections &sel_before,
  const TimelineSelections &sel_after,
  bool                      already_recorded);
extern template ArrangerSelectionsAction::RecordAction::RecordAction (
  const MidiSelections &sel_before,
  const MidiSelections &sel_after,
  bool                  already_recorded);
extern template ArrangerSelectionsAction::RecordAction::RecordAction (
  const AutomationSelections &sel_before,
  const AutomationSelections &sel_after,
  bool                        already_recorded);
extern template ArrangerSelectionsAction::RecordAction::RecordAction (
  const ChordSelections &sel_before,
  const ChordSelections &sel_after,
  bool                   already_recorded);
extern template ArrangerSelectionsAction::RecordAction::RecordAction (
  const AudioSelections &sel_before,
  const AudioSelections &sel_after,
  bool                   already_recorded);

extern template ArrangerSelectionsAction::ResizeAction::ResizeAction (
  const TimelineSelections  &sel_before,
  const TimelineSelections * sel_after,
  ResizeType                 type,
  double                     ticks);
extern template ArrangerSelectionsAction::ResizeAction::ResizeAction (
  const MidiSelections  &sel_before,
  const MidiSelections * sel_after,
  ResizeType             type,
  double                 ticks);
extern template ArrangerSelectionsAction::ResizeAction::ResizeAction (
  const AutomationSelections  &sel_before,
  const AutomationSelections * sel_after,
  ResizeType                   type,
  double                       ticks);
extern template ArrangerSelectionsAction::ResizeAction::ResizeAction (
  const ChordSelections  &sel_before,
  const ChordSelections * sel_after,
  ResizeType              type,
  double                  ticks);
extern template ArrangerSelectionsAction::ResizeAction::ResizeAction (
  const AudioSelections  &sel_before,
  const AudioSelections * sel_after,
  ResizeType              type,
  double                  ticks);

extern template ArrangerSelectionsAction::QuantizeAction::QuantizeAction (
  const TimelineSelections &sel,
  const QuantizeOptions    &opts);
extern template ArrangerSelectionsAction::QuantizeAction::QuantizeAction (
  const MidiSelections  &sel,
  const QuantizeOptions &opts);
extern template ArrangerSelectionsAction::QuantizeAction::QuantizeAction (
  const AutomationSelections &sel,
  const QuantizeOptions      &opts);
extern template ArrangerSelectionsAction::QuantizeAction::QuantizeAction (
  const ChordSelections &sel,
  const QuantizeOptions &opts);
extern template ArrangerSelectionsAction::QuantizeAction::QuantizeAction (
  const AudioSelections &sel,
  const QuantizeOptions &opts);

extern template std::unique_ptr<EditArrangerSelectionsAction>
EditArrangerSelectionsAction::create (
  const MidiRegion &obj_before,
  const MidiRegion &obj_after,
  EditType          type,
  bool              already_edited);
extern template std::unique_ptr<EditArrangerSelectionsAction>
EditArrangerSelectionsAction::create (
  const AudioRegion &obj_before,
  const AudioRegion &obj_after,
  EditType           type,
  bool               already_edited);
extern template std::unique_ptr<EditArrangerSelectionsAction>
EditArrangerSelectionsAction::create (
  const ChordRegion &obj_before,
  const ChordRegion &obj_after,
  EditType           type,
  bool               already_edited);
extern template std::unique_ptr<EditArrangerSelectionsAction>
EditArrangerSelectionsAction::create (
  const AutomationRegion &obj_before,
  const AutomationRegion &obj_after,
  EditType                type,
  bool                    already_edited);
extern template std::unique_ptr<EditArrangerSelectionsAction>
EditArrangerSelectionsAction::create (
  const Marker &obj_before,
  const Marker &obj_after,
  EditType      type,
  bool          already_edited);

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

#endif
