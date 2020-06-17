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

#ifndef __UTILS_OBJECTS_H__
#define __UTILS_OBJECTS_H__

/**
 * @addtogroup utils
 *
 * @{
 */

/** Calls _free_later after doing the casting so the
 * caller doesn't have to. */
#define free_later(obj,func) \
  _free_later((void *)obj, (void (*) (void *)) func)

/**
 * Frees the object after a while.
 *
 * This is useful when the object will be in use for a
 * while, for example in the current processing cycle.
 */
void
_free_later (
  void * object,
  void (*dfunc) (void *));

/**
 * Inits the subsystems for the object utils in this
 * file.
 */
void
object_utils_init (void);

/**
 * Allocates memory for an object of type \ref type.
 */
#define object_new(type) \
  calloc (1, sizeof (type))

/**
 * Zero's out the struct pointed to by \ref ptr.
 *
 * @param ptr A pointer to a struct.
 */
#define object_set_to_zero(ptr) \
 memset (ptr, 0, sizeof (*(ptr)))

/**
 * Zero's out a struct pointed to by \ref ptr and
 * frees the object.
 */
#define object_zero_and_free(ptr) \
  object_set_to_zero (ptr); \
  free (ptr)

/**
 * Frees memory, sets the pointer to NULL and
 * zero's out the struct.
 *
 * @param ptr A pointer to a pointer to be free'd.
 */
#define object_free_zero_and_null(ptr) \
  { object_set_to_zero (ptr); \
    free (ptr); ptr = NULL; }

/**
 * Frees memory, sets the pointer to NULL and
 * zero's out the struct.
 *
 * @param ptr A pointer to a pointer to be free'd.
 */
#define g_object_unref_and_null(ptr) \
  { g_object_unref (ptr); ptr = NULL; }

#define g_free_if_exists(ptr) \
  { if (ptr) g_free (ptr); (ptr) = NULL; }

/**
 * Frees memory and sets the pointer to NULL.
 */
#define g_free_and_null(ptr) \
  { g_free (ptr); ptr = NULL; }

/**
 * Call the function \ref _func to free \ref _obj
 * and set \ref _obj to NULL.
 */
#define object_free_w_func_and_null(_func,_obj) \
  { _func (_obj); _obj = NULL; }

#define g_source_remove_and_zero(src_id) \
  { g_source_remove (src_id); src_id = 0; }

/**
 * @}
 */

#endif
