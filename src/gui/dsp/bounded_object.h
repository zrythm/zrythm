// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_BOUNDED_OBJECT_H__
#define __AUDIO_BOUNDED_OBJECT_H__

#include "gui/dsp/arranger_object.h"

class LoopableObject;
class FadeableObject;

#define DEFINE_BOUNDED_OBJECT_QML_PROPERTIES(ClassType) \
public: \
  /* ================================================================ */ \
  /* endPosition */ \
  /* ================================================================ */ \
  Q_PROPERTY (PositionProxy * endPosition READ getEndPosition CONSTANT) \
  PositionProxy * getEndPosition () const \
  { \
    return end_pos_; \
  }

/**
 * @brief Base class for all objects in the arranger that have a length.
 *
 * The BoundedObject class is the base class for all objects that have a length,
 * such as regions, MIDI notes, etc. It provides common functionality and
 * properties shared by all these objects, such as start and end positions,
 * and methods to resize and check if the object is hit by a position or range.
 *
 * BoundedObject inherits from ArrangerObject and extends it with length-related
 * functionality.
 */
class BoundedObject : virtual public ArrangerObject
{
public:
  BoundedObject ();
  ~BoundedObject () override = default;
  Z_DISABLE_COPY_MOVE (BoundedObject)

  /**
   * Getter.
   */
  auto get_end_position () const
  {
    return *static_cast<dsp::Position *> (end_pos_);
  }

  void parent_base_qproperties (QObject &derived);

  /**
   * The setter is for use in e.g. the digital meters whereas the set_pos func
   * is used during arranger actions.
   */
  void end_position_setter_validated (
    const dsp::Position &pos,
    dsp::TicksPerFrame   ticks_per_frame)
  {
    set_position (pos, PositionType::End, true, ticks_per_frame);
  }

  void set_end_position_unvalidated (const dsp::Position &pos)
  {
    // FIXME qobject updates...
    *static_cast<dsp::Position *> (end_pos_) = pos;
  }

  /**
   * Returns the length in ticks.
   *
   * (End Position - start Position).
   */
  auto get_length_in_ticks () const
  {
    return end_pos_->ticks_ - get_position ().ticks_;
  }

  /**
   * Returns the length in frames.
   *
   * (End Position - start Position).
   */
  auto get_length_in_frames () const
  {
    return end_pos_->frames_ - get_position ().frames_;
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
  template <typename SelfT>
  void set_start_pos_full_size (
    this SelfT        &self,
    const Position    &pos,
    dsp::FramesPerTick frames_per_tick)
    requires (FinalArrangerObjectSubclass<SelfT>)
  {
    self.position_setter_validated (
      pos, dsp::to_ticks_per_frame (frames_per_tick));
    self.set_loop_and_fade_positions_from_length (frames_per_tick);
    assert (pos.frames_ == self.pos_->frames_);
  }

  /**
   * Sets the end position of the ArrangerObject and also sets the loop end and
   * fade out (if object supports those) to that position.
   */
  template <typename SelfT>
  void set_end_pos_full_size (
    this SelfT          &self,
    const dsp::Position &pos,
    dsp::FramesPerTick   frames_per_tick)
    requires (FinalArrangerObjectSubclass<SelfT>)
  {
    self.end_position_setter_validated (
      pos, dsp::to_ticks_per_frame (frames_per_tick));
    self.set_loop_and_fade_positions_from_length (frames_per_tick);
    assert (pos.frames_ == self.end_pos_->frames_);
  }

  /**
   * Returns whether the object is hit by the given position (local position if
   * non-timeline object).
   *
   * @param pos Position to check.
   * @param obj_end_pos_inclusive Whether @ref end_pos_ is considered as part of
   * the object. This is probably always false.
   */
  bool
  is_hit (const dsp::Position &pos, bool object_end_pos_inclusive = false) const
  {
    return is_hit (pos.frames_, object_end_pos_inclusive);
  }

  /**
   * @brief
   *
   * @param frames Local position if non-timeline object.
   * @param object_end_pos_inclusive
   */
  bool
  is_hit (const signed_frame_t frames, bool object_end_pos_inclusive = false) const
  {
    signed_frame_t obj_start = get_position ().frames_;
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
    const dsp::Position &start,
    const dsp::Position &end,
    bool                 range_start_inclusive = true,
    bool                 range_end_inclusive = false,
    bool                 object_end_pos_inclusive = false) const
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
    signed_frame_t obj_start = get_position ().frames_;
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
  is_inside_range (const dsp::Position &start, const dsp::Position &end) const
  {
    return get_position ().is_between_excl_both (start, end)
           || end_pos_->is_between_excl_both (start, end)
           || (get_position () < start && *end_pos_ >= end);
  }

protected:
  void
  copy_members_from (const BoundedObject &other, ObjectCloneType clone_type);

  bool
  are_members_valid (bool is_project, dsp::FramesPerTick frames_per_tick) const;

private:
  /**
   * @brief Called internally by @ref set_start_pos_full_size() and @ref
   * set_end_pos_full_size().
   */
  template <typename SelfT>
  void set_loop_and_fade_positions_from_length (
    this SelfT        &self,
    dsp::FramesPerTick frames_per_tick)
    requires (FinalArrangerObjectSubclass<SelfT>)
  {
    // note: not sure if using ticks is OK here, maybe getting the length in
    // frames might be less bug-prone
    const auto ticks = self.get_length_in_ticks ();
    if constexpr (std::derived_from<SelfT, LoopableObject>)
      {
        self.loop_end_pos_.from_ticks (ticks, frames_per_tick);
      }
    if constexpr (std::derived_from<SelfT, FadeableObject>)
      {
        self.fade_out_pos_.from_ticks (ticks, frames_per_tick);
      }
  }

  friend auto to_json (nlohmann::json &j, const BoundedObject &object)
  {
    j["endPos"] = object.end_pos_;
  }
  friend auto from_json (const nlohmann::json &j, BoundedObject &object)
  {
    j.at ("endPos").get_to (*object.end_pos_);
  }

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
concept FinalBoundedObjectSubclass =
  std::derived_from<T, BoundedObject> && FinalClass<T>;

using BoundedObjectVariant =
  std::variant<MidiRegion, AudioRegion, ChordRegion, AutomationRegion, MidiNote>;
using BoundedObjectPtrVariant = to_pointer_variant<BoundedObjectVariant>;

#endif // __AUDIO_BOUNDED_OBJECT_H__
