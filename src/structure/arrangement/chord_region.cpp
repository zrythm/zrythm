// SPDX-FileCopyrightText: © 2019-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/chord_region.h"

namespace zrythm::structure::arrangement
{
ChordRegion::ChordRegion (
  const dsp::TempoMap          &tempo_map,
  ArrangerObjectRegistry       &object_registry,
  dsp::FileAudioSourceRegistry &file_audio_source_registry,
  QObject *                     parent)
    : ArrangerObject (Type::ChordRegion, tempo_map, parent),
      ArrangerObjectOwner (object_registry, file_audio_source_registry, *this),
      region_mixin_ (utils::make_qobject_unique<RegionMixin> (*position ()))
{
}

void
init_from (
  ChordRegion           &obj,
  const ChordRegion     &other,
  utils::ObjectCloneType clone_type)
{
  init_from (
    static_cast<ArrangerObject &> (obj),
    static_cast<const ArrangerObject &> (other), clone_type);
  init_from (*obj.region_mixin_, *other.region_mixin_, clone_type);
  init_from (
    static_cast<ArrangerObjectOwner<ChordObject> &> (obj),
    static_cast<const ArrangerObjectOwner<ChordObject> &> (other), clone_type);
}
}
