// SPDX-FileCopyrightText: © 2019-2021, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "gui/dsp/automatable_track.h"
#include "gui/dsp/channel_track.h"
#include "gui/dsp/piano_roll_track.h"
#include "utils/initializable_object.h"

/**
 * @addtogroup dsp
 *
 * @{
 */

class MidiTrack final
    : public QObject,
      public PianoRollTrack,
      public ChannelTrack,
      public ICloneable<MidiTrack>,
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
  void
  init_loaded (PluginRegistry &plugin_registry, PortRegistry &port_registry)
    override;

  bool validate () const override;

  void
  append_ports (std::vector<Port *> &ports, bool include_plugins) const final;

  void init_after_cloning (const MidiTrack &other, ObjectCloneType clone_type)
    override;

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

/**
 * @}
 */
