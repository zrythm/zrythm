// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/arranger_object_list_model.h"
#include "utils/math.h"

namespace zrythm::structure::arrangement
{

ArrangerObjectListModel::ArrangerObjectListModel (
  ArrangerObjectRefMultiIndexContainer &objects,
  QObject *                             parent)
    : QAbstractListModel (parent), objects_ (objects)
{
  setup_signals (false);
}

ArrangerObjectListModel::ArrangerObjectListModel (
  ArrangerObjectRefMultiIndexContainer &objects,
  ArrangerObject                       &parent_arranger_object)
    : QAbstractListModel (&parent_arranger_object), objects_ (objects)
{
  setup_signals (true);
}
void
ArrangerObjectListModel::setup_signals (bool is_parent_arranger_object)
{
  // Connect signals for existing objects
  for (size_t i = 0; i < objects_.size (); ++i)
    {
      connect_object_signals (static_cast<int> (i));
    }
  QObject::connect (
    this, &ArrangerObjectListModel::rowsInserted, this,
    [this, is_parent_arranger_object] (const QModelIndex &, int first, int last) {
      for (int i = first; i <= last; ++i)
        {
          if (is_parent_arranger_object)
            {
              auto * obj_base =
                objects_.get<random_access_index> ().at (i).get_object_base ();
              obj_base->setParentObject (
                qobject_cast<ArrangerObject *> (parent ()));
            }

          connect_object_signals (i);
        }
    });

  QObject::connect (
    this, &ArrangerObjectListModel::rowsAboutToBeRemoved, this,
    [this, is_parent_arranger_object] (const QModelIndex &, int first, int last) {
      for (int i = first; i <= last; ++i)
        {
          if (is_parent_arranger_object)
            {
              auto * obj_base =
                objects_.get<random_access_index> ().at (i).get_object_base ();
              obj_base->setParentObject (nullptr);
            }

          disconnect_object_signals (i);
        }
    });
}

void
ArrangerObjectListModel::connect_object_signals (int index)
{
  const auto * obj_base =
    objects_.get<random_access_index> ().at (index).get_object_base ();

  // Emit once for the fact that we've added this object
  Q_EMIT handle_object_change (obj_base);

  // Store initial position
  {
    const auto obj_tick_range = get_object_tick_range (obj_base);
    previous_object_ranges_[obj_base->get_uuid ()] = std::make_pair (
      units::ticks (obj_tick_range.first), units::ticks (obj_tick_range.second));
  }

  // Emit on property changes
  QObject::connect (
    obj_base, &ArrangerObject::propertiesChanged, this,
    [this, obj_base] () { handle_object_change (obj_base); });

  // For objects that contain other objects, also emit on children
  // content changes
  const auto obj_var =
    objects_.get<random_access_index> ().at (index).get_object ();
  std::visit (
    [&] (auto &&obj) {
      using ObjectT = base_type<decltype (obj)>;
      if constexpr (is_derived_from_template_v<ArrangerObjectOwner, ObjectT>)
        {
          // Connect to children's contentChanged
          QObject::connect (
            obj->get_model (), &ArrangerObjectListModel::contentChanged, this,
            [this, obj_base] (utils::ExpandableTickRange) {
              // Emit contentChanged on parent object (e.e., MidiRegion)
              handle_object_change (obj_base);
            });
        }
    },
    obj_var);
}

void
ArrangerObjectListModel::disconnect_object_signals (int index)
{
  const auto * obj_base =
    objects_.get<random_access_index> ().at (index).get_object_base ();

  // Emit content changed for the object being removed
  handle_object_change (obj_base);

  // Remove from previous ranges cache
  previous_object_ranges_.erase (obj_base->get_uuid ());

  // Disconnect from property changes
  QObject::disconnect (
    obj_base, &ArrangerObject::propertiesChanged, this, nullptr);

  // Disconnect from children's contentChanged for ArrangerObjectOwner types
  const auto obj_var =
    objects_.get<random_access_index> ().at (index).get_object ();
  std::visit (
    [&] (auto &&obj) {
      using ObjectT = base_type<decltype (obj)>;
      if constexpr (is_derived_from_template_v<ArrangerObjectOwner, ObjectT>)
        {
          QObject::disconnect (
            obj->get_model (), &ArrangerObjectListModel::contentChanged, this,
            nullptr);
        }
    },
    obj_var);
}

QHash<int, QByteArray>
ArrangerObjectListModel::roleNames () const
{
  QHash<int, QByteArray> roles;
  roles[ArrangerObjectPtrRole] = "arrangerObject";
  roles[ArrangerObjectUuidReferenceRole] = "arrangerObjectReference";
  return roles;
}

QVariant
ArrangerObjectListModel::data (const QModelIndex &index, int role) const
{
  const auto index_int = index.row ();
  if (!index.isValid () || index_int >= static_cast<int> (objects_.size ()))
    return {};

  if (role == ArrangerObjectPtrRole)
    {
      return QVariant::fromStdVariant (
        objects_.get<random_access_index> ()[static_cast<size_t> (index_int)]
          .get_object ());
    }
  if (role == ArrangerObjectUuidReferenceRole)
    {
      return QVariant::fromValue (
        const_cast<ArrangerObjectUuidReference *> (
          &objects_.get<random_access_index> ()[static_cast<size_t> (index_int)]));
    }

  return {};
}

void
ArrangerObjectListModel::clear ()
{
  if (objects_.empty ())
    return;

  beginRemoveRows ({}, 0, static_cast<int> (objects_.size ()) - 1);
  objects_.clear ();
  endRemoveRows ();
}

bool
ArrangerObjectListModel::insertObject (
  const ArrangerObjectUuidReference &object,
  int                                index)
{
  if (index < 0 || index > static_cast<int> (objects_.size ()))
    return false;

  beginInsertRows (QModelIndex (), index, index);
  objects_.get<random_access_index> ().insert (
    std::next (objects_.get<random_access_index> ().begin (), index), object);
  endInsertRows ();
  return true;
}

bool
ArrangerObjectListModel::removeRows (int row, int count, const QModelIndex &parent)
{
  if (parent.isValid ())
    return false;
  if (row < 0 || row + count > static_cast<int> (objects_.size ()))
    return false;

  beginRemoveRows ({}, row, row + count - 1);
  auto &container = objects_.get<random_access_index> ();
  auto  first = std::next (container.begin (), row);
  auto  last = std::next (first, count);
  container.erase (first, last);
  endRemoveRows ();
  return true;
}

void
ArrangerObjectListModel::handle_object_change (const ArrangerObject * object)
{
  // Update the boost container
  objects_.modify (
    objects_.get<uuid_hash_index> ().find (object->get_uuid ()),
    [] (ArrangerObjectUuidReference &) { });

  // Get current and previous ranges
  auto current_range = get_object_tick_range (object);
  auto previous_range = get_and_update_previous_range (object);

  // Convert to double for comparison
  double prev_start = previous_range.first.in (units::ticks);
  double prev_end = previous_range.second.in (units::ticks);
  double curr_start = current_range.first;
  double curr_end = current_range.second;

  // Check if position or size actually changed
  bool position_changed = !utils::math::floats_equal (prev_start, curr_start);
  bool size_changed = !utils::math::floats_equal (prev_end, curr_end);

  if (position_changed || size_changed)
    {
      // Create expanded range covering both positions
      utils::ExpandableTickRange combined_range (
        std::make_pair (prev_start, prev_end));
      combined_range.expand (std::make_pair (curr_start, curr_end));

      // Update stored previous range for next change
      previous_object_ranges_[object->get_uuid ()] =
        std::make_pair (units::ticks (curr_start), units::ticks (curr_end));

      Q_EMIT contentChanged (combined_range);
    }
  else
    {
      // Just use current range for other changes
      Q_EMIT contentChanged ({ current_range });
    }
}

std::pair<units::precise_tick_t, units::precise_tick_t>
ArrangerObjectListModel::get_and_update_previous_range (
  const ArrangerObject * object)
{
  auto it = previous_object_ranges_.find (object->get_uuid ());
  if (it != previous_object_ranges_.end ())
    {
      return it->second;
    }

  // If not found, use current range as previous
  auto current_range = get_object_tick_range (object);
  auto stored_range = std::make_pair (
    units::ticks (current_range.first), units::ticks (current_range.second));
  previous_object_ranges_[object->get_uuid ()] = stored_range;
  return stored_range;
}
}
