// SPDX-FileCopyrightText: © 2023-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/dsp/arranger_object.h"
#include "gui/dsp/audio_region.h"
#include "gui/dsp/automation_region.h"
#include "gui/dsp/chord_object.h"
#include "gui/dsp/chord_region.h"
#include "gui/dsp/marker.h"
#include "gui/dsp/midi_region.h"
#include "gui/dsp/region.h"
#include "gui/dsp/scale_object.h"

void
ArrangerObject::define_base_fields (const Context &ctx)
{
  using T = ISerializable<ArrangerObject>;
  T::serialize_fields (
    ctx, T::make_field ("type", type_), T::make_field ("flags", flags_),
    T::make_field ("trackNameHash", track_id_), T::make_field ("pos", pos_));
}

void
BoundedObject::define_base_fields (const Context &ctx)
{
  ISerializable<BoundedObject>::serialize_fields (
    ctx, ISerializable<BoundedObject>::make_field ("endPos", end_pos_));
}

void
LoopableObject::define_base_fields (const Context &ctx)
{
  ISerializable<LoopableObject>::serialize_fields (
    ctx,
    ISerializable<LoopableObject>::make_field ("clipStartPos", clip_start_pos_),
    ISerializable<LoopableObject>::make_field ("loopStartPos", loop_start_pos_),
    ISerializable<LoopableObject>::make_field ("loopEndPos", loop_end_pos_));
}

void
FadeableObject::define_base_fields (const Context &ctx)
{
  ISerializable<FadeableObject>::serialize_fields (
    ctx, ISerializable<FadeableObject>::make_field ("fadeInPos", fade_in_pos_),
    ISerializable<FadeableObject>::make_field ("fadeOutPos", fade_out_pos_),
    ISerializable<FadeableObject>::make_field ("fadeInOpts", fade_in_opts_),
    ISerializable<FadeableObject>::make_field ("fadeOutOpts", fade_out_opts_));
}

void
MuteableObject::define_base_fields (const Context &ctx)
{
  ISerializable<MuteableObject>::serialize_fields (
    ctx, ISerializable<MuteableObject>::make_field ("mute", muted_));
}

void
NamedObject::define_base_fields (const Context &ctx)
{
  ISerializable<NamedObject>::serialize_fields (
    ctx, ISerializable<NamedObject>::make_field ("name", name_));
}

void
ColoredObject::define_base_fields (const Context &ctx)
{
  ISerializable<ColoredObject>::serialize_fields (
    ctx, ISerializable<ColoredObject>::make_field ("color", color_),
    ISerializable<ColoredObject>::make_field ("useColor", use_color_));
}

void
Region::define_base_fields (const Context &ctx)
{
  using T = ISerializable<Region>;
  T::serialize_fields (ctx, T::make_field ("linkGroup", link_group_));
}

void
RegionOwnedObject::define_base_fields (const Context &ctx)
{
  using T = ISerializable<RegionOwnedObject>;
  T::serialize_fields (ctx, T::make_field ("regionId", region_id_));
}

void
MidiRegion::define_fields (const Context &ctx)
{
  using T = ISerializable<MidiRegion>;
  T::call_all_base_define_fields<
    ArrangerObject, BoundedObject, LoopableObject, MuteableObject, NamedObject,
    ColoredObject, Region> (ctx);
  T::serialize_fields (ctx, T::make_field ("midiNotes", midi_notes_));
}

void
Velocity::define_fields (const Context &ctx)
{
  ISerializable<Velocity>::call_all_base_define_fields<
    ArrangerObject, RegionOwnedObject> (ctx);
  ISerializable<Velocity>::serialize_fields (
    ctx, ISerializable<Velocity>::make_field ("velocity", vel_));
}

void
MidiNote::define_fields (const Context &ctx)
{
  using T = ISerializable<MidiNote>;
  T::call_all_base_define_fields<
    ArrangerObject, BoundedObject, MuteableObject, RegionOwnedObject> (ctx);
  T::serialize_fields (
    ctx, T::make_field ("velocity", vel_), T::make_field ("value", pitch_));
}

void
AutomationPoint::define_fields (const Context &ctx)
{
  ArrangerObject::define_base_fields (ctx);
  RegionOwnedObject::define_base_fields (ctx);

  using T = ISerializable<AutomationPoint>;
  T::serialize_fields (
    ctx, T::make_field ("fValue", fvalue_),
    T::make_field ("normalizedValue", normalized_val_),
    T::make_field ("curveOpts", curve_opts_));
}

void
ChordObject::define_fields (const Context &ctx)
{
  using T = ISerializable<ChordObject>;
  T::call_all_base_define_fields<
    ArrangerObject, MuteableObject, RegionOwnedObject> (ctx);
  ISerializable<ChordObject>::serialize_fields (
    ctx, ISerializable<ChordObject>::make_field ("chordIndex", chord_index_));
}

void
AudioRegion::define_fields (const Context &ctx)
{
  ArrangerObject::define_base_fields (ctx);
  BoundedObject::define_base_fields (ctx);
  LoopableObject::define_base_fields (ctx);
  FadeableObject::define_base_fields (ctx);
  MuteableObject::define_base_fields (ctx);
  NamedObject::define_base_fields (ctx);
  ColoredObject::define_base_fields (ctx);
  Region::define_base_fields (ctx);

  ISerializable<AudioRegion>::serialize_fields (
    ctx, ISerializable<AudioRegion>::make_field ("clipId", clip_id_),
    ISerializable<AudioRegion>::make_field ("gain", gain_));
}

void
ChordRegion::define_fields (const Context &ctx)
{
  ArrangerObject::define_base_fields (ctx);
  BoundedObject::define_base_fields (ctx);
  LoopableObject::define_base_fields (ctx);
  MuteableObject::define_base_fields (ctx);
  NamedObject::define_base_fields (ctx);
  ColoredObject::define_base_fields (ctx);
  Region::define_base_fields (ctx);

  ISerializable<ChordRegion>::serialize_fields (
    ctx,
    ISerializable<ChordRegion>::make_field ("chordObjects", chord_objects_));
}

void
AutomationRegion::define_fields (const Context &ctx)
{
  using T = ISerializable<AutomationRegion>;
  T::call_all_base_define_fields<
    ArrangerObject, BoundedObject, LoopableObject, MuteableObject, NamedObject,
    ColoredObject, Region> (ctx);

  T::serialize_fields (ctx, T::make_field ("automationPoints", aps_));
}

void
ScaleObject::define_fields (const Context &ctx)
{
  ArrangerObject::define_base_fields (ctx);
  MuteableObject::define_base_fields (ctx);

  ISerializable<ScaleObject>::serialize_fields (
    ctx, ISerializable<ScaleObject>::make_field ("index", index_in_chord_track_),
    ISerializable<ScaleObject>::make_field ("scale", scale_));
}

void
Marker::define_fields (const Context &ctx)
{
  ArrangerObject::define_base_fields (ctx);
  NamedObject::define_base_fields (ctx);

  ISerializable<Marker>::serialize_fields (
    ctx, ISerializable<Marker>::make_field ("markerType", marker_type_),
    ISerializable<Marker>::make_field ("markerTrackIndex", marker_track_index_));
}
