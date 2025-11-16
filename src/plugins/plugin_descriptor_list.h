// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <utility>

#include "plugins/plugin_descriptor.h"
#include "utils/debouncer.h"

#include <QtQmlIntegration>

namespace zrythm::plugins::discovery
{

class PluginDescriptorList : public QAbstractListModel
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

  // TODO: move this somewhere else as a general util that allows using a lambda
  // for juce callback listeners
  class KnownPluginListChangeListener final : public juce::ChangeListener
  {
  public:
    using CallbackFunc = std::function<void (juce::ChangeBroadcaster *)>;

    KnownPluginListChangeListener (CallbackFunc callback);

  private:
    void changeListenerCallback (juce::ChangeBroadcaster * source) override
    {
      callback_ (source);
    }

  private:
    CallbackFunc callback_;
  };

public:
  explicit PluginDescriptorList (
    std::shared_ptr<juce::KnownPluginList> known_plugin_list,
    QObject *                              parent = nullptr);

  enum PluginDescriptorRole
  {
    DescriptorRole = Qt::UserRole + 1,
    DescriptorNameRole,
  };

  QHash<int, QByteArray> roleNames () const override;

  QModelIndex
  index (int row, int column, const QModelIndex &parent = QModelIndex ())
    const override;

  QModelIndex parent (const QModelIndex &child) const override;

  int rowCount (const QModelIndex &parent = QModelIndex ()) const override;

  int columnCount (const QModelIndex &parent = QModelIndex ()) const override;

  QVariant
  data (const QModelIndex &index, int role = Qt::DisplayRole) const override;

  void reset_model ();

private:
  /** Updates the cached plugin descriptors from the known plugin list. */
  void update_cache ();

  /** Schedules an update of the cache with debouncing to avoid excessive
   * updates. */
  void schedule_update_cache ();

private:
  KnownPluginListChangeListener          known_plugin_list_change_listener_;
  std::shared_ptr<juce::KnownPluginList> known_plugin_list_;
  std::vector<utils::QObjectUniquePtr<PluginDescriptor>> cached_descriptors_;
  utils::QObjectUniquePtr<utils::Debouncer>              update_debouncer_;
};
}
