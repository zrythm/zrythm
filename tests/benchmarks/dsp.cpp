// SPDX-FileCopyrightText: © 2020-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "common/utils/dsp.h"
#include "common/utils/objects.h"
#include "gui/backend/backend/zrythm.h"

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/project_helper.h"
#include "tests/helpers/zrythm_helper.h"

#include <benchmark/benchmark.h>

// Buffer sizes
constexpr int BUFFER_SIZE = 20;
constexpr int LARGE_BUFFER_SIZE = 2000;

/**
 * @brief Concrete class to use in benchmarks.
 */
class BenchmarkZrythmFixture : public ZrythmFixture
{
public:
  BenchmarkZrythmFixture (bool optimized)
      : ZrythmFixture (optimized, 0, 0, false, false)
  {
    SetUp ();
  }
  ~BenchmarkZrythmFixture () override { TearDown (); }
  void TestBody () override { }
};

static void
BM_DspFunctions (benchmark::State &state)
{
  bool optimized = state.range (0);
  bool large_buff = state.range (1);
  int  algo_to_run = state.range (2);

  BenchmarkZrythmFixture fixture (optimized);

  std::vector<float> buf (LARGE_BUFFER_SIZE, 0.5f);
  std::vector<float> src (LARGE_BUFFER_SIZE, 0.5f);
  constexpr float    val = 0.3f;

  const size_t buf_size = large_buff ? LARGE_BUFFER_SIZE : BUFFER_SIZE;

  spdlog::set_level (spdlog::level::off);
  for (auto _ : state)
    {
      switch (algo_to_run)
        {
        case 0:
          dsp_fill (buf.data (), val, buf_size);
          break;
        case 1:
          dsp_clip (buf.data (), -1.0f, 1.1f, buf_size);
          break;
        case 2:
          dsp_add2 (buf.data (), src.data (), buf_size);
          break;
        case 3:
          {
            [[maybe_unused]] float abs_max = dsp_abs_max (buf.data (), buf_size);
          }
          break;
        case 4:
          dsp_min (buf.data (), buf_size);
          break;
        case 5:
          dsp_max (buf.data (), buf_size);
          break;
        case 6:
          dsp_mul_k2 (buf.data (), 0.99f, buf_size);
          break;
        case 7:
          dsp_copy (buf.data (), src.data (), buf_size);
          break;
        case 8:
          dsp_mix_add2 (
            buf.data (), src.data (), src.data (), 0.1f, 0.2f, buf_size);
          break;
        }
    }
}

// Define the range of values for the second argument
static const std::vector<std::vector<int64_t>> argument_ranges = {
  { 0, 1 }, // First argument: optimized (0 or 1)
  { 0, 1 }, // Second argument: large buffer (0 or 1)
  { 0, 1, 2, 3, 4, 5, 6, 7, 8 }  // Third argument: algorithm to run
};

BENCHMARK (BM_DspFunctions)->ArgsProduct (argument_ranges);

#if 0
static void
BM_RunEngine (benchmark::State &state)
{
  bool optimized = state.range (0);

  ZrythmFixture fixture(optimized, 0, 0, false, false);

  test_project_stop_dummy_engine ();

#  ifdef HAVE_LSP_COMPRESSOR
// Track count
// constexpr int NUM_TRACKS = 4;
  test_plugin_manager_create_tracks_from_plugin (
    LSP_COMPRESSOR_BUNDLE, LSP_COMPRESSOR_URI, false, false, NUM_TRACKS);
#  endif

  for (auto _ : state)
    {
      AUDIO_ENGINE->process (AUDIO_ENGINE->block_length_);
    }
}

BENCHMARK (BM_RunEngine)->Arg (0)->Arg (1);
#endif

BENCHMARK_MAIN ();