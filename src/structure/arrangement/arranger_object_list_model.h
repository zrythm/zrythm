// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/arranger_object.h"

namespace zrythm::structure::arrangement
{

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
    std::vector<ArrangerObjectUuidReference> &objects,
    QObject *                                 parent = nullptr)
      : QAbstractListModel (parent), objects_ (objects)
  {
  }

  QHash<int, QByteArray> roleNames () const override
  {
    QHash<int, QByteArray> roles;
    roles[ArrangerObjectPtrRole] = "arrangerObject";
    roles[ArrangerObjectUuidReferenceRole] = "arrangerObjectReference";
    return roles;
  }

  int rowCount (const QModelIndex &parent = QModelIndex ()) const override
  {
    if (parent.isValid ())
      return 0;
    return static_cast<int> (objects_.size ());
  }

  QVariant
  data (const QModelIndex &index, int role = Qt::DisplayRole) const override
  {
    if (!index.isValid () || index.row () >= static_cast<int> (objects_.size ()))
      return {};

    if (role == ArrangerObjectPtrRole)
      {
        return QVariant::fromStdVariant (
          objects_[static_cast<size_t> (index.row ())].get_object ());
      }
    if (role == ArrangerObjectUuidReferenceRole)
      {
        return QVariant::fromValue (
          &objects_[static_cast<size_t> (index.row ())]);
      }

    return {};
  }

  void clear ()
  {
    beginResetModel ();
    objects_.clear ();
    endResetModel ();
  }

  bool insertObject (const ArrangerObjectUuidReference &object, int index)
  {
    if (index < 0 || index > static_cast<int> (objects_.size ()))
      return false;

    beginInsertRows (QModelIndex (), index, index);
    objects_.insert (objects_.begin () + index, object);
    endInsertRows ();
    return true;
  }

  bool
  removeRows (int row, int count, const QModelIndex &parent = QModelIndex ())
    override
  {
    if (parent.isValid ())
      return false;
    if (row < 0 || row + count > static_cast<int> (objects_.size ()))
      return false;

    beginRemoveRows (parent, row, row + count - 1);
    objects_.erase (objects_.begin () + row, objects_.begin () + row + count);
    endRemoveRows ();
    return true;
  }

private:
  std::vector<ArrangerObjectUuidReference> &objects_;
};
}
