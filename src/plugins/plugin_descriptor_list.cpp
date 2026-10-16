// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "plugins/plugin_descriptor_list.h"

#include <QHash>
#include <QSet>

using namespace std::chrono_literals;

namespace zrythm::plugins::discovery
{
PluginDescriptorList::KnownPluginListChangeListener::
  KnownPluginListChangeListener (CallbackFunc callback)
    : callback_ (std::move (callback))
{
}

PluginDescriptorList::PluginDescriptorList (
  std::shared_ptr<juce::KnownPluginList> known_plugin_list,
  QObject *                              parent)
    : QAbstractListModel (parent),
      known_plugin_list_change_listener_ ([this] (juce::ChangeBroadcaster *) {
        schedule_update_cache ();
      }),
      known_plugin_list_ (std::move (known_plugin_list)),
      update_debouncer_ (
        utils::make_qobject_unique<
          utils::Debouncer> (150ms, [this] () { update_cache (); }, this))
{
  known_plugin_list_->addChangeListener (&known_plugin_list_change_listener_);
  schedule_update_cache ();
}

void
PluginDescriptorList::schedule_update_cache ()
{
  // Debouncing must be done on the main thread because it involves QTimer's
  QMetaObject::invokeMethod (this, [this] () {
    update_debouncer_->debounce ();
  });
}

QString
PluginDescriptorList::descriptor_key (const juce::PluginDescription &desc)
{
  return utils::Utf8String::from_juce_string (desc.fileOrIdentifier).to_qstring ()
         + QStringLiteral ("/") + QString::number (desc.deprecatedUid)
         + QStringLiteral ("/") + QString::number (desc.uniqueId);
}

bool
PluginDescriptorList::same_content (
  const juce::PluginDescription &a,
  const juce::PluginDescription &b)
{
  const auto tie = [] (const juce::PluginDescription &d) {
    return std::tie (
      d.name, d.descriptiveName, d.pluginFormatName, d.category,
      d.manufacturerName, d.version, d.fileOrIdentifier, d.deprecatedUid,
      d.uniqueId, d.isInstrument, d.numInputChannels, d.numOutputChannels,
      d.hasSharedContainer, d.hasARAExtension);
  };
  return tie (a) == tie (b);
}

void
PluginDescriptorList::update_cache ()
{
  const auto types = known_plugin_list_->getTypes ();

  // builds a new cache entry for a (fresh) source description
  const auto make_entry = [] (const juce::PluginDescription &source) {
    CacheEntry entry;
    entry.key = descriptor_key (source);
    entry.source = source;
    entry.descriptor.reset (
      PluginDescriptor::from_juce_description (source).release ());
    return entry;
  };
  const QHash<QString, const juce::PluginDescription *> fresh_by_key = [&] {
    QHash<QString, const juce::PluginDescription *> hash;
    for (const auto &type : types)
      hash.insert (descriptor_key (type), &type);
    return hash;
  }();
  const auto row_count = [this] {
    return static_cast<int> (cached_entries_.size ());
  };
  const auto is_fresh = [&fresh_by_key] (const CacheEntry &entry) {
    return fresh_by_key.contains (entry.key);
  };

  // remove rows of plugins no longer in the known list: contiguous row runs
  // are removed in a single signal pair each, from the end backwards so row
  // indices stay valid
  const auto removed_rows =
    std::views::iota (0, row_count ()) | std::views::filter ([&] (int row) {
      return !is_fresh (cached_entries_[row]);
    })
    | std::ranges::to<std::vector> ();
  for (
    const auto &run :
    removed_rows | std::views::chunk_by ([] (int a, int b) {
      return a + 1 == b;
    }) | std::views::reverse)
    {
      beginRemoveRows ({}, run.front (), run.back ());
      cached_entries_.erase (
        cached_entries_.begin () + run.front (),
        cached_entries_.begin () + run.back () + 1);
      endRemoveRows ();
    }

  // refresh rows whose plugin information changed and report them in
  // contiguous runs
  const auto changed_rows =
    std::views::iota (0, row_count ()) | std::views::filter ([&] (int row) {
      const auto  &entry = cached_entries_[row];
      const auto * fresh = fresh_by_key.value (entry.key, nullptr);
      return fresh != nullptr && !same_content (entry.source, *fresh);
    })
    | std::ranges::to<std::vector> ();
  for (const int row : changed_rows)
    {
      const auto * fresh =
        fresh_by_key.value (cached_entries_[row].key, nullptr);
      cached_entries_[row] = make_entry (*fresh);
    }
  for (
    const auto &run :
    changed_rows | std::views::chunk_by ([] (int a, int b) {
      return a + 1 == b;
    }))
    {
      Q_EMIT dataChanged (index (run.front (), 0), index (run.back (), 0));
    }

  // append rows for newly found plugins, in fresh-list order
  const auto cached_keys =
    cached_entries_
    | std::views::transform ([] (const CacheEntry &entry) { return entry.key; })
    | std::ranges::to<QSet<QString>> ();
  auto new_entries =
    types | std::views::filter ([&] (const juce::PluginDescription &type) {
      return !cached_keys.contains (descriptor_key (type));
    })
    | std::views::transform (make_entry) | std::ranges::to<std::vector> ();
  if (!new_entries.empty ())
    {
      beginInsertRows (
        {}, row_count (),
        row_count () + static_cast<int> (new_entries.size ()) - 1);
      cached_entries_.insert (
        cached_entries_.end (), std::make_move_iterator (new_entries.begin ()),
        std::make_move_iterator (new_entries.end ()));
      endInsertRows ();
    }

  // collect the format list alongside the descriptors
  auto formats =
    cached_entries_ | std::views::transform ([] (const CacheEntry &entry) {
      return entry.descriptor->format ();
    })
    | std::ranges::to<QStringList> ();
  formats.removeDuplicates ();
  formats.sort ();
  if (formats != available_formats_)
    {
      available_formats_ = formats;
      Q_EMIT availableFormatsChanged ();
    }
}

QHash<int, QByteArray>
PluginDescriptorList::roleNames () const
{
  return {
    { DescriptorRole,     "descriptor" },
    { DescriptorNameRole, "name"       },
  };
}

QModelIndex
PluginDescriptorList::index (int row, int column, const QModelIndex &parent) const
{
  if (row < 0 || row >= static_cast<int> (cached_entries_.size ()) || column > 0)
    return {};
  return createIndex (row, column);
}

QModelIndex
PluginDescriptorList::parent (const QModelIndex &child) const
{
  return {};
}

int
PluginDescriptorList::rowCount (const QModelIndex &parent) const
{
  return static_cast<int> (cached_entries_.size ());
}

int
PluginDescriptorList::columnCount (const QModelIndex &parent) const
{
  return 1;
}

QVariant
PluginDescriptorList::data (const QModelIndex &index, int role) const
{
  if (
    index.row () < 0
    || index.row () >= static_cast<int> (cached_entries_.size ()))
    return {};

  const auto &descriptor = cached_entries_.at (index.row ()).descriptor;

  if (role == Qt::DisplayRole || role == DescriptorNameRole)
    return descriptor->name_.to_qstring ();
  if (role == DescriptorRole)
    {
      return QVariant::fromValue (descriptor.get ());
    }
  return {};
}
}
