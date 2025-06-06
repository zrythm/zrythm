// Copyright 2012-2016 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef LV2_OPTIONS_OPTIONS_H
#define LV2_OPTIONS_OPTIONS_H

/**
   @defgroup options Options
   @ingroup lv2

   Instantiation time options.

   See <http://lv2plug.in/ns/ext/options> for details.

   @{
*/

#include <lv2/core/lv2.h>
#include <lv2/urid/urid.h>

#include <stdint.h>

// clang-format off

#define LV2_OPTIONS_URI    "http://lv2plug.in/ns/ext/options"  ///< http://lv2plug.in/ns/ext/options
#define LV2_OPTIONS_PREFIX LV2_OPTIONS_URI "#"                 ///< http://lv2plug.in/ns/ext/options#

#define LV2_OPTIONS__Option          LV2_OPTIONS_PREFIX "Option"           ///< http://lv2plug.in/ns/ext/options#Option
#define LV2_OPTIONS__interface       LV2_OPTIONS_PREFIX "interface"        ///< http://lv2plug.in/ns/ext/options#interface
#define LV2_OPTIONS__options         LV2_OPTIONS_PREFIX "options"          ///< http://lv2plug.in/ns/ext/options#options
#define LV2_OPTIONS__requiredOption  LV2_OPTIONS_PREFIX "requiredOption"   ///< http://lv2plug.in/ns/ext/options#requiredOption
#define LV2_OPTIONS__supportedOption LV2_OPTIONS_PREFIX "supportedOption"  ///< http://lv2plug.in/ns/ext/options#supportedOption

// clang-format on

#ifdef __cplusplus
extern "C" {
#endif

/**
   The context of an Option, which defines the subject it applies to.
*/
typedef enum {
  /**
     This option applies to the instance itself.  The subject must be
     ignored.
  */
  LV2_OPTIONS_INSTANCE,

  /**
     This option applies to some named resource.  The subject is a URI mapped
     to an integer (a LV2_URID, like the key)
  */
  LV2_OPTIONS_RESOURCE,

  /**
     This option applies to some blank node.  The subject is a blank node
     identifier, which is valid only within the current local scope.
  */
  LV2_OPTIONS_BLANK,

  /**
     This option applies to a port on the instance.  The subject is the
     port's index.
  */
  LV2_OPTIONS_PORT
} LV2_Options_Context;

/**
   An option.

   This is a property with a subject, also known as a triple or statement.

   This struct is useful anywhere a statement needs to be passed where no
   memory ownership issues are present (since the value is a const pointer).

   Options can be passed to an instance via the feature LV2_OPTIONS__options
   with data pointed to an array of options terminated by a zeroed option, or
   accessed/manipulated using LV2_Options_Interface.
*/
typedef struct {
  LV2_Options_Context context; /**< Context (type of subject). */
  uint32_t            subject; /**< Subject. */
  LV2_URID            key;     /**< Key (property). */
  uint32_t            size;    /**< Size of value in bytes. */
  LV2_URID            type;    /**< Type of value (datatype). */
  const void*         value;   /**< Pointer to value (object). */
} LV2_Options_Option;

/** A status code for option functions. */
typedef enum {
  LV2_OPTIONS_SUCCESS         = 0U,       /**< Completed successfully. */
  LV2_OPTIONS_ERR_UNKNOWN     = 1U,       /**< Unknown error. */
  LV2_OPTIONS_ERR_BAD_SUBJECT = 1U << 1U, /**< Invalid/unsupported subject. */
  LV2_OPTIONS_ERR_BAD_KEY     = 1U << 2U, /**< Invalid/unsupported key. */
  LV2_OPTIONS_ERR_BAD_VALUE   = 1U << 3U  /**< Invalid/unsupported value. */
} LV2_Options_Status;

/**
   Interface for dynamically setting options (LV2_OPTIONS__interface).
*/
typedef struct {
  /**
     Get the given options.

     Each element of the passed options array MUST have type, subject, and
     key set.  All other fields (size, type, value) MUST be initialised to
     zero, and are set to the option value if such an option is found.

     This function is in the "instantiation" LV2 threading class, so no other
     instance functions may be called concurrently.

     @return Bitwise OR of LV2_Options_Status values.
  */
  uint32_t (*get)(LV2_Handle instance, LV2_Options_Option* options);

  /**
     Set the given options.

     This function is in the "instantiation" LV2 threading class, so no other
     instance functions may be called concurrently.

     @return Bitwise OR of LV2_Options_Status values.
  */
  uint32_t (*set)(LV2_Handle instance, const LV2_Options_Option* options);
} LV2_Options_Interface;

#ifdef __cplusplus
} /* extern "C" */
#endif

/**
   @}
*/

#endif // LV2_OPTIONS_OPTIONS_H
