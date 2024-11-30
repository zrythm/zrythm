// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/dsp/control_port.h"
#include "gui/dsp/recordable_track.h"

#include "utils/rt_thread_id.h"

using namespace zrythm;

RecordableTrack::RecordableTrack ()
{
  recording_ =
    std::make_unique<ControlPort> (QObject::tr ("Track record").toStdString ());
  recording_->set_owner (this);
  recording_->id_->sym_ = "track_record";
  recording_->set_toggled (false, false);
  recording_->id_->flags2_ |= dsp::PortIdentifier::Flags2::TrackRecording;
  recording_->id_->flags_ |= dsp::PortIdentifier::Flags::Toggle;
}

void
RecordableTrack::init_loaded ()
{
  ProcessableTrack::init_loaded ();
}

void
RecordableTrack::set_recording (bool recording, bool fire_events)
{
  z_debug (
    "{}: setting recording {} (fire events: {})", name_, recording, fire_events);

  recording_->set_toggled (recording, false);

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

  if (fire_events)
    {
      // EVENTS_PUSH (EventType::ET_TRACK_STATE_CHANGED, this);
    }
}

void
RecordableTrack::append_member_ports (
  std::vector<Port *> &ports,
  bool                 include_plugins) const
{
  if (recording_)
    {
      ports.push_back (recording_.get ());
    }
  else
    {
      z_warning ("recording port unset");
    }
}
