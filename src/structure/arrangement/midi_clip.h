// SPDX-FileCopyrightText: © 2019-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/midi_event.h"
#include "dsp/timebase.h"
#include "structure/arrangement/arranger_object_owner.h"
#include "structure/arrangement/clip.h"
#include "structure/arrangement/midi_control_event.h"
#include "structure/arrangement/midi_note.h"

namespace zrythm::structure::arrangement
{
/**
 * @brief A Clip containing MIDI events.
 *
 * MidiClip represents a clip in the timeline that holds MIDI note and
 * controller data. It is specific to instrument/MIDI tracks and can be
 * constructed from a MIDI file or a chord descriptor.
 */
class MidiClip final
    : public Clip,
      public ArrangerObjectOwner<MidiNote>,
      public ArrangerObjectOwner<MidiControlEvent>
{
  Q_OBJECT
  DEFINE_ARRANGER_OBJECT_OWNER_QML_PROPERTIES (MidiClip, midiNotes, MidiNote)
  DEFINE_ARRANGER_OBJECT_OWNER_QML_PROPERTIES (
    MidiClip,
    midiControlEvents,
    MidiControlEvent)
  QML_ELEMENT
  QML_UNCREATABLE ("")

  friend class ArrangerObjectFactory;

public:
  MidiClip (
    const dsp::TempoMapWrapper &tempo_map_wrapper,
    utils::IObjectRegistry     &object_registry,
    QObject *                   parent = nullptr);

  // ========================================================================
  // QML Interface
  // ========================================================================

  Q_SIGNAL void contentChanged ();

  // ========================================================================

  /// Source tempo for Absolute-mode anchor (default 0 = no anchor).
  /// Absolute mode is a no-op while this is 0.
  [[nodiscard]] units::bpm_t source_bpm () const { return source_bpm_; }
  void                       set_source_bpm (units::bpm_t bpm);

  // ========================================================================

  std::string get_field_name_for_serialization (const MidiNote *) const override
  {
    return "midiNotes";
  }
  std::string
  get_field_name_for_serialization (const MidiControlEvent *) const override
  {
    return "midiControlEvents";
  }

  std::vector<ArrangerObjectListModel *> get_child_list_models () const override
  {
    return {
      ArrangerObjectOwner<MidiNote>::get_model (),
      ArrangerObjectOwner<MidiControlEvent>::get_model (),
    };
  }

private:
  friend void init_from (
    MidiClip              &obj,
    const MidiClip        &other,
    utils::ObjectCloneType clone_type);

  /// Reconfigures ContentTimeWarp based on effective timebase and source BPM.
  void update_warp_configuration ();

  friend void to_json (nlohmann::json &j, const MidiClip &clip)
  {
    to_json (j, static_cast<const Clip &> (clip));
    to_json (j, static_cast<const ArrangerObjectOwner<MidiNote> &> (clip));
    to_json (
      j, static_cast<const ArrangerObjectOwner<MidiControlEvent> &> (clip));
    j["sourceBpm"] = clip.source_bpm_;
  }
  friend void from_json (const nlohmann::json &j, MidiClip &clip)
  {
    from_json (j, static_cast<Clip &> (clip));
    from_json (j, static_cast<ArrangerObjectOwner<MidiNote> &> (clip));
    from_json (j, static_cast<ArrangerObjectOwner<MidiControlEvent> &> (clip));
    if (j.contains ("sourceBpm"))
      j.at ("sourceBpm").get_to (clip.source_bpm_);
    clip.update_warp_configuration ();
  }

private:
  units::bpm_t source_bpm_{};

  BOOST_DESCRIBE_CLASS (
    MidiClip,
    (Clip, ArrangerObjectOwner<MidiNote>, ArrangerObjectOwner<MidiControlEvent>),
    (),
    (),
    (source_bpm_))
};
}
