// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Clipboard serialization.
 */

#ifndef __IO_SERIALIZATION_CLIPBOARD_H__
#define __IO_SERIALIZATION_CLIPBOARD_H__

#include "utils/types.h"

#include <glib.h>

#include <yyjson.h>

TYPEDEF_STRUCT (Clipboard);

/**
 * Serialize an (optionally) zstd-compressed clipboard JSON.
 *
 * @param compress Compress FIXME doesn't work.
 */
char *
clipboard_serialize_to_json_str (
  const Clipboard * clipboard,
  bool              compress,
  GError **         error);

/**
 * Deserialize an (optionally) zstd-compressed clipboard JSON.
 *
 * @param decompress Decompress FIXME doesn't work.
 */
Clipboard *
clipboard_deserialize_from_json_str (
  const char * json,
  bool         decompress,
  GError **    error);

#endif // __IO_SERIALIZATION_CLIPBOARD_H__
