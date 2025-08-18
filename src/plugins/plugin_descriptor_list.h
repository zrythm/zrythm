// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <utility>

#include "plugins/plugin_descriptor.h"

#include <QtQmlIntegration>

namespace zrythm::plugins::discovery
{

class PluginDescriptorList : public QAbstractListModel
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  explicit PluginDescriptorList (
    std::shared_ptr<juce::KnownPluginList> known_plugin_list,
    QObject *                              parent = nullptr)
      : QAbstractListModel (parent),
        known_plugin_list_ (std::move (known_plugin_list))
  {
  }

  enum PluginDescriptorRole
  {
    DescriptorRole = Qt::UserRole + 1,
  };

  QHash<int, QByteArray> roleNames () const override
  {
    return {
      { DescriptorRole, "descriptor" },
    };
  }

  QModelIndex
  index (int row, int column, const QModelIndex &parent = QModelIndex ())
    const override
  {
    if (row < 0 || row >= known_plugin_list_->getNumTypes ())
      return {};
    return createIndex (row, column);
  }

  QModelIndex parent (const QModelIndex &child) const override { return {}; }

  int rowCount (const QModelIndex &parent = QModelIndex ()) const override
  {
    return known_plugin_list_->getNumTypes ();
  }

  int columnCount (const QModelIndex &parent = QModelIndex ()) const override
  {
    return 1;
  }

  auto at (int index) const
  {
    return PluginDescriptor::from_juce_description (
      known_plugin_list_->getTypes ().getReference (index));
  }

  QVariant
  data (const QModelIndex &index, int role = Qt::DisplayRole) const override;

  void reset_model ()
  {
    beginResetModel ();
    endResetModel ();
  }

private:
  std::shared_ptr<juce::KnownPluginList> known_plugin_list_;
};
}
