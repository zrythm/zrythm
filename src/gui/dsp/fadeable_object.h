// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __DSP_FADEABLE_OBJECT_H__
#define __DSP_FADEABLE_OBJECT_H__

#include "dsp/curve.h"
#include "dsp/position.h"
#include "gui/dsp/bounded_object.h"

using namespace zrythm;

#define DEFINE_FADEABLE_OBJECT_QML_PROPERTIES(ClassType)

class FadeableObject : virtual public BoundedObject
{
public:
  // = default deletes it for some reason on gcc
  FadeableObject () noexcept { };
  ~FadeableObject () noexcept override = default;
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

private:
  static constexpr std::string_view kFadeInPosKey = "fadeInPos";
  static constexpr std::string_view kFadeOutPosKey = "fadeOutPos";
  static constexpr std::string_view kFadeInOptsKey = "fadeInOpts";
  static constexpr std::string_view kFadeOutOptsKey = "fadeOutOpts";
  friend auto to_json (nlohmann::json &j, const FadeableObject &object)
  {
    j[kFadeInPosKey] = object.fade_in_pos_;
    j[kFadeOutPosKey] = object.fade_out_pos_;
    j[kFadeInOptsKey] = object.fade_in_opts_;
    j[kFadeOutOptsKey] = object.fade_out_opts_;
  }
  friend auto from_json (const nlohmann::json &j, FadeableObject &object)
  {
    j.at (kFadeInPosKey).get_to (object.fade_in_pos_);
    j.at (kFadeOutPosKey).get_to (object.fade_out_pos_);
    j.at (kFadeInOptsKey).get_to (object.fade_in_opts_);
    j.at (kFadeOutOptsKey).get_to (object.fade_out_opts_);
  }

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
