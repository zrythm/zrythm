// SPDX-FileCopyrightText: © 2019-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Region for ChordObject's.
 */

#ifndef __AUDIO_CHORD_REGION_H__
#define __AUDIO_CHORD_REGION_H__

#include "dsp/chord_object.h"
#include "dsp/region.h"

/**
 * @addtogroup dsp
 *
 * @{
 */

class ChordRegion final
    : public RegionImpl<ChordRegion>,
      public ICloneable<ChordRegion>,
      public ISerializable<ChordRegion>
{
  // Rule of 0
public:
  ChordRegion () = default;

  /**
   * Creates a new Region for chords.
   *
   * @param idx Index inside chord track.
   */
  ChordRegion (const Position &start_pos, const Position &end_pos, int idx);

  using RegionT = RegionImpl<ChordRegion>;

  void init_loaded () override;

  bool validate (bool is_project, double frames_per_tick) const override;

  void append_children (
    std::vector<RegionOwnedObjectImpl<ChordRegion> *> &children) const override
  {
    for (auto &chord : chord_objects_)
      {
        children.emplace_back (chord.get ());
      }
  }

  void add_ticks_to_children (const double ticks) override
  {
    for (auto &chord : chord_objects_)
      {
        chord->move (ticks);
      }
  }

  void init_after_cloning (const ChordRegion &other) override
  {
    init (
      other.pos_, other.end_pos_, other.id_.track_name_hash_, 0, other.id_.idx_);
    clone_unique_ptr_container (chord_objects_, other.chord_objects_);
    RegionT::copy_members_from (other);
    TimelineObject::copy_members_from (other);
    NameableObject::copy_members_from (other);
    LoopableObject::copy_members_from (other);
    MuteableObject::copy_members_from (other);
    LengthableObject::copy_members_from (other);
    ColoredObject::copy_members_from (other);
    ArrangerObject::copy_members_from (other);
  }

  DECLARE_DEFINE_FIELDS_METHOD ();

  ArrangerSelections * get_arranger_selections () const override;
  ArrangerWidget *     get_arranger_for_children () const override;

public:
  /** ChordObject's in this Region. */
  std::vector<std::shared_ptr<ChordObject>> chord_objects_;
};

inline bool
operator== (const ChordRegion &lhs, const ChordRegion &rhs)
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

DEFINE_OBJECT_FORMATTER (ChordRegion, [] (const ChordRegion &cr) {
  return fmt::format ("ChordRegion[id: {}]", cr.id_);
})

/**
 * @}
 */

#endif
