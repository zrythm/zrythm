// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/tempo_object.h"

namespace zrythm::structure::arrangement
{
TempoObject::TempoObject (const dsp::TempoMap &tempo_map, QObject * parent)
    : ArrangerObject (ArrangerObject::Type::TempoObject, tempo_map, {}, parent)
{
  QObject::connect (
    this, &TempoObject::tempoChanged, this, &ArrangerObject::propertiesChanged);
  QObject::connect (
    this, &TempoObject::curveChanged, this, &ArrangerObject::propertiesChanged);
}

void
TempoObject::setTempo (double tempo)
{
  if (qFuzzyCompare (tempo, tempo_))
    return;

  tempo_ = tempo;
  Q_EMIT tempoChanged (tempo);
}

void
TempoObject::setCurve (CurveType curve)
{
  if (curve == curve_)
    return;

  curve_ = curve;
  Q_EMIT curveChanged (curve);
}

void
init_from (
  TempoObject           &obj,
  const TempoObject     &other,
  utils::ObjectCloneType clone_type)
{
  obj.tempo_ = other.tempo_;
  obj.curve_ = other.curve_;
  init_from (
    static_cast<ArrangerObject &> (obj),
    static_cast<const ArrangerObject &> (other), clone_type);
}

void
to_json (nlohmann::json &j, const TempoObject &so)
{
  to_json (j, static_cast<const ArrangerObject &> (so));
  j[TempoObject::kTempoKey] = so.tempo_;
  j[TempoObject::kCurveTypeKey] = so.curve_;
}
void
from_json (const nlohmann::json &j, TempoObject &so)
{
  from_json (j, static_cast<ArrangerObject &> (so));
  j.at (TempoObject::kTempoKey).get_to (so.tempo_);
  j.at (TempoObject::kCurveTypeKey).get_to (so.curve_);
}
}
