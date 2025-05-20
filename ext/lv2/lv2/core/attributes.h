// Copyright 2018 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef LV2_CORE_ATTRIBUTES_H
#define LV2_CORE_ATTRIBUTES_H

/**
   @defgroup attributes Attributes
   @ingroup lv2

   Macros for source code attributes.

   @{
*/

#if defined(__GNUC__) && __GNUC__ > 3
#  define LV2_DEPRECATED __attribute__((__deprecated__))
#else
#  define LV2_DEPRECATED
#endif

#if defined(__clang__)
#  define LV2_DISABLE_DEPRECATION_WARNINGS \
    _Pragma("clang diagnostic push")       \
    _Pragma("clang diagnostic ignored \"-Wdeprecated-declarations\"")
#elif defined(__GNUC__) && __GNUC__ > 4
#  define LV2_DISABLE_DEPRECATION_WARNINGS \
    _Pragma("GCC diagnostic push")         \
    _Pragma("GCC diagnostic ignored \"-Wdeprecated-declarations\"")
#else
#  define LV2_DISABLE_DEPRECATION_WARNINGS
#endif

#if defined(__clang__)
#  define LV2_RESTORE_WARNINGS _Pragma("clang diagnostic pop")
#elif defined(__GNUC__) && __GNUC__ > 4
#  define LV2_RESTORE_WARNINGS _Pragma("GCC diagnostic pop")
#else
#  define LV2_RESTORE_WARNINGS
#endif

/**
   @}
*/

#endif // LV2_CORE_ATTRIBUTES_H
