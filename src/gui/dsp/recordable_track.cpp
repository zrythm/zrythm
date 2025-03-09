// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/dsp/control_port.h"
#include "gui/dsp/recordable_track.h"

#include "utils/rt_thread_id.h"

using namespace zrythm;

RecordableTrack::RecordableTrack (PortRegistry &port_registry, bool new_identity)
    : ProcessableTrack (port_registry, new_identity),
      port_registry_ (port_registry)
{
  if (new_identity)
    {
      auto * recording = port_registry.create_object<ControlPort> (
        QObject::tr ("Track record").toStdString ());
      recording->id_->sym_ = "track_record";
      recording->set_toggled (false, false);
      recording->id_->flags2_ |= dsp::PortIdentifier::Flags2::TrackRecording;
      recording->id_->flags_ |= dsp::PortIdentifier::Flags::Toggle;
      recording_id_ = recording->get_uuid ();
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
RecordableTrack::set_recording (bool recording)
{
  z_debug ("{}: setting recording {}", name_, recording);

  get_recording_port ().set_toggled (recording, false);

  if (recording)
    {
      z_info ("enabled recording on {}", name_);
    }
  else
    {
      z_info ("disabled recording on {}", name_);

      /* send all notes off if can record MIDI */
      processor_->pending_midi_panic_ = true;
    }
}

void
RecordableTrack::append_member_ports (
  std::vector<Port *> &ports,
  bool                 include_plugins) const
{
  ports.push_back (std::addressof (get_recording_port ()));
}
