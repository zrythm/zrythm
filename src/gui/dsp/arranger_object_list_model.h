#pragma once

#include "gui/dsp/arranger_object.h"

class ArrangerObjectListModel final : public QAbstractListModel
{
  Q_OBJECT
  QML_ELEMENT

public:
  enum ArrangerObjectListModelRoles
  {
    ArrangerObjectPtrRole = Qt::UserRole + 1,
  };

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
        return QVariant::fromStdVariant (objects_[index.row ()].get_object ());
      }

    return {};
  }

  // expose some list model methods for easy access
  void begin_insert_rows (int first, int last)
  {
    beginInsertRows ({}, first, last);
  }
  void end_insert_rows () { endInsertRows (); }
  void begin_remove_rows (int first, int last)
  {
    beginRemoveRows ({}, first, last);
  }
  void end_remove_rows () { endRemoveRows (); }
  void begin_reset_model () { beginResetModel (); }
  void end_reset_model () { endResetModel (); }

private:
  std::vector<ArrangerObjectUuidReference> &objects_;
};
