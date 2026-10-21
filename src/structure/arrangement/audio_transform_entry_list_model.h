// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/audio_clip.h"
#include "structure/arrangement/audio_function.h"

#include <QAbstractListModel>
#include <QPointer>

namespace zrythm::structure::arrangement
{

/**
 * @brief A list model over an audio clip's transform entries.
 *
 * The model follows the clip's renderedAudioChanged signal: a change
 * that keeps the entry count (reordering, toggling an entry) updates
 * the existing rows in place, so views keep their scroll position and
 * item delegates; a change that alters the entry count resets the
 * list.
 */
class AudioTransformEntryListModel : public QAbstractListModel
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY (
    zrythm::structure::arrangement::AudioClip * clip READ clip WRITE setClip
      NOTIFY clipChanged)
  Q_PROPERTY (int count READ count NOTIFY countChanged)

public:
  enum Roles
  {
    EntryRole = Qt::UserRole + 1,
  };
  Q_ENUM (Roles)

  explicit AudioTransformEntryListModel (QObject * parent = nullptr);

  QHash<int, QByteArray> roleNames () const override;
  int rowCount (const QModelIndex &parent = QModelIndex ()) const override;
  QVariant
  data (const QModelIndex &index, int role = Qt::DisplayRole) const override;

  AudioClip * clip () const;
  void        setClip (AudioClip * clip);

  /** Number of entries currently in the list. */
  int count () const { return static_cast<int> (entries_.size ()); }

Q_SIGNALS:
  void clipChanged ();
  void countChanged ();

private:
  void rebuild ();

  QPointer<AudioClip> clip_;

  std::vector<AudioFunction::Entry> entries_;
};
} // namespace zrythm::structure::arrangement
