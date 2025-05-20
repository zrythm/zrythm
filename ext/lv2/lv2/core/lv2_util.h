// Copyright 2016 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef LV2_CORE_LV2_UTIL_H
#define LV2_CORE_LV2_UTIL_H

/**
   @defgroup lv2_util Utilities
   @ingroup lv2core
   @{
*/

#include <lv2/core/lv2.h>

#include <stdarg.h>
#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
   Return the data for a feature in a features array.

   If the feature is not found, NULL is returned.  Note that this function is
   only useful for features with data, and can not detect features that are
   present but have NULL data.
*/
static inline void*
lv2_features_data(const LV2_Feature* const* features, const char* const uri)
{
  if (features) {
    for (const LV2_Feature* const* f = features; *f; ++f) {
      if (!strcmp(uri, (*f)->URI)) {
        return (*f)->data;
      }
    }
  }
  return NULL;
}

/**
   Query a features array.

   This function allows getting several features in one call, and detect
   missing required features, with the same caveat of lv2_features_data().

   The arguments should be a series of const char* uri, void** data, bool
   required, terminated by a NULL URI.  The data pointers MUST be initialized
   to NULL.  For example:

   @code
   LV2_URID_Log* log = NULL;
   LV2_URID_Map* map = NULL;
   const char* missing = lv2_features_query(
        features,
        LV2_LOG__log,  &log, false,
        LV2_URID__map, &map, true,
        NULL);
   @endcode

   @return NULL on success, otherwise the URI of this missing feature.
*/
static inline const char*
lv2_features_query(const LV2_Feature* const* features, ...)
{
  va_list args; // NOLINT(cppcoreguidelines-init-variables)
  va_start(args, features);

  const char* uri = NULL;
  while ((uri = va_arg(args, const char*))) {
    void**     data     = va_arg(args, void**);
    const bool required = (bool)va_arg(args, int);

    *data = lv2_features_data(features, uri);
    if (required && !*data) {
      va_end(args);
      return uri;
    }
  }

  va_end(args);
  return NULL;
}

#ifdef __cplusplus
} /* extern "C" */
#endif

/**
   @}
*/

#endif // LV2_CORE_LV2_UTIL_H
