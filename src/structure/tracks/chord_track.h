// SPDX-FileCopyrightText: © 2018-2020, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/arranger_object_owner.h"
#include "structure/arrangement/chord_region.h"
#include "structure/arrangement/scale_object.h"
#include "structure/tracks/track.h"

namespace zrythm::structure::tracks
{
/**
 * The ChordTrack class is responsible for managing the chord and scale
 * information in the project.
 */
class ChordTrack
    : public Track,
      public arrangement::ArrangerObjectOwner<arrangement::ChordRegion>,
      public arrangement::ArrangerObjectOwner<arrangement::ScaleObject>
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")
  DEFINE_ARRANGER_OBJECT_OWNER_QML_PROPERTIES (
    ChordTrack,
    chordRegions,
    zrythm::structure::arrangement::ChordRegion)
  DEFINE_ARRANGER_OBJECT_OWNER_QML_PROPERTIES (
    ChordTrack,
    scaleObjects,
    zrythm::structure::arrangement::ScaleObject)

public:
  using ScaleObject = arrangement::ScaleObject;
  using ChordRegion = arrangement::ChordRegion;
  using ChordObject = arrangement::ChordObject;
  using ScaleObjectPtr = ScaleObject *;
  using NotePitchToChordDescriptorFunc =
    std::function<const dsp::ChordDescriptor *(midi_byte_t)>;

  ChordTrack (FinalTrackDependencies dependencies);

  // ========================================================================
  // QML Interface
  // ========================================================================

  // ========================================================================

  const dsp::ChordDescriptor *
  note_pitch_to_chord_descriptor (midi_byte_t note_pitch) const
  {
    assert (note_pitch_to_descriptor_.has_value ());
    return std::invoke (*note_pitch_to_descriptor_, note_pitch);
  }

  // FIXME: eventually this dependency should be injected via a constructor
  // argument
  void set_note_pitch_to_descriptor_func (NotePitchToChordDescriptorFunc func)
  {
    note_pitch_to_descriptor_ = func;
  }

  ScaleObject * get_scale_at (size_t index) const;

  /**
   * @brief Transforms note-on/off events into chord events.
   *
   * For each note-on/off in @p src within the given @p range, looks up the
   * corresponding chord descriptor via @p note_to_chord. If found, expands
   * the single note into all notes of the chord.
   *
   * @param dest Destination buffer.
   * @param src Source events.
   * @param note_to_chord Maps a MIDI note number to a ChordDescriptor.
   * @param velocity Velocity to use for generated note-ons.
   * @param range Time range [start, end) to filter events.
   */
  static void transform_chord_and_append (
    dsp::MidiEventBuffer                               &dest,
    const dsp::MidiEventBuffer                         &src,
    const NotePitchToChordDescriptorFunc               &note_to_chord,
    midi_byte_t                                         velocity,
    std::pair<units::sample_u32_t, units::sample_u32_t> range);

  /**
   * Returns the ChordObject at the given Position
   * in the TimelineArranger.
   */
  ChordObject * get_chord_at_ticks (units::precise_tick_t timeline_ticks) const;

  /**
   * Returns the ScaleObject at the given Position
   * in the TimelineArranger.
   */
  ScaleObject * get_scale_at_ticks (units::precise_tick_t timeline_ticks) const;

  friend void init_from (
    ChordTrack            &obj,
    const ChordTrack      &other,
    utils::ObjectCloneType clone_type);

  std::string
  get_field_name_for_serialization (const ChordRegion *) const override
  {
    return "regions";
  }
  std::string
  get_field_name_for_serialization (const ScaleObject *) const override
  {
    return "scales";
  }

private:
  friend void to_json (nlohmann::json &j, const ChordTrack &track)
  {
    to_json (j, static_cast<const Track &> (track));
    to_json (j, static_cast<const ArrangerObjectOwner<ChordRegion> &> (track));
    to_json (j, static_cast<const ArrangerObjectOwner<ScaleObject> &> (track));
  }
  friend void from_json (const nlohmann::json &j, ChordTrack &track)
  {
    from_json (j, static_cast<Track &> (track));
    from_json (j, static_cast<ArrangerObjectOwner<ChordRegion> &> (track));
    from_json (j, static_cast<ArrangerObjectOwner<ScaleObject> &> (track));
  }

  bool initialize ();

private:
  std::optional<NotePitchToChordDescriptorFunc> note_pitch_to_descriptor_;
};

} // namespace zrythm::structure::tracks
