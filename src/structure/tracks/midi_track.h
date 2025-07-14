// SPDX-FileCopyrightText: © 2019-2021, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/tracks/automatable_track.h"
#include "structure/tracks/channel_track.h"
#include "structure/tracks/piano_roll_track.h"
#include "utils/initializable_object.h"

namespace zrythm::structure::tracks
{
class MidiTrack final
    : public QObject,
      public PianoRollTrack,
      public ChannelTrack,
      public utils::InitializableObject<MidiTrack>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_TRACK_QML_PROPERTIES (MidiTrack)
  DEFINE_LANED_TRACK_QML_PROPERTIES (MidiTrack)
  DEFINE_AUTOMATABLE_TRACK_QML_PROPERTIES (MidiTrack)
  DEFINE_CHANNEL_TRACK_QML_PROPERTIES (MidiTrack)
  DEFINE_PIANO_ROLL_TRACK_QML_PROPERTIES (MidiTrack)

  friend class InitializableObject;

  DECLARE_FINAL_TRACK_CONSTRUCTORS (MidiTrack)

public:
  void init_loaded (
    PluginRegistry                  &plugin_registry,
    dsp::PortRegistry               &port_registry,
    dsp::ProcessorParameterRegistry &param_registry) override;

  friend void init_from (
    MidiTrack             &obj,
    const MidiTrack       &other,
    utils::ObjectCloneType clone_type);

  void temporary_virtual_method_hack () const override { }

private:
  friend void to_json (nlohmann::json &j, const MidiTrack &track)
  {
    to_json (j, static_cast<const Track &> (track));
    to_json (j, static_cast<const ProcessableTrack &> (track));
    to_json (j, static_cast<const AutomatableTrack &> (track));
    to_json (j, static_cast<const RecordableTrack &> (track));
    to_json (j, static_cast<const PianoRollTrack &> (track));
    to_json (j, static_cast<const ChannelTrack &> (track));
    to_json (j, static_cast<const LanedTrackImpl &> (track));
  }
  friend void from_json (const nlohmann::json &j, MidiTrack &track)
  {
    from_json (j, static_cast<Track &> (track));
    from_json (j, static_cast<ProcessableTrack &> (track));
    from_json (j, static_cast<AutomatableTrack &> (track));
    from_json (j, static_cast<RecordableTrack &> (track));
    from_json (j, static_cast<PianoRollTrack &> (track));
    from_json (j, static_cast<ChannelTrack &> (track));
    from_json (j, static_cast<LanedTrackImpl &> (track));
  }

  bool initialize ();
};
}
