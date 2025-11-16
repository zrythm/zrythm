// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "plugins/plugin_descriptor_list.h"

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

void
PluginDescriptorList::update_cache ()
{
  if (!cached_descriptors_.empty ())
    {
      beginRemoveRows (
        {}, 0, static_cast<int> (cached_descriptors_.size ()) - 1);
      cached_descriptors_.clear ();
      endRemoveRows ();
    }
  const auto types = known_plugin_list_->getTypes ();
  if (!types.isEmpty ())
    {
      beginInsertRows ({}, 0, types.size () - 1);

      for (const auto &juce_descriptor : types)
        {
          auto descriptor =
            plugins::PluginDescriptor::from_juce_description (juce_descriptor);
          cached_descriptors_.emplace_back (descriptor.release ());
        }
      endInsertRows ();
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
  if (row < 0 || row >= known_plugin_list_->getNumTypes () || column > 0)
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
  return static_cast<int> (cached_descriptors_.size ());
}

int
PluginDescriptorList::columnCount (const QModelIndex &parent) const
{
  return 1;
}

void
PluginDescriptorList::reset_model ()
{
  beginResetModel ();
  endResetModel ();
}

QVariant
PluginDescriptorList::data (const QModelIndex &index, int role) const
{
  if (index.row () < 0 || index.row () >= known_plugin_list_->getNumTypes ())
    return {};

  const auto &descriptor = cached_descriptors_.at (index.row ());

  if (role == Qt::DisplayRole || role == DescriptorNameRole)
    return descriptor->name_.to_qstring ();
  if (role == DescriptorRole)
    {
      return QVariant::fromValue (descriptor.get ());
    }
  return {};
}
}
