// SPDX-FileCopyrightText: © 2019-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_MARKER_H__
#define __AUDIO_MARKER_H__

/**
 * @addtogroup dsp
 *
 * @{
 */

#include "common/dsp/nameable_object.h"
#include "common/dsp/timeline_object.h"
#include "common/utils/icloneable.h"

constexpr int MARKER_WIDGET_TRIANGLE_W = 10;

/**
 * Marker for the MarkerTrack.
 */
class Marker final
    : public TimelineObject,
      public NameableObject,
      public ICloneable<Marker>,
      public ISerializable<Marker>
{
public:
  /**
   * Marker type.
   */
  enum class Type
  {
    /** Song start Marker that cannot be deleted. */
    Start,
    /** Song end Marker that cannot be deleted. */
    End,
    /** Custom Marker. */
    Custom,
  };

  // Rule of 0
  Marker () = default;

  Marker (const std::string &name)
      : ArrangerObject (ArrangerObject::Type::Marker), NameableObject (name)
  {
  }

  bool is_start () const { return marker_type_ == Type::Start; }
  bool is_end () const { return marker_type_ == Type::End; }

  bool validate_name (const std::string &name) override
  {
    /* valid if no other marker with the same name exists*/
    return find_by_name (name) == nullptr;
  }

  void set_marker_track_index (int index) { marker_track_index_ = index; }

  static Marker * find_by_name (const std::string &name);

  bool is_deletable () const override
  {
    return marker_type_ != Type::Start && marker_type_ != Type::End;
  }

  void init_loaded () override;

  void init_after_cloning (const Marker &other) override;

  std::string print_to_str () const override;

  ArrangerObjectPtr find_in_project () const override;

  ArrangerObjectPtr add_clone_to_project (bool fire_events) const override;

  ArrangerObjectPtr insert_clone_to_project () const override;

  bool validate (bool is_project, double frames_per_tick) const override;

  DECLARE_DEFINE_FIELDS_METHOD ();

public:
  /** Marker type. */
  Type marker_type_ = Type::Custom;

  /** Index in the marker track. */
  int marker_track_index_ = -1;
};

inline bool
operator== (const Marker &lhs, const Marker &rhs)
{
  return static_cast<const TimelineObject &> (lhs)
           == static_cast<const TimelineObject &> (rhs)
         && static_cast<const NameableObject &> (lhs)
              == static_cast<const NameableObject &> (rhs)
         && static_cast<const ArrangerObject &> (lhs)
              == static_cast<const ArrangerObject &> (rhs)
         && lhs.marker_type_ == rhs.marker_type_
         && lhs.marker_track_index_ == rhs.marker_track_index_;
}

DEFINE_OBJECT_FORMATTER (Marker, [] (const Marker &m) {
  return fmt::format ("Marker[marker_track_index_: {}]", m.marker_track_index_);
})

/**
 * @}
 */

#endif
