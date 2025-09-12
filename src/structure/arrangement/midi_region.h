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
class MidiRegion final : public ArrangerObject, public ArrangerObjectOwner<MidiNote>
{
  Q_OBJECT
  Q_PROPERTY (RegionMixin * regionMixin READ regionMixin CONSTANT)
  DEFINE_ARRANGER_OBJECT_OWNER_QML_PROPERTIES (MidiRegion, midiNotes, MidiNote)
  QML_ELEMENT
  QML_UNCREATABLE ("")

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
}
