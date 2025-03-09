// SPDX-FileCopyrightText: © 2018-2020, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * The master track.
 */

#ifndef __AUDIO_MASTER_TRACK_H__
#define __AUDIO_MASTER_TRACK_H__

#include "gui/dsp/group_target_track.h"

/**
 * @addtogroup dsp
 *
 * @{
 */

#define P_MASTER_TRACK (TRACKLIST->master_track_)

class MasterTrack final
    : public QObject,
      public GroupTargetTrack,
      public ICloneable<MasterTrack>,
      public zrythm::utils::serialization::ISerializable<MasterTrack>,
      public utils::InitializableObject
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_TRACK_QML_PROPERTIES (MasterTrack)
  DEFINE_AUTOMATABLE_TRACK_QML_PROPERTIES (MasterTrack)
  DEFINE_CHANNEL_TRACK_QML_PROPERTIES (MasterTrack)

  DECLARE_FINAL_TRACK_CONSTRUCTORS (MasterTrack)

public:
  friend class InitializableObject;

  void
  init_loaded (PluginRegistry &plugin_registry, PortRegistry &port_registry)
    override;

  void init_after_cloning (const MasterTrack &other, ObjectCloneType clone_type)
    override;

  bool validate () const override;

  void
  append_ports (std::vector<Port *> &ports, bool include_plugins) const final;

  DECLARE_DEFINE_FIELDS_METHOD ();

private:
  bool initialize ();
};

/**
 * @}
 */

#endif // __AUDIO_MASTER_TRACK_H__
