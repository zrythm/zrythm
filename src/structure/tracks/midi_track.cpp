// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// SPDX-FileCopyrightText: © 2018-2021, 2024 Alexandros Theodotou <alex@zrythm.org>

#include "zrythm-config.h"

#include "gui/backend/backend/settings_manager.h"
#include "gui/backend/backend/zrythm.h"
#include "structure/tracks/midi_track.h"

namespace zrythm::structure::tracks
{
MidiTrack::MidiTrack (
  TrackRegistry          &track_registry,
  PluginRegistry         &plugin_registry,
  PortRegistry           &port_registry,
  ArrangerObjectRegistry &obj_registry,
  bool                    new_identity)
    : Track (
        Track::Type::Midi,
        PortType::Event,
        PortType::Event,
        plugin_registry,
        port_registry,
        obj_registry),
      AutomatableTrack (port_registry, new_identity),
      ProcessableTrack (port_registry, new_identity),
      RecordableTrack (port_registry, new_identity),
      ChannelTrack (track_registry, plugin_registry, port_registry, new_identity)
{
  color_ = Color (QColor ("#F79616"));
  icon_name_ = u8"signal-midi";
  automation_tracklist_->setParent (this);
}

void
MidiTrack::init_loaded (
  PluginRegistry &plugin_registry,
  PortRegistry   &port_registry)
{
  // ChannelTrack must be initialized before AutomatableTrack
  ChannelTrack::init_loaded (plugin_registry, port_registry);
  AutomatableTrack::init_loaded (plugin_registry, port_registry);
  ProcessableTrack::init_loaded (plugin_registry, port_registry);
  RecordableTrack::init_loaded (plugin_registry, port_registry);
  LanedTrackImpl::init_loaded (plugin_registry, port_registry);
  PianoRollTrack::init_loaded (plugin_registry, port_registry);
}

void
MidiTrack::init_after_cloning (const MidiTrack &other, ObjectCloneType clone_type)
{
  Track::copy_members_from (other, clone_type);
  PianoRollTrack::copy_members_from (other, clone_type);
  ChannelTrack::copy_members_from (other, clone_type);
  ProcessableTrack::copy_members_from (other, clone_type);
  AutomatableTrack::copy_members_from (other, clone_type);
  RecordableTrack::copy_members_from (other, clone_type);
  LanedTrackImpl::copy_members_from (other, clone_type);
}

void
MidiTrack::append_ports (std::vector<Port *> &ports, bool include_plugins) const
{
  ChannelTrack::append_member_ports (ports, include_plugins);
  ProcessableTrack::append_member_ports (ports, include_plugins);
  RecordableTrack::append_member_ports (ports, include_plugins);
}

bool
MidiTrack::initialize ()
{
  init_channel ();
  generate_automation_tracks ();
  init_recordable_track ([] () {
    return ZRYTHM_HAVE_UI
           && zrythm::gui::SettingsManager::get_instance ()->get_trackAutoArm ();
  });

  return true;
}
}
