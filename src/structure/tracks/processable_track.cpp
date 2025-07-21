// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "engine/device_io/engine.h"
#include "engine/session/transport.h"
#include "gui/backend/backend/project.h"
#include "structure/arrangement/audio_region.h"
#include "structure/arrangement/chord_region.h"
#include "structure/tracks/chord_track.h"
#include "structure/tracks/laned_track.h"
#include "structure/tracks/processable_track.h"
#include "structure/tracks/track_processor.h"

namespace zrythm::structure::tracks
{
ProcessableTrack::ProcessableTrack (Dependencies dependencies)
    : automatable_track_mixin_ (
        utils::make_qobject_unique<AutomatableTrackMixin> (dependencies)),
      processor_ (
        utils::make_qobject_unique<TrackProcessor> (
          *this,
          TrackProcessor::ProcessorBaseDependencies{
            .port_registry_ = dependencies.port_registry_,
            .param_registry_ = dependencies.param_registry_ })),
      port_registry_ (dependencies.port_registry_),
      param_registry_ (dependencies.param_registry_)
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
    *obj.automatable_track_mixin_, *other.automatable_track_mixin_, clone_type);
  obj.processor_->track_ = &obj;
}

void
from_json (const nlohmann::json &j, ProcessableTrack &p)
{
  j[ProcessableTrack::kProcessorKey].get_to (*p.processor_);
  j[ProcessableTrack::kAutomatableTrackMixinKey].get_to (
    *p.automatable_track_mixin_);
}
}
