#pragma once

#include "common/dsp/region.h"

template <typename RegionT> class RegionOwnerImpl;

/**
 * @note This needs to be a separate class so that it can be a property of
 * RegionOwner derived classes.
 */
class RegionList final
    : public QAbstractListModel,
      public ICloneable<RegionList>,
      public ISerializable<RegionList>
{
  Q_OBJECT
  QML_ELEMENT

  friend class RegionOwnerImpl<MidiRegion>;
  friend class RegionOwnerImpl<ChordRegion>;
  friend class RegionOwnerImpl<AutomationRegion>;
  friend class RegionOwnerImpl<AudioRegion>;

public:
  RegionList (QObject * parent = nullptr);

  QHash<int, QByteArray> roleNames () const override;
  int rowCount (const QModelIndex &parent = QModelIndex ()) const override;
  QVariant
  data (const QModelIndex &index, int role = Qt::DisplayRole) const override;

  void init_after_cloning (const RegionList &other) override;

  void clear ();

  DECLARE_DEFINE_FIELDS_METHOD ();

public:
  /**
   * @brief Regions in this region owner.
   *
   * @note must always be sorted by position.
   */
  std::vector<RegionPtrVariant> regions_;
};