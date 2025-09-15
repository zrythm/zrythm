// SPDX-FileCopyrightText: © 2018-2019, 2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/scale_object.h"

namespace zrythm::structure::arrangement
{
ScaleObject::ScaleObject (const dsp::TempoMap &tempo_map, QObject * parent)
    : ArrangerObject (
        ArrangerObject::Type::ScaleObject,
        tempo_map,
        ArrangerObjectFeatures::Mute,
        parent),
      scale_ (utils::make_qobject_unique<MusicalScale> (this))
{
}

void
init_from (
  ScaleObject           &obj,
  const ScaleObject     &other,
  utils::ObjectCloneType clone_type)
{
  init_from (*obj.scale_, *other.scale_, clone_type);
  init_from (
    static_cast<ArrangerObject &> (obj),
    static_cast<const ArrangerObject &> (other), clone_type);
}
}
