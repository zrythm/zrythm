

#pragma once

#include <qqmlregistration.h>

#include "gui/dsp/plugin_descriptor.h"

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
    return descriptors_.size ();
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
      return QString::fromStdString (descriptors_.at (index.row ())->name_);
    return {};
  }

public:
  void addDescriptor (const zrythm::gui::dsp::plugins::PluginDescriptor &descriptor)
  {
    beginInsertRows (QModelIndex (), descriptors_.size (), descriptors_.size ());
    descriptors_.append (descriptor.clone_raw_ptr ());
    endInsertRows ();
  }

  void clear ()
  {
    beginResetModel ();
    descriptors_.clear ();
    endResetModel ();
  }

  zrythm::gui::dsp::plugins::PluginDescriptor * at (int index) const
  {
    return descriptors_.at (index);
  }

private:
  QList<zrythm::gui::dsp::plugins::PluginDescriptor *> descriptors_;
};
}
