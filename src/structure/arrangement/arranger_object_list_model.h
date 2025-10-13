// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <unordered_map>

#include "structure/arrangement/arranger_object.h"
#include "utils/expandable_tick_range.h"
#include "utils/units.h"

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

private:
  /**
   * @brief Connects signals for an object at the given index.
   *
   * Sets up connections to track property changes and stores the initial
   * position for cache invalidation purposes.
   *
   * @param index The index of the object in the container.
   */
  void connect_object_signals (int index);

  /**
   * @brief Disconnects signals for an object at the given index.
   *
   * Removes all signal connections and cleans up the previous position cache.
   *
   * @param index The index of the object in the container.
   */
  void disconnect_object_signals (int index);

  /**
   * @brief Sets up signal connections for the list model.
   *
   * Establishes connections for rowsInserted and rowsAboutToBeRemoved signals.
   *
   * @param is_parent_arranger_object Whether the parent is also an arranger
   * object.
   */
  void setup_signals (bool is_parent_arranger_object);

  /**
   * @brief Handles property changes for an arranger object.
   *
   * Compares the previous and current positions/sizes of the object and
   * emits contentChanged with an expanded range covering both positions
   * if the object moved or was resized.
   *
   * @param object The object that changed.
   */
  void handle_object_change (const ArrangerObject * object);

  /**
   * @brief Gets and updates the previous position range for an object.
   *
   * Retrieves the stored previous position range for the object. If no
   * previous position is stored, uses the current position and stores it.
   *
   * @param object The object to get the previous range for.
   * @return A pair containing the start and end ticks of the previous range.
   */
  std::pair<units::precise_tick_t, units::precise_tick_t>
  get_and_update_previous_range (const ArrangerObject * object);

private:
  ArrangerObjectRefMultiIndexContainer &objects_;

  /** Cache of previous positions for objects to handle movement cache
   * invalidation. Maps object UUID to a pair of start and end ticks. */
  std::unordered_map<
    ArrangerObject::Uuid,
    std::pair<units::precise_tick_t, units::precise_tick_t>>
    previous_object_ranges_;
};
}
