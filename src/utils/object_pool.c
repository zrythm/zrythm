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

#include <stdlib.h>

#include "utils/object_pool.h"
#include "utils/objects.h"

#include <gtk/gtk.h>

/**
 * Creates a new object pool.
 */
ObjectPool *
object_pool_new (
  ObjectCreatorFunc create_func,
  ObjectFreeFunc    free_func,
  int               max_objects)
{
  ObjectPool * self =
    calloc (1, sizeof (ObjectPool));

  self->free_func = free_func;
  self->max_objects = max_objects;
  self->obj_available =
    calloc ((size_t) max_objects, sizeof (void *));

  for (int i = 0; i < max_objects; i++)
    {
      void * obj = create_func ();
      self->obj_available[i] = obj;
      self->num_obj_available++;
    }

  zix_sem_init (&self->access_sem, 1);

  return self;
}

/**
 * Returns an available object.
 */
void *
object_pool_get (
  ObjectPool * self)
{
  g_return_val_if_fail (
    self->num_obj_available > 0, NULL);
  zix_sem_wait (&self->access_sem);
  void * obj =
    self->obj_available[--self->num_obj_available];
  zix_sem_post (&self->access_sem);
  return obj;
}

/**
 * Puts an object back in the pool.
 */
void
object_pool_return (
  ObjectPool * self,
  void *       obj)
{
  zix_sem_wait (&self->access_sem);
  int fail = 0;
  if (self->num_obj_available == self->max_objects)
    fail = 1;
  else
    {
      self->obj_available[
        self->num_obj_available++] =
          obj;
    }
  zix_sem_post (&self->access_sem);
  g_return_if_fail (fail == 0);
}

/**
 * Frees the pool and all its objects.
 */
void
object_pool_free (
  ObjectPool * self)
{
  zix_sem_wait (&self->access_sem);
  if (self->num_obj_available != self->max_objects)
    {
      g_critical (
        "%s: Cannot free: "
        "There are %d objects in use.",
        __func__,
        self->max_objects - self->num_obj_available);
      zix_sem_post (&self->access_sem);
      return;
    }

  /* free each object */
  for (int i = 0; i < self->num_obj_available; i++)
    {
      void * obj = self->obj_available[i];
      self->free_func (obj);
    }

  object_zero_and_free (self->obj_available);
  self->num_obj_available = 0;
  self->max_objects = 0;
  zix_sem_post (&self->access_sem);

  zix_sem_destroy (&self->access_sem);

  free (self);
}
