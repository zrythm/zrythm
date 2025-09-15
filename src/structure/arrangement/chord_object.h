// SPDX-FileCopyrightText: © 2018-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/arranger_object.h"
#include "utils/icloneable.h"

namespace zrythm::structure::arrangement
{
/**
 * @brief The ChordObject class represents a chord inside the chord editor.
 *
 * It maintains the index of the corresponding chord in the chord editor.
 *
 * This is normally part of a ChordRegion.
 */
class ChordObject final : public ArrangerObject
{
  Q_OBJECT
  Q_PROPERTY (
    int chordDescriptorIndex READ chordDescriptorIndex WRITE
      setChordDescriptorIndex NOTIFY chordDescriptorIndexChanged)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  ChordObject (const dsp::TempoMap &tempo_map, QObject * parent = nullptr);

  // ========================================================================
  // QML Interface
  // ========================================================================

  int           chordDescriptorIndex () const { return chord_index_; }
  void          setChordDescriptorIndex (int descr);
  Q_SIGNAL void chordDescriptorIndexChanged (int);

  // ========================================================================

private:
  friend void init_from (
    ChordObject           &obj,
    const ChordObject     &other,
    utils::ObjectCloneType clone_type);

  static constexpr auto kChordIndexKey = "chordIndex"sv;
  friend void           to_json (nlohmann::json &j, const ChordObject &co)
  {
    to_json (j, static_cast<const ArrangerObject &> (co));
    j[kChordIndexKey] = co.chord_index_;
  }
  friend void from_json (const nlohmann::json &j, ChordObject &co)
  {
    from_json (j, static_cast<ArrangerObject &> (co));
    j.at (kChordIndexKey).get_to (co.chord_index_);
  }

private:
  /** The index of the chord it belongs to (0 topmost). */
  int chord_index_ = 0;

  BOOST_DESCRIBE_CLASS (ChordObject, (ArrangerObject), (), (), (chord_index_))
};

}
