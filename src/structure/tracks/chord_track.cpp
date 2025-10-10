// SPDX-FileCopyrightText: © 2018-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>

#include "structure/tracks/chord_track.h"

namespace zrythm::structure::tracks
{
ChordTrack::ChordTrack (FinalTrackDependencies dependencies)
    : Track (
        Track::Type::Chord,
        PortType::Midi,
        PortType::Midi,
        TrackFeatures::Automation | TrackFeatures::Recording,
        dependencies.to_base_dependencies ()),
      arrangement::ArrangerObjectOwner<ChordRegion> (
        dependencies.obj_registry_,
        dependencies.file_audio_source_registry_,
        *this),
      arrangement::ArrangerObjectOwner<ScaleObject> (
        dependencies.obj_registry_,
        dependencies.file_audio_source_registry_,
        *this)
{
  color_ = Color (QColor ("#1C8FFB"));
  icon_name_ = u8"gnome-icon-library-library-music-symbolic";

  processor_ = make_track_processor (
    std::nullopt, std::nullopt,
    [this] (
      dsp::MidiEventVector &out_events, const dsp::MidiEventVector &in_events,
      const EngineProcessTimeInfo &time_nfo) {
      out_events.transform_chord_and_append (
        in_events,
        [this] (midi_byte_t note_number) {
          return note_pitch_to_chord_descriptor (note_number);
        },
        arrangement::MidiNote::DEFAULT_VELOCITY, time_nfo.local_offset_,
        time_nfo.nframes_);
    });
}

void
init_from (
  ChordTrack            &obj,
  const ChordTrack      &other,
  utils::ObjectCloneType clone_type)
{
  init_from (
    static_cast<Track &> (obj), static_cast<const Track &> (other), clone_type);
  init_from (
    static_cast<arrangement::ArrangerObjectOwner<arrangement::ChordRegion> &> (
      obj),
    static_cast<const arrangement::ArrangerObjectOwner<
      arrangement::ChordRegion> &> (other),
    clone_type);
  init_from (
    static_cast<arrangement::ArrangerObjectOwner<arrangement::ScaleObject> &> (
      obj),
    static_cast<const arrangement::ArrangerObjectOwner<
      arrangement::ScaleObject> &> (other),
    clone_type);
}

void
ChordTrack::set_playback_caches ()
{
// FIXME: TODO
#if 0
  region_snapshots_.clear ();
  region_snapshots_.reserve (region_list_->regions_.size ());
  foreach_region ([&] (auto &region) {
    z_return_if_fail (region.track_id_ == get_uuid ());
    region_snapshots_.push_back (region.clone_unique ());
  });

  scale_snapshots_.clear ();
  scale_snapshots_.reserve (scales_.size ());
  for (const auto &scale : get_scales_view ())
    {
      scale_snapshots_.push_back (scale->clone_unique ());
    }
#endif
}

auto
ChordTrack::get_scale_at (size_t index) const -> ScaleObject *
{
  return ArrangerObjectOwner<ScaleObject>::get_children_view ()[index];
}

auto
ChordTrack::get_scale_at_ticks (units::precise_tick_t timeline_ticks) const
  -> ScaleObject *
{
  auto view = std::ranges::reverse_view (
    ArrangerObjectOwner<ScaleObject>::get_children_view ());
  auto it = std::ranges::find_if (view, [timeline_ticks] (const auto &scale) {
    return units::ticks (scale->position ()->ticks ()) <= timeline_ticks;
  });

  return it != view.end () ? (*it) : nullptr;
}

auto
ChordTrack::get_chord_at_ticks (units::precise_tick_t timeline_ticks) const
  -> ChordObject *
{
  const auto timeline_frames =
    base_dependencies_.tempo_map_.get_tempo_map ().tick_to_samples_rounded (
      timeline_ticks);

  auto chord_regions_view = arrangement::ArrangerObjectOwner<
    arrangement::ChordRegion>::get_children_view ();
  auto region_var = std::ranges::find_if (
    chord_regions_view, [timeline_frames] (const auto &chord_region) {
      return chord_region->bounds ()->is_hit (timeline_frames, false);
    });
  if (region_var == chord_regions_view.end ())
    return nullptr;

  const auto * region = *region_var;

  const auto local_frames =
    timeline_frames_to_local (*region, timeline_frames, true);

  auto chord_objects_view = region->get_children_view () | std::views::reverse;
  auto it =
    std::ranges::find_if (chord_objects_view, [local_frames] (const auto &co) {
      return units::samples (co->position ()->samples ()) <= local_frames;
    });

  return it != chord_objects_view.end () ? (*it) : nullptr;
}

}
