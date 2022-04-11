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

/**
 * \file
 *
 * Thread-safe object pool implementation.
 */

#ifndef __UTILS_OBJECT_POOL_H__
#define __UTILS_OBJECT_POOL_H__

#include "zix/sem.h"

/**
 * Function to call to create the objects in the
 * pool.
 */
typedef void * (*ObjectCreatorFunc) (void);

/**
 * Function to call to free the objects in the
 * pool.
 */
typedef void (*ObjectFreeFunc) (void *);

typedef struct ObjectPool
{
  int max_objects;

  /** Available objects. */
  void ** obj_available;
  int     num_obj_available;

  /** Object free func. */
  ObjectFreeFunc free_func;

  /** Semaphore for atomic operations. */
  ZixSem access_sem;
} ObjectPool;

/**
 * Creates a new object pool.
 */
ObjectPool *
object_pool_new (
  ObjectCreatorFunc create_func,
  ObjectFreeFunc    free_func,
  int               max_objects);

/**
 * Returns an available object.
 */
void *
object_pool_get (ObjectPool * self);

/**
 * Returns the number of available objects.
 *
 * @note This is not accurate (since the number may
 *   change after it's called) and is used only
 *   for debugging purposes.
 */
int
object_pool_get_num_available (ObjectPool * self);

/**
 * Puts an object back in the pool.
 */
void
object_pool_return (
  ObjectPool * self,
  void *       object);

/**
 * Frees the pool and all its objects.
 */
void
object_pool_free (ObjectPool * self);

#endif
