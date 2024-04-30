// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Plugin serialization.
 */

#ifndef __IO_SERIALIZATION_EXTRA_H__
#define __IO_SERIALIZATION_EXTRA_H__

#include "utils/types.h"

#include <glib.h>

#include <yyjson.h>

TYPEDEF_STRUCT_UNDERSCORED (GdkRGBA);
TYPEDEF_STRUCT (Position);
TYPEDEF_STRUCT (CurveOptions);
TYPEDEF_STRUCT (ChordDescriptor);
TYPEDEF_STRUCT (Stack);

bool
gdk_rgba_serialize_to_json (
  yyjson_mut_doc * doc,
  yyjson_mut_val * rgba_obj,
  const GdkRGBA *  rgba,
  GError **        error);

bool
position_serialize_to_json (
  yyjson_mut_doc * doc,
  yyjson_mut_val * pos_obj,
  const Position * pos,
  GError **        error);

bool
curve_options_serialize_to_json (
  yyjson_mut_doc *     doc,
  yyjson_mut_val *     opts_obj,
  const CurveOptions * opts,
  GError **            error);

bool
stack_serialize_to_json (
  yyjson_mut_doc * doc,
  yyjson_mut_val * stack_obj,
  const Stack *    stack,
  GError **        error);

bool
gdk_rgba_deserialize_from_json (
  yyjson_doc * doc,
  yyjson_val * rgba_obj,
  GdkRGBA *    rgba,
  GError **    error);

bool
position_deserialize_from_json (
  yyjson_doc * doc,
  yyjson_val * pos_obj,
  Position *   pos,
  GError **    error);

bool
curve_options_deserialize_from_json (
  yyjson_doc *   doc,
  yyjson_val *   opts_obj,
  CurveOptions * opts,
  GError **      error);

bool
stack_deserialize_from_json (
  yyjson_doc * doc,
  yyjson_val * stack_obj,
  Stack *      stack,
  GError **    error);

#endif // __IO_SERIALIZATION_EXTRA_H__
