// Copyright 2012-2016 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef LV2_MORPH_MORPH_H
#define LV2_MORPH_MORPH_H

/**
   @defgroup morph Morph
   @ingroup lv2

   Ports that can dynamically change type.

   See <http://lv2plug.in/ns/ext/morph> for details.

   @{
*/

// clang-format off

#define LV2_MORPH_URI    "http://lv2plug.in/ns/ext/morph"  ///< http://lv2plug.in/ns/ext/morph
#define LV2_MORPH_PREFIX LV2_MORPH_URI "#"                 ///< http://lv2plug.in/ns/ext/morph#

#define LV2_MORPH__AutoMorphPort LV2_MORPH_PREFIX "AutoMorphPort"  ///< http://lv2plug.in/ns/ext/morph#AutoMorphPort
#define LV2_MORPH__MorphPort     LV2_MORPH_PREFIX "MorphPort"      ///< http://lv2plug.in/ns/ext/morph#MorphPort
#define LV2_MORPH__interface     LV2_MORPH_PREFIX "interface"      ///< http://lv2plug.in/ns/ext/morph#interface
#define LV2_MORPH__supportsType  LV2_MORPH_PREFIX "supportsType"   ///< http://lv2plug.in/ns/ext/morph#supportsType
#define LV2_MORPH__currentType   LV2_MORPH_PREFIX "currentType"    ///< http://lv2plug.in/ns/ext/morph#currentType

// clang-format on

/**
   @}
*/

#endif // LV2_MORPH_MORPH_H
