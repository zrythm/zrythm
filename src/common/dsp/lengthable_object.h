// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_LENGTHABLE_OBJECT_H__
#define __AUDIO_LENGTHABLE_OBJECT_H__

#include "common/dsp/arranger_object.h"

#define DEFINE_LENGTHABLE_OBJECT_QML_PROPERTIES(ClassType) \
public: \
  /* ================================================================ */ \
  /* endPosition */ \
  /* ================================================================ */ \
  Q_PROPERTY (PositionProxy * endPosition READ getEndPosition CONSTANT) \
  PositionProxy * getEndPosition () const \
  { \
    return end_pos_; \
  }

class LengthableObject
    : virtual public ArrangerObject,
      public ISerializable<LengthableObject>
{
public:
  LengthableObject ();
  ~LengthableObject () override = default;
  Q_DISABLE_COPY_MOVE (LengthableObject)

  /**
   * Getter.
   */
  void get_end_pos (Position * pos) const
  {
    *pos = *static_cast<Position *> (end_pos_);
  }

  void parent_base_qproperties (QObject &derived);

  /**
   * The setter is for use in e.g. the digital meters whereas the set_pos func
   * is used during arranger actions.
   */
  void end_pos_setter (const Position * pos)
  {
    set_position (pos, PositionType::End, true);
  }

  /**
   * Returns the length in ticks.
   *
   * (End Position - start Position).
   */
  inline double get_length_in_ticks () const
  {
    return end_pos_->ticks_ - pos_->ticks_;
  }

  /**
   * Returns the length in frames.
   *
   * (End Position - start Position).
   */
  inline signed_frame_t get_length_in_frames () const
  {
    return end_pos_->frames_ - pos_->frames_;
  }

  /**
   * Resizes the object on the left side or right side by given amount of ticks,
   * for objects that do not have loops (currently none? keep it as reference).
   *
   * @param left 1 to resize left side, 0 to resize right side.
   * @param ticks Number of ticks to resize.
   * @param during_ui_action Whether this is called during a UI action (not at
   *   the end).
   *
   * @throw ZrythmException on failure.
   */
  void resize (bool left, ResizeType type, double ticks, bool during_ui_action);

  /**
   * Sets the end position of the ArrangerObject and also sets the loop end and
   * fade out (if object supports those) so that they are at the end.
   */
  void set_start_pos_full_size (const Position * pos);

  /**
   * Sets the end position of the ArrangerObject and also sets the loop end and
   * fade out (if object supports those) to that position.
   */
  void set_end_pos_full_size (const Position * pos);

  /**
   * Splits the given object at the given Position.
   *
   * if @ref is_project is true, it deletes the original object and adds 2 new
   * objects in the same parent (Track or AutomationTrack or Region).
   *
   * @param region Object to split. This object will be deleted if @p is_project
   * is true.
   * @param pos The Position to split at.
   * @param pos_is_local Whether the position is a local one (starting at region
   * start for `RegionOwnedObject`s).
   * @param r1 Address to hold the pointer to the newly created object 1.
   * @param r2 Address to hold the pointer to the newly created object 2.
   * @param is_project Whether the object being passed is a project object. If
   * true, it will be removed from the project and the child objects will be
   * added to the project, otherwise it will be untouched and the children will
   * be mere clones.
   *
   * @throw ZrythmException on error.
   */
  template <typename T>
  static std::pair<T *, T *>
  split (T &self, Position pos, bool pos_is_local, bool is_project);

  /**
   * Undoes what @ref split() did.
   *
   * @throw ZrythmException on error.
   */
  template <typename T> static T * unsplit (T &r1, T &r2, bool fire_events);

  /**
   * Returns whether the object is hit by the given position.
   *
   * @param pos Position to check.
   * @param obj_end_pos_inclusive Whether @ref end_pos_ is considered as part of
   * the object. This is probably always false.
   */
  bool is_hit (const Position &pos, bool object_end_pos_inclusive = false) const
  {
    return is_hit (pos.frames_, object_end_pos_inclusive);
  }

  bool
  is_hit (const signed_frame_t frames, bool object_end_pos_inclusive = false) const
  {
    signed_frame_t obj_start = pos_->frames_;
    signed_frame_t obj_end =
      object_end_pos_inclusive ? end_pos_->frames_ : end_pos_->frames_ - 1;

    return obj_start <= frames && obj_end >= frames;
  }

  /**
   * Returns whether the given object is hit by the given  range.
   *
   * @param start Start position.
   * @param end End position.
   * @param range_start_inclusive Whether @ref pos_ == @p start is allowed.
   * Can't imagine a case where this would be false, but kept an option just
   * in case.
   * @param range_end_inclusive Whether @ref pos_ == @p end is allowed.
   * @param obj_end_pos_inclusive Whether @ref end_pos_ is considered as part
   * of the object. This is probably always false.
   */
  bool is_hit_by_range (
    const Position &start,
    const Position &end,
    bool            range_start_inclusive = true,
    bool            range_end_inclusive = false,
    bool            object_end_pos_inclusive = false) const
  {
    return is_hit_by_range (start.frames_, end.frames_, range_end_inclusive);
  }

  bool is_hit_by_range (
    signed_frame_t global_frames_start,
    signed_frame_t global_frames_end,
    bool           range_start_inclusive = true,
    bool           range_end_inclusive = false,
    bool           object_end_pos_inclusive = false) const
  {
    /*
     * Case 1: Object start is inside range
     *        |----- Range -----|
     *                |-- Object --|
     *          |-- Object --|           // also covers whole object inside
     * range
     *
     * Case 2: Object end is inside range
     *        |----- Range -----|
     *     |-- Object --|
     *
     * Case 3: Range start is inside object
     *       |-- Object --|
     *          |---- Range ----|
     *        |- Range -|              // also covers whole range inside object
     *
     * Case 4: Range end is inside object
     *            |-- Object --|
     *       |---- Range ----|
     */

    /* get adjusted values */
    signed_frame_t range_start =
      range_start_inclusive ? global_frames_start : global_frames_start + 1;
    signed_frame_t range_end =
      range_end_inclusive ? global_frames_end : global_frames_end - 1;
    signed_frame_t obj_start = pos_->frames_;
    signed_frame_t obj_end =
      object_end_pos_inclusive ? end_pos_->frames_ : end_pos_->frames_ - 1;

    /* 1. object start is inside range */
    if (obj_start >= range_start && obj_start <= range_end)
      return true;

    /* 2. object end is inside range */
    if (obj_end >= range_start && obj_end <= range_end)
      return true;

    /* 3. range start is inside object */
    if (range_start >= obj_start && range_start <= obj_end)
      return true;

    /* 4. range end is inside object */
    return (range_end >= obj_start && range_end <= obj_end);
  }

  /** Checks if any part of the object is hit by the given range. */
  virtual bool
  is_inside_range (const Position &start, const Position &end) const
  {
    return pos_->is_between_excl_both (start, end)
           || end_pos_->is_between_excl_both (start, end)
           || (*pos_ < start && *end_pos_ >= end);
  }

  friend bool
  operator== (const LengthableObject &lhs, const LengthableObject &rhs);

protected:
  void copy_members_from (const LengthableObject &other);

  void init_loaded_base ();

  bool are_members_valid (bool is_project) const;

  DECLARE_DEFINE_BASE_FIELDS_METHOD ();

private:
  /**
   * @brief Called internally by @ref set_start_pos_full_size() and @ref
   * set_end_pos_full_size().
   *
   * FIXME breaks encapsulation, probably belongs somewhere else.
   */
  void set_loop_and_fade_to_full_size ();

public:
  /**
   * End Position, if the object has one.
   *
   * This is exclusive of the material, i.e., the data at this position is not
   * counted (for audio regions at least, TODO check for others).
   */
  PositionProxy * end_pos_ = nullptr;
};

template <typename T>
concept FinalLengthedObjectSubclass =
  std::derived_from<T, LengthableObject> && FinalClass<T>;

inline bool
operator== (const LengthableObject &lhs, const LengthableObject &rhs)
{
  return *lhs.end_pos_ == *rhs.end_pos_;
}

using LengthableObjectVariant =
  std::variant<MidiRegion, AudioRegion, ChordRegion, AutomationRegion, MidiNote>;
using LengthableObjectPtrVariant = to_pointer_variant<LengthableObjectVariant>;

#define DEFINE_OR_DECLARE_TEMPLATES_FOR_LENGTHABLE_OBJECT(_extern, subclass) \
  _extern template std::pair<subclass *, subclass *> \
  LengthableObject::split<subclass> ( \
    subclass &, const Position, const bool, bool); \
  _extern template subclass * \
  LengthableObject::unsplit<subclass> (subclass &, subclass &, bool);

#define DECLARE_EXTERN_TEMPLATES_FOR_LENGTHABLE_OBJECT(subclass) \
  DEFINE_OR_DECLARE_TEMPLATES_FOR_LENGTHABLE_OBJECT (extern, subclass)

#define DEFINE_TEMPLATES_FOR_LENGTHABLE_OBJECT(subclass) \
  DEFINE_OR_DECLARE_TEMPLATES_FOR_LENGTHABLE_OBJECT (, subclass)

DECLARE_EXTERN_TEMPLATES_FOR_LENGTHABLE_OBJECT (MidiRegion);
DECLARE_EXTERN_TEMPLATES_FOR_LENGTHABLE_OBJECT (AudioRegion);
DECLARE_EXTERN_TEMPLATES_FOR_LENGTHABLE_OBJECT (ChordRegion);
DECLARE_EXTERN_TEMPLATES_FOR_LENGTHABLE_OBJECT (AutomationRegion);
DECLARE_EXTERN_TEMPLATES_FOR_LENGTHABLE_OBJECT (MidiNote);

#undef DECLARE_EXTERN_TEMPLATES_FOR_LENGTHABLE_OBJECT

#endif // __AUDIO_LENGTHABLE_OBJECT_H__
