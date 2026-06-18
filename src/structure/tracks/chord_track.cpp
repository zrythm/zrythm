// SPDX-FileCopyrightText: © 2018-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>

#include "structure/tracks/chord_track.h"

namespace zrythm::structure::tracks
{

void
ChordTrack::transform_chord_and_append (
  dsp::MidiEventBuffer                               &dest,
  const dsp::MidiEventBuffer                         &src,
  const NotePitchToPitchesFunc                       &note_to_pitches,
  midi_byte_t                                         velocity,
  std::pair<units::sample_u32_t, units::sample_u32_t> range)
{
  const auto [start, end] = range;
  for (const auto &src_ev : src)
    {
      if (src_ev.time () < start || src_ev.time () >= end)
        continue;

      const auto data = src_ev.data ();
      if (
        !utils::midi::midi_is_note_on (data)
        && !utils::midi::midi_is_note_off (data))
        continue;

      const midi_byte_t note_number = utils::midi::midi_get_note_number (data);
      const auto        pitches_opt = note_to_pitches (note_number);
      if (!pitches_opt.has_value ())
        continue;

      if (utils::midi::midi_is_note_on (data))
        {
          for (auto pitch : *pitches_opt)
            {
              const auto ev = dsp::midi_event::make_note_on (
                0, pitch, velocity, src_ev.time ());
              if (
                dest.size_in_bytes () + dsp::MidiEventBuffer::kHeaderSize
                  + ev.data ().size ()
                > dest.capacity ())
                return;
              dest.push_back (ev.time_, ev.data ());
            }
        }
      else
        {
          for (auto pitch : *pitches_opt)
            {
              const auto ev =
                dsp::midi_event::make_note_off (0, pitch, src_ev.time ());
              if (
                dest.size_in_bytes () + dsp::MidiEventBuffer::kHeaderSize
                  + ev.data ().size ()
                > dest.capacity ())
                return;
              dest.push_back (ev.time_, ev.data ());
            }
        }
    }
}

ChordTrack::ChordTrack (FinalTrackDependencies dependencies)
    : Track (
        Track::Type::Chord,
        PortType::Midi,
        PortType::Midi,
        TrackFeatures::Automation | TrackFeatures::Recording,
        dependencies.to_base_dependencies ()),
      arrangement::ArrangerObjectOwner<ChordRegion> (dependencies.registry_, *this),
      arrangement::ArrangerObjectOwner<ScaleObject> (dependencies.registry_, *this)
{
  color_ = Color (QColor ("#1C8FFB"));
  icon_name_ = u8"gnome-icon-library-library-music-symbolic";

  processor_ = make_track_processor (
    std::nullopt, std::nullopt,
    [this] (
      auto &out_events, const auto &in_events,
      const dsp::graph::ProcessBlockInfo &time_nfo) {
      audition_state_.process (out_events, time_nfo);
      transform_chord_and_append (
        out_events, in_events,
        [this] (midi_byte_t note_number) {
          return note_pitch_to_pitches (note_number);
        },
        arrangement::MidiNote::DEFAULT_VELOCITY,
        std::pair{
          time_nfo.buffer_offset_, time_nfo.buffer_offset_ + time_nfo.nframes_ });
    });

  preview_timer_ = utils::make_qobject_unique<QTimer> ();
  preview_timer_->setSingleShot (true);
  QObject::connect (
    preview_timer_.get (), &QTimer::timeout, this, &ChordTrack::stopPreview);
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

auto
ChordTrack::chordAtTicks (double ticks) const -> ChordObject *
{
  return get_chord_at_ticks (units::ticks (ticks));
}

auto
ChordTrack::scaleAtTicks (double ticks) const -> ScaleObject *
{
  return get_scale_at_ticks (units::ticks (ticks));
}

void
ChordTrack::auditionChord (
  const zrythm::dsp::ChordDescriptor * descriptor,
  bool                                 on,
  midi_byte_t                          velocity)
{
  if (on && descriptor != nullptr)
    audition_state_.start (*descriptor, velocity);
  else if (!on && descriptor != nullptr)
    audition_state_.stop (*descriptor);
  else
    audition_state_.stopAll ();
}

void
ChordTrack::previewChord (
  const zrythm::dsp::ChordDescriptor * descriptor,
  int                                  duration_ms)
{
  if (descriptor == nullptr)
    return;

  if (preview_timer_->isActive ())
    audition_state_.stop (previewing_pitches_);

  previewing_pitches_ = descriptor->getMidiPitches ();
  audition_state_.start (*descriptor);
  preview_timer_->start (duration_ms);
}

void
ChordTrack::stopPreview ()
{
  audition_state_.stop (previewing_pitches_);
}

}
