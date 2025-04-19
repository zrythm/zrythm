// SPDX-FileCopyrightText: © 2019-2021, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_CHORD_REGION_H__
#define __AUDIO_CHORD_REGION_H__

#include "gui/dsp/arranger_object_owner.h"
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
      public ArrangerObjectOwner<ChordObject>,
      public ICloneable<ChordRegion>,
      public zrythm::utils::serialization::ISerializable<ChordRegion>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_ARRANGER_OBJECT_QML_PROPERTIES (ChordRegion)
  DEFINE_REGION_QML_PROPERTIES (ChordRegion)
  DEFINE_ARRANGER_OBJECT_OWNER_QML_PROPERTIES (
    ChordRegion,
    chordObjects,
    ChordObject)

  friend class RegionImpl<ChordRegion>;

public:
  ChordRegion (const DeserializationDependencyHolder &dh)
      : ChordRegion (
          dh.get<std::reference_wrapper<ArrangerObjectRegistry>> ().get ())
  {
  }
  ChordRegion (ArrangerObjectRegistry &obj_registry, QObject * parent = nullptr);

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

  Location get_location (const ChordObject &) const override
  {
    return { .track_id_ = track_id_, .owner_ = get_uuid () };
  }

  std::string
  get_field_name_for_serialization (const ChordObject *) const override
  {
    return "chordObjects";
  }

  void init_after_cloning (const ChordRegion &other, ObjectCloneType clone_type)
    override;

  DECLARE_DEFINE_FIELDS_METHOD ();
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
  return fmt::format ("ChordRegion[id: {}]", cr.get_uuid ());
})

/**
 * @}
 */

#endif
