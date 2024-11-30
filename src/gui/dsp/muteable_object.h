// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __DSP_MUTEABLE_OBJECT_H__
#define __DSP_MUTEABLE_OBJECT_H__

#include "gui/dsp/arranger_object.h"

class MuteableObject
    : virtual public ArrangerObject,
      public zrythm::utils::serialization::ISerializable<MuteableObject>
{
public:
  MuteableObject () noexcept = default;
  ~MuteableObject () noexcept override = default;
  Q_DISABLE_COPY_MOVE (MuteableObject)

  /**
   * Gets the mute status of the object.
   *
   * @param check_parent Whether to check parent (parent region or parent track
   * lane if region), otherwise only whether this object itself is muted is
   * returned. This will take the solo status of other lanes if true and if this
   * is a region that can have lanes.
   */
  virtual bool get_muted (bool check_parent) const { return muted_; };

  /**
   * Sets the mute status of the object.
   */
  void set_muted (bool muted, bool fire_events);

  friend bool operator== (const MuteableObject &lhs, const MuteableObject &rhs);

protected:
  void copy_members_from (const MuteableObject &other);

  void init_loaded_base ();

  DECLARE_DEFINE_BASE_FIELDS_METHOD ();

public:
  /** Whether muted or not. */
  bool muted_ = false;
};

inline bool
operator== (const MuteableObject &lhs, const MuteableObject &rhs)
{
  return lhs.muted_ == rhs.muted_;
}

#endif // __DSP_MUTEABLE_OBJECT_H__
