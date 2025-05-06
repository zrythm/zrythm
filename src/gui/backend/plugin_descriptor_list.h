// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "gui/dsp/plugin_descriptor.h"

#include <qqmlregistration.h>

namespace zrythm::gui
{

class PluginDescriptorList : public QAbstractListModel
{
  Q_OBJECT
  QML_ELEMENT

public:
  explicit PluginDescriptorList (QObject * parent = nullptr)
      : QAbstractListModel (parent)
  {
  }

  QModelIndex
  index (int row, int column, const QModelIndex &parent = QModelIndex ())
    const override
  {
    if (row < 0 || row >= descriptors_.size ())
      return {};
    return createIndex (row, column);
  }

  QModelIndex parent (const QModelIndex &child) const override { return {}; }

  int rowCount (const QModelIndex &parent = QModelIndex ()) const override
  {
    return static_cast<int> (descriptors_.size ());
  }

  int columnCount (const QModelIndex &parent = QModelIndex ()) const override
  {
    return 1;
  }

  QVariant
  data (const QModelIndex &index, int role = Qt::DisplayRole) const override
  {
    if (index.row () < 0 || index.row () >= descriptors_.size ())
      return {};
    if (role == Qt::DisplayRole)
      return descriptors_.at (index.row ())->name_.to_qstring ();
    return {};
  }

public:
  void addDescriptor (const gui::old_dsp::plugins::PluginDescriptor &descriptor)
  {
    beginInsertRows (
      QModelIndex (), static_cast<int> (descriptors_.size ()),
      static_cast<int> (descriptors_.size ()));
    auto * new_obj = descriptor.clone_qobject (this);
    descriptors_.push_back (new_obj);
    endInsertRows ();
  }

  void clear ()
  {
    beginResetModel ();
    std::ranges::for_each (descriptors_, [] (auto * descriptor) {
      delete descriptor;
    });
    descriptors_.clear ();
    endResetModel ();
  }

  auto at (int index) const { return descriptors_.at (index); }

private:
  QList<gui::old_dsp::plugins::PluginDescriptor *> descriptors_;
};
}
