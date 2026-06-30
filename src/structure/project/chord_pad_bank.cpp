// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <stdexcept>

#include "dsp/chord_preset.h"
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
      root, type, accent, inversion, bass, this));
  endInsertRows ();
  connect_chord_signal (row);
  rebuild_playback_data ();
}

void
ChordPadBank::insertChord (
  int                             index,
  dsp::MusicalNote                root,
  dsp::ChordType                  type,
  dsp::ChordAccent                accent,
  int                             inversion,
  std::optional<dsp::MusicalNote> bass)
{
  if (index < 0 || index > static_cast<int> (chords_.size ()))
    throw std::out_of_range ("insertChord: index out of range");

  beginInsertRows ({}, index, index);
  chords_.insert (
    std::next (chords_.begin (), index),
    zrythm::utils::make_qobject_unique<dsp::ChordDescriptor> (
      root, type, accent, inversion, bass, this));
  endInsertRows ();
  connect_chord_signal (index);
  rebuild_playback_data ();
}

void
ChordPadBank::removeChord (int index)
{
  if (index < 0 || index >= static_cast<int> (chords_.size ()))
    return;

  beginRemoveRows ({}, index, index);
  chords_.erase (chords_.begin () + index);
  endRemoveRows ();
  rebuild_playback_data ();
}

void
ChordPadBank::moveChord (int from, int to)
{
  if (from < 0 || from >= static_cast<int> (chords_.size ()))
    return;
  if (to < 0 || to >= static_cast<int> (chords_.size ()))
    return;
  if (from == to)
    return;

  beginMoveRows ({}, from, from, {}, to > from ? to + 1 : to);
  auto ptr = std::move (chords_[from]);
  chords_.erase (chords_.begin () + from);
  chords_.insert (chords_.begin () + to, std::move (ptr));
  endMoveRows ();
  rebuild_playback_data ();
}

void
ChordPadBank::transposeChords (bool up)
{
  for (const auto &descr : chords_)
    {
      QSignalBlocker blocker (descr.get ());
      descr->setRootNote (dsp::chords::transpose_note (descr->rootNote (), up));
      if (descr->hasBass ())
        descr->setBassNote (
          dsp::chords::transpose_note (descr->bassNote (), up));
    }

  const auto last = static_cast<int> (chords_.size ()) - 1;
  if (last >= 0)
    Q_EMIT dataChanged (index (0), index (last), { ChordDescriptorRole });
  rebuild_playback_data ();
}

dsp::ChordDescriptor *
ChordPadBank::chordAt (int index)
{
  if (index < 0 || index >= static_cast<int> (chords_.size ()))
    {
      z_error ("chordAt: index {} out of range [0, {})", index, chords_.size ());
      return nullptr;
    }
  return chords_[index].get ();
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

  const auto triads = dsp::MusicalScale::get_diatonic_triads (scale, root_note);

  beginResetModel ();
  chords_.clear ();

  for (const auto &triad : triads)
    {
      chords_.push_back (
        zrythm::utils::make_qobject_unique<dsp::ChordDescriptor> (
          triad.root_note, triad.chord_type, dsp::ChordAccent::None, 0,
          std::nullopt, this));
    }

  endResetModel ();
  connect_chord_signals ();
  rebuild_playback_data ();
}

void
ChordPadBank::applyPreset (ChordPreset * preset)
{
  if (preset == nullptr)
    return;

  apply_preset (*preset);
}

void
ChordPadBank::apply_preset (const ChordPreset &preset)
{
  beginResetModel ();
  chords_.clear ();

  for (const auto &descr : preset.descriptors ())
    {
      auto new_descr =
        zrythm::utils::make_qobject_unique<dsp::ChordDescriptor> (this);
      new_descr->setRootNote (descr->rootNote ());
      new_descr->setChordType (descr->chordType ());
      new_descr->setChordAccent (descr->chordAccent ());
      new_descr->setInversion (descr->inversion ());
      if (descr->hasBass ())
        new_descr->setBassNote (descr->bassNote ());
      chords_.push_back (std::move (new_descr));
    }

  endResetModel ();
  connect_chord_signals ();
  rebuild_playback_data ();
}

void
ChordPadBank::rebuild_playback_data ()
{
  ChordPadPlaybackData data;
  for (size_t i = 0; i < chords_.size () && i < 12; ++i)
    {
      data.slots[i].has_chord = true;
      data.slots[i].pitches = chords_[i]->getMidiPitches ();
    }

  decltype (playback_data_)::ScopedAccess<farbot::ThreadType::nonRealtime>
    access{ playback_data_ };
  *access = std::move (data);
}

void
ChordPadBank::connect_chord_signals ()
{
  for (const auto &descr : chords_)
    {
      // Disconnect any existing connection from this chord's changed signal
      // to avoid accumulating duplicates when called repeatedly (e.g., after
      // applyPreset*/from_json).
      QObject::disconnect (
        descr.get (), &dsp::ChordDescriptor::changed, this, nullptr);
      QObject::connect (
        descr.get (), &dsp::ChordDescriptor::changed, this,
        [this] () { rebuild_playback_data (); });
    }
}

void
ChordPadBank::connect_chord_signal (int index)
{
  const auto &descr = chords_[index];
  QObject::connect (
    descr.get (), &dsp::ChordDescriptor::changed, this,
    [this] () { rebuild_playback_data (); });
}

std::optional<dsp::ChordDescriptor::ChordPitches>
ChordPadBank::get_pitches_for_note (midi_byte_t note_number) noexcept
{
  if (note_number < 60 || note_number >= 72)
    return std::nullopt;

  decltype (playback_data_)::ScopedAccess<farbot::ThreadType::realtime> access{
    playback_data_
  };

  const auto &slot = (*access).slots[note_number - 60];
  if (!slot.has_chord)
    return std::nullopt;

  return slot.pitches;
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
          auto ptr =
            zrythm::utils::make_qobject_unique<dsp::ChordDescriptor> (&bank);
          from_json (chord_json, *ptr);
          bank.chords_.push_back (std::move (ptr));
        }
      bank.endResetModel ();
      bank.connect_chord_signals ();
      bank.rebuild_playback_data ();
    }
}

}
