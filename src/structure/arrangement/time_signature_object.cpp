// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/time_signature_object.h"

namespace zrythm::structure::arrangement
{
TimeSignatureObject::TimeSignatureObject (
  const dsp::TempoMap &tempo_map,
  QObject *            parent)
    : ArrangerObject (ArrangerObject::Type::TimeSignatureObject, tempo_map, {}, parent)
{
  QObject::connect (
    this, &TimeSignatureObject::numeratorChanged, this,
    &ArrangerObject::propertiesChanged);
  QObject::connect (
    this, &TimeSignatureObject::denominatorChanged, this,
    &ArrangerObject::propertiesChanged);
}

void
TimeSignatureObject::setNumerator (int numerator)
{
  if (numerator == numerator_)
    return;

  numerator_ = numerator;
  Q_EMIT numeratorChanged (numerator);
}

void
TimeSignatureObject::setDenominator (int denominator)
{
  if (denominator == denominator_)
    return;

  denominator_ = denominator;
  Q_EMIT denominatorChanged (denominator);
}

void
init_from (
  TimeSignatureObject       &obj,
  const TimeSignatureObject &other,
  utils::ObjectCloneType     clone_type)
{
  obj.numerator_ = other.numerator_;
  obj.denominator_ = other.denominator_;
  init_from (
    static_cast<ArrangerObject &> (obj),
    static_cast<const ArrangerObject &> (other), clone_type);
}

void
to_json (nlohmann::json &j, const TimeSignatureObject &so)
{
  to_json (j, static_cast<const ArrangerObject &> (so));
  j[TimeSignatureObject::kNumeratorKey] = so.numerator_;
  j[TimeSignatureObject::kDenominatorKey] = so.denominator_;
}
void
from_json (const nlohmann::json &j, TimeSignatureObject &so)
{
  from_json (j, static_cast<ArrangerObject &> (so));
  j.at (TimeSignatureObject::kNumeratorKey).get_to (so.numerator_);
  j.at (TimeSignatureObject::kDenominatorKey).get_to (so.denominator_);
}
}
