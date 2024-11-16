// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_AUTOMATION_REGION_H__
#define __AUDIO_AUTOMATION_REGION_H__

#include "common/dsp/region.h"

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
    : public QAbstractListModel,
      public RegionImpl<AutomationRegion>,
      public ICloneable<AutomationRegion>,
      public ISerializable<AutomationRegion>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_ARRANGER_OBJECT_QML_PROPERTIES (AutomationRegion)
  DEFINE_REGION_QML_PROPERTIES (AutomationRegion)

  friend class RegionImpl<AutomationRegion>;

public:
  AutomationRegion (QObject * parent = nullptr);

  /**
   * @brief Construct a new Automation Region object.
   *
   * @param start_pos
   * @param end_pos
   * @param track_name_hash
   * @param at_idx
   * @param idx_inside_at
   */
  AutomationRegion (
    const Position &start_pos,
    const Position &end_pos,
    unsigned int    track_name_hash,
    int             at_idx,
    int             idx_inside_at,
    QObject *       parent = nullptr);

  // ========================================================================
  // QML Interface
  // ========================================================================
  enum AutomationRegionRoles
  {
    AutomationPointPtrRole = Qt::UserRole + 1,
  };

  QHash<int, QByteArray> roleNames () const override;
  int rowCount (const QModelIndex &parent = QModelIndex ()) const override;
  QVariant
  data (const QModelIndex &index, int role = Qt::DisplayRole) const override;

  // ========================================================================

  void init_loaded () override;

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

#if 0
  /**
   * Adds an AutomationPoint to the Region and returns a reference to it.
   */
  AutomationPoint &
  add_ap (std::unique_ptr<AutomationPoint> &&ap, bool pub_events);
#endif

  /**
   * Returns the AutomationPoint before the given one.
   */
  AutomationPoint * get_prev_ap (const AutomationPoint &ap) const;

  void add_ticks_to_children (const double ticks) override
  {
    for (auto &ap : aps_)
      {
        ap->move (ticks);
      }
  }

  /**
   * Returns the AutomationPoint after the given one.
   *
   * @param check_positions Compare positions instead of just getting the next
   * index.
   * @param check_transients Also check the transient of each object. This
   * only matters if @p check_positions is true.
   */
  ATTR_HOT AutomationPoint * get_next_ap (
    const AutomationPoint &ap,
    bool                   check_positions,
    bool                   check_transients) const;

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

  bool validate (bool is_project, double frames_per_tick) const override;

  /**
   * Sets the automation track.
   */
  void set_automation_track (AutomationTrack &at);

  /**
   * Gets the AutomationTrack using the saved index.
   */
  AutomationTrack * get_automation_track () const;

  void append_children (
    std::vector<RegionOwnedObjectImpl<AutomationRegion> *> &children)
    const override
  {
    for (const auto &ap : aps_)
      {
        children.push_back (ap);
      }
  }

  std::optional<ClipEditorArrangerSelectionsPtrVariant>
  get_arranger_selections () const override;

  void init_after_cloning (const AutomationRegion &other) override;

  DECLARE_DEFINE_FIELDS_METHOD ();

public:
  /**
   * The automation points this region contains.
   *
   * Could also be used in audio regions for volume automation.
   *
   * Must always stay sorted by position.
   */
  std::vector<AutomationPoint *> aps_;

  /** Last recorded automation point. */
  AutomationPoint * last_recorded_ap_ = nullptr;
};

inline bool
operator== (const AutomationRegion &lhs, const AutomationRegion &rhs)
{
  return static_cast<const Region &> (lhs) == static_cast<const Region &> (rhs)
         && static_cast<const TimelineObject &> (lhs)
              == static_cast<const TimelineObject &> (rhs)
         && static_cast<const NameableObject &> (lhs)
              == static_cast<const NameableObject &> (rhs)
         && static_cast<const LoopableObject &> (lhs)
              == static_cast<const LoopableObject &> (rhs)
         && static_cast<const ColoredObject &> (lhs)
              == static_cast<const ColoredObject &> (rhs)
         && static_cast<const MuteableObject &> (lhs)
              == static_cast<const MuteableObject &> (rhs)
         && static_cast<const LengthableObject &> (lhs)
              == static_cast<const LengthableObject &> (rhs)
         && static_cast<const ArrangerObject &> (lhs)
              == static_cast<const ArrangerObject &> (rhs);
}

DEFINE_OBJECT_FORMATTER (AutomationRegion, [] (const AutomationRegion &ar) {
  return fmt::format ("AutomationRegion[id: {}]", ar.id_);
})

/**
 * @}
 */

#endif // __AUDIO_AUTOMATION_REGION_H__
