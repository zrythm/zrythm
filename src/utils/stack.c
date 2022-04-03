/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
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

/**
 * \file
 *
 * Stack implementation.
 */

#include "utils/objects.h"
#include "utils/stack.h"

int
stack_size (Stack * s)
{
  return g_atomic_int_get (&s->top) + 1;
}

int
stack_is_empty (Stack * s)
{
  return stack_size (s) == 0;
}

int
stack_is_full (Stack * s)
{
  if (s->max_length == -1)
    return 0;
  return stack_size (s) == s->max_length;
}

/**
 * Creates a new stack of the given size.
 *
 * @param length Stack size. If -1, the stack will
 *   have unlimited size.
 */
Stack *
stack_new (int length)
{
  Stack * self = object_new (Stack);

  if (length == -1)
    {
      self->max_length = -1;
    }
  else
    {
      self->elements =
        object_new_n ((size_t) length, void *);
      self->max_length = length;
    }
  self->top = -1;

  return self;
}

void *
stack_peek (Stack * s)
{
  if (!stack_is_empty (s))
    return s->elements[g_atomic_int_get (&s->top)];

  g_warning ("Stack is empty");
  return NULL;
}

void *
stack_peek_last (Stack * s)
{
  if (!stack_is_empty (s))
    return s->elements[0];

  g_warning ("Stack is empty");
  return NULL;
}

void
stack_push (Stack * s, void * element)
{
  if (stack_is_full (s))
    g_warning ("stack is full, cannot push");
  else
    {
      gint top = g_atomic_int_get (&s->top);
      g_atomic_int_inc (&s->top);
      if (s->max_length == -1)
        {
          s->elements = g_realloc (
            s->elements,
            (size_t) (top + 2) * sizeof (void *));
        }
      s->elements[top + 1] = element;
    }
}

void *
stack_pop (Stack * s)
{
  if (stack_is_empty (s))
    {
      g_error ("Can't pop, stack is empty");
    }
  else
    {
      gint top = g_atomic_int_get (&s->top);
      g_atomic_int_dec_and_test (&s->top);
      return s->elements[top];
    }
}

/**
 * Pops the last element and moves everything back.
 */
void *
stack_pop_last (Stack * s)
{
  void * element = s->elements[0];

  for (int i = 0; i < s->max_length - 1; i++)
    {
      s->elements[i] = s->elements[i + 1];
    }
  g_atomic_int_dec_and_test (&s->top);

  return element;
}

void
stack_free_members (Stack * s)
{
  if (s->elements)
    free (s->elements);
}

void
stack_free (Stack * s)
{
  stack_free_members (s);
  free (s);
}
