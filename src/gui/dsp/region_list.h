#pragma once

#include "gui/dsp/region.h"

template <typename RegionT> class RegionOwner;

/**
 * @note This needs to be a separate class so that it can be a property of
 * RegionOwner derived classes.
 */
class RegionList final
    : public QAbstractListModel,
      public ICloneable<RegionList>,
      public zrythm::utils::serialization::ISerializable<RegionList>
{
  Q_OBJECT
  QML_ELEMENT

  friend class RegionOwner<MidiRegion>;
  friend class RegionOwner<ChordRegion>;
  friend class RegionOwner<AutomationRegion>;
  friend class RegionOwner<AudioRegion>;

public:
  RegionList (QObject * parent = nullptr);

  QHash<int, QByteArray> roleNames () const override;
  int rowCount (const QModelIndex &parent = QModelIndex ()) const override;
  QVariant
  data (const QModelIndex &index, int role = Qt::DisplayRole) const override;

  void init_after_cloning (const RegionList &other, ObjectCloneType clone_type)
    override;

  auto get_region_vars ()
  {
    return regions_
           | std::views::transform (&ArrangerObjectUuidReference::get_object);
  }
  auto get_region_vars () const
  {
    return regions_
           | std::views::transform (&ArrangerObjectUuidReference::get_object);
  }

  void clear ();

  DECLARE_DEFINE_FIELDS_METHOD ();

public:
  /**
   * @brief Regions in this region owner.
   *
   * @note must always be sorted by position.
   */
  std::vector<ArrangerObjectUuidReference> regions_;
};
