// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/arranger_object_owner.h"
#include "structure/arrangement/tempo_object.h"
#include "structure/arrangement/time_signature_object.h"
#include "utils/typed_uuid_reference.h"
#include "utils/uuid_identifiable_object.h"

namespace zrythm::structure::arrangement
{

/**
 * @brief Manages tempo and time signature objects for a project.
 */
class TempoObjectManager final
    : public utils::UuidIdentifiableObject<TempoObjectManager>,
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
    utils::IObjectRegistry &registry,
    QObject *               parent = nullptr);

  /**
   * @brief Rebuilds the given tempo map from this manager's objects.
   *
   * Clears the map's inserted time-signature and tempo events and re-adds them
   * from the manager's sorted children (time signatures first, then tempos).
   * The map's base tempo / base time signature are unaffected (they are
   * intrinsic to the map and govern the region from tick 0 up to the first
   * inserted event).
   *
   * Must not be called while the audio engine is running.
   */
  void sync_to_tempo_map (dsp::TempoMapWrapper &wrapper);

  friend void init_from (
    TempoObjectManager       &obj,
    const TempoObjectManager &other,
    utils::ObjectCloneType    clone_type);

  std::string
  get_field_name_for_serialization (const TempoObject *) const override
  {
    return std::string{ kTempoObjectsKey };
  }

  std::string
  get_field_name_for_serialization (const TimeSignatureObject *) const override
  {
    return std::string{ kTimeSignatureObjectsKey };
  }

private:
  static constexpr auto kTempoObjectsKey = "tempoObjects"sv;
  static constexpr auto kTimeSignatureObjectsKey = "timeSignatureObjects"sv;
  friend void to_json (nlohmann::json &j, const TempoObjectManager &manager);
  friend void from_json (const nlohmann::json &j, TempoObjectManager &manager);
};

using TempoObjectManagerUuidReference =
  utils::TypedUuidReference<TempoObjectManager>;

} // namespace zrythm::structure::arrangement
