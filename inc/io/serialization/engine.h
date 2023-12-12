// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Track serialization.
 */

#ifndef __IO_ENGINE_H__
#define __IO_ENGINE_H__

#include "utils/types.h"

#include <glib.h>

#include <yyjson.h>

TYPEDEF_STRUCT (AudioEngine);
TYPEDEF_STRUCT (Transport);

bool
transport_serialize_to_json (
  yyjson_mut_doc *  doc,
  yyjson_mut_val *  transport_obj,
  const Transport * transport,
  GError **         error);

bool
audio_engine_serialize_to_json (
  yyjson_mut_doc *    doc,
  yyjson_mut_val *    engine_obj,
  const AudioEngine * engine,
  GError **           error);

bool
transport_deserialize_from_json (
  yyjson_doc * doc,
  yyjson_val * transport_obj,
  Transport *  transport,
  GError **    error);

bool
audio_engine_deserialize_from_json (
  yyjson_doc *  doc,
  yyjson_val *  engine_obj,
  AudioEngine * engine,
  GError **     error);

#endif // __IO_ENGINE_H__
