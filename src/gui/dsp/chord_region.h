// SPDX-FileCopyrightText: © 2019-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_CHORD_REGION_H__
#define __AUDIO_CHORD_REGION_H__

#include "gui/dsp/chord_object.h"
#include "gui/dsp/region.h"

/**
 * @addtogroup dsp
 *
 * @{
 */

class ChordRegion final
    : public QAbstractListModel,
      public RegionImpl<ChordRegion>,
      public ICloneable<ChordRegion>,
      public zrythm::utils::serialization::ISerializable<ChordRegion>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_ARRANGER_OBJECT_QML_PROPERTIES (ChordRegion)
  DEFINE_REGION_QML_PROPERTIES (ChordRegion)

  friend class RegionImpl<ChordRegion>;

public:
  ChordRegion (QObject * parent = nullptr);

  /**
   * Creates a new Region for chords.
   *
   * @param idx Index inside chord track.
   */
  ChordRegion (
    const Position &start_pos,
    const Position &end_pos,
    int             idx,
    double          ticks_per_frame,
    QObject *       parent = nullptr);

  using RegionT = RegionImpl<ChordRegion>;

  // ========================================================================
  // QML Interface
  // ========================================================================
  enum ChordRegionRoles
  {
    ChordObjectPtrRole = Qt::UserRole + 1,
  };

  QHash<int, QByteArray> roleNames () const override;
  int rowCount (const QModelIndex &parent = QModelIndex ()) const override;
  QVariant
  data (const QModelIndex &index, int role = Qt::DisplayRole) const override;

  // ========================================================================

  void init_loaded () override;

  bool validate (bool is_project, double frames_per_tick) const override;

  void append_children (
    std::vector<RegionOwnedObjectImpl<ChordRegion> *> &children) const override
  {
    for (const auto &chord : chord_objects_)
      {
        children.emplace_back (chord);
      }
  }

  void add_ticks_to_children (const double ticks) override
  {
    for (auto &chord : chord_objects_)
      {
        chord->move (ticks);
      }
  }

  void init_after_cloning (const ChordRegion &other, ObjectCloneType clone_type)
    override;

  DECLARE_DEFINE_FIELDS_METHOD ();

public:
  /** ChordObject's in this Region. */
  std::vector<ChordObject *> chord_objects_;
};

inline bool
operator== (const ChordRegion &lhs, const ChordRegion &rhs)
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

DEFINE_OBJECT_FORMATTER (ChordRegion, ChordRegion, [] (const ChordRegion &cr) {
  return fmt::format ("ChordRegion[id: {}]", cr.id_);
})

/**
 * @}
 */

#endif
