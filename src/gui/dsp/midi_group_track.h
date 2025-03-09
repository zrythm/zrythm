// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __MIDI_MIDI_GROUP_TRACK_H__
#define __MIDI_MIDI_GROUP_TRACK_H__

#include "gui/dsp/foldable_track.h"
#include "gui/dsp/group_target_track.h"

class MidiGroupTrack final
    : public QObject,
      public FoldableTrack,
      public GroupTargetTrack,
      public ICloneable<MidiGroupTrack>,
      public zrythm::utils::serialization::ISerializable<MidiGroupTrack>,
      public utils::InitializableObject
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_TRACK_QML_PROPERTIES (MidiGroupTrack)
  DEFINE_AUTOMATABLE_TRACK_QML_PROPERTIES (MidiGroupTrack)
  DEFINE_CHANNEL_TRACK_QML_PROPERTIES (MidiGroupTrack)

  friend class InitializableObject;

  DECLARE_FINAL_TRACK_CONSTRUCTORS (MidiGroupTrack)

public:
  void
  init_after_cloning (const MidiGroupTrack &other, ObjectCloneType clone_type)
    override;

  void
  init_loaded (PluginRegistry &plugin_registry, PortRegistry &port_registry)
    override;

  bool validate () const override;

  void
  append_ports (std::vector<Port *> &ports, bool include_plugins) const final;

  DECLARE_DEFINE_FIELDS_METHOD ();

private:
  bool initialize ();
};

#endif // __MIDI_MIDI_BUS_TRACK_H__
