// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/settings/chord_preset.h"
#include "structure/project/chord_pad_bank.h"
#include "utils/logger.h"

#include <nlohmann/json.hpp>

namespace zrythm::structure::project
{

ChordPadBank::ChordPadBank (QObject * parent) : QAbstractListModel (parent) { }

QHash<int, QByteArray>
ChordPadBank::roleNames () const
{
  QHash<int, QByteArray> roles;
  roles[ChordDescriptorRole] = "chordDescriptor";
  return roles;
}

int
ChordPadBank::rowCount (const QModelIndex &parent) const
{
  if (parent.isValid ())
    return 0;
  return static_cast<int> (chords_.size ());
}

QVariant
ChordPadBank::data (const QModelIndex &index, int role) const
{
  if (!index.isValid () || index.row () >= static_cast<int> (chords_.size ()))
    return {};

  if (role == ChordDescriptorRole)
    return QVariant::fromValue (chords_.at (index.row ()).get ());

  return {};
}

void
ChordPadBank::addChord (
  dsp::MusicalNote                root,
  dsp::ChordType                  type,
  dsp::ChordAccent                accent,
  int                             inversion,
  std::optional<dsp::MusicalNote> bass)
{
  const auto row = static_cast<int> (chords_.size ());
  beginInsertRows ({}, row, row);
  chords_.push_back (
    zrythm::utils::make_qobject_unique<dsp::ChordDescriptor> (
      root, type, accent, inversion, bass));
  endInsertRows ();
}

void
ChordPadBank::removeChord (int index)
{
  if (index < 0 || index >= static_cast<int> (chords_.size ()))
    return;

  beginRemoveRows ({}, index, index);
  chords_.erase (chords_.begin () + index);
  endRemoveRows ();
}

void
ChordPadBank::transposeChords (bool up)
{
  const int add = up ? 1 : 11;
  for (auto &descr : chords_)
    {
      descr->setRootNote (
        static_cast<dsp::MusicalNote> (
          (static_cast<int> (descr->rootNote ()) + add) % 12));
      if (descr->hasBass ())
        descr->setBassNote (
          static_cast<dsp::MusicalNote> (
            (static_cast<int> (descr->bassNote ()) + add) % 12));
    }

  const auto last = static_cast<int> (chords_.size ()) - 1;
  if (last >= 0)
    Q_EMIT dataChanged (index (0), index (last), { ChordDescriptorRole });
}

void
ChordPadBank::applyPresetFromScale (
  dsp::MusicalScale::ScaleType scale,
  dsp::MusicalNote             root_note)
{
  z_debug (
    "applying preset from scale {}, root note {}",
    dsp::MusicalScale::type_to_string (scale),
    dsp::ChordDescriptor::note_to_string (root_note));

  const auto triads = dsp::MusicalScale::get_triad_types_for_type (scale, true);
  const bool * notes = dsp::MusicalScale::get_notes_for_type (scale, true);

  beginResetModel ();
  chords_.clear ();

  int cur_chord = 0;
  for (int i = 0; i < 12; i++)
    {
      if (!notes[i])
        continue;

      chords_.push_back (
        zrythm::utils::make_qobject_unique<dsp::ChordDescriptor> (
          static_cast<dsp::MusicalNote> ((static_cast<int> (root_note) + i) % 12),
          triads[cur_chord]));
      ++cur_chord;
    }

  endResetModel ();
}

void
ChordPadBank::apply_preset (const ChordPreset &preset)
{
  beginResetModel ();
  chords_.clear ();

  for (const auto &descr : preset.descr_)
    {
      auto new_descr =
        zrythm::utils::make_qobject_unique<dsp::ChordDescriptor> ();
      new_descr->setRootNote (descr->rootNote ());
      new_descr->setChordType (descr->chordType ());
      new_descr->setChordAccent (descr->chordAccent ());
      new_descr->setInversion (descr->inversion ());
      if (descr->hasBass ())
        new_descr->setBassNote (descr->bassNote ());
      chords_.push_back (std::move (new_descr));
    }

  endResetModel ();
}

dsp::ChordDescriptor *
ChordPadBank::get_chord_from_note_number (midi_byte_t note_number)
{
  if (note_number < 60 || note_number >= 72)
    return nullptr;

  const auto idx = static_cast<int> (note_number - 60);
  if (idx >= static_cast<int> (chords_.size ()))
    return nullptr;

  return chords_[idx].get ();
}

void
to_json (nlohmann::json &j, const ChordPadBank &bank)
{
  nlohmann::json chords_arr = nlohmann::json::array ();
  for (const auto &chord_ptr : bank.chords_)
    {
      nlohmann::json chord_json;
      to_json (chord_json, *chord_ptr);
      chords_arr.push_back (chord_json);
    }
  j[ChordPadBank::kChordsKey] = chords_arr;
}

void
from_json (const nlohmann::json &j, ChordPadBank &bank)
{
  if (j.contains (ChordPadBank::kChordsKey))
    {
      bank.beginResetModel ();
      bank.chords_.clear ();
      for (const auto &chord_json : j[ChordPadBank::kChordsKey])
        {
          auto ptr = zrythm::utils::make_qobject_unique<dsp::ChordDescriptor> ();
          from_json (chord_json, *ptr);
          bank.chords_.push_back (std::move (ptr));
        }
      bank.endResetModel ();
    }
}

}
