// SPDX-FileCopyrightText: © 2018-2020, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

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
      public utils::InitializableObject<MasterTrack>
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

private:
  friend void to_json (nlohmann::json &j, const MasterTrack &project)
  {
    to_json (j, static_cast<const Track &> (project));
    to_json (j, static_cast<const ProcessableTrack &> (project));
    to_json (j, static_cast<const AutomatableTrack &> (project));
    to_json (j, static_cast<const ChannelTrack &> (project));
    to_json (j, static_cast<const GroupTargetTrack &> (project));
  }
  friend void from_json (const nlohmann::json &j, MasterTrack &project)
  {
    from_json (j, static_cast<Track &> (project));
    from_json (j, static_cast<ProcessableTrack &> (project));
    from_json (j, static_cast<AutomatableTrack &> (project));
    from_json (j, static_cast<ChannelTrack &> (project));
    from_json (j, static_cast<GroupTargetTrack &> (project));
  }

  bool initialize ();
};

/**
 * @}
 */
