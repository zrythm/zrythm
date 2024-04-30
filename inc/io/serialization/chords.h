// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Plugin serialization.
 */

#ifndef __IO_SERIALIZATION_CHORD_PRESETS_H__
#define __IO_SERIALIZATION_CHORD_PRESETS_H__

#include "utils/types.h"

#include <glib.h>

#include <yyjson.h>

TYPEDEF_STRUCT (ChordPreset);
TYPEDEF_STRUCT (ChordDescriptor);

bool
chord_descriptor_serialize_to_json (
  yyjson_mut_doc *        doc,
  yyjson_mut_val *        descr_obj,
  const ChordDescriptor * descr,
  GError **               error);

bool
chord_preset_serialize_to_json (
  yyjson_mut_doc *    doc,
  yyjson_mut_val *    pset_obj,
  const ChordPreset * pset,
  GError **           error);

bool
chord_descriptor_deserialize_from_json (
  yyjson_doc *      doc,
  yyjson_val *      descr_obj,
  ChordDescriptor * descr,
  GError **         error);

bool
chord_preset_deserialize_from_json (
  yyjson_doc *  doc,
  yyjson_val *  pset_obj,
  ChordPreset * pset,
  GError **     error);

#endif /* __IO_SERIALIZATION_CHORD_PRESETS_H__ */
