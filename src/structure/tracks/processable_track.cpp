// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/processable_track.h"
#include "structure/tracks/track_processor.h"

namespace zrythm::structure::tracks
{
ProcessableTrack::ProcessableTrack (
  const dsp::ITransport &transport,
  PortType               signal_type,
  Dependencies           dependencies)
    : automation_tracklist_ (
        utils::make_qobject_unique<AutomationTracklist> (dependencies)),
      processor_ (
        utils::make_qobject_unique<TrackProcessor> (
          transport,
          signal_type,
          [this] () { return get_name (); },
          [this] () { return is_enabled (); },
          has_piano_roll () || is_chord (),
          has_piano_roll (),
          is_audio (),
          TrackProcessor::ProcessorBaseDependencies{
            .port_registry_ = dependencies.port_registry_,
            .param_registry_ = dependencies.param_registry_ }))
{
}

void
ProcessableTrack::init_loaded (
  PluginRegistry                  &plugin_registry,
  dsp::PortRegistry               &port_registry,
  dsp::ProcessorParameterRegistry &param_registry)
{
}

void
init_from (
  ProcessableTrack       &obj,
  const ProcessableTrack &other,
  utils::ObjectCloneType  clone_type)
{
  init_from (*obj.processor_, *other.processor_, clone_type);
  init_from (
    *obj.automation_tracklist_, *other.automation_tracklist_, clone_type);
}

void
from_json (const nlohmann::json &j, ProcessableTrack &p)
{
  j[ProcessableTrack::kProcessorKey].get_to (*p.processor_);
  j[ProcessableTrack::kAutomationTracklistKey].get_to (*p.automation_tracklist_);
}
}
