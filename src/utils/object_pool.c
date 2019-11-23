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

#include <gtk/gtk.h>

/**
 * Creates a new object pool.
 */
ObjectPool *
object_pool_new (
  ObjectCreatorFunc func,
  int               max_objects)
{
  ObjectPool * self =
    calloc (1, sizeof (ObjectPool));

  self->max_objects = max_objects;
  self->obj_available =
    calloc ((size_t) max_objects, sizeof (void *));

  for (int i = 0; i < max_objects; i++)
    {
      void * obj = func ();
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
  self->obj_available[self->num_obj_available++] =
    obj;
  zix_sem_post (&self->access_sem);
}

void
object_pool_free (
  ObjectPool * self)
{
  /* TODO */
}
