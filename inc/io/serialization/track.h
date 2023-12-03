// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Track serialization.
 */

#ifndef __IO_TRACK_H__
#define __IO_TRACK_H__

#include "utils/types.h"

#include <glib.h>

#include <yyjson.h>

TYPEDEF_STRUCT (ModulatorMacroProcessor);
TYPEDEF_STRUCT (Track);
TYPEDEF_STRUCT (TrackProcessor);
TYPEDEF_STRUCT (AutomationTrack);

bool
modulator_macro_processor_serialize_to_json (
  yyjson_mut_doc *                doc,
  yyjson_mut_val *                mmp_obj,
  const ModulatorMacroProcessor * mmp,
  GError **                       error);

bool
track_processor_serialize_to_json (
  yyjson_mut_doc *       doc,
  yyjson_mut_val *       tp_obj,
  const TrackProcessor * tp,
  GError **              error);

bool
automation_track_serialize_to_json (
  yyjson_mut_doc *        doc,
  yyjson_mut_val *        at_obj,
  const AutomationTrack * at,
  GError **               error);

bool
track_serialize_to_json (
  yyjson_mut_doc * doc,
  yyjson_mut_val * track_obj,
  const Track *    track,
  GError **        error);

#endif // __IO_TRACK_H__
