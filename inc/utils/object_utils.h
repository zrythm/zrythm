// SPDX-FileCopyrightText: © 2019-2020 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __UTILS_OBJECTS_UTILS_H__
#define __UTILS_OBJECTS_UTILS_H__

#include <stdbool.h>

#include "zix/sem.h"

typedef struct MPMCQueue MPMCQueue;
typedef struct Stack     Stack;

/**
 * @addtogroup utils
 *
 * @{
 */

#define OBJECT_UTILS (ZRYTHM->object_utils)

typedef struct ObjectUtils
{
  MPMCQueue * free_queue;
  Stack *     free_stack_for_source;
  ZixSem      free_stack_for_source_lock;

  /** Source func ID. */
  guint source_id;

  GThread * free_later_thread;

  /** Whether to quit the thread. */
  bool quit_thread;
} ObjectUtils;

/** Calls _free_later after doing the casting so the
 * caller doesn't have to. */
#define free_later(obj, func) \
  _free_later ( \
    (void *) obj, (void (*) (void *)) func, __FILE__, \
    __func__, __LINE__)

/**
 * Frees the object after a while.
 *
 * This is useful when the object will be in use for a
 * while, for example in the current processing cycle.
 */
void
_free_later (
  void * object,
  void (*dfunc) (void *),
  const char * file,
  const char * func,
  int          line);

/**
 * Inits the subsystems for the object utils in this
 * file.
 */
ObjectUtils *
object_utils_new (void);

void
object_utils_free (ObjectUtils * self);

/**
 * @}
 */

#endif
