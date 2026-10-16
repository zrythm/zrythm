// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "plugins/plugin_descriptor.h"
#include "utils/debouncer.h"

#include <QAbstractListModel>
#include <QtQmlIntegration/qqmlintegration.h>

#include <juce_audio_processors/juce_audio_processors.h>

namespace zrythm::plugins::discovery
{

/**
 * @brief List model exposing the plugins in a juce::KnownPluginList as
 * PluginDescriptor objects.
 *
 * When the known plugin list changes, the cache is refreshed with incremental
 * row insertions, removals and in-place data changes: existing rows keep their
 * position and their descriptor object unless the plugin's information
 * changed. Rows are in first-seen order and never move; views are responsible
 * for sorting.
 */
class PluginDescriptorList : public QAbstractListModel
{
  Q_OBJECT
  Q_PROPERTY (
    QStringList availableFormats READ availableFormats NOTIFY
      availableFormatsChanged FINAL)
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

  /**
   * Returns the distinct plugin formats present in the list (e.g. "VST3",
   * "LV2"), sorted alphabetically.
   */
  QStringList availableFormats () const { return available_formats_; }

Q_SIGNALS:
  void availableFormatsChanged ();

private:
  /** Updates the cached plugin descriptors from the known plugin list. */
  void update_cache ();

  /** Schedules an update of the cache with debouncing to avoid excessive
   * updates. */
  void schedule_update_cache ();

  /** One cached row: the identity key, the source description and the
   * descriptor exposed for it. */
  struct CacheEntry
  {
    QString                                   key;
    juce::PluginDescription                   source;
    utils::QObjectUniquePtr<PluginDescriptor> descriptor;
  };

  /** Identity key matching juce::KnownPluginList duplicate detection
   * (fileOrIdentifier, deprecatedUid, uniqueId). */
  static QString descriptor_key (const juce::PluginDescription &desc);

  /** Returns whether the two descriptions expose the same plugin information.
   * File timestamps are excluded: they change on every rescan regardless of
   * the plugin's actual information. */
  static bool same_content (
    const juce::PluginDescription &a,
    const juce::PluginDescription &b);

private:
  KnownPluginListChangeListener             known_plugin_list_change_listener_;
  std::shared_ptr<juce::KnownPluginList>    known_plugin_list_;
  std::vector<CacheEntry>                   cached_entries_;
  utils::QObjectUniquePtr<utils::Debouncer> update_debouncer_;

  /** Distinct plugin formats present in cached_entries_, sorted. */
  QStringList available_formats_;
};
}
