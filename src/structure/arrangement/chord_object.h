// SPDX-FileCopyrightText: © 2018-2022, 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/chord_descriptor.h"
#include "structure/arrangement/arranger_object.h"
#include "utils/icloneable.h"
#include "utils/qt.h"

namespace zrythm::structure::arrangement
{

/**
 * A chord placed inside a ChordRegion on the chord track.
 *
 * Owns its ChordDescriptor inline.
 */
class ChordObject final : public ArrangerObject
{
  Q_OBJECT
  Q_PROPERTY (
    zrythm::dsp::ChordDescriptor * chordDescriptor READ chordDescriptor CONSTANT)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  ChordObject (
    const dsp::TempoMapWrapper &tempo_map_wrapper,
    QObject *                   parent = nullptr);

  // ========================================================================
  // QML Interface
  // ========================================================================

  dsp::ChordDescriptor * chordDescriptor () const
  {
    return chord_descriptor_.get ();
  }

  // ========================================================================

private:
  friend void init_from (
    ChordObject           &obj,
    const ChordObject     &other,
    utils::ObjectCloneType clone_type);

  static constexpr auto kChordDescriptorKey = "chordDescriptor"sv;
  friend void           to_json (nlohmann::json &j, const ChordObject &co);
  friend void           from_json (const nlohmann::json &j, ChordObject &co);

private:
  utils::QObjectUniquePtr<dsp::ChordDescriptor> chord_descriptor_;

  BOOST_DESCRIBE_CLASS (ChordObject, (ArrangerObject), (), (), (chord_descriptor_))
};

}
