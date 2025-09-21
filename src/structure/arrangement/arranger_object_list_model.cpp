// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/arranger_object_list_model.h"

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
  QObject::connect (
    this, &ArrangerObjectListModel::contentChangedForObject, this,
    [this] (const ArrangerObject * object) {
      // Update the boost container
      objects_.modify (
        objects_.get<uuid_hash_index> ().find (object->get_uuid ()),
        [] (ArrangerObjectUuidReference &) { });

      // Emit contentChanged with the object's range
      Q_EMIT contentChanged ({ get_object_tick_range (object) });
    });

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
  Q_EMIT contentChangedForObject (obj_base);

  // Emit on property changes
  QObject::connect (
    obj_base, &ArrangerObject::propertiesChanged, this,
    [this, obj_base] () { Q_EMIT contentChangedForObject (obj_base); });

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
              Q_EMIT contentChangedForObject (obj_base);
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
  Q_EMIT contentChangedForObject (obj_base);

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
}
