// SPDX-FileCopyrightText: © 2019-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __UTILS_OBJECTS_H__
#define __UTILS_OBJECTS_H__

#include <stddef.h>

/**
 * @addtogroup utils
 *
 * @{
 */

/**
 * Allocates memory for an object of type \ref type.
 */
#define object_new(type) g_malloc0 (sizeof (type))

/**
 * Calloc equivalent.
 */
#define object_new_n_sizeof(n, sz) \
  g_malloc0_n (n, sz)

/**
 * Calloc \ref n blocks for type \ref type.
 */
#define object_new_n(n, type) \
  object_new_n_sizeof (n, sizeof (type))

#define object_realloc_n_sizeof(obj, prev_sz, sz) \
  realloc_zero (obj, prev_sz, sz)

/**
 * Reallocate memory for \ref obj.
 *
 * @param prev_n Previous number of blocks.
 * @param n New number of blocks.
 */
#define object_realloc_n(obj, prev_n, n, type) \
  object_realloc_n_sizeof ( \
    obj, prev_n * sizeof (type), \
    n * sizeof (type))

/**
 * Zero's out the struct pointed to by \ref ptr.
 *
 * @param ptr A pointer to a struct.
 */
#define object_set_to_zero(ptr) \
  memset (ptr, 0, sizeof (*(ptr)))

NONNULL
void
_object_zero_and_free (void ** ptr, size_t sz);

/**
 * Zero's out a struct pointed to by \ref ptr and
 * frees the object.
 */
#define object_zero_and_free(ptr) \
  _object_zero_and_free ( \
    (void **) &(ptr), sizeof (*(ptr)))

/**
 * Call the function \ref _func to free \ref _obj
 * and set \ref _obj to NULL.
 */
#define object_free_w_func_and_null(_func, _obj) \
  if (_obj) \
    { \
      _func (_obj); \
      _obj = NULL; \
    }

#define object_zero_and_free_if_nonnull(ptr) \
  object_free_w_func_and_null ( \
    object_zero_and_free, ptr)

/** Convenience wrapper. */
#define g_object_unref_and_null(ptr) \
  object_free_w_func_and_null (g_object_unref, ptr)

/** Convenience wrapper. */
#define g_free_and_null(ptr) \
  object_free_w_func_and_null (g_free, ptr)

/** Convenience wrapper. */
#define g_error_free_and_null(ptr) \
  object_free_w_func_and_null (g_error_free, ptr)

/** Convenience wrapper. */
#define object_free_w_func_and_null_cast( \
  _func, _cast, _obj) \
  if (_obj) \
    { \
      _func ((_cast) _obj); \
      _obj = NULL; \
    }

#define g_source_remove_and_zero(src_id) \
  { \
    g_source_remove (src_id); \
    src_id = 0; \
  }

/**
 * @}
 */

#endif
