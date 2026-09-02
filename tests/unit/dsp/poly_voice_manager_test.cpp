// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <memory>
#include <ranges>
#include <utility>
#include <vector>

#include "dsp/fork_join_executor.h"
#include "dsp/midi_event.h"
#include "dsp/poly_voice_manager.h"

#include <gtest/gtest.h>

namespace zrythm::dsp
{

/** Records calls for verification. Vectors are pre-reserved so recording is
 * allocation-free in RT context. */
class MockVoice : public SynthVoice
{
public:
  MockVoice () { render_calls_.reserve (64); }

  void note_on (
    int           channel,
    int           pitch,
    float         velocity,
    std::uint32_t note_sequence) noexcept override
  {
    SynthVoice::note_on (channel, pitch, velocity, note_sequence);
    ++start_count_;
    last_pitch_ = pitch;
    last_velocity_ = velocity;
  }

  void note_off () noexcept override
  {
    // Keep active (simulates an envelope voice with a release tail)
    ++note_off_count_;
  }

  void cut () noexcept override
  {
    SynthVoice::cut ();
    ++cut_count_;
  }

  void pitch_bend (int value) noexcept override { last_pitch_bend_ = value; }

  void render (
    juce::AudioBuffer<float> &output,
    int                       start_sample,
    int                       num_samples) noexcept override
  {
    render_calls_.push_back ({ start_sample, num_samples });
  }

  int   start_count_{};
  int   note_off_count_{};
  int   cut_count_{};
  int   last_pitch_{ -1 };
  float last_velocity_{};
  int   last_pitch_bend_{ -1 };

  std::vector<std::pair<int, int>> render_calls_;
};

class PolyVoiceManagerTest : public ::testing::Test
{
protected:
  MockVoice * add_mock_voice ()
  {
    auto   voice = std::make_unique<MockVoice> ();
    auto * ptr = voice.get ();
    manager_.add_voice (std::move (voice));
    return ptr;
  }

  static MidiEventBuffer make_buffer ()
  {
    MidiEventBuffer buf;
    buf.reserve (4096);
    return buf;
  }

  template <typename TimeType>
  static void push_event (MidiEventBuffer &buf, const MidiEvent<TimeType> &ev)
  {
    buf.push_back (ev.time_, ev.data ());
  }

  PolyVoiceManager         manager_;
  juce::AudioBuffer<float> output_{ 2, 512 };
};

TEST_F (PolyVoiceManagerTest, NoteOnAllocatesFreeVoicesInOrder)
{
  auto * v0 = add_mock_voice ();
  auto * v1 = add_mock_voice ();

  auto buf = make_buffer ();
  push_event (buf, midi_event::make_note_on (0, 60, 100, units::samples (0u)));
  push_event (buf, midi_event::make_note_on (0, 62, 100, units::samples (1u)));
  manager_.process (output_, buf, units::samples (0u), units::samples (256u));

  EXPECT_TRUE (v0->is_active ());
  EXPECT_EQ (v0->current_note (), 60);
  EXPECT_TRUE (v1->is_active ());
  EXPECT_EQ (v1->current_note (), 62);
  EXPECT_FLOAT_EQ (v0->last_velocity_, 100.f / 127.f);
}

TEST_F (PolyVoiceManagerTest, StealsOldestVoiceWhenFull)
{
  auto * v0 = add_mock_voice ();
  auto * v1 = add_mock_voice ();

  auto buf = make_buffer ();
  push_event (buf, midi_event::make_note_on (0, 60, 100, units::samples (0u)));
  push_event (buf, midi_event::make_note_on (0, 62, 100, units::samples (1u)));
  push_event (buf, midi_event::make_note_on (0, 64, 100, units::samples (2u)));
  manager_.process (output_, buf, units::samples (0u), units::samples (256u));

  // v0 was the oldest - stolen for the third note
  EXPECT_EQ (v0->current_note (), 64);
  EXPECT_EQ (v0->start_count_, 2);
  EXPECT_EQ (v0->cut_count_, 1);
  EXPECT_EQ (v1->current_note (), 62);
  EXPECT_EQ (v1->start_count_, 1);
}

TEST_F (PolyVoiceManagerTest, NoteOffMatchesPitchAndChannel)
{
  auto * v0 = add_mock_voice ();
  auto * v1 = add_mock_voice ();

  auto buf = make_buffer ();
  // Same pitch on two channels
  push_event (buf, midi_event::make_note_on (0, 60, 100, units::samples (0u)));
  push_event (buf, midi_event::make_note_on (1, 60, 100, units::samples (1u)));
  manager_.process (output_, buf, units::samples (0u), units::samples (256u));
  ASSERT_TRUE (v0->is_active ());
  ASSERT_TRUE (v1->is_active ());

  // Note-off on channel 0 only releases the channel-0 voice
  auto buf2 = make_buffer ();
  push_event (
    buf2,
    midi_event::make_note_off_with_default_velocity (0, 60, units::samples (0u)));
  manager_.process (output_, buf2, units::samples (0u), units::samples (256u));

  EXPECT_EQ (v0->note_off_count_, 1);
  // Tail-off is the voice's choice: the mock stays active after note_off
  EXPECT_TRUE (v0->is_active ());
  EXPECT_EQ (v1->note_off_count_, 0);
}

TEST_F (PolyVoiceManagerTest, NoteOnWithZeroVelocityActsAsNoteOff)
{
  auto * v0 = add_mock_voice ();

  auto buf = make_buffer ();
  push_event (buf, midi_event::make_note_on (0, 60, 100, units::samples (0u)));
  manager_.process (output_, buf, units::samples (0u), units::samples (256u));
  ASSERT_TRUE (v0->is_active ());

  // A note-on with velocity 0 is a note-off (construct raw - make_note_on
  // asserts velocity != 0)
  auto                             buf2 = make_buffer ();
  const std::array<midi_byte_t, 3> note_on_zero_vel = { 0x90, 60, 0 };
  buf2.push_back (units::samples (0u), note_on_zero_vel);
  manager_.process (output_, buf2, units::samples (0u), units::samples (256u));
  EXPECT_EQ (v0->note_off_count_, 1);
}

TEST_F (PolyVoiceManagerTest, RendersSampleAccuratelyAroundEvents)
{
  auto * v0 = add_mock_voice ();

  auto buf = make_buffer ();
  push_event (buf, midi_event::make_note_on (0, 60, 100, units::samples (64u)));
  push_event (buf, midi_event::make_pitchbend (0, 4000, units::samples (128u)));
  manager_.process (output_, buf, units::samples (0u), units::samples (256u));

  // Render starts at the note-on, then splits at the pitch bend
  ASSERT_EQ (v0->render_calls_.size (), 2u);
  EXPECT_EQ (v0->render_calls_[0], (std::pair<int, int>{ 64, 64 }));
  EXPECT_EQ (v0->render_calls_[1], (std::pair<int, int>{ 128, 128 }));
}

TEST_F (PolyVoiceManagerTest, EventsOutsideBlockAreIgnored)
{
  auto * v0 = add_mock_voice ();

  auto buf = make_buffer ();
  // Event before the block start
  push_event (buf, midi_event::make_note_on (0, 60, 100, units::samples (100u)));
  manager_.process (output_, buf, units::samples (512u), units::samples (256u));

  EXPECT_FALSE (v0->is_active ());
  EXPECT_TRUE (v0->render_calls_.empty ());
}

TEST_F (PolyVoiceManagerTest, PitchWheelBroadcastAndTrackedForNewNotes)
{
  auto * v0 = add_mock_voice ();
  auto * v1 = add_mock_voice ();

  auto buf = make_buffer ();
  push_event (buf, midi_event::make_note_on (0, 60, 100, units::samples (0u)));
  push_event (buf, midi_event::make_pitchbend (0, 4000, units::samples (64u)));
  manager_.process (output_, buf, units::samples (0u), units::samples (256u));

  // Active voice received the bend
  EXPECT_EQ (v0->last_pitch_bend_, 4000);
  EXPECT_EQ (v1->last_pitch_bend_, 4000);

  // A note started later inherits the channel's current bend
  auto buf2 = make_buffer ();
  push_event (buf2, midi_event::make_note_on (0, 62, 100, units::samples (0u)));
  manager_.process (output_, buf2, units::samples (0u), units::samples (256u));
  EXPECT_EQ (v1->last_pitch_bend_, 4000);
}

TEST_F (PolyVoiceManagerTest, AllNotesOffDeactivatesImmediately)
{
  auto * v0 = add_mock_voice ();
  auto * v1 = add_mock_voice ();

  auto buf = make_buffer ();
  push_event (buf, midi_event::make_note_on (0, 60, 100, units::samples (0u)));
  push_event (buf, midi_event::make_note_on (0, 62, 100, units::samples (1u)));
  manager_.process (output_, buf, units::samples (0u), units::samples (256u));
  ASSERT_TRUE (v0->is_active ());
  ASSERT_TRUE (v1->is_active ());

  manager_.all_notes_off ();
  EXPECT_FALSE (v0->is_active ());
  EXPECT_FALSE (v1->is_active ());
  EXPECT_EQ (v0->cut_count_, 1);
  EXPECT_EQ (v1->cut_count_, 1);
}

TEST_F (PolyVoiceManagerTest, AllNotesOffCcReleasesChannelVoices)
{
  auto * v0 = add_mock_voice ();
  auto * v1 = add_mock_voice ();

  auto buf = make_buffer ();
  push_event (buf, midi_event::make_note_on (0, 60, 100, units::samples (0u)));
  push_event (buf, midi_event::make_note_on (1, 62, 100, units::samples (1u)));
  manager_.process (output_, buf, units::samples (0u), units::samples (256u));
  ASSERT_TRUE (v0->is_active ());
  ASSERT_TRUE (v1->is_active ());

  // CC 123 on channel 0 releases only the channel-0 voice
  auto buf2 = make_buffer ();
  push_event (buf2, midi_event::make_all_notes_off (0, units::samples (0u)));
  manager_.process (output_, buf2, units::samples (0u), units::samples (256u));

  EXPECT_EQ (v0->note_off_count_, 1);
  EXPECT_EQ (v0->cut_count_, 0);
  // Tail-off is the voice's choice: the mock stays active after note_off
  EXPECT_TRUE (v0->is_active ());
  EXPECT_EQ (v1->note_off_count_, 0);
  EXPECT_TRUE (v1->is_active ());
}

TEST_F (PolyVoiceManagerTest, AllSoundOffCcCutsChannelVoices)
{
  auto * v0 = add_mock_voice ();
  auto * v1 = add_mock_voice ();

  auto buf = make_buffer ();
  push_event (buf, midi_event::make_note_on (0, 60, 100, units::samples (0u)));
  push_event (buf, midi_event::make_note_on (1, 62, 100, units::samples (1u)));
  manager_.process (output_, buf, units::samples (0u), units::samples (256u));
  ASSERT_TRUE (v0->is_active ());
  ASSERT_TRUE (v1->is_active ());

  // CC 120 on channel 0 cuts only the channel-0 voice
  auto                             buf2 = make_buffer ();
  const std::array<midi_byte_t, 3> all_sound_off = { 0xb0, 120, 0 };
  buf2.push_back (units::samples (0u), all_sound_off);
  manager_.process (output_, buf2, units::samples (0u), units::samples (256u));

  EXPECT_EQ (v0->cut_count_, 1);
  EXPECT_FALSE (v0->is_active ());
  EXPECT_EQ (v1->cut_count_, 0);
  EXPECT_TRUE (v1->is_active ());
}

/** Adds position-dependent pseudo-random values so the serial vs parallel
 * comparison is meaningful. */
class DeterministicVoice : public SynthVoice
{
public:
  explicit DeterministicVoice (std::uint32_t seed) : seed_ (seed) { }

  void render (
    juce::AudioBuffer<float> &output,
    int                       start_sample,
    int                       num_samples) noexcept override
  {
    // Instrumentation for the fork-join tests: record whether the target
    // region already contained signal at render entry. Parallel-path
    // scratch is always zeroed; serial output accumulates earlier voices.
    if (!saw_nonzero_target_)
      {
        for (const auto ch : std::views::iota (0, output.getNumChannels ()))
          {
            const auto * in = output.getReadPointer (ch, start_sample);
            saw_nonzero_target_ = std::ranges::any_of (
              in, in + num_samples, [] (const float v) { return v != 0.f; });
            if (saw_nonzero_target_)
              break;
          }
      }

    for (const auto ch : std::views::iota (0, output.getNumChannels ()))
      {
        auto * out = output.getWritePointer (ch, start_sample);
        for (const auto i : std::views::iota (0, num_samples))
          {
            const auto hash =
              (static_cast<std::uint32_t> (start_sample + i) * 2654435761u)
              ^ (seed_ * 40503u) ^ (static_cast<std::uint32_t> (ch) * 969u);
            out[static_cast<size_t> (i)] +=
              static_cast<float> (hash % 10000u) * 0.0001f;
          }
      }
  }

  bool saw_nonzero_target () const { return saw_nonzero_target_; }

private:
  bool          saw_nonzero_target_ = false;
  std::uint32_t seed_;
};

/** Renders a fresh 4-voice manager into @p output; @p executor may be
 * nullptr (serial path). Returns true if any voice observed an already
 * non-zero target region at render entry — expected for serial rendering
 * (voices accumulate into shared output), false when the parallel path
 * rendered into zeroed scratch. */
static bool
render_four_voices (
  juce::AudioBuffer<float> &output,
  graph::ForkJoinExecutor * executor,
  bool                      prepare = true)
{
  PolyVoiceManager                    manager;
  std::array<DeterministicVoice *, 4> voice_ptrs{};
  for (const auto i : std::views::iota (0u, 4u))
    {
      auto voice = std::make_unique<DeterministicVoice> (i + 1);
      voice_ptrs[i] = voice.get ();
      manager.add_voice (std::move (voice));
    }
  if (prepare)
    manager.prepare_for_processing (
      output.getNumChannels (), units::samples (512u));

  MidiEventBuffer buf;
  buf.reserve (4096);
  for (const auto i : std::views::iota (0u, 4u))
    {
      const auto ev = midi_event::make_note_on (
        0, 60 + static_cast<int> (i), 100, units::samples (0u));
      buf.push_back (ev.time_, ev.data ());
    }
  manager.process (
    output, buf, units::samples (0u), units::samples (256u), executor);

  return std::ranges::any_of (voice_ptrs, [] (const auto * voice) {
    return voice->saw_nonzero_target ();
  });
}

/** Maximum ULP distance accepted between serial and parallel rendering.
 * The only permitted difference is floating-point contraction (fused
 * multiply-add): a serial accumulate rounds once, while the parallel
 * path rounds each voice's scratch contribution and the summing pass
 * separately. With 4 voices this stays within a few ULP, while dropped
 * or duplicated voices differ by orders of magnitude more.
 */
constexpr std::uint32_t kMaxSerialParallelUlpDistance = 4;

/** Maps a float onto a monotonic integer axis so ULP distance can be
 * computed across zero. */
static std::uint32_t
ordered_bits (float value)
{
  const auto bits = std::bit_cast<std::uint32_t> (value);
  return (bits & 0x8000'0000u) != 0u ? ~bits : (bits | 0x8000'0000u);
}

static void
expect_equal_within_ulp (
  const juce::AudioBuffer<float> &a,
  const juce::AudioBuffer<float> &b,
  int                             num_samples)
{
  ASSERT_EQ (a.getNumChannels (), b.getNumChannels ());
  for (const auto ch : std::views::iota (0, a.getNumChannels ()))
    {
      for (const auto i : std::views::iota (0, num_samples))
        {
          const auto ua = ordered_bits (a.getReadPointer (ch)[i]);
          const auto ub = ordered_bits (b.getReadPointer (ch)[i]);
          const auto distance = ua > ub ? ua - ub : ub - ua;
          EXPECT_LE (distance, kMaxSerialParallelUlpDistance)
            << "ch " << ch << " sample " << i;
        }
    }
}

// The parallel path must produce the same output as serial rendering,
// within floating-point contraction rounding
TEST_F (
  PolyVoiceManagerTest,
  ParallelRenderingMatchesSerialWithinRoundingTolerance)
{
  juce::AudioBuffer<float> serial_out (2, 512);
  juce::AudioBuffer<float> parallel_out (2, 512);
  serial_out.clear ();
  parallel_out.clear ();

  EXPECT_TRUE (render_four_voices (serial_out, nullptr));

  graph::ForkJoinExecutor executor;
  executor.start (2);
  // False here means the parallel path actually rendered into scratch;
  // true would mean it silently degraded to serial
  EXPECT_FALSE (render_four_voices (parallel_out, &executor));
  executor.stop ();

  expect_equal_within_ulp (serial_out, parallel_out, 512);
}

// A rejected fork-join job (executor unavailable) must fall back to serial
// rendering with identical output
TEST_F (PolyVoiceManagerTest, ParallelRenderingFallsBackWhenExecutorUnavailable)
{
  juce::AudioBuffer<float> serial_out (2, 512);
  juce::AudioBuffer<float> fallback_out (2, 512);
  serial_out.clear ();
  fallback_out.clear ();

  EXPECT_TRUE (render_four_voices (serial_out, nullptr));

  graph::ForkJoinExecutor executor; // never started: exec() rejects the job
  // True here proves the fallback actually rendered serially
  EXPECT_TRUE (render_four_voices (fallback_out, &executor));

  expect_equal_within_ulp (serial_out, fallback_out, 512);
}

// Without prepare_for_processing(), the parallel path is unavailable and
// rendering falls back to serial even with an executor
TEST_F (PolyVoiceManagerTest, UnpreparedManagerWithExecutorTakesSerialPath)
{
  juce::AudioBuffer<float> serial_out (2, 512);
  juce::AudioBuffer<float> executor_out (2, 512);
  serial_out.clear ();
  executor_out.clear ();

  EXPECT_TRUE (render_four_voices (serial_out, nullptr));

  graph::ForkJoinExecutor executor;
  executor.start (2);
  EXPECT_TRUE (render_four_voices (executor_out, &executor, false));
  executor.stop ();

  expect_equal_within_ulp (serial_out, executor_out, 512);
}

// Adding voices after prepare_for_processing() invalidates the parallel
// scratch setup: rendering falls back to serial until the next prepare
TEST_F (PolyVoiceManagerTest, VoicesAddedAfterPrepareTakeSerialPath)
{
  juce::AudioBuffer<float> serial_out (2, 512);
  juce::AudioBuffer<float> executor_out (2, 512);
  serial_out.clear ();
  executor_out.clear ();

  const auto render_with_late_fifth_voice =
    [] (juce::AudioBuffer<float> &output, graph::ForkJoinExecutor * executor) {
      PolyVoiceManager                    manager;
      std::array<DeterministicVoice *, 5> voice_ptrs{};
      for (const auto i : std::views::iota (0u, 4u))
        {
          auto voice = std::make_unique<DeterministicVoice> (i + 1);
          voice_ptrs[i] = voice.get ();
          manager.add_voice (std::move (voice));
        }
      manager.prepare_for_processing (
        output.getNumChannels (), units::samples (512u));
      // Fifth voice joins after preparation
      auto late_voice = std::make_unique<DeterministicVoice> (5u);
      voice_ptrs[4] = late_voice.get ();
      manager.add_voice (std::move (late_voice));

      MidiEventBuffer buf;
      buf.reserve (4096);
      for (const auto i : std::views::iota (0u, 5u))
        {
          const auto ev = midi_event::make_note_on (
            0, 60 + static_cast<int> (i), 100, units::samples (0u));
          buf.push_back (ev.time_, ev.data ());
        }
      manager.process (
        output, buf, units::samples (0u), units::samples (256u), executor);

      return std::ranges::any_of (voice_ptrs, [] (const auto * voice) {
        return voice->saw_nonzero_target ();
      });
    };

  EXPECT_TRUE (render_with_late_fifth_voice (serial_out, nullptr));

  graph::ForkJoinExecutor executor;
  executor.start (2);
  EXPECT_TRUE (render_with_late_fifth_voice (executor_out, &executor));
  executor.stop ();

  expect_equal_within_ulp (serial_out, executor_out, 512);
}

/** Deactivates itself once rendering reaches kDeactivateFromSample (mimics
 * a voice whose release tail ends mid-block). */
class SelfDeactivatingVoice final : public DeterministicVoice
{
public:
  using DeterministicVoice::DeterministicVoice;

  void render (
    juce::AudioBuffer<float> &output,
    int                       start_sample,
    int                       num_samples) noexcept override
  {
    DeterministicVoice::render (output, start_sample, num_samples);
    if (start_sample >= kDeactivateFromSample)
      cut ();
  }

  static constexpr int kDeactivateFromSample = 128;
};

// A mid-block event splits rendering into sub-blocks; parallel rendering
// must match serial within contraction rounding, with sub-blocks starting
// past sample 0 rendered at the same absolute range as serial
TEST_F (PolyVoiceManagerTest, MidBlockEventSplitsParallelSubBlocks)
{
  juce::AudioBuffer<float> serial_out (2, 512);
  juce::AudioBuffer<float> parallel_out (2, 512);
  serial_out.clear ();
  parallel_out.clear ();

  const auto render_split_block =
    [] (juce::AudioBuffer<float> &output, graph::ForkJoinExecutor * executor) {
      PolyVoiceManager                    manager;
      std::array<DeterministicVoice *, 4> voice_ptrs{};
      for (const auto i : std::views::iota (0u, 4u))
        {
          auto voice = std::make_unique<DeterministicVoice> (i + 1);
          voice_ptrs[i] = voice.get ();
          manager.add_voice (std::move (voice));
        }
      manager.prepare_for_processing (
        output.getNumChannels (), units::samples (512u));

      MidiEventBuffer buf;
      buf.reserve (4096);
      for (const auto i : std::views::iota (0u, 2u))
        {
          const auto ev = midi_event::make_note_on (
            0, 60 + static_cast<int> (i), 100, units::samples (0u));
          buf.push_back (ev.time_, ev.data ());
        }
      for (const auto i : std::views::iota (0u, 2u))
        {
          const auto ev = midi_event::make_note_on (
            0, 64 + static_cast<int> (i), 100, units::samples (128u));
          buf.push_back (ev.time_, ev.data ());
        }
      manager.process (
        output, buf, units::samples (0u), units::samples (256u), executor);

      return std::ranges::any_of (voice_ptrs, [] (const auto * voice) {
        return voice->saw_nonzero_target ();
      });
    };

  EXPECT_TRUE (render_split_block (serial_out, nullptr));

  graph::ForkJoinExecutor executor;
  executor.start (2);
  EXPECT_FALSE (render_split_block (parallel_out, &executor));
  executor.stop ();

  expect_equal_within_ulp (serial_out, parallel_out, 512);
}

// A voice that deactivates itself mid-render inside a parallel task must
// produce output matching serial within contraction rounding, and
// identical final voice state
TEST_F (PolyVoiceManagerTest, SelfDeactivatingVoiceInParallelTask)
{
  juce::AudioBuffer<float> serial_out (2, 512);
  juce::AudioBuffer<float> parallel_out (2, 512);
  serial_out.clear ();
  parallel_out.clear ();

  // Returns { any voice saw a non-zero target, voices still active after }
  const auto render_with_deactivating_voices =
    [] (juce::AudioBuffer<float> &output, graph::ForkJoinExecutor * executor) {
      PolyVoiceManager                       manager;
      std::array<SelfDeactivatingVoice *, 4> voice_ptrs{};
      for (const auto i : std::views::iota (0u, 4u))
        {
          auto voice = std::make_unique<SelfDeactivatingVoice> (i + 1);
          voice_ptrs[i] = voice.get ();
          manager.add_voice (std::move (voice));
        }
      manager.prepare_for_processing (
        output.getNumChannels (), units::samples (512u));

      MidiEventBuffer buf;
      buf.reserve (4096);
      for (const auto i : std::views::iota (0u, 2u))
        {
          const auto ev = midi_event::make_note_on (
            0, 60 + static_cast<int> (i), 100, units::samples (0u));
          buf.push_back (ev.time_, ev.data ());
        }
      for (const auto i : std::views::iota (0u, 2u))
        {
          const auto ev = midi_event::make_note_on (
            0, 64 + static_cast<int> (i), 100,
            units::samples (
              static_cast<std::uint32_t> (
                SelfDeactivatingVoice::kDeactivateFromSample)));
          buf.push_back (ev.time_, ev.data ());
        }
      manager.process (
        output, buf, units::samples (0u), units::samples (256u), executor);

      // Read results before the manager (and its voices) are destroyed
      const bool saw_nonzero =
        std::ranges::any_of (voice_ptrs, [] (const auto * voice) {
          return voice->saw_nonzero_target ();
        });
      const auto still_active = std::ranges::count_if (
        voice_ptrs, [] (const auto * voice) { return voice->is_active (); });
      return std::pair{ saw_nonzero, still_active };
    };

  const auto serial_result =
    render_with_deactivating_voices (serial_out, nullptr);
  EXPECT_TRUE (serial_result.first);
  // Every voice reaches the deactivation sample in its final sub-block
  EXPECT_EQ (serial_result.second, 0);

  graph::ForkJoinExecutor executor;
  executor.start (2);
  const auto parallel_result =
    render_with_deactivating_voices (parallel_out, &executor);
  executor.stop ();
  EXPECT_FALSE (parallel_result.first);
  EXPECT_EQ (parallel_result.second, 0);

  expect_equal_within_ulp (serial_out, parallel_out, 512);
}

} // namespace zrythm::dsp
