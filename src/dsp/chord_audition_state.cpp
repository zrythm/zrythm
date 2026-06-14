// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>

#include "dsp/chord_audition_state.h"

namespace zrythm::dsp
{

void
ChordAuditionState::start (
  const ChordDescriptor &descriptor,
  midi_byte_t            velocity)
{
  active_chords_.push_back ({ descriptor.getMidiPitches (), velocity });
  rebuild_active_pitches ();
}

void
ChordAuditionState::stop (const ChordDescriptor &descriptor)
{
  stop (descriptor.getMidiPitches ());
}

void
ChordAuditionState::stop (const ChordDescriptor::ChordPitches &pitches)
{
  auto result =
    std::ranges::find_last_if (active_chords_, [&] (const auto &entry) {
      return entry.pitches == pitches;
    });
  if (result.begin () != active_chords_.end ())
    active_chords_.erase (result.begin ());
  rebuild_active_pitches ();
}

void
ChordAuditionState::stopAll ()
{
  active_chords_.clear ();
  rebuild_active_pitches ();
}

void
ChordAuditionState::rebuild_active_pitches ()
{
  ActivePitches        merged;
  std::array<int, 128> ref_count{};

  for (const auto &chord : active_chords_)
    {
      for (auto pitch : chord.pitches)
        {
          if (ref_count[pitch] == 0)
            merged.push_back (
              { static_cast<std::uint8_t> (pitch), chord.velocity });
          ++ref_count[pitch];
        }
    }

  std::ranges::sort (merged, {}, &PitchEntry::pitch);

  decltype (active_pitches_)::ScopedAccess<farbot::ThreadType::nonRealtime>
    access{ active_pitches_ };
  *access = merged;
}

void
ChordAuditionState::process (
  MidiEventBuffer               &out_events,
  const graph::ProcessBlockInfo &time_nfo) noexcept
{
  decltype (active_pitches_)::ScopedAccess<farbot::ThreadType::realtime> access{
    active_pitches_
  };

  const auto &desired = *access;
  const auto &sounding = sounding_pitches_;

  const bool was_playing = !sounding.empty ();
  const bool now_playing = !desired.empty ();

  if (was_playing && desired != sounding)
    {
      for (const auto &entry : sounding)
        {
          const auto ev = dsp::midi_event::make_note_off (
            0, entry.pitch, time_nfo.buffer_offset_);
          if (!out_events.push_back (ev.time_, ev.data ()))
            return;
        }
    }

  if (now_playing && desired != sounding)
    {
      for (const auto &entry : desired)
        {
          const auto ev = dsp::midi_event::make_note_on (
            0, entry.pitch, entry.velocity, time_nfo.buffer_offset_);
          if (!out_events.push_back (ev.time_, ev.data ()))
            return;
        }
    }

  sounding_pitches_ = desired;
}

} // namespace zrythm::dsp
