// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_AUTOMATION_REGION_H__
#define __AUDIO_AUTOMATION_REGION_H__

#include "dsp/region.h"

/**
 * @addtogroup dsp
 *
 * @{
 */

/**
 * Represents an automation region, which contains a collection of automation
 * points. The automation points must always stay sorted by position. The last
 * recorded automation point is also stored.
 */
class AutomationRegion final
    : public RegionImpl<AutomationRegion>,
      public ICloneable<AutomationRegion>,
      public ISerializable<AutomationRegion>
{
public:
  // Rule of 0
  AutomationRegion () = default;

  /**
   * @brief Construct a new Automation Region object.
   *
   * @param start_pos
   * @param end_pos
   * @param track_name_hash
   * @param at_idx
   * @param idx_inside_at
   */
  AutomationRegion (
    const Position &start_pos,
    const Position &end_pos,
    unsigned int    track_name_hash,
    int             at_idx,
    int             idx_inside_at);

  void init_loaded () override
  {
    for (auto &ap : aps_)
      {
        ap->init_loaded ();
      }
  }

  /**
   * Prints the automation in this Region.
   */
  void print_automation () const;

  static int sort_func (const void * _a, const void * _b);

  /**
   * Forces sort of the automation points.
   */
  void force_sort ();

  bool get_muted (bool check_parent) const override;

#if 0
  /**
   * Adds an AutomationPoint to the Region and returns a reference to it.
   */
  AutomationPoint &
  add_ap (std::unique_ptr<AutomationPoint> &&ap, bool pub_events);
#endif

  /**
   * Returns the AutomationPoint before the given one.
   */
  AutomationPoint * get_prev_ap (const AutomationPoint &ap) const;

  void add_ticks_to_children (const double ticks) override
  {
    for (auto &ap : aps_)
      {
        ap->move (ticks);
      }
  }

  /**
   * Returns the AutomationPoint after the given one.
   *
   * @param check_positions Compare positions instead of just getting the next
   * index.
   * @param check_transients Also check the transient of each object. This
   * only matters if @p check_positions is true.
   */
  HOT AutomationPoint * get_next_ap (
    const AutomationPoint &ap,
    bool                   check_positions,
    bool                   check_transients) const;

  /**
   * Returns the automation points since the last recorded automation point
   * (if the last recorded automation point was before the current pos).
   */
  void
  get_aps_since_last_recorded (Position pos, std::vector<AutomationPoint *> &aps);

  /**
   * Returns an automation point found within +/-
   * delta_ticks from the position, or NULL.
   *
   * @param before_only Only check previous automation
   *   points.
   */
  AutomationPoint * get_ap_around (
    Position * _pos,
    double     delta_ticks,
    bool       before_only,
    bool       use_snapshots);

  bool validate (bool is_project, double frames_per_tick) const override;

  /**
   * Sets the automation track.
   */
  void set_automation_track (AutomationTrack &at);

  /**
   * Gets the AutomationTrack using the saved index.
   */
  AutomationTrack * get_automation_track () const;

  void append_children (
    std::vector<RegionOwnedObjectImpl<AutomationRegion> *> &children)
    const override
  {
    for (const auto &ap : aps_)
      {
        children.push_back (ap.get ());
      }
  }

  ArrangerSelections * get_arranger_selections () const override;
  ArrangerWidget *     get_arranger_for_children () const override;

  void init_after_cloning (const AutomationRegion &other) override
  {
    init (
      other.pos_, other.end_pos_, other.id_.track_name_hash_, other.id_.at_idx_,
      other.id_.idx_);
    clone_unique_ptr_container (aps_, other.aps_);
    RegionImpl::copy_members_from (other);
    TimelineObject::copy_members_from (other);
    NameableObject::copy_members_from (other);
    LoopableObject::copy_members_from (other);
    MuteableObject::copy_members_from (other);
    LengthableObject::copy_members_from (other);
    ColoredObject::copy_members_from (other);
    ArrangerObject::copy_members_from (other);
    force_sort ();
  }

  DECLARE_DEFINE_FIELDS_METHOD ();

public:
  /**
   * The automation points this region contains.
   *
   * Could also be used in audio regions for volume
   * automation.
   *
   * Must always stay sorted by position.
   */
  std::vector<std::shared_ptr<AutomationPoint>> aps_;

  /** Last recorded automation point. */
  AutomationPoint * last_recorded_ap_ = nullptr;
};

inline bool
operator== (const AutomationRegion &lhs, const AutomationRegion &rhs)
{
  return static_cast<const Region &> (lhs) == static_cast<const Region &> (rhs)
         && static_cast<const TimelineObject &> (lhs)
              == static_cast<const TimelineObject &> (rhs)
         && static_cast<const NameableObject &> (lhs)
              == static_cast<const NameableObject &> (rhs)
         && static_cast<const LoopableObject &> (lhs)
              == static_cast<const LoopableObject &> (rhs)
         && static_cast<const ColoredObject &> (lhs)
              == static_cast<const ColoredObject &> (rhs)
         && static_cast<const MuteableObject &> (lhs)
              == static_cast<const MuteableObject &> (rhs)
         && static_cast<const LengthableObject &> (lhs)
              == static_cast<const LengthableObject &> (rhs)
         && static_cast<const ArrangerObject &> (lhs)
              == static_cast<const ArrangerObject &> (rhs);
}

/**
 * @}
 */

#endif // __AUDIO_AUTOMATION_REGION_H__
