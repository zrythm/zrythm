// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// SPDX-FileCopyrightText: © 2018-2021, 2024 Alexandros Theodotou <alex@zrythm.org>

#include "zrythm-config.h"

#include "gui/backend/backend/settings_manager.h"
#include "gui/backend/backend/zrythm.h"
#include "structure/tracks/midi_track.h"

namespace zrythm::structure::tracks
{
MidiTrack::MidiTrack (
  dsp::FileAudioSourceRegistry    &file_audio_source_registry,
  TrackRegistry                   &track_registry,
  PluginRegistry                  &plugin_registry,
  dsp::PortRegistry               &port_registry,
  dsp::ProcessorParameterRegistry &param_registry,
  ArrangerObjectRegistry          &obj_registry,
  bool                             new_identity)
    : Track (
        Track::Type::Midi,
        PortType::Event,
        PortType::Event,
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
      RecordableTrack (port_registry, param_registry, new_identity),
      ChannelTrack (
        track_registry,
        plugin_registry,
        port_registry,
        param_registry,
        new_identity)
{
  color_ = Color (QColor ("#F79616"));
  icon_name_ = u8"signal-midi";
  automation_tracklist_->setParent (this);
}

void
MidiTrack::init_loaded (
  PluginRegistry                  &plugin_registry,
  dsp::PortRegistry               &port_registry,
  dsp::ProcessorParameterRegistry &param_registry)
{
  // ChannelTrack must be initialized before AutomatableTrack
  ChannelTrack::init_loaded (plugin_registry, port_registry, param_registry);
  AutomatableTrack::init_loaded (plugin_registry, port_registry, param_registry);
  ProcessableTrack::init_loaded (plugin_registry, port_registry, param_registry);
  RecordableTrack::init_loaded (plugin_registry, port_registry, param_registry);
  LanedTrackImpl::init_loaded (plugin_registry, port_registry, param_registry);
  PianoRollTrack::init_loaded (plugin_registry, port_registry, param_registry);
}

void
init_from (
  MidiTrack             &obj,
  const MidiTrack       &other,
  utils::ObjectCloneType clone_type)
{
  init_from (
    static_cast<Track &> (obj), static_cast<const Track &> (other), clone_type);
  init_from (
    static_cast<PianoRollTrack &> (obj),
    static_cast<const PianoRollTrack &> (other), clone_type);
  init_from (
    static_cast<ChannelTrack &> (obj),
    static_cast<const ChannelTrack &> (other), clone_type);
  init_from (
    static_cast<ProcessableTrack &> (obj),
    static_cast<const ProcessableTrack &> (other), clone_type);
  init_from (
    static_cast<AutomatableTrack &> (obj),
    static_cast<const AutomatableTrack &> (other), clone_type);
  init_from (
    static_cast<RecordableTrack &> (obj),
    static_cast<const RecordableTrack &> (other), clone_type);
  init_from (
    static_cast<LanedTrackImpl<MidiLane> &> (obj),
    static_cast<const LanedTrackImpl<MidiLane> &> (other), clone_type);
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
