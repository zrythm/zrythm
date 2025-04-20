// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_AUTOMATION_REGION_H__
#define __AUDIO_AUTOMATION_REGION_H__

#include "gui/dsp/arranger_object_owner.h"
#include "gui/dsp/region.h"

/**
 * @addtogroup dsp
 *
 * @{
 */

/**
 * Represents an automation region, which contains a collection of automation
 * points. The automation points must always stay sorted by position. The last
 * recorded automation point is also stored.
 */
class AutomationRegion final
    : public QObject,
      public RegionImpl<AutomationRegion>,
      public ArrangerObjectOwner<AutomationPoint>,
      public ICloneable<AutomationRegion>,
      public zrythm::utils::serialization::ISerializable<AutomationRegion>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_ARRANGER_OBJECT_QML_PROPERTIES (AutomationRegion)
  DEFINE_REGION_QML_PROPERTIES (AutomationRegion)

  friend class RegionImpl<AutomationRegion>;

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

  void
  init_after_cloning (const AutomationRegion &other, ObjectCloneType clone_type)
    override;

  DECLARE_DEFINE_FIELDS_METHOD ();

public:
  /** Last recorded automation point. */
  AutomationPoint * last_recorded_ap_ = nullptr;

  /**
   * @brief Owner automation track.
   */
  ControlPort::Uuid automatable_port_id_;
};

inline bool
operator== (const AutomationRegion &lhs, const AutomationRegion &rhs)
{
  return static_cast<const Region &> (lhs) == static_cast<const Region &> (rhs)
         && static_cast<const TimelineObject &> (lhs)
              == static_cast<const TimelineObject &> (rhs)
         && static_cast<const NamedObject &> (lhs)
              == static_cast<const NamedObject &> (rhs)
         && static_cast<const LoopableObject &> (lhs)
              == static_cast<const LoopableObject &> (rhs)
         && static_cast<const ColoredObject &> (lhs)
              == static_cast<const ColoredObject &> (rhs)
         && static_cast<const MuteableObject &> (lhs)
              == static_cast<const MuteableObject &> (rhs)
         && static_cast<const BoundedObject &> (lhs)
              == static_cast<const BoundedObject &> (rhs)
         && static_cast<const ArrangerObject &> (lhs)
              == static_cast<const ArrangerObject &> (rhs);
}

DEFINE_OBJECT_FORMATTER (
  AutomationRegion,
  AutomationRegion,
  [] (const AutomationRegion &ar) {
    return fmt::format (
      "AutomationRegion[id: {}, {}]", ar.get_uuid (),
      static_cast<const Region &> (ar));
  })

/**
 * @}
 */

#endif // __AUDIO_AUTOMATION_REGION_H__
