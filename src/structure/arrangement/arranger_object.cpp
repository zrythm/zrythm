// SPDX-FileCopyrightText: © 2019-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/arranger_object.h"

namespace zrythm::structure::arrangement
{

ArrangerObject::ArrangerObject (
  Type                   type,
  const dsp::TempoMap   &tempo_map,
  ArrangerObjectFeatures features,
  QObject *              parent) noexcept
    : QObject (parent), type_ (type), position_ (tempo_map),
      position_adapter_ (
        utils::make_qobject_unique<dsp::AtomicPositionQmlAdapter> (position_, this))
{
  if (ENUM_BITSET_TEST (features, ArrangerObjectFeatures::Bounds))
    {
      bounds_ =
        utils::make_qobject_unique<ArrangerObjectBounds> (*position (), this);
      QObject::connect (
        bounds ()->length (), &dsp::AtomicPositionQmlAdapter::positionChanged,
        this, &ArrangerObject::propertiesChanged);
    }
  if (ENUM_BITSET_TEST (features, ArrangerObjectFeatures::LoopingBit))
    {
      loop_range_ =
        utils::make_qobject_unique<ArrangerObjectLoopRange> (*bounds (), this);
      QObject::connect (
        loopRange (), &ArrangerObjectLoopRange::loopableObjectPropertiesChanged,
        this, &ArrangerObject::propertiesChanged);
    }
  if (ENUM_BITSET_TEST (features, ArrangerObjectFeatures::Name))
    {
      name_ = utils::make_qobject_unique<ArrangerObjectName> (this);
      QObject::connect (
        name (), &ArrangerObjectName::nameChanged, this,
        &ArrangerObject::propertiesChanged);
    }
  if (ENUM_BITSET_TEST (features, ArrangerObjectFeatures::Color))
    {
      color_ = utils::make_qobject_unique<ArrangerObjectColor> (this);
      QObject::connect (
        color (), &ArrangerObjectColor::colorChanged, this,
        &ArrangerObject::propertiesChanged);
      QObject::connect (
        color (), &ArrangerObjectColor::useColorChanged, this,
        &ArrangerObject::propertiesChanged);
    }
  if (ENUM_BITSET_TEST (features, ArrangerObjectFeatures::Mute))
    {
      mute_ = utils::make_qobject_unique<ArrangerObjectMuteFunctionality> (this);
      QObject::connect (
        mute (), &ArrangerObjectMuteFunctionality::mutedChanged, this,
        &ArrangerObject::propertiesChanged);
    }
  if (ENUM_BITSET_TEST (features, ArrangerObjectFeatures::Fading))
    {
      fade_range_ =
        utils::make_qobject_unique<ArrangerObjectFadeRange> (tempo_map, this);
      QObject::connect (
        fadeRange (), &ArrangerObjectFadeRange::fadePropertiesChanged, this,
        &ArrangerObject::propertiesChanged);
    }

  QObject::connect (
    position (), &dsp::AtomicPositionQmlAdapter::positionChanged, this,
    &ArrangerObject::propertiesChanged);
}

void
init_from (
  ArrangerObject        &obj,
  const ArrangerObject  &other,
  utils::ObjectCloneType clone_type)
{
  init_from (
    static_cast<ArrangerObject::UuidIdentifiableObject &> (obj),
    static_cast<const ArrangerObject::UuidIdentifiableObject &> (other),
    clone_type);
  obj.position_.set_ticks (other.position_.get_ticks ());
  if (other.bounds_)
    {
      init_from (*obj.bounds_, *other.bounds_, clone_type);
    }
  if (other.loop_range_)
    init_from (*obj.loop_range_, *other.loop_range_, clone_type);
  if (other.name_)
    init_from (*obj.name_, *other.name_, clone_type);
  if (other.color_)
    init_from (*obj.color_, *other.color_, clone_type);
  if (other.mute_)
    init_from (*obj.mute_, *other.mute_, clone_type);
  if (other.fade_range_)
    init_from (*obj.fade_range_, *other.fade_range_, clone_type);
}

void
to_json (nlohmann::json &j, const ArrangerObject &arranger_object)
{
  to_json (
    j,
    static_cast<const ArrangerObject::UuidIdentifiableObject &> (
      arranger_object));
  j[ArrangerObject::kPositionKey] = arranger_object.position_;
  j[ArrangerObject::kBoundsKey] = arranger_object.bounds_;
  j[ArrangerObject::kLoopRangeKey] = arranger_object.loop_range_;
  j[ArrangerObject::kFadeRangeKey] = arranger_object.fade_range_;
  j[ArrangerObject::kNameKey] = arranger_object.name_;
  j[ArrangerObject::kColorKey] = arranger_object.color_;
  j[ArrangerObject::kMuteKey] = arranger_object.mute_;
}

void
from_json (const nlohmann::json &j, ArrangerObject &arranger_object)
{
  from_json (
    j, static_cast<ArrangerObject::UuidIdentifiableObject &> (arranger_object));
  j.at (ArrangerObject::kPositionKey).get_to (arranger_object.position_);
  if (arranger_object.bounds_)
    {
      j.at (ArrangerObject::kBoundsKey).get_to (*arranger_object.bounds_);
    }
  if (arranger_object.loop_range_)
    j.at (ArrangerObject::kLoopRangeKey).get_to (*arranger_object.loop_range_);
  if (arranger_object.name_)
    j.at (ArrangerObject::kNameKey).get_to (*arranger_object.name_);
  if (arranger_object.color_)
    j.at (ArrangerObject::kColorKey).get_to (*arranger_object.color_);
  if (arranger_object.mute_)
    j.at (ArrangerObject::kMuteKey).get_to (*arranger_object.mute_);
  if (arranger_object.fade_range_)
    j.at (ArrangerObject::kFadeRangeKey).get_to (*arranger_object.fade_range_);
}

} // namespace zrythm::structure:arrangement
