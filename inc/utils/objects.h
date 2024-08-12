// SPDX-FileCopyrightText: © 2019-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __UTILS_OBJECTS_H__
#define __UTILS_OBJECTS_H__

#include <cstddef>

#include <gmodule.h>

/**
 * @addtogroup utils
 *
 * @{
 */

NONNULL static inline void
_object_zero_and_free (void ** ptr, size_t sz)
{
  if (!*ptr)
    return;

  memset (*ptr, 0, sz);
  free (*ptr);
  *ptr = NULL;
}

/**
 * @note Some objects are created by libcyaml which uses
 *   plain malloc so avoid using this on serializable objects.
 */
NONNULL static inline void
_object_zero_and_free_unresizable (void ** ptr, size_t sz)
{
  if (!*ptr)
    return;

  memset (*ptr, 0, sz);
  free (*ptr);
  *ptr = NULL;
}

/**
 * Allocates memory for an object of type @ref type.
 */
#define object_new(type) (type *) g_malloc0 (sizeof (type))

/**
 * Allocates memory for an object of type @ref type.
 *
 * This is more efficient than object_new() but does not
 * support resizing.
 *
 * Must be free'd with object_free_unresizable().
 *
 * @note g_slice_*() API was removed from glib so use the
 * same mechanism as above for now.
 */
#define object_new_unresizable(type) object_new (type)

/**
 * Calloc equivalent.
 */
#define object_new_n_sizeof(n, sz) g_malloc0_n (n, sz)

/**
 * Calloc @ref n blocks for type @ref type.
 */
#define object_new_n(n, type) \
  (static_cast<type *> (object_new_n_sizeof (n, sizeof (type))))

#define object_realloc_n_sizeof(obj, prev_sz, sz) \
  realloc_zero (obj, prev_sz, sz)

/**
 * Reallocate memory for @ref obj.
 *
 * @param prev_n Previous number of blocks.
 * @param n New number of blocks.
 */
#define object_realloc_n(obj, prev_n, n, type) \
  static_cast<type *> ( \
    object_realloc_n_sizeof (obj, prev_n * sizeof (type), n * sizeof (type)))

/**
 * Zero's out the struct pointed to by @ref ptr.
 *
 * @param ptr A pointer to a struct.
 */
#define object_set_to_zero(ptr) memset (ptr, 0, sizeof (*(ptr)))

/**
 * Frees memory for objects created with object_new().
 *
 * @note Prefer object_zero_and_free_unresizable().
 */
#define object_free_unresizable(type, obj) free (obj)

/**
 * Zero's out a struct pointed to by @ref ptr and
 * frees the object.
 */
#define object_zero_and_free(ptr) \
  _object_zero_and_free ((void **) &(ptr), sizeof (*(ptr)))

#define object_zero_and_free_unresizable(type, ptr) \
  _object_zero_and_free_unresizable ((void **) &(ptr), sizeof (type))

/**
 * Call the function @ref _func to free @ref _obj
 * and set @ref _obj to NULL.
 */
#define object_free_w_func_and_null(_func, _obj) \
  if (_obj) \
    { \
      _func (_obj); \
      _obj = NULL; \
    }

#define object_delete_and_null(_obj) \
  if (_obj) \
    { \
      delete _obj; \
      _obj = nullptr; \
    }

#define object_zero_and_free_if_nonnull(ptr) \
  object_free_w_func_and_null (object_zero_and_free, ptr)

/** Convenience wrapper. */
#define g_object_unref_and_null(ptr) \
  object_free_w_func_and_null (g_object_unref, ptr)

/** Convenience wrapper. */
#define g_free_and_null(ptr) object_free_w_func_and_null (g_free, ptr)

/** Convenience wrapper. */
#define g_error_free_and_null(ptr) \
  object_free_w_func_and_null (g_error_free, ptr)

/** Convenience wrapper. */
#define object_free_w_func_and_null_cast(_func, _cast, _obj) \
  if (_obj) \
    { \
      _func ((_cast) _obj); \
      _obj = NULL; \
    }

#define g_source_remove_and_zero(src_id) \
  if (src_id != 0) \
    { \
      g_source_remove (src_id); \
      src_id = 0; \
    }

/**
 * @}
 */

#endif
