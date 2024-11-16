// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_BACKEND_ARRANGER_OBJECT_H__
#define __GUI_BACKEND_ARRANGER_OBJECT_H__

#include <memory>

#include "gui/backend/position_proxy.h"

#include <QtGlobal>

class ArrangerObject;
class ArrangerSelections;
class UndoableAction;
class ChordObject;
class ScaleObject;
class AutomationPoint;
class Velocity;
class Marker;

/**
 * @addtogroup dsp
 *
 * @{
 */

constexpr int ARRANGER_OBJECT_MAGIC = 347616554;
#define IS_ARRANGER_OBJECT(tr) \
  (static_cast<ArrangerObject *> (tr)->magic_ == ARRANGER_OBJECT_MAGIC \
   && static_cast<ArrangerObject *> (tr)->type_ >= ArrangerObject::Type::Region \
   && static_cast<ArrangerObject *> (tr)->type_ \
        <= ArrangerObject::Type::Velocity)
#define IS_ARRANGER_OBJECT_AND_NONNULL(x) (x && IS_ARRANGER_OBJECT (x))

static const char * arranger_object_type_strings[] = {
  QT_TR_NOOP ("None"),         QT_TR_NOOP ("All"),
  QT_TR_NOOP ("Region"),       QT_TR_NOOP ("Midi Note"),
  QT_TR_NOOP ("Chord Object"), QT_TR_NOOP ("Scale Object"),
  QT_TR_NOOP ("Marker"),       QT_TR_NOOP ("Automation Point"),
  QT_TR_NOOP ("Velocity"),
};

template <typename T>
concept ArrangerObjectSubclass = std::derived_from<T, ArrangerObject>;

class LengthableObject;
class MidiNote;
class MidiRegion;
class AudioRegion;
class AutomationRegion;
class ChordRegion;
using ArrangerObjectVariant = std::variant<
  MidiNote,
  ChordObject,
  ScaleObject,
  MidiRegion,
  AudioRegion,
  ChordRegion,
  AutomationRegion,
  AutomationPoint,
  Marker,
  Velocity>;
using ArrangerObjectWithoutVelocityVariant = std::variant<
  MidiNote,
  ChordObject,
  ScaleObject,
  MidiRegion,
  AudioRegion,
  ChordRegion,
  AutomationRegion,
  Marker,
  AutomationPoint>;
using ArrangerObjectPtrVariant = to_pointer_variant<ArrangerObjectVariant>;
using ArrangerObjectWithoutVelocityPtrVariant =
  to_pointer_variant<ArrangerObjectWithoutVelocityVariant>;

class MarkerTrack;
class InstrumentTrack;
class MidiTrack;
class MasterTrack;
class MidiGroupTrack;
class AudioGroupTrack;
class FolderTrack;
class MidiBusTrack;
class AudioBusTrack;
class AudioTrack;
class ChordTrack;
class ModulatorTrack;
class TempoTrack;
class LanedTrack;

using TrackVariant = std::variant<
  MarkerTrack,
  InstrumentTrack,
  MidiTrack,
  MasterTrack,
  MidiGroupTrack,
  AudioGroupTrack,
  FolderTrack,
  MidiBusTrack,
  AudioBusTrack,
  AudioTrack,
  ChordTrack,
  ModulatorTrack,
  TempoTrack>;
using TrackPtrVariant = to_pointer_variant<TrackVariant>;
using OptionalTrackPtrVariant = std::optional<TrackPtrVariant>;

class TimelineSelections;
class MidiSelections;
class ChordSelections;
class AutomationSelections;
class AudioSelections;
using ArrangerSelectionsVariant = std::variant<
  TimelineSelections,
  MidiSelections,
  ChordSelections,
  AutomationSelections,
  AudioSelections>;
using ArrangerSelectionsPtrVariant =
  to_pointer_variant<ArrangerSelectionsVariant>;
// using ArrangerSelectionsConstPtrVariant =
// to_const_pointer_variant<ArrangerSelectionsVariant>;
using ClipEditorArrangerSelectionsVariant = std::
  variant<MidiSelections, ChordSelections, AutomationSelections, AudioSelections>;
using ClipEditorArrangerSelectionsPtrVariant =
  to_pointer_variant<ClipEditorArrangerSelectionsVariant>;

#define DEFINE_ARRANGER_OBJECT_QML_PROPERTIES(ClassType) \
public: \
  /* ================================================================ */ \
  /* type */ \
  /* ================================================================ */ \
  Q_PROPERTY (int type READ getType CONSTANT) \
  int getType () const \
  { \
    return ENUM_VALUE_TO_INT (type_); \
  } \
  /* ================================================================ */ \
  /* hasLength */ \
  /* ================================================================ */ \
  Q_PROPERTY (bool hasLength READ getHasLength CONSTANT) \
  bool getHasLength () const \
  { \
    return std::derived_from<ClassType, LengthableObject>; \
  } \
  /* ================================================================ */ \
  /* position */ \
  /* ================================================================ */ \
  Q_PROPERTY (PositionProxy * position READ getPosition CONSTANT) \
  PositionProxy * getPosition () const \
  { \
    return pos_; \
  }

/**
 * @brief Base class for all objects in the arranger.
 *
 * The ArrangerObject class is the base class for all objects that can be
 * placed in the arranger, such as regions, MIDI notes, chord objects, etc.
 * It provides common functionality and properties shared by all these
 * objects.
 *
 * Arranger objects should be managed as shared pointers because their ownership
 * is shared by both their parents and the project selections. Logically, only
 * the ArrangerObject parent actually owns the object, but if we use unique_ptr
 * we would have to use more complex strategies like observer pattern so we
 * avoid that by using shared_ptr.
 *
 * We also need shared_from_this() in various cases (TODO explain).
 */
class ArrangerObject : public ISerializable<ArrangerObject>
{
  Q_DISABLE_COPY_MOVE (ArrangerObject)

public:
  /**
   * Flag used in some functions.
   */
  enum class ResizeType
  {
    Normal,
    Loop,
    Fade,
    Stretch,

    /**
     * Used when we want to resize to contents when BPM changes.
     *
     * @note Only applies to audio.
     */
    RESIZE_STRETCH_BPM_CHANGE,
  };

  /**
   * The type of the object.
   */
  enum class Type
  {
    /* These two are not actual object types. */
    None,
    All,

    Region,
    MidiNote,
    ChordObject,
    ScaleObject,
    Marker,
    AutomationPoint,
    Velocity,
  };

  /**
   * Flags.
   */
  enum class Flags
  {
    /** This object is not a project object, but an object used temporarily eg.
     * when undoing/ redoing. */
    NonProject = 1 << 0,

    /** The object is a snapshot (cache used during DSP). TODO */
    // FLAG_SNAPSHOT = 1 << 1,
  };

  enum class PositionType
  {
    Start,
    End,
    ClipStart,
    LoopStart,
    LoopEnd,
    FadeIn,
    FadeOut,
  };

public:
  ArrangerObject (Type type);

  ~ArrangerObject () override = default;

  using ArrangerObjectPtr = ArrangerObject *;

  /**
   * @brief Generate @ref transient_.
   */
  void generate_transient ();

  /**
   * Returns if the object type has a length.
   */
  static inline bool type_has_length (Type type)
  {
    return (type == Type::Region || type == Type::MidiNote);
  }

  bool has_length () const { return type_has_length (type_); }

  /**
   * Returns if the object type has a global position.
   */
  static inline bool type_has_global_pos (Type type)
  {
    return (
      type == Type::Region || type == Type::ScaleObject || type == Type::Marker);
  }

  static inline bool type_has_name (Type type)
  {
    return (type == Type::Region || type == Type::Marker);
  }

  /** Returns if the object can loop. */
  static inline bool type_can_loop (Type type)
  {
    return (type == Type::Region);
  }

  static inline bool type_can_mute (Type type)
  {
    return (type == Type::Region || type == Type::MidiNote);
  }

  static inline bool type_owned_by_region (Type type)
  {
    return (
      type == Type::Velocity || type == Type::MidiNote
      || type == Type::ChordObject || type == Type::AutomationPoint);
  }

  bool owned_by_region () const { return type_owned_by_region (type_); }

  bool can_mute () const { return type_can_mute (type_); }

  bool has_name () const { return type_has_name (type_); }

  /**
   * @brief Returns whether the object is hovered in the corresponding arranger.
   *
   * @return true
   * @return false
   */
  bool is_hovered () const;

  /**
   * Generates a human readable name for the object.
   *
   * If the object has a name, this returns a copy of the name, otherwise
   * generates something appropriate.
   */
  virtual std::string gen_human_friendly_name () const
  {
    /* this will be called if unimplemented - it's not needed for things like
     * Velocity, which don't have reasonable names. */
    z_return_val_if_reached ("");
  };

  /**
   * Initializes the object after loading a Project.
   */
  virtual void init_loaded () = 0;

  /**
   * Returns the ArrangerSelections corresponding to the given object type.
   */
  template <typename T = ArrangerSelections>
  static T * get_selections_for_type (Type type);

  /**
   * @brief Create an ArrangerSelections instance with a clone of this object.
   */
  std::unique_ptr<ArrangerSelections>
  create_arranger_selections_from_this () const;

  /**
   * Selects the object by adding it to its corresponding selections or making
   * it the only selection.
   *
   * @param select 1 to select, 0 to deselect.
   * @param append 1 to append, 0 to make it the only selection.
   */
  static void select (
    const ArrangerObjectPtr &obj,
    bool                     select,
    bool                     append,
    bool                     fire_events);

  void select (bool select, bool append, bool fire_events)
  {
    ArrangerObject::select (this, select, append, fire_events);
  }

  /**
   * Returns whether the given object is hit by the given  range.
   *
   * @param start Start position.
   * @param end End position.
   * @param range_start_inclusive Whether @ref pos_ == @p start is allowed. Can't
   * imagine a case where this would be false, but kept an option just in case.
   * @param range_end_inclusive Whether @ref pos_ == @p end is allowed.
   */
  bool is_start_hit_by_range (
    const Position &start,
    const Position &end,
    bool            range_start_inclusive = true,
    bool            range_end_inclusive = false) const
  {
    return is_start_hit_by_range (
      start.frames_, end.frames_, range_start_inclusive, range_end_inclusive);
  }

  /**
   * @brief @see @ref is_start_hit_by_range().
   */
  bool is_start_hit_by_range (
    const signed_frame_t global_frames_start,
    const signed_frame_t global_frames_end,
    bool                 range_start_inclusive = true,
    bool                 range_end_inclusive = false) const
  {
    return (range_start_inclusive
              ? (pos_->frames_ >= global_frames_start)
              : (pos_->frames_ > global_frames_start))
           && (range_end_inclusive ? (pos_->frames_ <= global_frames_end) : (pos_->frames_ < global_frames_end));
  }

  void parent_base_qproperties (QObject &derived);

  /**
   * Returns if the object is in the selections.
   */
  bool is_selected () const;

  /**
   * @brief Prints the given object to a string.
   */
  virtual std::string print_to_str () const = 0;

  /**
   * @brief Prints debug information about the given object.
   *
   * Uses @ref print_to_str() internally.
   */
  void print () const;

  /**
   * Getter.
   */
  void get_pos (Position * pos) const
  {
    *pos = *static_cast<Position *> (pos_);
  };

  void get_position_from_type (Position * pos, PositionType type) const;

  template <typename T = ArrangerObject> T * get_transient () const
  {
    return dynamic_cast<T *> (transient_);
  };

  /**
   * Callback when beginning to edit the object.
   *
   * This saves a clone of its current state to its arranger.
   */
  void edit_begin () const;

  /**
   * Callback when finishing editing the object.
   *
   * This performs an undoable action.
   *
   * @param action_edit_type A @ref ArrangerSelectionsAction::EditType.
   */
  void edit_finish (int action_edit_type) const;

  void edit_position_finish () const;

  /**
   * The setter is for use in e.g. the digital meters whereas the set_pos func
   * is used during arranger actions.
   *
   * @note This validates the position.
   */
  void pos_setter (const Position * pos);

  /**
   * Returns if the given Position is valid.
   *
   * @param pos The position to set to.
   * @param pos_type The type of Position to set in the ArrangerObject.
   */
  [[nodiscard]] ATTR_HOT ATTR_NONNULL bool
  is_position_valid (const Position &pos, PositionType pos_type) const;

  /**
   * Sets the given position on the object, optionally attempting to validate
   * before.
   *
   * @param pos The position to set to.
   * @param pos_type The type of Position to set in the ArrangerObject.
   * @param validate Validate the Position before setting it.
   *
   * @return Whether the position was set (false if invalid).
   */
  bool
  set_position (const Position * pos, PositionType pos_type, bool validate);

  /**
   * Copies identifier values from src to this object.
   *
   * Used by `UndoableAction`s where only partial copies that identify the
   * original objects are needed.
   *
   * Need to rethink this as it's not easily maintainable.
   */
  void copy_identifier (const ArrangerObject &src);

  /**
   * Moves the object by the given amount of ticks.
   */
  void move (double ticks);

  /**
   * Returns if the object is allowed to have lanes.
   */
  virtual bool can_have_lanes () const { return false; }

  virtual bool can_fade () const { return false; }

  inline void set_track_name_hash (unsigned int track_name_hash)
  {
    track_name_hash_ = track_name_hash;
  }

  /**
   * Updates the positions in each child recursively.
   *
   * @param from_ticks Whether to update the positions based on ticks (true) or
   * frames (false).
   * @param action To be passed when called from an undoable action.
   */
  void update_positions (
    bool             from_ticks,
    bool             bpm_change,
    UndoableAction * action = nullptr);

  /**
   * Returns the Track this ArrangerObject is in.
   */
  ATTR_HOT virtual TrackPtrVariant get_track () const;

  static inline const char * get_type_as_string (Type type)
  {
    return arranger_object_type_strings[static_cast<int> (type)];
  }

  const char * get_type_as_string () const
  {
    return get_type_as_string (type_);
  }

  /**
   * @brief Performs some post-deserialization logic, like adjusting positions
   * to BPM changes.
   */
  void post_deserialize ();

  /**
   * Validates the arranger object.
   *
   * @param frames_per_tick Frames per tick used when validating audio
   * regions. Passing 0 will use the value from the current engine.
   * @return Whether valid.
   *
   * Must only be implemented by final objects.
   */
  virtual bool validate (bool is_project, double frames_per_tick) const = 0;

  /**
   * Returns the project ArrangerObject matching this.
   *
   * This should be called when we have a copy or a clone, to get the actual
   * region in the project.
   */
  virtual std::optional<ArrangerObjectPtrVariant> find_in_project () const = 0;

  /**
   * Appends the ArrangerObject to where it belongs in the project (eg, a
   * Track), without taking into account its previous index (eg, before deletion
   * if undoing).
   *
   * @throw ZrythmError on failure.
   * @return A reference to the newly added clone.
   */
  virtual ArrangerObjectPtrVariant
  add_clone_to_project (bool fire_events) const = 0;

  /**
   * Inserts the object where it belongs in the project (eg, a Track).
   *
   * This function assumes that the object already knows the index where it
   * should be inserted in its parent.
   *
   * This is mostly used when undoing.
   *
   * @throw ZrythmException on failure.
   * @return A reference to the newly inserted clone.
   */
  virtual ArrangerObjectPtrVariant insert_clone_to_project () const = 0;

  /**
   * Removes the object (which can be obtained from @ref find_in_project()) from
   * its parent in the project.
   *
   * @return A pointer to the removed object (whose lifetime is now the
   * responsibility of the caller).
   */
  std::optional<ArrangerObjectPtrVariant>
  remove_from_project (bool free_obj, bool fire_events = false);

  /**
   * Returns whether the arranger object is part of a frozen track.
   *
   * @note Currently unused.
   */
  bool is_frozen () const;

  /**
   * Returns whether the given object is deletable or not (eg, start marker).
   */
  virtual bool is_deletable () const { return true; };

  bool is_renamable () const
  {
    return type_has_name (type_) && is_deletable ();
  };

  inline bool can_loop () const { return type_can_loop (type_); };

  inline bool is_region () const { return type_ == Type::Region; };

  bool is_scale_object () const { return type_ == Type::ScaleObject; };
  bool is_marker () const { return type_ == Type::Marker; };
  bool is_chord_object () const { return type_ == Type::ChordObject; };

protected:
  void copy_members_from (const ArrangerObject &other);

  void init_loaded_base ();

  DECLARE_DEFINE_BASE_FIELDS_METHOD ();

  /**
   * @brief To be called by @ref validate() implementations.
   */
  bool are_members_valid (bool is_project) const;

private:
  Position * get_position_ptr (PositionType type);

public:
  /**
   * Position (or start Position if the object has length).
   *
   * For audio/MIDI, the material starts at this frame.
   *
   * Midway Position between previous and next AutomationPoint's, if
   * AutomationCurve.
   */
  PositionProxy * pos_ = nullptr;

  Type type_{};

  /** Hash of the name of the track this object belongs to. */
  unsigned int track_name_hash_ = 0;

  /** Track this object belongs to (cache to be set during graph calculation). */
  OptionalTrackPtrVariant track_;

  /**
   * A copy ArrangerObject corresponding to this, such as when ctrl+dragging.
   *
   * This is generated when an object is added to the project selections.
   *
   * This will be the clone object saved in the cloned arranger selections in
   * each arranger during actions, and would get drawn separately.
   */
  ArrangerObject * transient_ = nullptr;

  /**
   * The opposite of the above. This will be set on the transient objects.
   */
  ArrangerObject * main_ = nullptr;

  int magic_ = ARRANGER_OBJECT_MAGIC;

  /**
   * Whether deleted with delete tool.
   *
   * This is used to simply hide these objects until the action finishes so that
   * they can be cloned for the actions.
   */
  bool deleted_temporarily_ = false;

  /** Flags. */
  Flags flags_ = {};

  /**
   * Whether part of an auditioner track. */
  bool is_auditioner_ = false;
};

inline bool
operator== (const ArrangerObject &lhs, const ArrangerObject &rhs)
{
  return lhs.type_ == rhs.type_ && *lhs.pos_ == *rhs.pos_
         && lhs.track_name_hash_ == rhs.track_name_hash_;
}

template <typename T>
concept FinalArrangerObjectSubclass =
  std::derived_from<T, ArrangerObject> && FinalClass<T> && CompleteType<T>;

/**
 * @}
 */

#endif
