// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __DSP_MUTEABLE_OBJECT_H__
#define __DSP_MUTEABLE_OBJECT_H__

#include "gui/dsp/arranger_object.h"

class MuteableObject : virtual public ArrangerObject
{
public:
  // = default deletes it for some reason on gcc
  MuteableObject () noexcept { };
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

protected:
  void
  copy_members_from (const MuteableObject &other, ObjectCloneType clone_type);

private:
  static constexpr std::string_view kMutedKey = "muted";
  friend void to_json (nlohmann::json &j, const MuteableObject &object)
  {
    j[kMutedKey] = object.muted_;
  }
  friend void from_json (const nlohmann::json &j, MuteableObject &object)
  {
    j.at (kMutedKey).get_to (object.muted_);
  }

public:
  /** Whether muted or not. */
  bool muted_ = false;
};

#endif // __DSP_MUTEABLE_OBJECT_H__
