// SPDX-FileCopyrightText: © 2019-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Stack implementation.
 */

#ifndef __UTILS_STACK_H__
#define __UTILS_STACK_H__

#include <stdlib.h>

#include "utils/yaml.h"

#include <gtk/gtk.h>

/**
 * @addtogroup utils
 *
 * @{
 */

#define STACK_PUSH(s, element) stack_push (s, (void *) element)

/**
 * Stack implementation.
 *
 */
typedef struct Stack
{
  void ** elements;

  /**
   * Max stack size, or -1 for unlimited.
   *
   * If the stack has a fixed length, it will be
   * real-time safe. If it has unlimited length,
   * it will not be real-time safe.
   */
  int max_length;

  /**
   * Index of the top of the stack.
   *
   * This is an index and not a count.
   * Eg., if there is 1 element, this will be 0.
   */
  gint top;
} Stack;

/**
 * Creates a new stack of the given size.
 *
 * @param length Stack size. If -1, the stack will
 *   have unlimited size.
 */
Stack *
stack_new (int length);

int
stack_size (Stack * s);

int
stack_is_empty (Stack * s);

int
stack_is_full (Stack * s);

void *
stack_peek (Stack * s);

void *
stack_peek_last (Stack * s);

void
stack_push (Stack * s, void * element);

void *
stack_pop (Stack * s);

/**
 * Pops the last element and moves everything back.
 */
void *
stack_pop_last (Stack * s);

void
stack_free_members (Stack * s);

void
stack_free (Stack * s);

/**
 * @}
 */

#endif
