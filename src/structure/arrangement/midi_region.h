// SPDX-FileCopyrightText: © 2019-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/midi_event.h"
#include "structure/arrangement/arranger_object_owner.h"
#include "structure/arrangement/midi_note.h"

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
  DEFINE_ARRANGER_OBJECT_OWNER_QML_PROPERTIES (MidiRegion, midiNotes, MidiNote)
  Q_PROPERTY (int minVisiblePitch READ minVisiblePitch NOTIFY contentChanged)
  Q_PROPERTY (int maxVisiblePitch READ maxVisiblePitch NOTIFY contentChanged)
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

  int           minVisiblePitch () const;
  int           maxVisiblePitch () const;
  Q_SIGNAL void contentChanged ();

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

  friend void to_json (nlohmann::json &j, const MidiRegion &region)
  {
    to_json (j, static_cast<const ArrangerObject &> (region));
    to_json (j, static_cast<const ArrangerObjectOwner &> (region));
  }
  friend void from_json (const nlohmann::json &j, MidiRegion &region)
  {
    from_json (j, static_cast<ArrangerObject &> (region));
    from_json (j, static_cast<ArrangerObjectOwner &> (region));
  }

private:
  BOOST_DESCRIBE_CLASS (
    MidiRegion,
    (ArrangerObject, ArrangerObjectOwner<MidiNote>),
    (),
    (),
    ())
};
}
