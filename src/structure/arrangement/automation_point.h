// SPDX-FileCopyrightText: © 2018-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/curve.h"
#include "dsp/position.h"
#include "gui/dsp/control_port.h"
#include "structure/arrangement/arranger_object.h"
#include "structure/arrangement/bounded_object.h"
#include "structure/arrangement/region_owned_object.h"
#include "utils/icloneable.h"
#include "utils/math.h"
#include "utils/types.h"

class Port;

namespace zrythm::structure::tracks
{
class AutomationTrack;
}

namespace zrythm::structure::arrangement
{
class AutomationRegion;

/**
 * An automation point inside an AutomationTrack.
 */
class AutomationPoint final : public QObject, public RegionOwnedObject
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_ARRANGER_OBJECT_QML_PROPERTIES (AutomationPoint)
  Q_PROPERTY (double value READ getValue WRITE setValue NOTIFY valueChanged)
public:
  using RegionT = AutomationRegion;

  DECLARE_FINAL_ARRANGER_OBJECT_CONSTRUCTORS (AutomationPoint)
  Q_DISABLE_COPY_MOVE (AutomationPoint)
  ~AutomationPoint () override;

  // ========================================================================
  // QML Interface
  // ========================================================================

  double getValue () const { return fvalue_; }

  void setValue (double dval)
  {
    const auto val = static_cast<float> (dval);
    if (!utils::math::floats_equal (fvalue_, val))
      {
        set_fvalue (val, false);
        Q_EMIT valueChanged (dval);
      }
  }
  Q_SIGNAL void valueChanged (double);

  // ========================================================================

  /**
   * Sets the value from given real or normalized value and notifies interested
   * parties.
   *
   * @param is_normalized Whether the given value is normalized.
   */
  void set_fvalue (float real_val, bool is_normalized);

  /** String getter for the value. */
  std::string get_fvalue_as_string () const;

  /** String setter. */
  void set_fvalue_with_action (const std::string &fval_str);

  /**
   * The function to return a point on the curve.
   *
   * See
   * https://stackoverflow.com/questions/17623152/how-map-tween-a-number-based-on-a-dynamic-curve
   *
   * @param region region The automation region (if known), otherwise the
   * non-cached region will be used.
   * @param x Normalized x.
   */
  [[gnu::hot]] double
  get_normalized_value_in_curve (AutomationRegion * region, double x) const;

  /**
   * Sets the curviness of the AutomationPoint.
   */
  void set_curviness (curviness_t curviness);

  /**
   * Convenience function to return the control port that this AutomationPoint
   * is for.
   */
  ControlPort * get_port () const;

  /**
   * Convenience function to return the AutomationTrack that this
   * AutomationPoint is in.
   */
  tracks::AutomationTrack * get_automation_track () const;

  /**
   * Returns if the curve of the AutomationPoint curves upwards as you move
   * right on the x axis.
   */
  bool curves_up () const;

  friend void init_from (
    AutomationPoint       &obj,
    const AutomationPoint &other,
    utils::ObjectCloneType clone_type);

  ArrangerObjectPtrVariant
  add_clone_to_project (bool fire_events) const override;

  ArrangerObjectPtrVariant insert_clone_to_project () const override;

  bool
  validate (bool is_project, dsp::FramesPerTick frames_per_tick) const override;

  friend bool operator< (const AutomationPoint &a, const AutomationPoint &b)
  {
#if 0
  if (a.pos_ == b.pos_) [[unlikely]]
    {
      return a.index_ < b.index_;
    }
#endif

    return *a.pos_ < *b.pos_;
  }

  friend bool operator== (const AutomationPoint &a, const AutomationPoint &b)
  {
    /* note: we don't care about the index, only the position and the value */
    /* note2: previously, this code was comparing position ticks, now it only
     * compares frames. TODO: if no problems are caused delete this note */
    return *a.pos_ == *b.pos_
           && utils::math::floats_near (a.fvalue_, b.fvalue_, 0.001f);
  }

private:
  static constexpr std::string_view kValueKey = "value";
  static constexpr std::string_view kNormalizedValueKey = "normalized_value";
  static constexpr std::string_view kCurveOptionsKey = "curve_options";
  friend void to_json (nlohmann::json &j, const AutomationPoint &point)
  {
    to_json (j, static_cast<const ArrangerObject &> (point));
    to_json (j, static_cast<const RegionOwnedObject &> (point));
    j[kValueKey] = point.fvalue_;
    j[kNormalizedValueKey] = point.normalized_val_;
    j[kCurveOptionsKey] = point.curve_opts_;
  }
  friend void from_json (const nlohmann::json &j, AutomationPoint &point)
  {
    from_json (j, static_cast<ArrangerObject &> (point));
    from_json (j, static_cast<RegionOwnedObject &> (point));
    j.at (kValueKey).get_to (point.fvalue_);
    j.at (kNormalizedValueKey).get_to (point.normalized_val_);
    j.at (kCurveOptionsKey).get_to (point.curve_opts_);
  }

public:
  /** Float value (real). */
  float fvalue_ = 0.f;

  /** Normalized value (0 to 1) used as a cache. */
  float normalized_val_ = 0.f;

  dsp::CurveOptions curve_opts_{};

  BOOST_DESCRIBE_CLASS (
    AutomationPoint,
    (ArrangerObject, RegionOwnedObject),
    (fvalue_, normalized_val_, curve_opts_),
    (),
    ())
};

} // namespace zrythm::structure::arrangement
