// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __DSP_FADEABLE_OBJECT_H__
#define __DSP_FADEABLE_OBJECT_H__

#include "dsp/curve.h"
#include "dsp/position.h"
#include "gui/dsp/bounded_object.h"

using namespace zrythm;

#define DEFINE_FADEABLE_OBJECT_QML_PROPERTIES(ClassType)

class FadeableObject
    : virtual public BoundedObject,
      public zrythm::utils::serialization::ISerializable<FadeableObject>
{
public:
  FadeableObject () = default;
  ~FadeableObject () override = default;
  Q_DISABLE_COPY_MOVE (FadeableObject)

  /**
   * Getter.
   */
  void get_fade_in_pos (Position * pos) const { *pos = fade_in_pos_; }

  /**
   * Getter.
   */
  void get_fade_out_pos (Position * pos) const { *pos = fade_out_pos_; }

  void set_fade_in_position_unvalidated (const dsp::Position &pos)
  {
    // FIXME qobject updates...
    *static_cast<dsp::Position *> (&fade_in_pos_) = pos;
  }
  void set_fade_out_position_unvalidated (const dsp::Position &pos)
  {
    // FIXME qobject updates...
    *static_cast<dsp::Position *> (&fade_out_pos_) = pos;
  }

protected:
  void
  copy_members_from (const FadeableObject &other, ObjectCloneType clone_type);

  bool
  are_members_valid (bool is_project, dsp::FramesPerTick frames_per_tick) const;

  DECLARE_DEFINE_BASE_FIELDS_METHOD ();

public:
  /**
   * Fade in position, relative to the object's start.
   *
   * Must always be before Arranger_object.fade_out_pos_.
   */
  Position fade_in_pos_;

  /**
   * Fade out position, relative to the object's start.
   *
   * Must always be after ArrangerObject.fade_in_pos_.
   */
  Position fade_out_pos_;

  /** Fade in curve options. */
  dsp::CurveOptions fade_in_opts_;

  /** Fade out curve options. */
  dsp::CurveOptions fade_out_opts_;
};

#endif // __DSP_FADEABLE_OBJECT_H__
