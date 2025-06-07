// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/curve.h"
#include "dsp/position.h"
#include "structure/arrangement/bounded_object.h"

#define DEFINE_FADEABLE_OBJECT_QML_PROPERTIES(ClassType)

namespace zrythm::structure::arrangement
{
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
  friend void init_from (
    FadeableObject        &obj,
    const FadeableObject  &other,
    utils::ObjectCloneType clone_type);

  bool
  are_members_valid (bool is_project, dsp::FramesPerTick frames_per_tick) const;

private:
  static constexpr auto kFadeInPosKey = "fadeInPos"sv;
  static constexpr auto kFadeOutPosKey = "fadeOutPos"sv;
  static constexpr auto kFadeInOptsKey = "fadeInOpts"sv;
  static constexpr auto kFadeOutOptsKey = "fadeOutOpts"sv;
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

  BOOST_DESCRIBE_CLASS (
    FadeableObject,
    (BoundedObject),
    (fade_in_pos_, fade_out_pos_, fade_in_opts_, fade_out_opts_),
    (),
    ())
};
} // namespace zrythm::structure::arrangement
