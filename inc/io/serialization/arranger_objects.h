// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Arranger object serialization.
 */

#ifndef __IO_SERIALIZATION_ARRANGER_OBJECTS_H__
#define __IO_SERIALIZATION_ARRANGER_OBJECTS_H__

#include "utils/types.h"

#include <glib.h>

#include <yyjson.h>

TYPEDEF_STRUCT (ArrangerObject);
TYPEDEF_STRUCT (RegionIdentifier);
TYPEDEF_STRUCT (ZRegion);
TYPEDEF_STRUCT (Marker);
TYPEDEF_STRUCT (ScaleObject);
TYPEDEF_STRUCT (ChordObject);
TYPEDEF_STRUCT (AutomationPoint);
TYPEDEF_STRUCT (MidiNote);

bool
arranger_object_serialize_to_json (
  yyjson_mut_doc *       doc,
  yyjson_mut_val *       ao_obj,
  const ArrangerObject * ao,
  GError **              error);

bool
region_identifier_serialize_to_json (
  yyjson_mut_doc *         doc,
  yyjson_mut_val *         id_obj,
  const RegionIdentifier * id,
  GError **                error);

bool
midi_note_serialize_to_json (
  yyjson_mut_doc * doc,
  yyjson_mut_val * mn_obj,
  const MidiNote * mn,
  GError **        error);

bool
automation_point_serialize_to_json (
  yyjson_mut_doc *        doc,
  yyjson_mut_val *        ap_obj,
  const AutomationPoint * ap,
  GError **               error);

bool
chord_object_serialize_to_json (
  yyjson_mut_doc *    doc,
  yyjson_mut_val *    co_obj,
  const ChordObject * co,
  GError **           error);

bool
region_serialize_to_json (
  yyjson_mut_doc * doc,
  yyjson_mut_val * r_obj,
  const ZRegion *  r,
  GError **        error);

bool
scale_object_serialize_to_json (
  yyjson_mut_doc *    doc,
  yyjson_mut_val *    so_obj,
  const ScaleObject * so,
  GError **           error);

bool
marker_serialize_to_json (
  yyjson_mut_doc * doc,
  yyjson_mut_val * m_obj,
  const Marker *   m,
  GError **        error);

#endif // __IO_SERIALIZATION_ARRANGER_OBJECTS_H__
