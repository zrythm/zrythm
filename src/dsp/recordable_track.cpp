// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/control_port.h"
#include "dsp/recordable_track.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "project.h"
#include "utils/rt_thread_id.h"
#include "zrythm.h"
#include "zrythm_app.h"

RecordableTrack::RecordableTrack ()
{
  recording_ = std::make_unique<ControlPort> (
    _ ("Track record"), PortIdentifier::OwnerType::Track, this);
  recording_->id_.sym_ = "track_record";
  recording_->set_toggled (false, false);
  recording_->id_.flags2_ |= PortIdentifier::Flags2::TrackRecording;
  recording_->id_.flags_ |= PortIdentifier::Flags::Toggle;
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
    "%s: setting recording %d (fire events: %d)", name_, recording, fire_events);

  recording_->set_toggled (recording, false);

  if (recording)
    {
      z_info ("enabled recording on {}");
    }
  else
    {
      z_info ("disabled recording on {}", name_);

      /* send all notes off if can record MIDI */
      processor_->pending_midi_panic_ = true;
    }

  if (fire_events)
    {
      EVENTS_PUSH (EventType::ET_TRACK_STATE_CHANGED, this);
    }
}