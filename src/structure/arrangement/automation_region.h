// SPDX-FileCopyrightText: © 2019-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/arranger_object_owner.h"
#include "structure/arrangement/automation_point.h"

namespace zrythm::structure::arrangement
{
/**
 * Represents an automation region, which contains a collection of automation
 * points. The automation points must always stay sorted by position. The last
 * recorded automation point is also stored.
 */
class AutomationRegion final
    : public ArrangerObject,
      public ArrangerObjectOwner<AutomationPoint>
{
  Q_OBJECT
  DEFINE_ARRANGER_OBJECT_OWNER_QML_PROPERTIES (
    AutomationRegion,
    automationPoints,
    AutomationPoint)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  AutomationRegion (
    const dsp::TempoMap          &tempo_map,
    ArrangerObjectRegistry       &object_registry,
    dsp::FileAudioSourceRegistry &file_audio_source_registry,
    QObject *                     parent = nullptr);

  // ========================================================================
  // QML Interface
  // ========================================================================

  // ========================================================================

  /**
   * Returns the AutomationPoint before the given one.
   */
  AutomationPoint * get_prev_ap (const AutomationPoint &ap) const;

  /**
   * Returns the AutomationPoint after the given one.
   *
   * @param check_positions Compare positions instead of just getting the next
   * index.
   */
  [[gnu::hot]] AutomationPoint *
  get_next_ap (const AutomationPoint &ap, bool check_positions) const;

  /**
   * The function to return a point on the curve.
   *
   * See
   * https://stackoverflow.com/questions/17623152/how-map-tween-a-number-based-on-a-dynamic-curve
   *
   * @param x Normalized x.
   */
  [[gnu::hot]] double
  get_normalized_value_in_curve (const AutomationPoint &ap, double x) const;

  /**
   * Returns if the curve of the AutomationPoint curves upwards as you move
   * right on the x axis.
   */
  bool curves_up (const AutomationPoint &ap) const;

  std::string
  get_field_name_for_serialization (const AutomationPoint *) const override
  {
    return "automationPoints";
  }

private:
  friend void init_from (
    AutomationRegion       &obj,
    const AutomationRegion &other,
    utils::ObjectCloneType  clone_type);

  friend void to_json (nlohmann::json &j, const AutomationRegion &region)
  {
    to_json (j, static_cast<const ArrangerObject &> (region));
    to_json (j, static_cast<const ArrangerObjectOwner &> (region));
  }
  friend void from_json (const nlohmann::json &j, AutomationRegion &region)
  {
    from_json (j, static_cast<ArrangerObject &> (region));
    from_json (j, static_cast<ArrangerObjectOwner &> (region));
  }

private:
  BOOST_DESCRIBE_CLASS (
    AutomationRegion,
    (ArrangerObject, ArrangerObjectOwner<AutomationPoint>),
    (),
    (),
    ())
};
}
