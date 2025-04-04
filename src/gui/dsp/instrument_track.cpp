// SPDX-FileCopyrightText: © 2018-2019, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings_manager.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/dsp/instrument_track.h"
#include "gui/dsp/region.h"
#include "gui/dsp/track.h"
#include "utils/rt_thread_id.h"

InstrumentTrack::InstrumentTrack (
  TrackRegistry          &track_registry,
  PluginRegistry         &plugin_registry,
  PortRegistry           &port_registry,
  ArrangerObjectRegistry &obj_registry,
  bool                    new_identity)
    : Track (
        Track::Type::Instrument,
        PortType::Event,
        PortType::Audio,
        port_registry,
        obj_registry),
      AutomatableTrack (port_registry, new_identity),
      ProcessableTrack (port_registry, new_identity),
      ChannelTrack (track_registry, plugin_registry, port_registry, new_identity),
      RecordableTrack (port_registry, new_identity)
{
  color_ = Color (QColor ("#FF9616"));
  icon_name_ = "instrument";
  automation_tracklist_->setParent (this);
}

bool
InstrumentTrack::initialize ()
{
  init_channel ();
  generate_automation_tracks ();
  init_recordable_track ([] () {
    return ZRYTHM_HAVE_UI
           && zrythm::gui::SettingsManager::get_instance ()->get_trackAutoArm ();
  });

  return true;
}

bool
InstrumentTrack::validate () const
{
  return Track::validate_base () && ChannelTrack::validate_base ()
         && AutomatableTrack::validate_base () && LanedTrackImpl::validate_base ()
         && GroupTargetTrack::validate_base ();
}

InstrumentTrack::Plugin *
InstrumentTrack::get_instrument ()
{
  auto plugin = channel_->get_instrument ();
  z_return_val_if_fail (plugin, nullptr);
  return Plugin::from_variant (*plugin);
}

const InstrumentTrack::Plugin *
InstrumentTrack::get_instrument () const
{
  auto plugin = channel_->get_instrument ();
  z_return_val_if_fail (plugin, nullptr);
  return Plugin::from_variant (*plugin);
}

bool
InstrumentTrack::is_plugin_visible () const
{
  const auto &plugin = get_instrument ();
  z_return_val_if_fail (plugin, false);
  return plugin->visible_;
}

void
InstrumentTrack::toggle_plugin_visible ()
{
  zrythm::gui::old_dsp::plugins::Plugin * plugin = get_instrument ();
  z_return_if_fail (plugin);
  plugin->visible_ = !plugin->visible_;

  // EVENTS_PUSH (EventType::ET_PLUGIN_VISIBILITY_CHANGED, plugin);
}

void
InstrumentTrack::append_ports (std::vector<Port *> &ports, bool include_plugins)
  const
{
  ChannelTrack::append_member_ports (ports, include_plugins);
  ProcessableTrack::append_member_ports (ports, include_plugins);
  RecordableTrack::append_member_ports (ports, include_plugins);
}

void
InstrumentTrack::init_after_cloning (
  const InstrumentTrack &other,
  ObjectCloneType        clone_type)
{
  Track::copy_members_from (other, clone_type);
  AutomatableTrack::copy_members_from (other, clone_type);
  ProcessableTrack::copy_members_from (other, clone_type);
  ChannelTrack::copy_members_from (other, clone_type);
  GroupTargetTrack::copy_members_from (other, clone_type);
  RecordableTrack::copy_members_from (other, clone_type);
  LanedTrackImpl<MidiLane>::copy_members_from (other, clone_type);
  PianoRollTrack::copy_members_from (other, clone_type);
}

void
InstrumentTrack::init_loaded (
  PluginRegistry &plugin_registry,
  PortRegistry   &port_registry)
{
  // ChannelTrack must be initialized before AutomatableTrack
  ChannelTrack::init_loaded (plugin_registry, port_registry);
  AutomatableTrack::init_loaded (plugin_registry, port_registry);
  ProcessableTrack::init_loaded (plugin_registry, port_registry);
  RecordableTrack::init_loaded (plugin_registry, port_registry);
  LanedTrackImpl<MidiLane>::init_loaded (plugin_registry, port_registry);
  PianoRollTrack::init_loaded (plugin_registry, port_registry);
}
