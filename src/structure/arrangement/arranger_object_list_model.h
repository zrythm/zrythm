// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/arranger_object.h"
#include "utils/expandable_tick_range.h"

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
    QObject *                                 parent = nullptr);

  /**
   * @brief To be used when the parent is also an arranger object.
   *
   * This will set up additional connections to call setParentObject() at
   * appropriate times.
   */
  ArrangerObjectListModel (
    std::vector<ArrangerObjectUuidReference> &objects,
    ArrangerObject                           &parent_arranger_object);

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
  std::vector<ArrangerObjectUuidReference> &objects_;
};
}
