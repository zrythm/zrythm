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
chord_descriptor_serialize_to_json (
  yyjson_mut_doc *        doc,
  yyjson_mut_val *        descr_obj,
  const ChordDescriptor * descr,
  GError **               error);

bool
stack_serialize_to_json (
  yyjson_mut_doc * doc,
  yyjson_mut_val * stack_obj,
  const Stack *    stack,
  GError **        error);

#endif // __IO_SERIALIZATION_EXTRA_H__
