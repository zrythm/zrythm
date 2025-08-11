// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// SPDX-FileCopyrightText: © 2018-2021, 2024 Alexandros Theodotou <alex@zrythm.org>

#include "zrythm-config.h"

#include "gui/backend/backend/settings_manager.h"
#include "gui/backend/backend/zrythm.h"
#include "structure/tracks/midi_track.h"

namespace zrythm::structure::tracks
{
MidiTrack::MidiTrack (FinalTrackDependencies dependencies)
    : Track (
        Track::Type::Midi,
        PortType::Midi,
        PortType::Midi,
        dependencies.to_base_dependencies ()),
      ProcessableTrack (
        dependencies.transport_,
        PortType::Midi,
        Dependencies{
          dependencies.tempo_map_, dependencies.file_audio_source_registry_,
          dependencies.port_registry_, dependencies.param_registry_,
          dependencies.obj_registry_ }),
      RecordableTrack (
        dependencies.transport_,
        Dependencies{
          dependencies.tempo_map_, dependencies.file_audio_source_registry_,
          dependencies.port_registry_, dependencies.param_registry_,
          dependencies.obj_registry_ }),
      ChannelTrack (dependencies)
{
  color_ = Color (QColor ("#F79616"));
  icon_name_ = u8"signal-midi";
  automationTracklist ()->setParent (this);
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
  generate_automation_tracks (*this);
  init_recordable_track ([] () {
    return ZRYTHM_HAVE_UI
           && zrythm::gui::SettingsManager::get_instance ()->get_trackAutoArm ();
  });

  return true;
}
}
