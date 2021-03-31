/*
 * Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
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

#include "utils/mpmc_queue.h"
#include "utils/concurrent_queue.h"
#include "zrythm.h"

#include "tests/helpers/plugin_manager.h"
#include "tests/helpers/project.h"
#include "tests/helpers/zrythm.h"

typedef struct DspBenchmark
{
  /* function called */
  const char * func_name;
  /* microseconds taken */
  long         concurrent_usec;
  long         mpmc_usec;
} DspBenchmark;

static DspBenchmark benchmarks[400];
static int num_benchmarks = 0;
static size_t num_nodes = 40000;
static MPMCQueue * mpmc_queue;
static void * concurrent_queue;
static bool finished_enqueuing = false;

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

static void *
benchmark_thread (void * arg)
{
  bool concurrent = *((bool *) arg);

  const char * fname = "thread";
  DspBenchmark * benchmark = benchmark_find (fname);
  if (!benchmark)
    {
      benchmark = &benchmarks[num_benchmarks];
      num_benchmarks++;
    }
  benchmark->func_name = fname;

  /*g_message ("start thread");*/

  gint64 start = g_get_monotonic_time ();

  void * el;
  if (concurrent)
    {
      while (concurrent_queue_try_dequeue (
               concurrent_queue, &el) ||
             !finished_enqueuing)
        {
          g_usleep (1);
        }
    }
  else
    {
      while (mpmc_queue_dequeue (
               mpmc_queue, &el) ||
             !finished_enqueuing)
        {
          g_usleep (1);
        }
    }

  gint64 end = g_get_monotonic_time ();

  /*g_message ("end thread");*/
  if (concurrent)
    {
      benchmark->concurrent_usec = end - start;
      g_warn_if_fail (end > start);
    }
  else
    {
      benchmark->mpmc_usec = end - start;
      g_warn_if_fail (end > start);
    }

  return 0;
}

static void
_test_enqueue_dequeue (
  bool concurrent)
{
  /*test_helper_zrythm_init ();*/

  int num_threads = 16;
  pthread_t threads[num_threads];
  memset (
    threads, 0,
    (size_t) num_threads * sizeof (pthread_t));
  void * nodes[num_nodes];

  mpmc_queue = mpmc_queue_new ();
  mpmc_queue_reserve (mpmc_queue, num_nodes);
  bool ret =
    concurrent_queue_create (
      &concurrent_queue, num_nodes);
  g_assert_true (ret);

  finished_enqueuing = false;

  for (int i = 0; i < num_threads; i++)
    {
      int iret =
        pthread_create (
          &threads[i], NULL, &benchmark_thread,
          &concurrent);
      if (iret)
        {
          g_message ("%s", strerror (iret));
        }
      g_assert_cmpint (iret, ==, 0);
    }

  /*DspBenchmark * benchmark;*/
  /*gint64 start, end;*/

  if (concurrent)
    {
      for (size_t i = 0; i < num_nodes; i++)
        {
          g_assert_true (
            concurrent_queue_try_enqueue (
              concurrent_queue, &nodes[i]));
          if (i % 6 == 0)
            g_usleep (1);
        }
    }
  else
    {
      for (size_t i = 0; i < num_nodes; i++)
        {
          g_assert_true (
            mpmc_queue_push_back (
              mpmc_queue, &nodes[i]));
          if (i % 6 == 0)
            g_usleep (1);
        }
    }

  g_message ("finished enqueueing");
  finished_enqueuing = true;

  for (int i = 0; i < num_threads; i++)
    {
      g_assert_false (
        pthread_join (threads[i], NULL));
    }

  mpmc_queue_free (mpmc_queue);
  concurrent_queue_destroy (concurrent_queue);

  /*test_helper_zrythm_cleanup ();*/
}

static void
test_enqueue_dequeue ()
{
  DspBenchmark * benchmark;
  gint64 start, end;

#define LOOP_START \
  start = g_get_monotonic_time (); \
  for (size_t i = 0; i < 3; i++) \
    {

#define LOOP_END(fname,is_concurrent) \
    } \
  end = g_get_monotonic_time (); \
  benchmark = benchmark_find (fname); \
  if (!benchmark) \
    { \
      benchmark = &benchmarks[num_benchmarks]; \
      num_benchmarks++; \
    } \
  benchmark->func_name = fname; \
  if (is_concurrent) \
    { \
      benchmark->concurrent_usec = end - start; \
    } \
  else \
    { \
      benchmark->mpmc_usec = end - start; \
    }

  LOOP_START
  _test_enqueue_dequeue (true);
  LOOP_END ("full", true);
  LOOP_START
  _test_enqueue_dequeue (false);
  LOOP_END ("full", false);
}

static void
print_benchmark_results ()
{
  for (int i = 0; i < num_benchmarks; i++)
    {
      DspBenchmark * benchmark = &benchmarks[i];
      fprintf (
        stderr,
        "---- %s ----\n"
        "concurrent queue: %ld.%ldms\n"
        "mpmc queue: %ld.%ldms\n",
        benchmark->func_name,
        benchmark->concurrent_usec / 1000,
        benchmark->concurrent_usec % 1000,
        benchmark->mpmc_usec / 1000,
        benchmark->mpmc_usec % 1000);
    }
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/benchmarks/queues/"

  g_test_add_func (
    TEST_PREFIX "test enqueue dequeue",
    (GTestFunc) test_enqueue_dequeue);
  g_test_add_func (
    TEST_PREFIX "print benchmark results",
    (GTestFunc) print_benchmark_results);

  return g_test_run ();
}

