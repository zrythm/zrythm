// SPDX-FileCopyrightText: © 2019 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Thread-safe object pool implementation.
 */

#ifndef __UTILS_OBJECT_POOL_H__
#define __UTILS_OBJECT_POOL_H__

#include "utils/types.h"

#include "zix/sem.h"

/**
 * Function to call to create the objects in the
 * pool.
 */
typedef void * (*ObjectCreatorFunc) (void);

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
object_pool_return (ObjectPool * self, void * object);

/**
 * Frees the pool and all its objects.
 */
void
object_pool_free (ObjectPool * self);

#endif
