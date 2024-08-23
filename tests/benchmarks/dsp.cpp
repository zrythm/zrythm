// SPDX-FileCopyrightText: © 2020-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "utils/dsp.h"
#include "utils/objects.h"
#include "zrythm.h"

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/project_helper.h"
#include "tests/helpers/zrythm_helper.h"

#ifdef HAVE_LSP_DSP
#  include <lsp-plug.in/dsp/dsp.h>
#endif

#define BUFFER_SIZE 20
#define LARGE_BUFFER_SIZE 2000

#define NUM_ITERATIONS_ENGINE 1000
#define NUM_ITERATIONS_MANY 30000

#define F_OPTIMIZED 1
#define F_NOT_OPTIMIZED 0

#define F_LARGE_BUF 1
#define F_NOT_LARGE_BUF 0

#define NUM_TRACKS 100

typedef struct DspBenchmark
{
  /* function called */
  const char * func_name;
  /* microseconds taken */
  long unoptimized_usec;
  long optimized_usec;
} DspBenchmark;

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
  if (optimized)
    {
      test_helper_zrythm_init_optimized ();
    }
  else
    {
      test_helper_zrythm_init ();
    }

  gint64  start, end;
  float * buf = object_new_n (LARGE_BUFFER_SIZE, float);
  float * src = object_new_n (LARGE_BUFFER_SIZE, float);
  float   val = 0.3f;

  size_t buf_size = large_buff ? LARGE_BUFFER_SIZE : BUFFER_SIZE;

  DspBenchmark * benchmark;

#define LOOP_START \
  start = g_get_monotonic_time (); \
  for (int i = 0; i < NUM_ITERATIONS_MANY; i++) \
    {

#define LOOP_END(fname, is_optimized) \
  } \
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
    }

  LOOP_START
  dsp_fill (buf, val, buf_size);
  LOOP_END ("fill", optimized);

  LOOP_START
  dsp_limit1 (buf, -1.0f, 1.1f, buf_size);
  LOOP_END ("limit1", optimized);

  LOOP_START
  dsp_add2 (buf, src, buf_size);
  LOOP_END ("add2", optimized);

  LOOP_START
  float abs_max = dsp_abs_max (buf, buf_size);
  (void) abs_max;
  LOOP_END ("abs_max", optimized);

  LOOP_START
  dsp_min (buf, buf_size);
  LOOP_END ("min", optimized);

  LOOP_START
  dsp_max (buf, buf_size);
  LOOP_END ("max", optimized);

  LOOP_START
  dsp_mul_k2 (buf, 0.99f, buf_size);
  LOOP_END ("mul_k2", optimized);

  LOOP_START
  dsp_copy (buf, src, buf_size);
  LOOP_END ("copy", optimized);

  LOOP_START
  dsp_mix2 (buf, src, 0.1f, 0.2f, buf_size);
  LOOP_END ("mix2", optimized);

  LOOP_START
  dsp_mix_add2 (buf, src, src, 0.1f, 0.2f, buf_size);
  LOOP_END ("mix_add2", optimized);

  free (buf);
  free (src);

  test_helper_zrythm_cleanup ();
}

static void
test_dsp_fill (void)
{
  _test_dsp_fill (F_NOT_OPTIMIZED, F_LARGE_BUF);
#ifdef HAVE_LSP_DSP
  _test_dsp_fill (F_OPTIMIZED, F_LARGE_BUF);
#endif
}

static void
_test_run_engine (bool optimized)
{
  if (optimized)
    {
      test_helper_zrythm_init_optimized ();
    }
  else
    {
      test_helper_zrythm_init ();
    }

  AUDIO_ENGINE->stop_dummy_audio_thread = true;
  g_usleep (20000);

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
      LOOP_END ("engine cycles", optimized);

      z_info ("{}optimized time: {}", optimized ? "" : "un", end - start);
      /*z_warn_if_reached ();*/

#ifdef HAVE_LSP_DSP
      if (optimized)
        {
          lsp::dsp::finish (&ctx);
        }
#endif

      test_helper_zrythm_cleanup ();
    }

  static void test_run_engine (void)
  {
#ifdef HAVE_LSP_DSP
    _test_run_engine (F_OPTIMIZED);
#endif
    _test_run_engine (F_NOT_OPTIMIZED);
  }

  static void print_benchmark_results (void)
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

  int main (int argc, char * argv[])
  {
    g_test_init (&argc, &argv, nullptr);

#ifdef HAVE_LSP_DSP
    lsp::dsp::init ();
#endif

#define TEST_PREFIX "/benchmarks/dsp/"

    g_test_add_func (TEST_PREFIX "test dsp fill", (GTestFunc) test_dsp_fill);
    g_test_add_func (TEST_PREFIX "test run engine", (GTestFunc) test_run_engine);
    g_test_add_func (
      TEST_PREFIX "print benchmark results",
      (GTestFunc) print_benchmark_results);

    return g_test_run ();
  }
