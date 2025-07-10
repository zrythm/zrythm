// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/port_all.h"
#include "structure/tracks/recordable_track.h"
#include "utils/rt_thread_id.h"

namespace zrythm::structure::tracks
{
RecordableTrack::RecordableTrack (
  dsp::PortRegistry               &port_registry,
  dsp::ProcessorParameterRegistry &param_registry,
  bool                             new_identity)
    : ProcessableTrack (port_registry, param_registry, new_identity),
      recording_id_ (param_registry)
{
  recording_id_ = param_registry.create_object<dsp::ProcessorParameter> (
    port_registry, dsp::ProcessorParameter::UniqueId (u8"track_record"),
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

void
RecordableTrack::append_member_ports (
  std::vector<dsp::Port *> &ports,
  bool                      include_plugins) const
{
  // ports.push_back (std::addressof (get_recording_port ()));
}
}
