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
InstrumentTrack::InstrumentTrack (FinalTrackDependencies dependencies)
    : Track (
        Track::Type::Instrument,
        PortType::Event,
        PortType::Audio,
        dependencies.to_base_dependencies ()),
      ProcessableTrack (
        Dependencies{
          dependencies.tempo_map_, dependencies.file_audio_source_registry_,
          dependencies.port_registry_, dependencies.param_registry_,
          dependencies.obj_registry_ }),
      ChannelTrack (dependencies),
      RecordableTrack (
        Dependencies{
          dependencies.tempo_map_, dependencies.file_audio_source_registry_,
          dependencies.port_registry_, dependencies.param_registry_,
          dependencies.obj_registry_ })
{
  color_ = Color (QColor ("#FF9616"));
  icon_name_ = u8"instrument";
  automationTracklist ()->setParent (this);
}

bool
InstrumentTrack::initialize ()
{
  init_channel ();
  generate_automation_tracks (*this);
  init_recordable_track ([] () {
    return ZRYTHM_HAVE_UI
           && zrythm::gui::SettingsManager::get_instance ()->get_trackAutoArm ();
  });

  return true;
}

Plugin *
InstrumentTrack::get_instrument ()
{
  auto plugin = channel_->get_instrument ();
  z_return_val_if_fail (plugin, nullptr);
  return Plugin::from_variant (*plugin);
}

const Plugin *
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
  ProcessableTrack::init_loaded (plugin_registry, port_registry, param_registry);
  RecordableTrack::init_loaded (plugin_registry, port_registry, param_registry);
  LanedTrackImpl<MidiLane>::init_loaded (
    plugin_registry, port_registry, param_registry);
  PianoRollTrack::init_loaded (plugin_registry, port_registry, param_registry);
}
}
