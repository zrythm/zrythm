/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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
#include "utils/object_utils.h"
#include "utils/objects.h"
#include "utils/stack.h"
#include "zrythm.h"

/** Time to wait in seconds for object deletion. */
#define TIME_TO_WAIT_SEC 20
/** Time to wait in microseconds. */
#define TIME_TO_WAIT_USEC \
  (TIME_TO_WAIT_SEC * 1000000)

/**
 * Type to be used in the stacks.
 */
typedef struct FreeElement
{
  /** Time the element was added to the stack. */
  gint64 time_added;
  /** The object to be deleted. */
  void * obj;
  /** The delete function. */
  void (*dfunc) (void *);

  /** File. */
  const char * file;

  /** Func. */
  const char * func;

  /** Line. */
  int line;
} FreeElement;

static gboolean
free_later_source (ObjectUtils * self)
{
  zix_sem_wait (&self->free_stack_for_source_lock);

  /* it might change so get its size at this
   * point. */
  int ssize = MIN (
    stack_size (self->free_stack_for_source), 28);

  FreeElement * el;
  for (int i = 0; i < ssize; i++)
    {
      /* peek the element at 0 */
      el = (FreeElement *) stack_pop (
        self->free_stack_for_source);
      el->dfunc (el->obj);
      free (el);
    }

  zix_sem_post (&self->free_stack_for_source_lock);

  return G_SOURCE_CONTINUE;
}

static void
add_all_elements_to_source_stack (
  ObjectUtils * self,
  bool          check_time)
{
  gint64 curr_time = g_get_monotonic_time ();

  FreeElement * el = NULL;
  while (mpmc_queue_dequeue (
    self->free_queue, (void **) &el))
    {
      /* if enough time has passed */
      if (
        !check_time
        || (curr_time - el->time_added > TIME_TO_WAIT_USEC))
        {
          zix_sem_wait (
            &self->free_stack_for_source_lock);
          stack_push (
            self->free_stack_for_source,
            (void *) el);
          zix_sem_post (
            &self->free_stack_for_source_lock);
        }
      else
        {
          /* not enough time passed, exit */
          break;
        }
    }
}

/*
 * this thread is only for traversing
 * the stack and checking the time. if an object
 * needs to be deleted, it is deferred to
 * the source above. */
static gpointer
free_later_thread_func (ObjectUtils * self)
{
  while (true)
    {
      int time_slept = 0;
      while (time_slept < 800000)
        {
          g_usleep (2000);
          if (self->quit_thread)
            {
              g_thread_exit (NULL);
            }
          time_slept += 2000;
        }

      if (!self->free_queue)
        {
          g_critical ("free_stack uninitialized");
          return NULL;
        }

      add_all_elements_to_source_stack (self, true);
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
  void (*dfunc) (void *),
  const char * file,
  const char * func,
  int          line)
{
  g_return_if_fail (
    OBJECT_UTILS->free_queue != NULL);

  if (ZRYTHM_TESTING)
    {
      dfunc (object);
      return;
    }

  FreeElement * free_element =
    object_new (FreeElement);
  free_element->obj = object;
  free_element->dfunc = dfunc;
  free_element->time_added = g_get_monotonic_time ();
  free_element->func = func;
  free_element->line = line;
  free_element->file = file;
  /*stack_push (free_stack, (void *) free_element);*/
  mpmc_queue_push_back (
    OBJECT_UTILS->free_queue, (void *) free_element);
}

/**
 * Inits the subsystems for the object utils in this
 * file.
 */
ObjectUtils *
object_utils_new ()
{
  g_message (
    "%s: Creating object utils...", __func__);

  ObjectUtils * self = object_new (ObjectUtils);

  /*free_stack = stack_new (800000);*/
  self->free_queue = mpmc_queue_new ();
  mpmc_queue_reserve (
    self->free_queue,
    800000 * sizeof (FreeElement *));
  self->free_stack_for_source = stack_new (8000);
  zix_sem_init (
    &self->free_stack_for_source_lock, 1);

  self->source_id = g_timeout_add (
    1000, (GSourceFunc) free_later_source, self);
  self->free_later_thread = g_thread_new (
    "obj_utils_free_later",
    (GThreadFunc) free_later_thread_func, self);

  return self;

  g_message ("%s: done", __func__);
}

void
object_utils_free (ObjectUtils * self)
{
  g_message (
    "%s: Freeing object utils...", __func__);

  /* stop thread and source */
  g_source_remove_and_zero (self->source_id);
  self->quit_thread = true;
  g_usleep (10000);

  /* free all pending objects */
  add_all_elements_to_source_stack (self, false);
  while (
    !stack_is_empty (self->free_stack_for_source))
    {
      free_later_source (self);
    }
  object_free_w_func_and_null (
    stack_free, self->free_stack_for_source);

  mpmc_queue_free (self->free_queue);

  zix_sem_destroy (
    &self->free_stack_for_source_lock);

  object_zero_and_free (self);

  g_message ("%s: done", __func__);
}
