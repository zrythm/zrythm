// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <cassert>
#include <ranges>

#include "dsp/fork_join_executor.h"
#include "dsp/poly_voice_manager.h"
#include "utils/midi.h"

#if defined(__has_feature) && __has_feature(realtime_sanitizer)
#  include <sanitizer/rtsan_interface.h>
#endif

namespace zrythm::dsp
{

PolyVoiceManager::PolyVoiceManager ()
{
  last_pitch_bend_.fill (SynthVoice::kPitchBendCenter);
}

void
PolyVoiceManager::prepare_for_processing (
  const int                 num_channels,
  const units::sample_u32_t max_block_length)
{
  num_channels_ = num_channels;
  voice_scratch_.resize (voices_.size ());
  for (auto &scratch : voice_scratch_)
    {
      scratch.setSize (
        num_channels, max_block_length.in<int> (units::samples), false, false,
        true);
    }
  active_voice_indices_.resize (voices_.size ());
}

void
PolyVoiceManager::all_notes_off () noexcept
{
  for (auto &voice : voices_)
    voice->cut ();
}

void
PolyVoiceManager::note_on (int channel, int pitch, float velocity) noexcept
{
  // Find a free voice, or steal the oldest active one
  auto * voice = [&] () -> SynthVoice * {
    for (auto &v : voices_)
      {
        if (!v->is_active ())
          return v.get ();
      }
    const auto oldest = std::ranges::min_element (
      voices_, {}, [] (const auto &v) { return v->note_sequence (); });
    return oldest != voices_.end () ? oldest->get () : nullptr;
  }();
  if (voice == nullptr)
    return;

  if (voice->is_active ())
    voice->cut ();

  voice->note_on (channel, pitch, velocity, next_note_sequence_++);
  // Apply the channel's current bend so bent notes start bent
  voice->pitch_bend (last_pitch_bend_[channel]);
}

void
PolyVoiceManager::note_off (int channel, int pitch) noexcept
{
  for (auto &voice : voices_)
    {
      if (
        voice->is_active () && voice->current_note () == pitch
        && voice->current_channel () == channel)
        {
          voice->note_off ();
        }
    }
}

void
PolyVoiceManager::dispatch_event (std::span<const midi_byte_t> data) noexcept
{
  // Validate size against the status byte: this lets through only well-formed
  // messages of the relevant types (note on/off/pitch wheel/control change
  // are 3 bytes each) and silently skips shorter messages that don't apply
  // to voice allocation (program change, channel pressure, active sensing,
  // etc.). Further CC support would slot in as another else-if branch with
  // the same length check.
  if (data.empty ())
    return;
  const auto expected_len = utils::midi::midi_get_msg_length (data[0]);
  if (data.size () < static_cast<size_t> (expected_len))
    return;

  const int channel =
    static_cast<int> (utils::midi::midi_get_channel_0_to_15 (data));
  if (utils::midi::midi_is_note_on (data))
    {
      note_on (
        channel, utils::midi::midi_get_note_number (data),
        static_cast<float> (utils::midi::midi_get_velocity (data)) / 127.f);
    }
  else if (utils::midi::midi_is_note_off (data))
    {
      note_off (channel, utils::midi::midi_get_note_number (data));
    }
  else if (utils::midi::midi_is_pitch_wheel (data))
    {
      const int value =
        static_cast<int> (utils::midi::midi_get_14_bit_value (data));
      last_pitch_bend_[channel] = value;
      for (auto &voice : voices_)
        voice->pitch_bend (value);
    }
  else if (utils::midi::midi_is_all_sound_off (data))
    {
      // CC 120: silence immediately (no release tail)
      for (auto &voice : voices_)
        {
          if (voice->is_active () && voice->current_channel () == channel)
            voice->cut ();
        }
    }
  else if (utils::midi::midi_is_all_notes_off (data))
    {
      // CC 123: release all notes on the channel (envelopes still tail off)
      for (auto &voice : voices_)
        {
          if (voice->is_active () && voice->current_channel () == channel)
            voice->note_off ();
        }
    }
}

void
PolyVoiceManager::render_active (
  juce::AudioBuffer<float> &output,
  const int                 start_sample,
  const int                 num_samples,
  graph::ForkJoinExecutor * fork_join_executor) noexcept
{
  if (num_samples <= 0)
    return;

  // Serial path: each active voice renders (adds) into the output directly
  const auto render_serial = [&] {
    for (auto &voice : voices_)
      {
        if (voice->is_active ())
          voice->render (output, start_sample, num_samples);
      }
  };

  // The index list is sized at prepare time; a size mismatch means the
  // voice set changed afterwards, so the scratch setup does not match the
  // current voices
  if (
    fork_join_executor == nullptr
    || active_voice_indices_.size () != voices_.size ())
    {
      render_serial ();
      return;
    }

  // Too short to be worth the fork-join overhead (common with dense MIDI,
  // where events split the block into small segments)
  if (num_samples < kMinParallelBlockSamples)
    {
      render_serial ();
      return;
    }

  // Collect the active voices (index list preallocated at prepare time)
  std::uint32_t num_active = 0;
  for (const auto i : std::views::iota (0u, voices_.size ()))
    {
      if (voices_[i]->is_active ())
        active_voice_indices_[num_active++] = i;
    }

  if (num_active < kMinParallelVoices)
    {
      render_serial ();
      return;
    }

  // Parallel-path preconditions (see process() docs): assert to catch
  // caller bugs in debug builds, and fall back to the serial path in
  // release instead of reading/writing past the scratch buffers
  const bool fits_preparation =
    num_channels_ == output.getNumChannels ()
    && voice_scratch_.size () == voices_.size ()
    && start_sample + num_samples <= voice_scratch_.front ().getNumSamples ();
  assert (fits_preparation);
  if (!fits_preparation)
    {
      render_serial ();
      return;
    }

  // Render each active voice into its own scratch buffer in parallel, then
  // sum the scratch buffers serially in voice order: the additions happen
  // in the same order as serial rendering, so the result is bit-identical
  struct RenderContext
  {
    PolyVoiceManager * self;
    int                start_sample;
    int                num_samples;
  } ctx{ this, start_sample, num_samples };
  const auto render_task = [] (void * context, std::uint32_t i) noexcept {
    const auto &c = *static_cast<const RenderContext *> (context);
    const auto  voice_index = c.self->active_voice_indices_[i];
    auto       &scratch = c.self->voice_scratch_[voice_index];
    // Render at the same sample range as the output: voices may depend on
    // the absolute position
    scratch.clear (c.start_sample, c.num_samples);
    c.self->voices_[voice_index]->render (
      scratch, c.start_sample, c.num_samples);
  };

  bool completed = false;
  {
#if defined(__has_feature) && __has_feature(realtime_sanitizer)
    // exec() blocks the calling thread by design (fork-join)
    __rtsan::ScopedDisabler disabler;
#endif
    completed = fork_join_executor->exec (render_task, &ctx, num_active);
  }

  if (!completed)
    {
      // exec() guarantees that a false return means no task ran, so the
      // serial fallback here does not double-render any voice
      for (const auto i : std::views::iota (0u, num_active))
        {
          voices_[active_voice_indices_[i]]->render (
            output, start_sample, num_samples);
        }
      return;
    }

  // fits_preparation guarantees the channel counts match
  for (const auto i : std::views::iota (0u, num_active))
    {
      const auto &scratch = voice_scratch_[active_voice_indices_[i]];
      for (const auto ch : std::views::iota (0, num_channels_))
        {
          output.addFrom (
            ch, start_sample, scratch, ch, start_sample, num_samples);
        }
    }
}

void
PolyVoiceManager::process (
  juce::AudioBuffer<float> &output,
  const MidiEventBuffer    &midi_events,
  units::sample_u32_t       offset,
  units::sample_u32_t       nframes,
  graph::ForkJoinExecutor * fork_join_executor) noexcept
{
  const int block_start = offset.in<int> (units::samples);
  const int block_end = block_start + nframes.in<int> (units::samples);

  int current = block_start;
  for (const auto &ev : midi_events)
    {
      const int ev_pos = ev.time ().in<int> (units::samples);
      if (ev_pos < block_start || ev_pos >= block_end)
        continue;
      render_active (output, current, ev_pos - current, fork_join_executor);
      current = ev_pos;
      dispatch_event (ev.data ());
    }
  render_active (output, current, block_end - current, fork_join_executor);
}

} // namespace zrythm::dsp
