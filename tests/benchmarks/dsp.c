/*
 * Copyright (C) 2020-2022 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "zrythm-test-config.h"

#include "utils/dsp.h"
#include "utils/objects.h"
#include "zrythm.h"

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/project.h"
#include "tests/helpers/zrythm.h"

#ifdef HAVE_LSP_DSP
#include <lsp-plug.in/dsp/dsp.h>
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
  char *       func_name;
  /* microseconds taken */
  long         unoptimized_usec;
  long         optimized_usec;
  unsigned int max_split;
} DspBenchmark;

static DspBenchmark benchmarks[400];
static int num_benchmarks = 0;

static DspBenchmark *
benchmark_find (
  const char * func_name)
{
  for (int i = 0; i < num_benchmarks; i++)
    {
      DspBenchmark * benchmark = &benchmarks[i];
      if (string_is_equal (
            benchmark->func_name, func_name))
        {
          return benchmark;
        }
    }
  return NULL;
}

static void
_test_dsp_fill (
  bool optimized,
  bool large_buff)
{
  if (optimized)
    {
      test_helper_zrythm_init_optimized ();
    }
  else
    {
      test_helper_zrythm_init ();
    }

  gint64 start, end;
  float * buf =
    object_new_n (LARGE_BUFFER_SIZE, float);
  float * src =
    object_new_n (LARGE_BUFFER_SIZE, float);
  float val = 0.3f;

  size_t buf_size =
    large_buff ? LARGE_BUFFER_SIZE : BUFFER_SIZE;

  DspBenchmark * benchmark;

#define LOOP_START \
  start = g_get_monotonic_time (); \
  for (int i = 0; i < NUM_ITERATIONS_MANY; i++) \
    {

#define LOOP_END(fname,is_optimized,_max_split) \
    } \
  end = g_get_monotonic_time (); \
  benchmark = benchmark_find (fname); \
  if (!benchmark) \
    { \
      benchmark = &benchmarks[num_benchmarks]; \
      num_benchmarks++; \
    } \
  benchmark->func_name = g_strdup (fname); \
  benchmark->max_split = _max_split; \
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
  LOOP_END ("fill", optimized, 0);

  LOOP_START
  dsp_limit1 (buf, -1.0f, 1.1f, buf_size);
  LOOP_END ("limit1", optimized, 0);

  LOOP_START
  dsp_add2 (buf, src, buf_size);
  LOOP_END ("add2", optimized, 0);

  LOOP_START
  float abs_max = dsp_abs_max (buf, buf_size);
  (void) abs_max;
  LOOP_END ("abs_max", optimized, 0);

  LOOP_START
  dsp_min (buf, buf_size);
  LOOP_END ("min", optimized, 0);

  LOOP_START
  dsp_max (buf, buf_size);
  LOOP_END ("max", optimized, 0);

  LOOP_START
  dsp_mul_k2 (buf, 0.99f, buf_size);
  LOOP_END ("mul_k2", optimized, 0);

  LOOP_START
  dsp_copy (buf, src, buf_size);
  LOOP_END ("copy", optimized, 0);

  LOOP_START
  dsp_mix2 (buf, src, 0.1f, 0.2f, buf_size);
  LOOP_END ("mix2", optimized, 0);

  LOOP_START
  dsp_mix_add2 (buf, src, src, 0.1f, 0.2f, buf_size);
  LOOP_END ("mix_add2", optimized, 0);

  free (buf);
  free (src);

  test_helper_zrythm_cleanup ();
}

static void
test_dsp_fill (void)
{
  _test_dsp_fill (
    F_NOT_OPTIMIZED, F_LARGE_BUF);
#ifdef HAVE_LSP_DSP
  _test_dsp_fill (
    F_OPTIMIZED, F_LARGE_BUF);
#endif
}

static void
_test_run_engine (
  bool         optimized,
  unsigned int max_split)
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
  AUDIO_ENGINE->max_split = max_split;
  g_usleep (20000);

#ifdef HAVE_LSP_DSP
  lsp_dsp_context_t ctx;
  if (optimized)
    {
      lsp_dsp_start (&ctx);
    }
#endif

  /* create a few tracks with plugins */
#ifdef HAVE_LSP_COMPRESSOR
  test_plugin_manager_create_tracks_from_plugin (
    LSP_COMPRESSOR_BUNDLE,
    LSP_COMPRESSOR_URI, false, false, NUM_TRACKS);
#endif

  DspBenchmark * benchmark;

  char prof_name[600];
  sprintf (
    prof_name, "engine cycles (max split %u)",
    max_split);

  gint64 start, end;
  start = g_get_monotonic_time ();
  for (int i = 0; i < NUM_ITERATIONS_ENGINE; i++)
    {
      engine_process (
        AUDIO_ENGINE, AUDIO_ENGINE->block_length);
  LOOP_END (prof_name, optimized, max_split);

  g_message (
    "%soptimized time: %ld",
    optimized ? "" : "un", end - start);
  /*g_warn_if_reached ();*/

#ifdef HAVE_LSP_DSP
  if (optimized)
    {
      lsp_dsp_finish (&ctx);
    }
#endif

  test_helper_zrythm_cleanup ();
}

static void
test_run_engine (void)
{
  const unsigned int split_szs[] = { 0, 32, 48, 64, 128, 256 };

  for (size_t i = 0; i < G_N_ELEMENTS (split_szs);
       i++)
    {
#ifdef HAVE_LSP_DSP
      _test_run_engine (F_OPTIMIZED, split_szs[i]);
#endif
      _test_run_engine (
        F_NOT_OPTIMIZED, split_szs[i]);
    }
}

static void
print_benchmark_results (void)
{
  for (int i = 0; i < num_benchmarks; i++)
    {
      DspBenchmark * benchmark = &benchmarks[i];
      fprintf (
        stderr,
        "---- %s ----\n"
        "unoptimized: %ldms\n"
        "optimized: %ldms\n"
        "max split: %u frames\n",
        benchmark->func_name,
        benchmark->unoptimized_usec / 1000,
        benchmark->optimized_usec / 1000,
        benchmark->max_split);
    }
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

#ifdef HAVE_LSP_DSP
  lsp_dsp_init ();
#endif

#define TEST_PREFIX "/benchmarks/dsp/"

  g_test_add_func (
    TEST_PREFIX "test dsp fill",
    (GTestFunc) test_dsp_fill);
  g_test_add_func (
    TEST_PREFIX "test run engine",
    (GTestFunc) test_run_engine);
  g_test_add_func (
    TEST_PREFIX "print benchmark results",
    (GTestFunc) print_benchmark_results);

  return g_test_run ();
}
