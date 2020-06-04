/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "utils/mpmc_queue.h"
#include "utils/objects.h"
#include "utils/stack.h"
#include "zrythm.h"

/*static Stack * free_stack = NULL;*/
static MPMCQueue * free_queue = NULL;
static Stack * free_stack_for_source = NULL;
static ZixSem free_stack_for_source_lock;

/** Time to wait in seconds for object deletion. */
#define TIME_TO_WAIT_SEC 20
/** Time to wait in microseconds. */
#define TIME_TO_WAIT_USEC (TIME_TO_WAIT_SEC * 1000000)

/**
 * Type to be used in the stacks.
 */
typedef struct FreeElement
{
  /** Time the element was added to the stack. */
  gint64  time_added;
  /** The object to be deleted. */
  void * obj;
  /** The delete function. */
  void (*dfunc) (void *);
} FreeElement;

static gboolean
free_later_source ()
{
  zix_sem_wait (&free_stack_for_source_lock);

  /* it might change so get its size at this
   * point. */
  int ssize =
    MIN (stack_size (free_stack_for_source), 28);

  FreeElement * el;
  for (int i = 0; i < ssize; i++)
    {
      /* peek the element at 0 */
      el =
        (FreeElement *)
        stack_pop (free_stack_for_source);
      el->dfunc (el->obj);
      free (el);
    }

  zix_sem_post (&free_stack_for_source_lock);

  return G_SOURCE_CONTINUE;
}

/*
 * this thread is only for traversing
 * the stack and checking the time. if an object
 * needs to be deleted, it is deferred to
 * the source above. */
static gpointer
free_later_thread_func (
  gpointer data)
{
  while (true)
    {
      g_usleep (800000);

      if (!free_queue)
        {
          g_critical ("free_stack uninitialized");
          return NULL;
        }

      gint64 curr_time =
        g_get_monotonic_time ();

      FreeElement * el = NULL;
      while (mpmc_queue_dequeue (
               free_queue, (void **) &el))
        {
          /* if enough time has passed */
          if (curr_time - el->time_added >
                TIME_TO_WAIT_USEC)
            {
              zix_sem_wait (
                &free_stack_for_source_lock);
              stack_push (
                free_stack_for_source,
                (void *) el);
              zix_sem_post (
                &free_stack_for_source_lock);
            }
          else
            {
              /* not enough time passed, exit */
              break;
            }
        }
    }
  g_warn_if_reached ();
}

/**
 * Frees the object after a while.
 *
 * This is useful when the object will be in use
 * for a while, for example in the current
 * processing cycle.
 */
void
_free_later (
  void * object,
  void (*dfunc) (void *))
{
  g_warn_if_fail (free_queue != NULL);

  if (ZRYTHM_TESTING)
    {
      dfunc (object);
      return;
    }

  FreeElement * free_element =
    calloc (1, sizeof (FreeElement));
  free_element->obj = object;
  free_element->dfunc = dfunc;
  free_element->time_added = g_get_monotonic_time ();
  /*stack_push (free_stack, (void *) free_element);*/
  mpmc_queue_push_back (
    free_queue, (void *) free_element);
}

/**
 * Inits the subsystems for the object utils in this
 * file.
 */
void
object_utils_init ()
{
  /*free_stack = stack_new (800000);*/
  free_queue = mpmc_queue_new ();
  mpmc_queue_reserve (
    free_queue,
    800000 * sizeof (FreeElement *));
  free_stack_for_source = stack_new (8000);
  zix_sem_init (&free_stack_for_source_lock, 1);

  g_timeout_add (1000, free_later_source, NULL);
  g_thread_new (
    "obj_utils_free_later", free_later_thread_func,
    NULL);
}
