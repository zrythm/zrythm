// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <ranges>

#include "structure/arrangement/audio_transform_entry_list_model.h"

namespace zrythm::structure::arrangement
{

AudioTransformEntryListModel::AudioTransformEntryListModel (QObject * parent)
    : QAbstractListModel (parent)
{
}

QHash<int, QByteArray>
AudioTransformEntryListModel::roleNames () const
{
  return {
    { EntryRole, "entry" }
  };
}

int
AudioTransformEntryListModel::rowCount (const QModelIndex &parent) const
{
  if (parent.isValid ())
    return 0;
  return static_cast<int> (entries_.size ());
}

QVariant
AudioTransformEntryListModel::data (const QModelIndex &index, int role) const
{
  if (!index.isValid () || role != EntryRole)
    return {};
  const auto row = index.row ();
  if (row < 0 || row >= static_cast<int> (entries_.size ()))
    return {};
  return QVariant::fromValue (entries_[row]);
}

AudioClip *
AudioTransformEntryListModel::clip () const
{
  return clip_;
}

void
AudioTransformEntryListModel::setClip (AudioClip * clip)
{
  if (clip_ == clip)
    return;

  const auto old_count = count ();
  beginResetModel ();
  entries_.clear ();
  if (clip_ != nullptr)
    {
      QObject::disconnect (
        clip_, &AudioClip::renderedAudioChanged, this,
        &AudioTransformEntryListModel::rebuild);
    }
  clip_ = clip;
  if (clip_ != nullptr)
    {
      QObject::connect (
        clip_, &AudioClip::renderedAudioChanged, this,
        &AudioTransformEntryListModel::rebuild);
      entries_ = clip_->transform_entries () | std::ranges::to<std::vector> ();
    }
  endResetModel ();
  if (count () != old_count)
    Q_EMIT countChanged ();
  Q_EMIT clipChanged ();
}

void
AudioTransformEntryListModel::rebuild ()
{
  if (clip_ == nullptr)
    {
      if (!entries_.empty ())
        {
          beginResetModel ();
          entries_.clear ();
          endResetModel ();
          Q_EMIT countChanged ();
        }
      return;
    }

  auto new_entries =
    clip_->transform_entries () | std::ranges::to<std::vector> ();

  if (new_entries.size () == entries_.size ())
    {
      entries_ = std::move (new_entries);
      if (!entries_.empty ())
        {
          Q_EMIT dataChanged (
            index (0), index (static_cast<int> (entries_.size ()) - 1),
            { EntryRole });
        }
      return;
    }

  beginResetModel ();
  entries_ = std::move (new_entries);
  endResetModel ();
  Q_EMIT countChanged ();
}
} // namespace zrythm::structure::arrangement
