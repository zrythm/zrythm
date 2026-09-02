// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <vector>

#include "dsp/midi_event_buffer.h"
#include "dsp/synth_voice.h"
#include "utils/units.h"

namespace zrythm::dsp::graph
{
class ForkJoinExecutor;
}

namespace zrythm::dsp
{

/**
 * @brief Owns and drives a set of polyphonic synthesizer voices.
 *
 * Dispatches MIDI events (note on/off, pitch wheel, all-sound-off and
 * all-notes-off CCs) to voices with sample-accurate timing and renders the
 * active voices between events.
 * Voice allocation picks the first free voice, stealing the oldest active
 * voice when all are busy.
 *
 * Real-time safety: process() performs no allocations or locking of its
 * own; when a fork-join executor is given, the calling thread blocks on
 * the executor until the voice tasks complete (by design). Voices must be
 * added and prepared before processing starts.
 *
 * TODO: handle sustain pedal (CC64) — defer note releases while the pedal
 * is down and flush them when it lifts (juce::Synthesiser, which this
 * replaces, handled it; currently sustained notes release immediately).
 */
class PolyVoiceManager
{
public:
  PolyVoiceManager ();
  /** Adds a voice (takes ownership). Not RT-safe; call before processing. */
  void add_voice (std::unique_ptr<SynthVoice> voice)
  {
    voices_.push_back (std::move (voice));
  }

  /** Removes all voices. Not RT-safe. */
  void clear_voices () { voices_.clear (); }

  /**
   * @brief Prepares per-voice scratch buffers used by the parallel rendering
   * path. Not RT-safe; call after all voices are added and before processing
   * starts, and again if voices are added or removed or the maximum block
   * length grows afterwards.
   *
   * Existing buffers are reused where possible. Note that the parallel path
   * costs one extra clear and one extra summing pass per active voice, plus
   * num_voices x @p num_channels x @p max_block_length floats of scratch
   * memory.
   */
  void
  prepare_for_processing (int num_channels, units::sample_u32_t max_block_length);

  /** Returns the voices (for per-voice setup such as control values). */
  std::span<const std::unique_ptr<SynthVoice>> voices () const
  {
    return voices_;
  }

  /** Releases all notes immediately (no tail-off). RT-safe. */
  void all_notes_off () noexcept [[clang::nonblocking]];

  /**
   * @brief Dispatches MIDI events and renders the block.
   *
   * Events outside [@p offset, @p offset + @p nframes) are ignored. Events
   * must be sorted by time. Active voices are rendered sample-accurately
   * between consecutive events.
   *
   * @param output Target buffer (voices add into it).
   * @param midi_events MIDI events for the current cycle (may be empty).
   * @param offset Sample offset of the block within the cycle.
   * @param nframes Number of samples to render.
   * @param fork_join_executor Executor for parallel per-voice rendering, or
   * nullptr for the serial path. When given and enough voices are active
   * for a large enough block (below internal voice-count and block-size
   * thresholds the serial path is used), voices render into per-voice
   * scratch buffers in parallel and are summed into the output serially in
   * voice order; if the executor rejects the job the serial path is used.
   *
   * Additions happen in the same order as serial rendering, so without
   * floating-point contraction the output is bit-identical to it, modulo
   * signed-zero sign bits. With contraction (fused multiply-add), a
   * serial accumulate rounds once while this path rounds each voice's
   * contribution and the sum separately, so targets with fused
   * multiply-adds (e.g. ARM64) may differ by a few ULP. Voices must only
   * add into the target buffer, never read from it (all current voices
   * comply). The parallel path asserts that the output channel count
   * matches the channel count passed to prepare_for_processing() and
   * that the rendered range fits the prepared block size.
   */
  void process (
    juce::AudioBuffer<float> &output,
    const MidiEventBuffer    &midi_events,
    units::sample_u32_t       offset,
    units::sample_u32_t       nframes,
    graph::ForkJoinExecutor * fork_join_executor = nullptr) noexcept
    [[clang::nonblocking]];

private:
  void dispatch_event (std::span<const midi_byte_t> data) noexcept
    [[clang::nonblocking]];
  void note_on (int channel, int pitch, float velocity) noexcept
    [[clang::nonblocking]];
  void note_off (int channel, int pitch) noexcept [[clang::nonblocking]];
  void render_active (
    juce::AudioBuffer<float> &output,
    int                       start_sample,
    int                       num_samples,
    graph::ForkJoinExecutor * fork_join_executor) noexcept
    [[clang::nonblocking]];

private:
  /** Below these thresholds, fork-join overhead outweighs the parallel
   * voice DSP and the serial path is used instead. */
  static constexpr std::uint32_t kMinParallelVoices = 2;
  static constexpr int           kMinParallelBlockSamples = 16;

  std::vector<std::unique_ptr<SynthVoice>> voices_;

  /** One scratch buffer per voice for the parallel rendering path, each
   * num_channels_ x max_block_length. Empty when unprepared. */
  std::vector<juce::AudioBuffer<float>> voice_scratch_;

  /** Preallocated list of active voice indices filled by render_active
   * (sized by prepare_for_processing). */
  std::vector<std::uint32_t> active_voice_indices_;

  /** Channel count of each scratch buffer (0 when unprepared). */
  int num_channels_ = 0;

  /** Last pitch bend value per MIDI channel (for notes started later). */
  std::array<int, 16> last_pitch_bend_;

  /** Monotonic counter for voice age (stealing). */
  std::uint32_t next_note_sequence_ = 1;
};

} // namespace zrythm::dsp
