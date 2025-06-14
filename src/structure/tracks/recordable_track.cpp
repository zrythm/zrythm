// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/dsp/port_all.h"
#include "structure/tracks/recordable_track.h"
#include "utils/rt_thread_id.h"

namespace zrythm::structure::tracks
{
RecordableTrack::RecordableTrack (PortRegistry &port_registry, bool new_identity)
    : ProcessableTrack (port_registry, new_identity)
{
  if (new_identity)
    {
      recording_id_ = port_registry.create_object<ControlPort> (
        utils::Utf8String::from_qstring (QObject::tr ("Track record")));
      auto * recording = &get_recording_port ();
      recording->id_->sym_ = u8"track_record";
      recording->set_toggled (false, false);
      recording->id_->flags2_ |=
        structure::tracks::PortIdentifier::Flags2::TrackRecording;
      recording->id_->flags_ |= structure::tracks::PortIdentifier::Flags::Toggle;
    }
}

void
RecordableTrack::init_loaded (
  PluginRegistry &plugin_registry,
  PortRegistry   &port_registry)
{
  ProcessableTrack::init_loaded (plugin_registry, port_registry);
}

void
RecordableTrack::append_member_ports (
  std::vector<Port *> &ports,
  bool                 include_plugins) const
{
  ports.push_back (std::addressof (get_recording_port ()));
}
}