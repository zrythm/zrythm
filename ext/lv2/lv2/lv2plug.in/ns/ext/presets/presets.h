// Copyright 2012-2016 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef LV2_PRESETS_PRESETS_H
#define LV2_PRESETS_PRESETS_H

/**
   @defgroup presets Presets
   @ingroup lv2

   Presets for plugins.

   See <http://lv2plug.in/ns/ext/presets> for details.

   @{
*/

// clang-format off

#define LV2_PRESETS_URI    "http://lv2plug.in/ns/ext/presets"  ///< http://lv2plug.in/ns/ext/presets
#define LV2_PRESETS_PREFIX LV2_PRESETS_URI "#"                 ///< http://lv2plug.in/ns/ext/presets#

#define LV2_PRESETS__Bank   LV2_PRESETS_PREFIX "Bank"    ///< http://lv2plug.in/ns/ext/presets#Bank
#define LV2_PRESETS__Preset LV2_PRESETS_PREFIX "Preset"  ///< http://lv2plug.in/ns/ext/presets#Preset
#define LV2_PRESETS__bank   LV2_PRESETS_PREFIX "bank"    ///< http://lv2plug.in/ns/ext/presets#bank
#define LV2_PRESETS__preset LV2_PRESETS_PREFIX "preset"  ///< http://lv2plug.in/ns/ext/presets#preset
#define LV2_PRESETS__value  LV2_PRESETS_PREFIX "value"   ///< http://lv2plug.in/ns/ext/presets#value

// clang-format on

/**
   @}
*/

#endif // LV2_PRESETS_PRESETS_H
