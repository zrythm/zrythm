// SPDX-FileCopyrightText: © 2018-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_SCALE_OBJECT_H__
#define __AUDIO_SCALE_OBJECT_H__

#include "dsp/muteable_object.h"
#include "dsp/scale.h"
#include "dsp/timeline_object.h"
#include "io/serialization/iserializable.h"
#include "utils/icloneable.h"
#include "utils/pango.h"

/**
 * @addtogroup dsp
 *
 * @{
 */

constexpr int SCALE_OBJECT_MAGIC = 13187994;
#define IS_SCALE_OBJECT(tr) (tr && tr->magic_ == SCALE_OBJECT_MAGIC)

class ScaleObject final
    : public TimelineObject,
      public MuteableObject,
      public ICloneable<ScaleObject>,
      public ISerializable<ScaleObject>
{
public:
  ScaleObject () = default;

  ScaleObject (const MusicalScale &descr);

  void init_after_cloning (const ScaleObject &other) override;

  void init_loaded () override;

  void set_index_in_chord_track (int index);

  std::string gen_human_friendly_name () const override;

  std::string print_to_str () const override;

  ArrangerObjectPtr find_in_project () const override;

  ArrangerObjectPtr add_clone_to_project (bool fire_events) const override;

  ArrangerObjectPtr insert_clone_to_project () const override;

  bool validate (bool is_project, double frames_per_tick) const override;

  DECLARE_DEFINE_FIELDS_METHOD ();

  friend bool operator== (const ScaleObject &a, const ScaleObject &b);

public:
  /** The scale descriptor. */
  MusicalScale scale_;

  /** The index of the scale in the chord tack. */
  int index_in_chord_track_ = -1;

  int magic_ = SCALE_OBJECT_MAGIC;

  /** Cache layout for drawing the name. */
  PangoLayoutUniquePtr layout_;
};

inline bool
operator== (const ScaleObject &a, const ScaleObject &b)
{
  return static_cast<const TimelineObject &> (a)
           == static_cast<const TimelineObject &> (b)
         && static_cast<const MuteableObject &> (a)
              == static_cast<const MuteableObject &> (b)
         && static_cast<const ArrangerObject &> (a)
              == static_cast<const ArrangerObject &> (b)
         && a.scale_ == b.scale_
         && a.index_in_chord_track_ == b.index_in_chord_track_;
}

/**
 * @}
 */

#endif
