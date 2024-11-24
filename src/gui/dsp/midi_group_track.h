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
      public InitializableObjectFactory<MidiGroupTrack>
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_TRACK_QML_PROPERTIES (MidiGroupTrack)
  DEFINE_AUTOMATABLE_TRACK_QML_PROPERTIES (MidiGroupTrack)
  DEFINE_CHANNEL_TRACK_QML_PROPERTIES (MidiGroupTrack)

  friend class InitializableObjectFactory<MidiGroupTrack>;

public:
  void init_after_cloning (const MidiGroupTrack &other) override;

  void init_loaded () override;

  bool validate () const override;

  void
  append_ports (std::vector<Port *> &ports, bool include_plugins) const final;

  DECLARE_DEFINE_FIELDS_METHOD ();

private:
  MidiGroupTrack (const std::string &name = "", int pos = 0);

  bool initialize () override;
};

#endif // __MIDI_MIDI_BUS_TRACK_H__
