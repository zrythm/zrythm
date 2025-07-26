// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/port_all.h"
#include "structure/tracks/recordable_track.h"
#include "utils/rt_thread_id.h"

namespace zrythm::structure::tracks
{
RecordableTrack::RecordableTrack (
  const dsp::ITransport         &transport,
  ProcessableTrack::Dependencies dependencies)
    : recording_id_ (dependencies.param_registry_)
{
  recording_id_ = dependencies.param_registry_.create_object<
    dsp::ProcessorParameter> (
    dependencies.port_registry_,
    dsp::ProcessorParameter::UniqueId (u8"track_record"),
    dsp::ParameterRange::make_toggle (false),
    utils::Utf8String::from_qstring (QObject::tr ("Track record")));
}

void
RecordableTrack::init_loaded (
  PluginRegistry                  &plugin_registry,
  dsp::PortRegistry               &port_registry,
  dsp::ProcessorParameterRegistry &param_registry)
{
  ProcessableTrack::init_loaded (plugin_registry, port_registry, param_registry);
}
}
