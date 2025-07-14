// SPDX-FileCopyrightText: © 2018-2019, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings_manager.h"
#include "gui/backend/backend/zrythm.h"
#include "structure/arrangement/region.h"
#include "structure/tracks/instrument_track.h"
#include "structure/tracks/track.h"
#include "utils/rt_thread_id.h"

namespace zrythm::structure::tracks
{
InstrumentTrack::InstrumentTrack (
  dsp::FileAudioSourceRegistry    &file_audio_source_registry,
  TrackRegistry                   &track_registry,
  PluginRegistry                  &plugin_registry,
  dsp::PortRegistry               &port_registry,
  dsp::ProcessorParameterRegistry &param_registry,
  ArrangerObjectRegistry          &obj_registry,
  bool                             new_identity)
    : Track (
        Track::Type::Instrument,
        PortType::Event,
        PortType::Audio,
        plugin_registry,
        port_registry,
        param_registry,
        obj_registry),
      AutomatableTrack (
        file_audio_source_registry,
        port_registry,
        param_registry,
        new_identity),
      ProcessableTrack (port_registry, param_registry, new_identity),
      ChannelTrack (
        track_registry,
        plugin_registry,
        port_registry,
        param_registry,
        new_identity),
      RecordableTrack (port_registry, param_registry, new_identity)
{
  color_ = Color (QColor ("#FF9616"));
  icon_name_ = u8"instrument";
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
init_from (
  InstrumentTrack       &obj,
  const InstrumentTrack &other,
  utils::ObjectCloneType clone_type)
{
  init_from (
    static_cast<Track &> (obj), static_cast<const Track &> (other), clone_type);
  init_from (
    static_cast<AutomatableTrack &> (obj),
    static_cast<const AutomatableTrack &> (other), clone_type);
  init_from (
    static_cast<ProcessableTrack &> (obj),
    static_cast<const ProcessableTrack &> (other), clone_type);
  init_from (
    static_cast<ChannelTrack &> (obj),
    static_cast<const ChannelTrack &> (other), clone_type);
  init_from (
    static_cast<GroupTargetTrack &> (obj),
    static_cast<const GroupTargetTrack &> (other), clone_type);
  init_from (
    static_cast<RecordableTrack &> (obj),
    static_cast<const RecordableTrack &> (other), clone_type);
  init_from (
    static_cast<LanedTrackImpl<MidiLane> &> (obj),
    static_cast<const LanedTrackImpl<MidiLane> &> (other), clone_type);
  init_from (
    static_cast<PianoRollTrack &> (obj),
    static_cast<const PianoRollTrack &> (other), clone_type);
}

void
InstrumentTrack::init_loaded (
  PluginRegistry                  &plugin_registry,
  dsp::PortRegistry               &port_registry,
  dsp::ProcessorParameterRegistry &param_registry)
{
  // ChannelTrack must be initialized before AutomatableTrack
  ChannelTrack::init_loaded (plugin_registry, port_registry, param_registry);
  AutomatableTrack::init_loaded (plugin_registry, port_registry, param_registry);
  ProcessableTrack::init_loaded (plugin_registry, port_registry, param_registry);
  RecordableTrack::init_loaded (plugin_registry, port_registry, param_registry);
  LanedTrackImpl<MidiLane>::init_loaded (
    plugin_registry, port_registry, param_registry);
  PianoRollTrack::init_loaded (plugin_registry, port_registry, param_registry);
}
}
