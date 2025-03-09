// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_AUDIO_GROUP_TRACK_H__
#define __AUDIO_AUDIO_GROUP_TRACK_H__

#include "gui/dsp/automatable_track.h"
#include "gui/dsp/foldable_track.h"
#include "gui/dsp/group_target_track.h"

/**
 * An audio group track that can be folded and is a target for other tracks.
 * It is also an automatable track, meaning it can have automation data.
 */
class AudioGroupTrack final
    : public QObject,
      public FoldableTrack,
      public GroupTargetTrack,
      public ICloneable<AudioGroupTrack>,
      public zrythm::utils::serialization::ISerializable<AudioGroupTrack>,
      public utils::InitializableObject
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_TRACK_QML_PROPERTIES (AudioGroupTrack)
  DEFINE_AUTOMATABLE_TRACK_QML_PROPERTIES (AudioGroupTrack)

  friend class InitializableObject;

  DECLARE_FINAL_TRACK_CONSTRUCTORS (AudioGroupTrack)

public:
  void
  init_after_cloning (const AudioGroupTrack &other, ObjectCloneType clone_type)
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

#endif // __AUDIO_AUDIO_BUS_TRACK_H__
