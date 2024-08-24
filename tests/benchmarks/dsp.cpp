// SPDX-FileCopyrightText: © 2020-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "utils/dsp.h"
#include "utils/objects.h"
#include "zrythm.h"

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/project_helper.h"
#include "tests/helpers/zrythm_helper.h"

#ifdef HAVE_LSP_DSP
#  include <lsp-plug.in/dsp/dsp.h>
#endif

TEST_SUITE_BEGIN ("benchmarks/dsp");

// Buffer sizes
constexpr int BUFFER_SIZE = 20;
constexpr int LARGE_BUFFER_SIZE = 2000;

// Iteration counts
constexpr int NUM_ITERATIONS_ENGINE = 1000;
constexpr int NUM_ITERATIONS_MANY = 30000;

// Optimization flags
constexpr int F_OPTIMIZED = 1;
constexpr int F_NOT_OPTIMIZED = 0;

// Buffer size flags
constexpr int F_LARGE_BUF = 1;

// Track count
constexpr int NUM_TRACKS = 100;

struct DspBenchmark
{
  /* function called */
  const char * func_name;
  /* microseconds taken */
  long unoptimized_usec;
  long optimized_usec;
};

static DspBenchmark benchmarks[400];
static int          num_benchmarks = 0;

static DspBenchmark *
benchmark_find (const char * func_name)
{
  for (int i = 0; i < num_benchmarks; i++)
    {
      DspBenchmark * benchmark = &benchmarks[i];
      if (string_is_equal (benchmark->func_name, func_name))
        {
          return benchmark;
        }
    }
  return NULL;
}

static void
_test_dsp_fill (bool optimized, bool large_buff)
{
  std::optional<ZrythmFixture> fixture;
  if (optimized)
    {
      fixture.emplace (ZrythmFixtureOptimized ());
    }
  else
    {
      fixture.emplace (ZrythmFixture ());
    }

  gint64  start, end;
  float * buf = object_new_n (LARGE_BUFFER_SIZE, float);
  float * src = object_new_n (LARGE_BUFFER_SIZE, float);
  float   val = 0.3f;

  size_t buf_size = large_buff ? LARGE_BUFFER_SIZE : BUFFER_SIZE;

  DspBenchmark * benchmark;

#define LOOP_START \
  start = g_get_monotonic_time (); \
  for (int i = 0; i < NUM_ITERATIONS_MANY; i++)

#define LOOP_END(fname, is_optimized) \
  { \
    end = g_get_monotonic_time (); \
    benchmark = benchmark_find (fname); \
    if (!benchmark) \
      { \
        benchmark = &benchmarks[num_benchmarks]; \
        num_benchmarks++; \
      } \
    benchmark->func_name = fname; \
    if (is_optimized) \
      { \
        benchmark->optimized_usec = end - start; \
      } \
    else \
      { \
        benchmark->unoptimized_usec = end - start; \
      } \
  }

  LOOP_START
  {
    dsp_fill (buf, val, buf_size);
  }
  LOOP_END ("fill", optimized);

  LOOP_START
  {
    dsp_limit1 (buf, -1.0f, 1.1f, buf_size);
  }
  LOOP_END ("limit1", optimized);

  LOOP_START
  {
    dsp_add2 (buf, src, buf_size);
  }
  LOOP_END ("add2", optimized);

  LOOP_START
  {
    float abs_max = dsp_abs_max (buf, buf_size);
    (void) abs_max;
  }
  LOOP_END ("abs_max", optimized);

  LOOP_START
  {
    dsp_min (buf, buf_size);
  }
  LOOP_END ("min", optimized);

  LOOP_START
  {
    dsp_max (buf, buf_size);
  }
  LOOP_END ("max", optimized);

  LOOP_START
  {
    dsp_mul_k2 (buf, 0.99f, buf_size);
  }
  LOOP_END ("mul_k2", optimized);

  LOOP_START
  {
    dsp_copy (buf, src, buf_size);
  }
  LOOP_END ("copy", optimized);

  LOOP_START
  {
    dsp_mix2 (buf, src, 0.1f, 0.2f, buf_size);
  }
  LOOP_END ("mix2", optimized);

  LOOP_START
  {
    dsp_mix_add2 (buf, src, src, 0.1f, 0.2f, buf_size);
  }
  LOOP_END ("mix_add2", optimized);

  free (buf);
  free (src);
}

TEST_CASE ("fill")
{
  _test_dsp_fill (F_NOT_OPTIMIZED, F_LARGE_BUF);
#ifdef HAVE_LSP_DSP
  _test_dsp_fill (F_OPTIMIZED, F_LARGE_BUF);
#endif
}

static void
_test_run_engine (bool optimized)
{
  std::optional<ZrythmFixture> fixture;
  if (optimized)
    {
      fixture.emplace (ZrythmFixtureOptimized ());
    }
  else
    {
      fixture.emplace (ZrythmFixture ());
    }

  test_project_stop_dummy_engine ();

#ifdef HAVE_LSP_DSP
  lsp::dsp::context_t ctx;
  if (optimized)
    {
      lsp::dsp::start (&ctx);
    }
#endif

    /* create a few tracks with plugins */
#ifdef HAVE_LSP_COMPRESSOR
  test_plugin_manager_create_tracks_from_plugin (
    LSP_COMPRESSOR_BUNDLE, LSP_COMPRESSOR_URI, false, false, NUM_TRACKS);
#endif

  DspBenchmark * benchmark;

  gint64 start, end;
  start = g_get_monotonic_time ();
  for (int i = 0; i < NUM_ITERATIONS_ENGINE; i++)
    {
      AUDIO_ENGINE->process (AUDIO_ENGINE->block_length_);
    }
  LOOP_END ("engine cycles", optimized);

  z_info ("{}optimized time: {}", optimized ? "" : "un", end - start);
  /*z_warn_if_reached ();*/

#ifdef HAVE_LSP_DSP
  if (optimized)
    {
      lsp::dsp::finish (&ctx);
    }
#endif
}

TEST_CASE ("run engine")
{
#ifdef HAVE_LSP_DSP
  _test_run_engine (F_OPTIMIZED);
#endif
  _test_run_engine (F_NOT_OPTIMIZED);
}

TEST_CASE ("print benchmark results")
{
  for (int i = 0; i < num_benchmarks; i++)
    {
      DspBenchmark * benchmark = &benchmarks[i];
      fprintf (
        stderr,
        "---- %s ----\n"
        "unoptimized: %ldms\n"
        "optimized: %ldms\n",
        benchmark->func_name, benchmark->unoptimized_usec / 1000,
        benchmark->optimized_usec / 1000);
    }
}

TEST_SUITE_END;