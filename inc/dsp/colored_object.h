// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __DSP_COLORED_OBJECT_H__
#define __DSP_COLORED_OBJECT_H__

#include "dsp/arranger_object.h"
#include "utils/color.h"

class ColoredObject
    : virtual public ArrangerObject,
      public ISerializable<ColoredObject>
{
public:
  virtual ~ColoredObject () = default;

protected:
  void copy_members_from (const ColoredObject &other);

  void init_loaded_base ();

  DECLARE_DEFINE_BASE_FIELDS_METHOD ();

public:
  /**
   * Color independent of owner (Track/Region etc.).
   */
  Color color_;

  /**
   * Whether to use the custom color.
   *
   * If false, the owner color will be used.
   */
  bool use_color_ = false;
};

inline bool
operator== (const ColoredObject &lhs, const ColoredObject &rhs)
{
  return lhs.color_ == rhs.color_ && lhs.use_color_ == rhs.use_color_;
}

#endif // __DSP_COLORED_OBJECT_H__