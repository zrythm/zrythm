// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/timeline_midi_event_provider.h"

namespace zrythm::structure::arrangement
{

void
TimelineMidiEventProvider::set_midi_events (
  const juce::MidiMessageSequence &events)
{
  decltype (active_midi_playback_sequence_)::ScopedAccess<
    farbot::ThreadType::nonRealtime>
    rt_events{ active_midi_playback_sequence_ };
  *rt_events = events;
}

// TrackEventProvider interface
void
TimelineMidiEventProvider::process_events (
  const EngineProcessTimeInfo &time_nfo,
  dsp::MidiEventVector        &output_buffer)
{
  decltype (active_midi_playback_sequence_)::ScopedAccess<
    farbot::ThreadType::realtime>
             midi_seq{ active_midi_playback_sequence_ };
  const auto first_idx = midi_seq->getNextIndexAtTime (
    static_cast<double> (time_nfo.g_start_frame_w_offset_));
  for (int i = first_idx; i < midi_seq->getNumEvents (); ++i)
    {
      const auto * event = midi_seq->getEventPointer (i);
      const auto   timestamp_dbl = event->message.getTimeStamp ();
      if (
        timestamp_dbl >= static_cast<double> (
          time_nfo.g_start_frame_w_offset_ + time_nfo.nframes_))
        {
          break;
        }

      const auto local_timestamp =
        static_cast<decltype (time_nfo.g_start_frame_)> (timestamp_dbl)
        - time_nfo.g_start_frame_;
      output_buffer.add_raw (
        event->message.getRawData (), event->message.getRawDataSize (),
        static_cast<midi_time_t> (local_timestamp));
    }

  // output_buffer.print (); // debug
}

} // namespace zrythm::structure::tracks
