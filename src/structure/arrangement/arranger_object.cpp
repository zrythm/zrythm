// SPDX-FileCopyrightText: © 2019-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/arranger_object.h"
#include "utils/serialization.h"

namespace zrythm::structure::arrangement
{

namespace
{

utils::QObjectUniquePtr<dsp::Position>
make_position (bool clip_owned, QObject * parent)
{
  if (clip_owned)
    return utils::make_qobject_unique<dsp::ContentPosition> (parent);
  return utils::make_qobject_unique<dsp::TimelinePosition> (parent);
}

} // namespace

ArrangerObject::ArrangerObject (
  Type                        type,
  const dsp::TempoMapWrapper &tempo_map_wrapper,
  ArrangerObjectFeatures      features,
  QObject *                   parent) noexcept
    : utils::UuidIdentifiableObject<ArrangerObject> (parent), type_ (type),
      tempo_map_wrapper_ (tempo_map_wrapper),
      position_ (make_position (
        ENUM_BITSET_TEST (features, ArrangerObjectFeatures::ClipOwned),
        this))
{
  if (ENUM_BITSET_TEST (features, ArrangerObjectFeatures::Bounds))
    {
      length_ = utils::make_qobject_unique<dsp::ContentPosition> (
        [] (units::precise_tick_t ticks) {
          return max (ticks, units::ticks (1.0));
        },
        this);
      QObject::connect (
        length_.get (), &dsp::Position::positionChanged, this,
        &ArrangerObject::propertiesChanged);
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

  QObject::connect (
    position (), &dsp::Position::positionChanged, this,
    &ArrangerObject::propertiesChanged);
  QObject::connect (
    this, &ArrangerObject::parentObjectChanged, this,
    &ArrangerObject::propertiesChanged);
}

const dsp::TempoMap &
ArrangerObject::get_tempo_map () const
{
  return tempo_map_wrapper_.get_tempo_map ();
}

// ========================================================================
// QML Interface
// ========================================================================

void
ArrangerObject::setParentObject (ArrangerObject * object)
{
  if (parent_object_.get () == object)
    return;

  parent_object_ = object;
  Q_EMIT parentObjectChanged (parent_object_);
}

// ========================================================================

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
  obj.position_->setTicks (other.position_->ticks ());
  if (other.length_)
    obj.length_->setTicks (other.length_->ticks ());
  if (other.name_)
    init_from (*obj.name_, *other.name_, clone_type);
  if (other.color_)
    init_from (*obj.color_, *other.color_, clone_type);
  if (other.mute_)
    init_from (*obj.mute_, *other.mute_, clone_type);
}

void
to_json (nlohmann::json &j, const ArrangerObject &arranger_object)
{
  to_json (
    j,
    static_cast<const ArrangerObject::UuidIdentifiableObject &> (
      arranger_object));
  j[ArrangerObject::kPositionKey] = *arranger_object.position_;
  if (arranger_object.length_)
    j[ArrangerObject::kLengthKey] = *arranger_object.length_;
  if (arranger_object.name_)
    j[ArrangerObject::kNameKey] = *arranger_object.name_;
  if (arranger_object.color_ && arranger_object.color ()->useColor ())
    to_json (j, *arranger_object.color_);
  if (arranger_object.mute_)
    to_json (j, *arranger_object.mute_);
}

void
from_json (const nlohmann::json &j, ArrangerObject &arranger_object)
{
  from_json (
    j, static_cast<ArrangerObject::UuidIdentifiableObject &> (arranger_object));
  j.at (ArrangerObject::kPositionKey).get_to (*arranger_object.position_);
  if (arranger_object.length_)
    j.at (ArrangerObject::kLengthKey).get_to (*arranger_object.length_);
  if (arranger_object.name_)
    j.at (ArrangerObject::kNameKey).get_to (*arranger_object.name_);
  if (arranger_object.color_ && j.contains (ArrangerObject::kColorKey))
    from_json (j, *arranger_object.color_);
  if (arranger_object.mute_)
    from_json (j, *arranger_object.mute_);
}

ArrangerObject::~ArrangerObject () noexcept = default;

} // namespace zrythm::structure:arrangement
