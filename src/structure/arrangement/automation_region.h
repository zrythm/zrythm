// SPDX-FileCopyrightText: © 2019-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/arranger_object_owner.h"
#include "structure/arrangement/region.h"

namespace zrythm::structure::arrangement
{
/**
 * Represents an automation region, which contains a collection of automation
 * points. The automation points must always stay sorted by position. The last
 * recorded automation point is also stored.
 */
class AutomationRegion final
    : public QObject,
      public RegionImpl<AutomationRegion>,
      public ArrangerObjectOwner<AutomationPoint>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_ARRANGER_OBJECT_QML_PROPERTIES (AutomationRegion)
  DEFINE_REGION_QML_PROPERTIES (AutomationRegion)

  friend class RegionImpl<AutomationRegion>;

  using AutomationTrack = structure::tracks::AutomationTrack;

public:
  DECLARE_FINAL_ARRANGER_OBJECT_CONSTRUCTORS (AutomationRegion)

  /**
   * Prints the automation in this Region.
   */
  void print_automation () const;

  static int sort_func (const void * _a, const void * _b);

  /**
   * Forces sort of the automation points.
   */
  void force_sort ();

  bool get_muted (bool check_parent) const override;

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
   * Returns the automation points since the last recorded automation point
   * (if the last recorded automation point was before the current pos).
   */
  void
  get_aps_since_last_recorded (Position pos, std::vector<AutomationPoint *> &aps);

  /**
   * Returns an automation point found within +/-
   * delta_ticks from the position, or NULL.
   *
   * @param before_only Only check previous automation
   *   points.
   */
  AutomationPoint * get_ap_around (
    Position * _pos,
    double     delta_ticks,
    bool       before_only,
    bool       use_snapshots);

  bool
  validate (bool is_project, dsp::FramesPerTick frames_per_tick) const override;

  /**
   * Sets the automation track.
   */
  void set_automation_track (AutomationTrack &at);

  /**
   * Gets the AutomationTrack using the saved index.
   */
  AutomationTrack * get_automation_track () const;

  Location get_location (const AutomationPoint &) const override
  {
    return { .track_id_ = track_id_, .owner_ = get_uuid () };
  }

  std::string
  get_field_name_for_serialization (const AutomationPoint *) const override
  {
    return "automationPoints";
  }

  friend void init_from (
    AutomationRegion       &obj,
    const AutomationRegion &other,
    utils::ObjectCloneType  clone_type);

private:
  friend void to_json (nlohmann::json &j, const AutomationRegion &ar)
  {
    to_json (j, static_cast<const ArrangerObject &> (ar));
    to_json (j, static_cast<const BoundedObject &> (ar));
    to_json (j, static_cast<const LoopableObject &> (ar));
    to_json (j, static_cast<const MuteableObject &> (ar));
    to_json (j, static_cast<const NamedObject &> (ar));
    to_json (j, static_cast<const ColoredObject &> (ar));
    to_json (j, static_cast<const Region &> (ar));
    to_json (j, static_cast<const ArrangerObjectOwner &> (ar));
  }
  friend void from_json (const nlohmann::json &j, AutomationRegion &ar)
  {
    from_json (j, static_cast<ArrangerObject &> (ar));
    from_json (j, static_cast<BoundedObject &> (ar));
    from_json (j, static_cast<LoopableObject &> (ar));
    from_json (j, static_cast<MuteableObject &> (ar));
    from_json (j, static_cast<NamedObject &> (ar));
    from_json (j, static_cast<ColoredObject &> (ar));
    from_json (j, static_cast<Region &> (ar));
    from_json (j, static_cast<ArrangerObjectOwner &> (ar));
  }

public:
  /** Last recorded automation point. */
  AutomationPoint * last_recorded_ap_ = nullptr;

  /**
   * @brief Owner automation track.
   */
  ControlPort::Uuid automatable_port_id_;

  BOOST_DESCRIBE_CLASS (
    AutomationRegion,
    (Region, ArrangerObjectOwner<AutomationPoint>),
    (automatable_port_id_),
    (),
    ())
};
}
