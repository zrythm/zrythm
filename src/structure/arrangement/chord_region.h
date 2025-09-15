// SPDX-FileCopyrightText: © 2019-2021, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/arranger_object_owner.h"
#include "structure/arrangement/chord_object.h"

namespace zrythm::structure::arrangement
{
class ChordRegion final
    : public ArrangerObject,
      public ArrangerObjectOwner<ChordObject>
{
  Q_OBJECT
  DEFINE_ARRANGER_OBJECT_OWNER_QML_PROPERTIES (
    ChordRegion,
    chordObjects,
    ChordObject)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  ChordRegion (
    const dsp::TempoMap          &tempo_map,
    ArrangerObjectRegistry       &object_registry,
    dsp::FileAudioSourceRegistry &file_audio_source_registry,
    QObject *                     parent = nullptr);

  // ========================================================================
  // QML Interface
  // ========================================================================

  // ========================================================================

  std::string
  get_field_name_for_serialization (const ChordObject *) const override
  {
    return "chordObjects";
  }

private:
  friend void init_from (
    ChordRegion           &obj,
    const ChordRegion     &other,
    utils::ObjectCloneType clone_type);

  friend void to_json (nlohmann::json &j, const ChordRegion &region)
  {
    to_json (j, static_cast<const ArrangerObject &> (region));
    to_json (j, static_cast<const ArrangerObjectOwner &> (region));
  }
  friend void from_json (const nlohmann::json &j, ChordRegion &region)
  {
    from_json (j, static_cast<ArrangerObject &> (region));
    from_json (j, static_cast<ArrangerObjectOwner &> (region));
  }

private:
  BOOST_DESCRIBE_CLASS (
    ChordRegion,
    (ArrangerObject, ArrangerObjectOwner<ChordObject>),
    (),
    (),
    ())
};

}
