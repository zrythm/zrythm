// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/uuid_identifiable_object.h"

class ArrangerObject;
class MidiNote;
class MidiRegion;
class AudioRegion;
class AutomationRegion;
class ChordRegion;
class ChordObject;
class ScaleObject;
class AutomationPoint;
class Marker;
using ArrangerObjectVariant = std::variant<
  MidiNote,
  ChordObject,
  ScaleObject,
  MidiRegion,
  AudioRegion,
  ChordRegion,
  AutomationRegion,
  AutomationPoint,
  Marker>;
using ArrangerObjectPtrVariant = to_pointer_variant<ArrangerObjectVariant>;

using ArrangerObjectUuid = utils::UuidIdentifiableObject<ArrangerObject>::Uuid;

DEFINE_UUID_HASH_SPECIALIZATION (ArrangerObjectUuid)