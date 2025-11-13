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
ScaleObject::setScale (MusicalScale * scale)
{
  if (scale == nullptr || scale == scale_.get ())
    return;

  scale->setParent (this);
  scale_ = scale;
  Q_EMIT scaleChanged (scale);
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

void
to_json (nlohmann::json &j, const ScaleObject &so)
{
  to_json (j, static_cast<const ArrangerObject &> (so));
  j[ScaleObject::kScaleKey] = so.scale_;
}
void
from_json (const nlohmann::json &j, ScaleObject &so)
{
  from_json (j, static_cast<ArrangerObject &> (so));
  j.at (ScaleObject::kScaleKey).get_to (*so.scale_);
}
}
