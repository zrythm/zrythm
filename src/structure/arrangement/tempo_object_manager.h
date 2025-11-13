// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/arranger_object_owner.h"
#include "structure/arrangement/tempo_object.h"
#include "structure/arrangement/time_signature_object.h"

namespace zrythm::structure::arrangement
{

/**
 * @brief Manages tempo and time signature objects for a project.
 */
class TempoObjectManager final
    : public QObject,
      public ArrangerObjectOwner<TempoObject>,
      public ArrangerObjectOwner<TimeSignatureObject>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_ARRANGER_OBJECT_OWNER_QML_PROPERTIES (
    TempoObjectManager,
    tempoObjects,
    TempoObject)
  DEFINE_ARRANGER_OBJECT_OWNER_QML_PROPERTIES (
    TempoObjectManager,
    timeSignatureObjects,
    TimeSignatureObject)
  QML_UNCREATABLE ("")

public:
  TempoObjectManager (
    ArrangerObjectRegistry       &arranger_object_registry,
    dsp::FileAudioSourceRegistry &file_audio_source_registry,
    QObject *                     parent = nullptr);

  friend void init_from (
    TempoObjectManager       &obj,
    const TempoObjectManager &other,
    utils::ObjectCloneType    clone_type);

  std::string
  get_field_name_for_serialization (const TempoObject *) const override
  {
    return "tempoObjects";
  }

  std::string
  get_field_name_for_serialization (const TimeSignatureObject *) const override
  {
    return "timeSignatureObjects";
  }

private:
  static constexpr auto kTempoObjectsKey = "tempoObjects"sv;
  static constexpr auto kTimeSignatureObjectsKey = "timeSignatureObjects"sv;
  friend void to_json (nlohmann::json &j, const TempoObjectManager &manager);
  friend void from_json (const nlohmann::json &j, TempoObjectManager &manager);
};

} // namespace zrythm::structure::arrangement
