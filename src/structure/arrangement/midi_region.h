// SPDX-FileCopyrightText: © 2019-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/midi_event.h"
#include "structure/arrangement/arranger_object_owner.h"
#include "structure/arrangement/midi_note.h"
#include "structure/arrangement/region.h"

namespace zrythm::structure::arrangement
{
/**
 * @brief A Region containing MIDI events.
 *
 * MidiRegion represents a region in the timeline that holds MIDI note and
 * controller data. It is specific to instrument/MIDI tracks and can be
 * constructed from a MIDI file or a chord descriptor.
 */
class MidiRegion final
    : public QObject,
      public ArrangerObject,
      public ArrangerObjectOwner<MidiNote>
{
  Q_OBJECT
  DEFINE_ARRANGER_OBJECT_QML_PROPERTIES (MidiRegion)
  Q_PROPERTY (RegionMixin * regionMixin READ regionMixin CONSTANT)
  DEFINE_ARRANGER_OBJECT_OWNER_QML_PROPERTIES (MidiRegion, midiNotes, MidiNote)
  QML_ELEMENT

  friend class ArrangerObjectFactory;

public:
  MidiRegion (
    const dsp::TempoMap          &tempo_map,
    ArrangerObjectRegistry       &object_registry,
    dsp::FileAudioSourceRegistry &file_audio_source_registry,
    QObject *                     parent = nullptr);

  // ========================================================================
  // QML Interface
  // ========================================================================

  RegionMixin * regionMixin () const { return region_mixin_.get (); }

  // ========================================================================

  /**
   * Adds the contents of the region converted into events.
   *
   * @note Event timings will be set in ticks.
   *
   * @param add_region_start Add the region start offset to the positions, so
   * that the event timings would be as they would be played from the start of
   * the whole timeline.
   * @param full Traverse loops and export the MIDI file as it would be played
   * inside Zrythm. If this is false, only the original region (from true start
   * to true end) is exported.
   * @param start Optional global start position in ticks to start adding events
   * from. Events before this position will be skipped.
   * @param end Optional global end position in ticks to stop adding events
   * from. Events after this position will be skipped.
   */
  void add_midi_region_events (
    dsp::MidiEventVector &events,
    std::optional<double> start,
    std::optional<double> end,
    bool                  add_region_start,
    bool                  full) const;

  std::string get_field_name_for_serialization (const MidiNote *) const override
  {
    return "midiNotes";
  }

private:
  friend void init_from (
    MidiRegion            &obj,
    const MidiRegion      &other,
    utils::ObjectCloneType clone_type);

  static constexpr auto kRegionMixinKey = "regionMixin"sv;
  friend void           to_json (nlohmann::json &j, const MidiRegion &region)
  {
    to_json (j, static_cast<const ArrangerObject &> (region));
    j[kRegionMixinKey] = region.region_mixin_;
    to_json (j, static_cast<const ArrangerObjectOwner &> (region));
  }
  friend void from_json (const nlohmann::json &j, MidiRegion &region)
  {
    from_json (j, static_cast<ArrangerObject &> (region));
    j.at (kRegionMixinKey).get_to (*region.region_mixin_);
    from_json (j, static_cast<ArrangerObjectOwner &> (region));
  }

private:
  /**
   * @brief Common region functionality.
   */
  utils::QObjectUniquePtr<RegionMixin> region_mixin_;

  BOOST_DESCRIBE_CLASS (
    MidiRegion,
    (ArrangerObject, ArrangerObjectOwner<MidiNote>),
    (),
    (),
    (region_mixin_))
};

/**
 * @brief Generates a filename for the given MIDI region.
 *
 * @param track The track that owns the region.
 * @param mr The MIDI region.
 */
constexpr auto
generate_filename_for_midi_region (const auto &track, const MidiRegion &mr)
{
  return fmt::format (
    "{}_{}.mid", track.get_name (), mr.regionMixin ()->name ()->get_name ());
}

}
