// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <utility>
#include <variant>

#include "utils/debug.h"
#include "utils/units.h"
#include "utils/uuid_identifiable_object.h"

namespace zrythm::structure::arrangement
{
class ArrangerObject;
class Clip;
class MidiControlEvent;
class MidiNote;
class MidiClip;
class AudioClip;
class AutomationClip;
class ChordClip;
class ChordObject;
class ScaleObject;
class AutomationPoint;
class Marker;
class AudioSourceObject;
class TempoObject;
class TimeSignatureObject;

template <typename T>
concept ClipObject =
  std::is_same_v<T, AudioClip> || std::is_same_v<T, MidiClip>
  || std::is_same_v<T, AutomationClip> || std::is_same_v<T, ChordClip>;

template <typename T>
concept TimelineObject =
  ClipObject<T> || std::is_same_v<T, ScaleObject> || std::is_same_v<T, Marker>
  || std::is_same_v<T, TempoObject> || std::is_same_v<T, TimeSignatureObject>;

template <typename T>
concept LaneOwnedObject =
  std::is_same_v<T, MidiClip> || std::is_same_v<T, AudioClip>;

template <typename T>
concept FadeableObject = std::is_same_v<T, AudioClip>;

template <typename T>
concept NamedObject = ClipObject<T> || std::is_same_v<T, Marker>;

template <typename T>
concept BoundedObject = ClipObject<T> || std::is_same_v<T, MidiNote>;

template <typename T>
concept EditorObject =
  std::is_same_v<T, MidiControlEvent> || std::is_same_v<T, MidiNote>
  || std::is_same_v<T, AutomationPoint> || std::is_same_v<T, ChordObject>
  || std::is_same_v<T, AudioSourceObject>;

using ArrangerObjectVariant = std::variant<
  MidiNote,
  ChordObject,
  ScaleObject,
  MidiClip,
  AudioClip,
  ChordClip,
  AutomationClip,
  AutomationPoint,
  Marker,
  AudioSourceObject,
  TempoObject,
  TimeSignatureObject,
  MidiControlEvent>;
using ArrangerObjectPtrVariant =
  utils::to_pointer_variant<ArrangerObjectVariant>;

template <typename Variant, std::size_t... Is>
consteval bool
each_alternative_has_exactly_one_position_space (std::index_sequence<Is...>)
{
  return (
    (TimelineObject<std::variant_alternative_t<Is, Variant>>
     != EditorObject<std::variant_alternative_t<Is, Variant>>)
    && ...);
}

// Every object type lives in exactly one position space (timeline or
// clip-relative); operations that keep the two spaces apart rely on this
static_assert (
  each_alternative_has_exactly_one_position_space<ArrangerObjectVariant> (
    std::make_index_sequence<std::variant_size_v<ArrangerObjectVariant>>{}),
  "every arranger object type is a timeline or an editor object");

using ArrangerObjectUuid = utils::UuidIdentifiableObject<ArrangerObject>::Uuid;

} // namespace zrythm::structure::arrangement

DEFINE_UUID_HASH_SPECIALIZATION (
  zrythm::structure::arrangement::ArrangerObjectUuid)
