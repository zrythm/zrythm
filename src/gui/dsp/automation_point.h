// SPDX-FileCopyrightText: © 2018-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_AUTOMATION_POINT_H__
#define __AUDIO_AUTOMATION_POINT_H__

#include "dsp/curve.h"
#include "dsp/position.h"
#include "gui/dsp/arranger_object.h"
#include "gui/dsp/bounded_object.h"
#include "gui/dsp/control_port.h"
#include "gui/dsp/region_owned_object.h"
#include "utils/icloneable.h"
#include "utils/math.h"
#include "utils/types.h"

class Port;
class AutomationRegion;
class AutomationTrack;

using namespace zrythm;

/**
 * @addtogroup dsp
 *
 * @{
 */

/**
 * An automation point inside an AutomationTrack.
 */
class AutomationPoint final
    : public QObject,
      public RegionOwnedObject,
      public ICloneable<AutomationPoint>,
      public zrythm::utils::serialization::ISerializable<AutomationPoint>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_ARRANGER_OBJECT_QML_PROPERTIES (AutomationPoint)
  Q_PROPERTY (double value READ getValue WRITE setValue NOTIFY valueChanged)
public:
  using RegionT = AutomationRegion;

  AutomationPoint (const DeserializationDependencyHolder &dh)
      : AutomationPoint (
          dh.get<std::reference_wrapper<ArrangerObjectRegistry>> ().get ())
  {
  }
  AutomationPoint (
    ArrangerObjectRegistry &obj_registry,
    QObject *               parent = nullptr);
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
  AutomationTrack * get_automation_track () const;

  /**
   * Returns if the curve of the AutomationPoint curves upwards as you move
   * right on the x axis.
   */
  bool curves_up () const;

  std::optional<ArrangerObjectPtrVariant> find_in_project () const override;

  void
  init_after_cloning (const AutomationPoint &other, ObjectCloneType clone_type)
    override;

  void init_loaded () override;

  ArrangerObjectPtrVariant
  add_clone_to_project (bool fire_events) const override;

  ArrangerObjectPtrVariant insert_clone_to_project () const override;

  bool validate (bool is_project, double frames_per_tick) const override;

  DECLARE_DEFINE_FIELDS_METHOD ();

public:
  /** Float value (real). */
  float fvalue_ = 0.f;

  /** Normalized value (0 to 1) used as a cache. */
  float normalized_val_ = 0.f;

  dsp::CurveOptions curve_opts_{};
};

inline bool
operator< (const AutomationPoint &a, const AutomationPoint &b)
{
#if 0
  if (a.pos_ == b.pos_) [[unlikely]]
    {
      return a.index_ < b.index_;
    }
#endif

  return a.pos_ < b.pos_;
}

inline bool
operator== (const AutomationPoint &a, const AutomationPoint &b)
{
  /* note: we don't care about the index, only the position and the value */
  /* note2: previously, this code was comparing position ticks, now it only
   * compares frames. TODO: if no problems are caused delete this note */
  return a.pos_ == b.pos_
         && utils::math::floats_near (a.fvalue_, b.fvalue_, 0.001f);
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
  QT_TR_NOOP ("On"),
  QT_TR_NOOP ("Rec"),
  QT_TR_NOOP ("Off"));

DEFINE_OBJECT_FORMATTER (
  AutomationPoint,
  AutomationPoint,
  [] (const AutomationPoint &ap) {
    return fmt::format (
      "AutomationPoint [{}]: val {}, normalized val {}", *ap.pos_, ap.fvalue_,
      ap.normalized_val_);
  });

/**
 * @}
 */

#endif // __AUDIO_AUTOMATION_POINT_H__
