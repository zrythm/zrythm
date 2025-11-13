// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/tempo_object_manager.h"

namespace zrythm::structure::arrangement
{
TempoObjectManager::TempoObjectManager (
  ArrangerObjectRegistry       &arranger_object_registry,
  dsp::FileAudioSourceRegistry &file_audio_source_registry,
  QObject *                     parent)
    : QObject (parent),
      ArrangerObjectOwner<
        TempoObject> (arranger_object_registry, file_audio_source_registry, *this),
      ArrangerObjectOwner<TimeSignatureObject> (
        arranger_object_registry,
        file_audio_source_registry,
        *this)
{
}

void
init_from (
  TempoObjectManager       &obj,
  const TempoObjectManager &other,
  utils::ObjectCloneType    clone_type)
{
  init_from (
    static_cast<ArrangerObjectOwner<TempoObject> &> (obj),
    static_cast<const ArrangerObjectOwner<TempoObject> &> (other), clone_type);
  init_from (
    static_cast<ArrangerObjectOwner<TimeSignatureObject> &> (obj),
    static_cast<const ArrangerObjectOwner<TimeSignatureObject> &> (other),
    clone_type);
}

void
to_json (nlohmann::json &j, const TempoObjectManager &manager)
{
  j[TempoObjectManager::kTempoObjectsKey] =
    static_cast<const ArrangerObjectOwner<TempoObject> &> (manager);
  j[TempoObjectManager::kTimeSignatureObjectsKey] =
    static_cast<const ArrangerObjectOwner<TimeSignatureObject> &> (manager);
}

void
from_json (const nlohmann::json &j, TempoObjectManager &manager)
{
  from_json (
    j.at (TempoObjectManager::kTempoObjectsKey),
    static_cast<ArrangerObjectOwner<TempoObject> &> (manager));
  from_json (
    j.at (TempoObjectManager::kTimeSignatureObjectsKey),
    static_cast<ArrangerObjectOwner<TimeSignatureObject> &> (manager));
}

} // namespace zrythm::structure::arrangement
