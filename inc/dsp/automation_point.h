// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Automation point API.
 */

#ifndef __AUDIO_AUTOMATION_POINT_H__
#define __AUDIO_AUTOMATION_POINT_H__

#include "dsp/arranger_object.h"
#include "dsp/control_port.h"
#include "dsp/curve.h"
#include "dsp/position.h"
#include "dsp/region_owned_object.h"
#include "utils/icloneable.h"
#include "utils/math.h"
#include "utils/types.h"

class Port;
class AutomationRegion;
class AutomationTrack;
TYPEDEF_STRUCT_UNDERSCORED (AutomationPointWidget);

/**
 * @addtogroup dsp
 *
 * @{
 */

/**
 * Used for caching.
 */
struct AutomationPointDrawSettings
{
  GdkRectangle draw_rect = {};
  CurveOptions curve_opts;
  float        normalized_val = 0.f;
};

/**
 * An automation point inside an AutomationTrack.
 */
class AutomationPoint final
    : public RegionOwnedObjectImpl<AutomationRegion>,
      public ICloneable<AutomationPoint>,
      public ISerializable<AutomationPoint>
{
public:
  // Rule of 0
  AutomationPoint () = default;

  AutomationPoint (const Position &pos);

  ~AutomationPoint ();

  /**
   * Creates an AutomationPoint at the given Position.
   */
  AutomationPoint (
    const float     value,
    const float     normalized_val,
    const Position &pos);

  /**
   * Sets the value from given real or normalized
   * value and notifies interested parties.
   *
   * @param is_normalized Whether the given value is
   *   normalized.
   */
  void set_fvalue (float real_val, bool is_normalized, bool pub_events);

  /** String getter for the value. */
  static std::string get_fvalue_as_string (void * self);

  /** String setter. */
  static void set_fvalue_with_action (void * self, const std::string &fval_str);

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
  HOT double
  get_normalized_value_in_curve (AutomationRegion * region, double x) const;

  std::string print_to_str () const override;

  /**
   * Sets the curviness of the AutomationPoint.
   */
  void set_curviness (const curviness_t curviness);

  /**
   * Convenience function to return the control port that this AutomationPoint
   * is for.
   */
  ControlPort * get_port () const;

  /**
   * Convenience function to return the AutomationTrack that this
   * AutomationPoint is in.
   */
  AutomationTrack * get_automation_track () const;

  /**
   * Returns if the curve of the AutomationPoint curves upwards as you move
   * right on the x axis.
   */
  bool curves_up () const;

  ArrangerWidget * get_arranger () const override;

  ArrangerObjectPtr find_in_project () const override;

  void init_after_cloning (const AutomationPoint &other) override;

  ArrangerObjectPtr add_clone_to_project (bool fire_events) const override;

  ArrangerObjectPtr insert_clone_to_project () const override;

  bool validate (bool is_project, double frames_per_tick) const override;

  DECLARE_DEFINE_FIELDS_METHOD ();

public:
  /** Float value (real). */
  float fvalue_ = 0.f;

  /** Normalized value (0 to 1) used as a cache. */
  float normalized_val_ = 0.f;

  CurveOptions curve_opts_ = {};

  /** Last settings used for drawing in editor. */
  AutomationPointDrawSettings last_settings_ = {};

  /** Last settings used for drawing in timeline. */
  AutomationPointDrawSettings last_settings_tl_ = {};

  /* FIXME these 2 break the rule of 0, and we should use normal snapshot (not
   * cairo) so we can remove them. */

  /** Cached cairo node to reuse when drawing in the editor. */
  GskRenderNode * cairo_node_ = nullptr;

  /** Cached cairo node to reuse when drawing in the timeline. */
  GskRenderNode * cairo_node_tl_ = nullptr;

  /** Temporary string used with StringEntryDialogWidget. */
  std::string tmp_str_;
};

inline bool
operator< (const AutomationPoint &a, const AutomationPoint &b)
{
  if (a.pos_ == b.pos_) [[unlikely]]
    {
      return a.index_ < b.index_;
    }

  return a.pos_ < b.pos_;
}

inline bool
operator== (const AutomationPoint &a, const AutomationPoint &b)
{
  /* note: we don't care about the index, only the position and the value */
  /* note2: previously, this code was comparing position ticks, now it only
   * compares frames. TODO: if no problems are caused delete this note */
  return a.pos_ == b.pos_
         && math_floats_equal_epsilon (a.fvalue_, b.fvalue_, 0.001f);
}

/**
 * @brief FIXME: move to a more appropriate place.
 *
 */
enum class AutomationMode
{
  Read,
  Record,
  Off,
  NUM_AUTOMATION_MODES,
};

constexpr size_t NUM_AUTOMATION_MODES =
  static_cast<size_t> (AutomationMode::NUM_AUTOMATION_MODES);

DEFINE_ENUM_FORMATTER (
  AutomationMode,
  AutomationMode,
  N_ ("On"),
  N_ ("Rec"),
  N_ ("Off"));

/**
 * @}
 */

#endif // __AUDIO_AUTOMATION_POINT_H__
