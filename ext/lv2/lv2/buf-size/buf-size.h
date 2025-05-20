// Copyright 2007-2016 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef LV2_BUF_SIZE_BUF_SIZE_H
#define LV2_BUF_SIZE_BUF_SIZE_H

/**
   @defgroup buf-size Buffer Size
   @ingroup lv2

   Access to, and restrictions on, buffer sizes.

   See <http://lv2plug.in/ns/ext/buf-size> for details.

   @{
*/

// clang-format off

#define LV2_BUF_SIZE_URI    "http://lv2plug.in/ns/ext/buf-size"  ///< http://lv2plug.in/ns/ext/buf-size
#define LV2_BUF_SIZE_PREFIX LV2_BUF_SIZE_URI "#" ///< http://lv2plug.in/ns/ext/buf-size#

#define LV2_BUF_SIZE__boundedBlockLength  LV2_BUF_SIZE_PREFIX "boundedBlockLength"   ///< http://lv2plug.in/ns/ext/buf-size#boundedBlockLength
#define LV2_BUF_SIZE__coarseBlockLength   LV2_BUF_SIZE_PREFIX "coarseBlockLength"    ///< http://lv2plug.in/ns/ext/buf-size#coarseBlockLength
#define LV2_BUF_SIZE__fixedBlockLength    LV2_BUF_SIZE_PREFIX "fixedBlockLength"     ///< http://lv2plug.in/ns/ext/buf-size#fixedBlockLength
#define LV2_BUF_SIZE__maxBlockLength      LV2_BUF_SIZE_PREFIX "maxBlockLength"       ///< http://lv2plug.in/ns/ext/buf-size#maxBlockLength
#define LV2_BUF_SIZE__minBlockLength      LV2_BUF_SIZE_PREFIX "minBlockLength"       ///< http://lv2plug.in/ns/ext/buf-size#minBlockLength
#define LV2_BUF_SIZE__nominalBlockLength  LV2_BUF_SIZE_PREFIX "nominalBlockLength"   ///< http://lv2plug.in/ns/ext/buf-size#nominalBlockLength
#define LV2_BUF_SIZE__powerOf2BlockLength LV2_BUF_SIZE_PREFIX "powerOf2BlockLength"  ///< http://lv2plug.in/ns/ext/buf-size#powerOf2BlockLength
#define LV2_BUF_SIZE__sequenceSize        LV2_BUF_SIZE_PREFIX "sequenceSize"         ///< http://lv2plug.in/ns/ext/buf-size#sequenceSize

// clang-format on

/**
   @}
*/

#endif // LV2_BUF_SIZE_BUF_SIZE_H
