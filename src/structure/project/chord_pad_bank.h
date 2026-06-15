// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/chord_descriptor.h"
#include "dsp/chord_preset.h"
#include "dsp/musical_scale.h"
#include "utils/midi.h"
#include "utils/qt.h"

#include <QAbstractListModel>
#include <QtQmlIntegration/qqmlintegration.h>

#include <farbot/RealtimeObject.hpp>
#include <nlohmann/json_fwd.hpp>

using namespace std::string_view_literals;

namespace zrythm::structure::project
{

/**
 * @brief A list model of chord descriptors for the chord pad panel.
 *
 * Provides a variable-length collection of ChordDescriptors that can be
 * used for auditioning, MIDI expansion, and composition shortcuts.
 * Owned by ProjectUiState and serialized as UI state.
 *
 * QML accesses individual chords via the `chordDescriptor` role.
 */
class ChordPadBank : public QAbstractListModel
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  enum Roles
  {
    ChordDescriptorRole = Qt::UserRole + 1,
  };

  explicit ChordPadBank (QObject * parent = nullptr);

  // ========================================================================
  // QAbstractListModel interface
  // ========================================================================

  QHash<int, QByteArray> roleNames () const override;
  int      rowCount (const QModelIndex &parent = {}) const override;
  QVariant data (const QModelIndex &index, int role) const override;

  // ========================================================================
  // QML Interface
  // ========================================================================

  void addChord (
    dsp::MusicalNote                root,
    dsp::ChordType                  type,
    dsp::ChordAccent                accent = dsp::ChordAccent::None,
    int                             inversion = 0,
    std::optional<dsp::MusicalNote> bass = std::nullopt);

  void insertChord (
    int                             index,
    dsp::MusicalNote                root,
    dsp::ChordType                  type,
    dsp::ChordAccent                accent = dsp::ChordAccent::None,
    int                             inversion = 0,
    std::optional<dsp::MusicalNote> bass = std::nullopt);

  void removeChord (int index);

  void moveChord (int from, int to);

  void transposeChords (bool up);

  void applyPresetFromScale (
    dsp::MusicalScale::ScaleType scale,
    dsp::MusicalNote             root_note);

  void applyPreset (ChordPreset * preset);

  void apply_preset (const ChordPreset &preset);

  // ========================================================================

  /**
   * @brief Returns the pre-computed MIDI pitches for a pad triggered by the
   * given MIDI note number.
   *
   * Maps notes 60-71 (C3-B3) to chord pad indices 0-11.
   * Returns std::nullopt if out of range or no chord at that index.
   *
   * Realtime-safe: reads from a lock-free snapshot updated by the main thread.
   */
  std::optional<dsp::ChordDescriptor::ChordPitches>
  get_pitches_for_note (midi_byte_t note_number) noexcept [[clang::nonblocking]];

  Q_INVOKABLE dsp::ChordDescriptor * chordAt (int index);

  friend void to_json (nlohmann::json &j, const ChordPadBank &bank);
  friend void from_json (const nlohmann::json &j, ChordPadBank &bank);

private:
  static constexpr auto kChordsKey = "chords"sv;

  /**
   * @brief Pre-computed chord pitches per pad slot, for audio-thread access.
   *
   * Index 0 = note 60 (C3), index 11 = note 71 (B3).
   */
  struct ChordPadPlaybackData
  {
    struct Slot
    {
      bool                               has_chord = false;
      dsp::ChordDescriptor::ChordPitches pitches;
    };
    std::array<Slot, 12> slots{};
  };

  void rebuild_playback_data ();
  void connect_chord_signals ();
  void connect_chord_signal (int index);

  std::vector<zrythm::utils::QObjectUniquePtr<dsp::ChordDescriptor>> chords_;

  farbot::RealtimeObject<
    ChordPadPlaybackData,
    farbot::RealtimeObjectOptions::nonRealtimeMutatable>
    playback_data_;
};

}
