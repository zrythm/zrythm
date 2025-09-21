// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/arranger_object.h"
#include "utils/expandable_tick_range.h"

#include <boost/multi_index/global_fun.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/ranked_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>

namespace zrythm::structure::arrangement
{

// Vector-like index for fast iteration
struct sequenced_index
{
};

// Random-access index for fast lookups by index
struct random_access_index
{
};

// HashTable-like index for fast lookups by ID
struct uuid_hash_index
{
};

// Sorted by position
struct sorted_index
{
};

inline double
get_ticks_from_arranger_object_uuid_ref (const ArrangerObjectUuidReference &ref)
{
  return ref.get_object_base ()->position ()->ticks ();
}

// Multi-index container for both quick iteration and quick lookups
using ArrangerObjectRefMultiIndexContainer = boost::multi_index_container<
  ArrangerObjectUuidReference,
  boost::multi_index::indexed_by<
    boost::multi_index::hashed_unique<
      boost::multi_index::tag<uuid_hash_index>,
      boost::multi_index::const_mem_fun<
        ArrangerObjectUuidReference,
        ArrangerObject::Uuid,
        &ArrangerObjectUuidReference::id>>,
    boost::multi_index::random_access<boost::multi_index::tag<random_access_index>>,
    boost::multi_index::ranked_non_unique<
      boost::multi_index::tag<sorted_index>,
      boost::multi_index::global_fun<
        const ArrangerObjectUuidReference &,
        double,
        &get_ticks_from_arranger_object_uuid_ref>>,
    boost::multi_index::sequenced<boost::multi_index::tag<sequenced_index>>>>;

/**
 * @brief A QML wrapper over a list of arranger objects.
 *
 * All operations on the list should be performed via this class so we can take
 * advantage of signal emissions.
 */
class ArrangerObjectListModel : public QAbstractListModel
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  enum ArrangerObjectListModelRoles
  {
    ArrangerObjectPtrRole = Qt::UserRole + 1,
    ArrangerObjectUuidReferenceRole
  };
  Q_ENUM (ArrangerObjectListModelRoles)

  ArrangerObjectListModel (
    ArrangerObjectRefMultiIndexContainer &objects,
    QObject *                             parent = nullptr);

  /**
   * @brief To be used when the parent is also an arranger object.
   *
   * This will set up additional connections to call setParentObject() at
   * appropriate times.
   */
  ArrangerObjectListModel (
    ArrangerObjectRefMultiIndexContainer &objects,
    ArrangerObject                       &parent_arranger_object);

  QHash<int, QByteArray> roleNames () const override;

  int rowCount (const QModelIndex &parent = QModelIndex ()) const override
  {
    if (parent.isValid ())
      return 0;
    return static_cast<int> (objects_.size ());
  }

  QVariant
  data (const QModelIndex &index, int role = Qt::DisplayRole) const override;

  void clear ();

  bool insertObject (const ArrangerObjectUuidReference &object, int index);

  bool
  removeRows (int row, int count, const QModelIndex &parent = QModelIndex ())
    override;

  Q_SIGNAL void contentChanged (utils::ExpandableTickRange affectedRange);
  Q_SIGNAL void contentChangedForObject (const ArrangerObject * object);

private:
  void connect_object_signals (int index);
  void disconnect_object_signals (int index);
  void setup_signals (bool is_parent_arranger_object);

private:
  ArrangerObjectRefMultiIndexContainer &objects_;
};
}
