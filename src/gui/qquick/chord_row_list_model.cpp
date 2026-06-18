// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <ranges>

#include "gui/qquick/chord_row_list_model.h"
#include "structure/arrangement/arranger_object_list_model.h"

namespace zrythm::gui::qquick
{

ChordRowListModel::ChordRowListModel (QObject * parent)
    : QAbstractListModel (parent)
{
}

void
ChordRowListModel::setRegion (structure::arrangement::ChordRegion * region)
{
  beginResetModel ();
  disconnect_from_region_model ();
  disconnect_descriptor_signals ();
  rows_.clear ();
  region_ = region;
  if (region_ != nullptr)
    {
      rebuild ();
      connect_to_region_model ();
      connect_descriptor_signals ();
    }
  endResetModel ();
  Q_EMIT regionChanged ();
}

QHash<int, QByteArray>
ChordRowListModel::roleNames () const
{
  return {
    { ChordNameRole,        "chordName"        },
    { ChordDescriptorRole,  "chordDescriptor"  },
    { ChordObjectCountRole, "chordObjectCount" },
  };
}

int
ChordRowListModel::rowCount (const QModelIndex &parent) const
{
  if (parent.isValid ())
    return 0;
  return static_cast<int> (rows_.size ());
}

QVariant
ChordRowListModel::data (const QModelIndex &index, int role) const
{
  if (
    !index.isValid () || index.row () < 0
    || index.row () >= static_cast<int> (rows_.size ()))
    return {};
  const auto &row = rows_[index.row ()];
  switch (role)
    {
    case ChordNameRole:
      return row.descriptor->displayName ();
    case ChordDescriptorRole:
      return QVariant::fromValue (row.descriptor);
    case ChordObjectCountRole:
      return static_cast<int> (row.objects.size ());
    }
  return {};
}

int
ChordRowListModel::rowForChordObject (
  structure::arrangement::ChordObject * object) const
{
  for (int i = 0; i < static_cast<int> (rows_.size ()); ++i)
    {
      if (std::ranges::contains (rows_[i].objects, object))
        return i;
    }
  return -1;
}

dsp::ChordDescriptor *
ChordRowListModel::descriptorAtRow (int row) const
{
  if (row < 0 || row >= static_cast<int> (rows_.size ()))
    return nullptr;
  return rows_[row].descriptor;
}

QVariantList
ChordRowListModel::chordObjectsAtRow (int row) const
{
  QVariantList list;
  if (row < 0 || row >= static_cast<int> (rows_.size ()))
    return list;
  for (auto * obj : rows_[row].objects)
    list.append (QVariant::fromValue (obj));
  return list;
}

void
ChordRowListModel::rebuild ()
{
  // Group all chord objects by descriptor equivalence.
  rows_.clear ();
  for (auto * co : region_->get_children_view ())
    {
      auto * desc = co->chordDescriptor ();
      auto   it = std::ranges::find_if (rows_, [&] (const Row &r) {
        return r.descriptor->isEquivalent (*desc);
      });
      if (it == rows_.end ())
        rows_.push_back ({ desc, { co } });
      else
        it->objects.push_back (co);
    }
  // Sort rows alphabetically by display name (stable).
  std::ranges::stable_sort (rows_, [] (const Row &a, const Row &b) {
    return a.descriptor->displayName () < b.descriptor->displayName ();
  });
}

void
ChordRowListModel::connect_to_region_model ()
{
  using structure::arrangement::ArrangerObjectListModel;
  const auto * model = region_->chordObjects ();

  // Structural changes: rebuild + rewire per-object descriptor connections.
  connect (model, &QAbstractItemModel::rowsInserted, this, [this] () {
    disconnect_descriptor_signals ();
    connect_descriptor_signals ();
    beginResetModel ();
    rebuild ();
    endResetModel ();
    Q_EMIT contentChanged ();
  });
  connect (model, &QAbstractItemModel::rowsRemoved, this, [this] () {
    disconnect_descriptor_signals ();
    connect_descriptor_signals ();
    beginResetModel ();
    rebuild ();
    endResetModel ();
    Q_EMIT contentChanged ();
  });
  // Position/property changes: forward for span recomputation only.
  // Grouping depends on descriptor fields, not positions, so no rebuild here.
  connect (model, &ArrangerObjectListModel::contentChanged, this, [this] () {
    Q_EMIT contentChanged ();
  });
}

void
ChordRowListModel::disconnect_from_region_model ()
{
  if (region_ != nullptr && region_->chordObjects () != nullptr)
    disconnect (region_->chordObjects (), nullptr, this, nullptr);
}

void
ChordRowListModel::connect_descriptor_signals ()
{
  for (auto * co : region_->get_children_view ())
    {
      auto * desc = co->chordDescriptor ();
      descriptor_connections_.push_back (
        connect (desc, &dsp::ChordDescriptor::changed, this, [this] () {
          beginResetModel ();
          rebuild ();
          endResetModel ();
          Q_EMIT contentChanged ();
        }));
    }
}

void
ChordRowListModel::disconnect_descriptor_signals ()
{
  for (const auto &conn : descriptor_connections_)
    QObject::disconnect (conn);
  descriptor_connections_.clear ();
}

} // namespace zrythm::gui::qquick
