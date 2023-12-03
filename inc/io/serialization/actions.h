// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Port serialization.
 */

#ifndef __IO_ACTIONS_H__
#define __IO_ACTIONS_H__

#include "utils/types.h"

#include <glib.h>

#include <yyjson.h>

TYPEDEF_STRUCT (UndoableAction);
TYPEDEF_STRUCT (ArrangerSelectionsAction);
TYPEDEF_STRUCT (MixerSelectionsAction);
TYPEDEF_STRUCT (TracklistSelectionsAction);
TYPEDEF_STRUCT (ChannelSendAction);
TYPEDEF_STRUCT (PortConnectionAction);
TYPEDEF_STRUCT (PortAction);
TYPEDEF_STRUCT (MidiMappingAction);
TYPEDEF_STRUCT (RangeAction);
TYPEDEF_STRUCT (TransportAction);
TYPEDEF_STRUCT (ChordAction);

bool
undoable_action_serialize_to_json (
  yyjson_mut_doc *       doc,
  yyjson_mut_val *       action_obj,
  const UndoableAction * action,
  GError **              error);

bool
arranger_selections_action_serialize_to_json (
  yyjson_mut_doc *                 doc,
  yyjson_mut_val *                 action_obj,
  const ArrangerSelectionsAction * action,
  GError **                        error);

bool
mixer_selections_action_serialize_to_json (
  yyjson_mut_doc *              doc,
  yyjson_mut_val *              action_obj,
  const MixerSelectionsAction * action,
  GError **                     error);

bool
tracklist_selections_action_serialize_to_json (
  yyjson_mut_doc *                  doc,
  yyjson_mut_val *                  action_obj,
  const TracklistSelectionsAction * action,
  GError **                         error);

bool
channel_send_action_serialize_to_json (
  yyjson_mut_doc *          doc,
  yyjson_mut_val *          action_obj,
  const ChannelSendAction * action,
  GError **                 error);

bool
port_connection_action_serialize_to_json (
  yyjson_mut_doc *             doc,
  yyjson_mut_val *             action_obj,
  const PortConnectionAction * action,
  GError **                    error);

bool
port_action_serialize_to_json (
  yyjson_mut_doc *   doc,
  yyjson_mut_val *   action_obj,
  const PortAction * action,
  GError **          error);

bool
midi_mapping_action_serialize_to_json (
  yyjson_mut_doc *          doc,
  yyjson_mut_val *          action_obj,
  const MidiMappingAction * action,
  GError **                 error);

bool
range_action_serialize_to_json (
  yyjson_mut_doc *    doc,
  yyjson_mut_val *    action_obj,
  const RangeAction * action,
  GError **           error);

bool
transport_action_serialize_to_json (
  yyjson_mut_doc *        doc,
  yyjson_mut_val *        action_obj,
  const TransportAction * action,
  GError **               error);

bool
chord_action_serialize_to_json (
  yyjson_mut_doc *    doc,
  yyjson_mut_val *    action_obj,
  const ChordAction * action,
  GError **           error);

#endif // __IO_ACTIONS_H__
