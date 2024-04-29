// SPDX-FileCopyrightText: © 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Plugin serialization.
 */

#ifndef __IO_SERIALIZATION_PLUGIN_H__
#define __IO_SERIALIZATION_PLUGIN_H__

#include "utils/types.h"

#include <glib.h>

#include <yyjson.h>

TYPEDEF_STRUCT (PluginIdentifier);
TYPEDEF_STRUCT (PluginDescriptor);
TYPEDEF_STRUCT (PluginSetting);
TYPEDEF_STRUCT (PluginPresetIdentifier);
TYPEDEF_STRUCT (PluginPreset);
TYPEDEF_STRUCT (PluginBank);
TYPEDEF_STRUCT (Plugin);
TYPEDEF_STRUCT (PluginCollection);
TYPEDEF_STRUCT (PluginCollections);

bool
plugin_identifier_serialize_to_json (
  yyjson_mut_doc *         doc,
  yyjson_mut_val *         pid_obj,
  const PluginIdentifier * pid,
  GError **                error);

bool
plugin_descriptor_serialize_to_json (
  yyjson_mut_doc *         doc,
  yyjson_mut_val *         pd_obj,
  const PluginDescriptor * pd,
  GError **                error);

bool
plugin_setting_serialize_to_json (
  yyjson_mut_doc *      doc,
  yyjson_mut_val *      ps_obj,
  const PluginSetting * ps,
  GError **             error);

bool
plugin_preset_identifier_serialize_to_json (
  yyjson_mut_doc *               doc,
  yyjson_mut_val *               pid_obj,
  const PluginPresetIdentifier * pid,
  GError **                      error);

bool
plugin_preset_serialize_to_json (
  yyjson_mut_doc *     doc,
  yyjson_mut_val *     pset_obj,
  const PluginPreset * pset,
  GError **            error);

bool
plugin_bank_serialize_to_json (
  yyjson_mut_doc *   doc,
  yyjson_mut_val *   bank_obj,
  const PluginBank * bank,
  GError **          error);

bool
plugin_serialize_to_json (
  yyjson_mut_doc * doc,
  yyjson_mut_val * plugin_obj,
  const Plugin *   plugin,
  GError **        error);

bool
plugin_collection_serialize_to_json (
  yyjson_mut_doc *         doc,
  yyjson_mut_val *         col_obj,
  const PluginCollection * col,
  GError **                error);

bool
plugin_identifier_deserialize_from_json (
  yyjson_doc *       doc,
  yyjson_val *       pid_obj,
  PluginIdentifier * pid,
  GError **          error);

bool
plugin_descriptor_deserialize_from_json (
  yyjson_doc *       doc,
  yyjson_val *       pd_obj,
  PluginDescriptor * pd,
  GError **          error);

bool
plugin_setting_deserialize_from_json (
  yyjson_doc *    doc,
  yyjson_val *    ps_obj,
  PluginSetting * ps,
  GError **       error);

bool
plugin_preset_identifier_deserialize_from_json (
  yyjson_doc *             doc,
  yyjson_val *             pid_obj,
  PluginPresetIdentifier * pid,
  GError **                error);

bool
plugin_preset_deserialize_from_json (
  yyjson_doc *   doc,
  yyjson_val *   pset_obj,
  PluginPreset * pset,
  GError **      error);

bool
plugin_bank_deserialize_from_json (
  yyjson_doc * doc,
  yyjson_val * bank_obj,
  PluginBank * bank,
  GError **    error);

bool
plugin_deserialize_from_json (
  yyjson_doc * doc,
  yyjson_val * plugin_obj,
  Plugin *     plugin,
  GError **    error);

bool
plugin_collection_deserialize_from_json (
  yyjson_doc *       doc,
  yyjson_val *       col_obj,
  PluginCollection * col,
  GError **          error);

#endif // __IO_SERIALIZATION_PLUGIN_H__
