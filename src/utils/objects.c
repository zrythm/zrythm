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

#include "utils/objects.h"
#include "utils/stack.h"

static Stack * free_stack = NULL;

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
  if (!free_stack)
    {
      g_critical ("free_stack uninitialized");
      return FALSE;
    }

  if (!stack_is_empty (free_stack))
    {
      /* it might change so get its size at this
       * point. */
      int ssize = stack_size (free_stack);
      gint64 curr_time =
        g_get_monotonic_time ();

      FreeElement * el;
      for (int i = 0; i < ssize; i++)
        {
          /* peek the element at 0 */
          el =
            (FreeElement *)
            stack_peek_last (free_stack);

          g_warn_if_fail (el != NULL);

          /* if enough time has passed */
          if (curr_time - el->time_added >
              TIME_TO_WAIT_USEC)
            {
              /* pop and free */
              el =
                (FreeElement *)
                stack_pop_last (free_stack);
              el->dfunc (el->obj);
            }
          else
            {
              /* not enough time passed, exit */
              break;
            }
        }
    }

  return G_SOURCE_CONTINUE;
}

/**
 * Frees the object after a while.
 *
 * This is useful when the object will be in use for a
 * while, for example in the current processing cycle.
 */
void
_free_later (
  void * object,
  void (*dfunc) (void *))
{
  g_warn_if_fail (free_stack != NULL);

  FreeElement * free_element =
    calloc (1, sizeof (FreeElement));
  free_element->obj = object;
  free_element->dfunc = dfunc;
  free_element->time_added =
    g_get_monotonic_time ();
  stack_push (free_stack, (void *) free_element);
}

/**
 * Inits the subsystems for the object utils in this
 * file.
 */
void
object_utils_init ()
{
  free_stack = stack_new (800);

  g_timeout_add (5000, free_later_source, NULL);
}
