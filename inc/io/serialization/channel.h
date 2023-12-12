// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Channel serialization.
 */

#ifndef __IO_SERIALIZATION_CHANNEL_H__
#define __IO_SERIALIZATION_CHANNEL_H__

#include "utils/types.h"

#include <glib.h>

#include <yyjson.h>

TYPEDEF_STRUCT (Channel);
TYPEDEF_STRUCT (ChannelSend);
TYPEDEF_STRUCT (Fader);

bool
channel_send_serialize_to_json (
  yyjson_mut_doc *    doc,
  yyjson_mut_val *    cs_obj,
  const ChannelSend * cs,
  GError **           error);

bool
fader_serialize_to_json (
  yyjson_mut_doc * doc,
  yyjson_mut_val * f_obj,
  const Fader *    f,
  GError **        error);

bool
channel_serialize_to_json (
  yyjson_mut_doc * doc,
  yyjson_mut_val * ch_obj,
  const Channel *  ch,
  GError **        error);

bool
channel_send_deserialize_from_json (
  yyjson_doc *  doc,
  yyjson_val *  cs_obj,
  ChannelSend * cs,
  GError **     error);

bool
fader_deserialize_from_json (
  yyjson_doc * doc,
  yyjson_val * f_obj,
  Fader *      f,
  GError **    error);

bool
channel_deserialize_from_json (
  yyjson_doc * doc,
  yyjson_val * ch_obj,
  Channel *    ch,
  GError **    error);

#endif // __IO_SERIALIZATION_CHANNEL_H__
