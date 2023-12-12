// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Selections serialization.
 */

#ifndef __IO_SELECTIONS_H__
#define __IO_SELECTIONS_H__

#include "utils/types.h"

#include <glib.h>

#include <yyjson.h>

TYPEDEF_STRUCT (MixerSelections);
TYPEDEF_STRUCT (ArrangerSelections);
TYPEDEF_STRUCT (TimelineSelections);
TYPEDEF_STRUCT (ChordSelections);
TYPEDEF_STRUCT (MidiArrangerSelections);
TYPEDEF_STRUCT (AutomationSelections);
TYPEDEF_STRUCT (AudioSelections);
TYPEDEF_STRUCT (TracklistSelections);

bool
mixer_selections_serialize_to_json (
  yyjson_mut_doc *        doc,
  yyjson_mut_val *        sel_obj,
  const MixerSelections * sel,
  GError **               error);

bool
arranger_selections_serialize_to_json (
  yyjson_mut_doc *           doc,
  yyjson_mut_val *           sel_obj,
  const ArrangerSelections * sel,
  GError **                  error);

bool
timeline_selections_serialize_to_json (
  yyjson_mut_doc *           doc,
  yyjson_mut_val *           sel_obj,
  const TimelineSelections * sel,
  GError **                  error);

bool
midi_arranger_selections_serialize_to_json (
  yyjson_mut_doc *               doc,
  yyjson_mut_val *               sel_obj,
  const MidiArrangerSelections * sel,
  GError **                      error);

bool
chord_selections_serialize_to_json (
  yyjson_mut_doc *        doc,
  yyjson_mut_val *        sel_obj,
  const ChordSelections * sel,
  GError **               error);

bool
automation_selections_serialize_to_json (
  yyjson_mut_doc *             doc,
  yyjson_mut_val *             sel_obj,
  const AutomationSelections * sel,
  GError **                    error);

bool
audio_selections_serialize_to_json (
  yyjson_mut_doc *        doc,
  yyjson_mut_val *        sel_obj,
  const AudioSelections * sel,
  GError **               error);

bool
tracklist_selections_serialize_to_json (
  yyjson_mut_doc *            doc,
  yyjson_mut_val *            sel_obj,
  const TracklistSelections * sel,
  GError **                   error);

bool
mixer_selections_deserialize_from_json (
  yyjson_doc *      doc,
  yyjson_val *      sel_obj,
  MixerSelections * sel,
  GError **         error);

bool
arranger_selections_deserialize_from_json (
  yyjson_doc *         doc,
  yyjson_val *         sel_obj,
  ArrangerSelections * sel,
  GError **            error);

bool
timeline_selections_deserialize_from_json (
  yyjson_doc *         doc,
  yyjson_val *         sel_obj,
  TimelineSelections * sel,
  GError **            error);

bool
midi_arranger_selections_deserialize_from_json (
  yyjson_doc *             doc,
  yyjson_val *             sel_obj,
  MidiArrangerSelections * sel,
  GError **                error);

bool
chord_selections_deserialize_from_json (
  yyjson_doc *      doc,
  yyjson_val *      sel_obj,
  ChordSelections * sel,
  GError **         error);

bool
automation_selections_deserialize_from_json (
  yyjson_doc *           doc,
  yyjson_val *           sel_obj,
  AutomationSelections * sel,
  GError **              error);

bool
audio_selections_deserialize_from_json (
  yyjson_doc *      doc,
  yyjson_val *      sel_obj,
  AudioSelections * sel,
  GError **         error);

bool
tracklist_selections_deserialize_from_json (
  yyjson_doc *          doc,
  yyjson_val *          sel_obj,
  TracklistSelections * sel,
  GError **             error);

#endif // __IO_SELECTIONS_H__
