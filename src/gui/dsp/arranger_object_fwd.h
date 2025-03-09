#pragma once

class MidiNote;
class MidiRegion;
class AudioRegion;
class AutomationRegion;
class ChordRegion;
class ChordObject;
class ScaleObject;
class AutomationPoint;
class Marker;
class Velocity;
using ArrangerObjectVariant = std::variant<
  MidiNote,
  ChordObject,
  ScaleObject,
  MidiRegion,
  AudioRegion,
  ChordRegion,
  AutomationRegion,
  AutomationPoint,
  Marker,
  Velocity>;
using ArrangerObjectWithoutVelocityVariant = std::variant<
  MidiNote,
  ChordObject,
  ScaleObject,
  MidiRegion,
  AudioRegion,
  ChordRegion,
  AutomationRegion,
  Marker,
  AutomationPoint>;
using ArrangerObjectPtrVariant = to_pointer_variant<ArrangerObjectVariant>;
using ArrangerObjectWithoutVelocityPtrVariant =
  to_pointer_variant<ArrangerObjectWithoutVelocityVariant>;
