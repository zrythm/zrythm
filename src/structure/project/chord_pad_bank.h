// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/chord_descriptor.h"
#include "dsp/musical_scale.h"
#include "utils/qt.h"

#include <QAbstractListModel>
#include <QtQmlIntegration/qqmlintegration.h>

#include <nlohmann/json_fwd.hpp>

class ChordPreset;

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

  Q_INVOKABLE void addChord (
    dsp::MusicalNote                root,
    dsp::ChordType                  type,
    dsp::ChordAccent                accent = dsp::ChordAccent::None,
    int                             inversion = 0,
    std::optional<dsp::MusicalNote> bass = std::nullopt);

  Q_INVOKABLE void removeChord (int index);

  Q_INVOKABLE void transposeChords (bool up);

  Q_INVOKABLE void applyPresetFromScale (
    dsp::MusicalScale::ScaleType scale,
    dsp::MusicalNote             root_note);

  void apply_preset (const ChordPreset &preset);

  // ========================================================================

  /**
   * @brief Returns the ChordDescriptor for a MIDI note number.
   *
   * Maps notes 60-71 (C3-B3) to chord indices 0-11.
   * Returns nullptr if out of range or no chord at that index.
   */
  dsp::ChordDescriptor * get_chord_from_note_number (midi_byte_t note_number);

  dsp::ChordDescriptor * chord_at (int index)
  {
    return chords_.at (index).get ();
  }

  friend void to_json (nlohmann::json &j, const ChordPadBank &bank);
  friend void from_json (const nlohmann::json &j, ChordPadBank &bank);

private:
  static constexpr auto kChordsKey = "chords"sv;

  std::vector<zrythm::utils::QObjectUniquePtr<dsp::ChordDescriptor>> chords_;
};

}
