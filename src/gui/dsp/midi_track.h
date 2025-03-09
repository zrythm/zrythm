// SPDX-FileCopyrightText: © 2019-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_MIDI_TRACK_H__
#define __AUDIO_MIDI_TRACK_H__

#include "gui/dsp/automatable_track.h"
#include "gui/dsp/channel_track.h"
#include "gui/dsp/piano_roll_track.h"

#include "utils/object_factory.h"

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
      public zrythm::utils::serialization::ISerializable<MidiTrack>,
      public utils::InitializableObject
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
  bool initialize ();

  DECLARE_DEFINE_FIELDS_METHOD ();
};

/**
 * @}
 */

#endif // __AUDIO_MIDI_TRACK_H__
