// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <ranges>

#include "dsp/fork_join_executor.h"
#include "dsp/midi_event.h"
#include "dsp/poly_voice_manager.h"

#include <benchmark/benchmark.h>

namespace zrythm::dsp
{

namespace
{
constexpr auto kNumVoices = 16;
constexpr auto kNumChannels = 2;
constexpr auto kBlockSize = 256;

/** Voice that burns ~100 ns per sample (heavy-patch scale) with a
 * loop-carried dependent FMA chain that cannot be vectorized or optimized
 * out. */
class BurnVoice : public SynthVoice
{
  static_assert (kNumChannels <= 16, "outs_ has 16 slots");

public:
  explicit BurnVoice (float seed) : acc_ (seed) { }

  void render (
    juce::AudioBuffer<float> &output,
    int                       start_sample,
    int                       num_samples) noexcept override
  {
    const auto num_channels =
      std::min (output.getNumChannels (), static_cast<int> (outs_.size ()));
    for (const auto ch : std::views::iota (0, num_channels))
      {
        outs_[static_cast<size_t> (ch)] =
          output.getWritePointer (ch, start_sample);
      }

    float acc = acc_;
    for (const auto i : std::views::iota (0, num_samples))
      {
        for (const auto _ : std::views::iota (0, kBurnIterationsPerSample))
          {
            // Contractive by construction (fixed point ~5000), so the
            // accumulator stays bounded no matter how many blocks run
            acc = acc * 0.9999f + 0.5f;
          }
        const float value = acc * 1e-9f;
        for (const auto ch : std::views::iota (0, num_channels))
          {
            outs_[static_cast<size_t> (ch)][i] += value;
          }
      }
    acc_ = acc;
  }

private:
  static constexpr int kBurnIterationsPerSample = 64;

  float                   acc_;
  std::array<float *, 16> outs_{};
};

/** Adds and activates kNumVoices burn voices. */
void
add_active_burn_voices (
  PolyVoiceManager         &manager,
  juce::AudioBuffer<float> &output)
{
  for (const auto i : std::views::iota (0, kNumVoices))
    {
      manager.add_voice (
        std::make_unique<BurnVoice> (static_cast<float> (i) + 1.f));
    }
  manager.prepare_for_processing (kNumChannels, units::samples (kBlockSize));

  MidiEventBuffer buf;
  buf.reserve (4096);
  for (const auto i : std::views::iota (0, kNumVoices))
    {
      const auto ev =
        midi_event::make_note_on (0, 36 + i, 100, units::samples (0u));
      buf.push_back (ev.time_, ev.data ());
    }
  manager.process (
    output, buf, units::samples (0u), units::samples (kBlockSize), nullptr);
}
} // namespace

/**
 * @brief PolyVoiceManager block rendering with the parallel voice path.
 *
 * The worker count is taken from the range argument; 0 workers means the
 * submitting thread self-drains (~ serial plus submission overhead).
 */
class PolyVoiceManagerBenchmark : public benchmark::Fixture
{
public:
  void SetUp (benchmark::State &state) override
  {
    executor_.start (static_cast<int> (state.range (0)));
    manager_.emplace ();
    output_.emplace (kNumChannels, kBlockSize);
    add_active_burn_voices (*manager_, *output_);
  }

  void TearDown (benchmark::State &) override
  {
    executor_.stop ();
    // Registered fixtures live until process exit; release the buffers or
    // JUCE's leak detector reports them at exit
    manager_.reset ();
    output_.reset ();
  }

protected:
  graph::ForkJoinExecutor                 executor_;
  std::optional<PolyVoiceManager>         manager_;
  std::optional<juce::AudioBuffer<float>> output_;
  MidiEventBuffer                         empty_buf_;
};

BENCHMARK_DEFINE_F (PolyVoiceManagerBenchmark, RenderBlock)
(benchmark::State &state)
{
  for (auto _ : state)
    {
      manager_->process (
        *output_, empty_buf_, units::samples (0u), units::samples (kBlockSize),
        &executor_);
    }
}

/** The same block rendered via the serial path (no executor). */
static void
BM_PolyVoiceManagerSerialBaseline (benchmark::State &state)
{
  PolyVoiceManager         manager;
  juce::AudioBuffer<float> output{ kNumChannels, kBlockSize };
  add_active_burn_voices (manager, output);
  MidiEventBuffer empty_buf;

  for (auto _ : state)
    {
      manager.process (
        output, empty_buf, units::samples (0u), units::samples (kBlockSize),
        nullptr);
    }
}

BENCHMARK_REGISTER_F (PolyVoiceManagerBenchmark, RenderBlock)
  ->Args ({ 0 })
  ->Args ({ 2 })
  ->Args ({ 4 })
  ->Args ({ 8 })
  ->Args ({ 16 })
  ->Unit (benchmark::kMicrosecond);

BENCHMARK (BM_PolyVoiceManagerSerialBaseline)->Unit (benchmark::kMicrosecond);

} // namespace zrythm::dsp
