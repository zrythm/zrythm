// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "io/serialization/plugin.h"
#include "io/serialization/port.h"
#include "plugins/plugin.h"
#include "plugins/plugin_preset.h"
#include "utils/objects.h"

#include <yyjson.h>

typedef enum
{
  Z_IO_SERIALIZATION_PLUGIN_ERROR_FAILED,
} ZIOSerializationPluginError;

#define Z_IO_SERIALIZATION_PLUGIN_ERROR z_io_serialization_plugin_error_quark ()
GQuark
z_io_serialization_plugin_error_quark (void);
G_DEFINE_QUARK (z - io - serialization - plugin - error - quark, z_io_serialization_plugin_error)

bool
plugin_identifier_serialize_to_json (
  yyjson_mut_doc *         doc,
  yyjson_mut_val *         pi_obj,
  const PluginIdentifier * pi,
  GError **                error)
{
  yyjson_mut_obj_add_int (doc, pi_obj, "slotType", pi->slot_type);
  yyjson_mut_obj_add_uint (doc, pi_obj, "trackNameHash", pi->track_name_hash);
  yyjson_mut_obj_add_int (doc, pi_obj, "slot", pi->slot);
  return true;
}

bool
plugin_descriptor_serialize_to_json (
  yyjson_mut_doc *         doc,
  yyjson_mut_val *         pd_obj,
  const PluginDescriptor * pd,
  GError **                error)
{
  if (pd->author)
    {
      yyjson_mut_obj_add_str (doc, pd_obj, "author", pd->author);
    }
  if (pd->name)
    {
      yyjson_mut_obj_add_str (doc, pd_obj, "name", pd->name);
    }
  if (pd->website)
    {
      yyjson_mut_obj_add_str (doc, pd_obj, "website", pd->website);
    }
  yyjson_mut_obj_add_int (doc, pd_obj, "category", pd->category);
  if (pd->category_str)
    {
      yyjson_mut_obj_add_str (doc, pd_obj, "categoryStr", pd->category_str);
    }
  yyjson_mut_obj_add_int (doc, pd_obj, "numAudioIns", pd->num_audio_ins);
  yyjson_mut_obj_add_int (doc, pd_obj, "numAudioOuts", pd->num_audio_outs);
  yyjson_mut_obj_add_int (doc, pd_obj, "numMidiIns", pd->num_midi_ins);
  yyjson_mut_obj_add_int (doc, pd_obj, "numMidiOuts", pd->num_midi_outs);
  yyjson_mut_obj_add_int (doc, pd_obj, "numCtrlIns", pd->num_ctrl_ins);
  yyjson_mut_obj_add_int (doc, pd_obj, "numCtrlOuts", pd->num_ctrl_outs);
  yyjson_mut_obj_add_int (doc, pd_obj, "numCvIns", pd->num_cv_ins);
  yyjson_mut_obj_add_int (doc, pd_obj, "numCvOuts", pd->num_cv_outs);
  yyjson_mut_obj_add_int (doc, pd_obj, "uniqueId", pd->unique_id);
  yyjson_mut_obj_add_int (doc, pd_obj, "architecture", pd->arch);
  yyjson_mut_obj_add_int (doc, pd_obj, "protocol", pd->protocol);
  if (pd->path)
    {
      yyjson_mut_obj_add_str (doc, pd_obj, "path", pd->path);
    }
  if (pd->uri)
    {
      yyjson_mut_obj_add_str (doc, pd_obj, "uri", pd->uri);
    }
  yyjson_mut_obj_add_int (doc, pd_obj, "minBridgeMode", pd->min_bridge_mode);
  yyjson_mut_obj_add_bool (doc, pd_obj, "hasCustomUI", pd->has_custom_ui);
  yyjson_mut_obj_add_uint (doc, pd_obj, "ghash", pd->ghash);
  return true;
}

bool
plugin_setting_serialize_to_json (
  yyjson_mut_doc *      doc,
  yyjson_mut_val *      ps_obj,
  const PluginSetting * ps,
  GError **             error)
{
  yyjson_mut_val * pd_obj = yyjson_mut_obj_add_obj (doc, ps_obj, "descriptor");
  plugin_descriptor_serialize_to_json (doc, pd_obj, ps->descr, error);
  yyjson_mut_obj_add_bool (doc, ps_obj, "openWithCarla", ps->open_with_carla);
  yyjson_mut_obj_add_bool (doc, ps_obj, "forceGenericUI", ps->force_generic_ui);
  yyjson_mut_obj_add_int (doc, ps_obj, "bridgeMode", ps->bridge_mode);
  if (ps->ui_uri)
    {
      yyjson_mut_obj_add_str (doc, ps_obj, "uiURI", ps->ui_uri);
    }
  return true;
}

bool
plugin_preset_identifier_serialize_to_json (
  yyjson_mut_doc *               doc,
  yyjson_mut_val *               pid_obj,
  const PluginPresetIdentifier * pid,
  GError **                      error)
{
  yyjson_mut_obj_add_int (doc, pid_obj, "index", pid->idx);
  yyjson_mut_obj_add_int (doc, pid_obj, "bankIndex", pid->bank_idx);
  yyjson_mut_val * pl_id_obj = yyjson_mut_obj_add_obj (doc, pid_obj, "pluginId");
  plugin_identifier_serialize_to_json (doc, pl_id_obj, &pid->plugin_id, error);
  return true;
}

bool
plugin_preset_serialize_to_json (
  yyjson_mut_doc *     doc,
  yyjson_mut_val *     pset_obj,
  const PluginPreset * pset,
  GError **            error)
{
  yyjson_mut_obj_add_str (doc, pset_obj, "name", pset->name);
  if (pset->uri)
    {
      yyjson_mut_obj_add_str (doc, pset_obj, "uri", pset->uri);
    }
  yyjson_mut_obj_add_int (doc, pset_obj, "carlaProgram", pset->carla_program);
  yyjson_mut_val * pid_obj = yyjson_mut_obj_add_obj (doc, pset_obj, "id");
  plugin_preset_identifier_serialize_to_json (doc, pid_obj, &pset->id, error);
  return true;
}

bool
plugin_bank_serialize_to_json (
  yyjson_mut_doc *   doc,
  yyjson_mut_val *   bank_obj,
  const PluginBank * bank,
  GError **          error)
{
  yyjson_mut_obj_add_str (doc, bank_obj, "name", bank->name);
  yyjson_mut_val * psets_arr = yyjson_mut_obj_add_arr (doc, bank_obj, "presets");
  for (int i = 0; i < bank->num_presets; i++)
    {
      PluginPreset *   pset = bank->presets[i];
      yyjson_mut_val * pset_obj = yyjson_mut_arr_add_obj (doc, psets_arr);
      plugin_preset_serialize_to_json (doc, pset_obj, pset, error);
    }
  if (bank->uri)
    {
      yyjson_mut_obj_add_str (doc, bank_obj, "uri", bank->uri);
    }
  yyjson_mut_val * id_obj = yyjson_mut_obj_add_obj (doc, bank_obj, "id");
  plugin_preset_identifier_serialize_to_json (doc, id_obj, &bank->id, error);
  return true;
}

bool
plugin_serialize_to_json (
  yyjson_mut_doc * doc,
  yyjson_mut_val * plugin_obj,
  const Plugin *   plugin,
  GError **        error)
{
  yyjson_mut_val * pi_obj = yyjson_mut_obj_add_obj (doc, plugin_obj, "id");
  plugin_identifier_serialize_to_json (doc, pi_obj, &plugin->id, error);
  yyjson_mut_val * ps_obj = yyjson_mut_obj_add_obj (doc, plugin_obj, "setting");
  plugin_setting_serialize_to_json (doc, ps_obj, plugin->setting, error);
  yyjson_mut_val * in_ports_arr =
    yyjson_mut_obj_add_arr (doc, plugin_obj, "inPorts");
  for (int i = 0; i < plugin->num_in_ports; i++)
    {
      Port *           port = plugin->in_ports[i];
      yyjson_mut_val * port_obj = yyjson_mut_arr_add_obj (doc, in_ports_arr);
      port_serialize_to_json (doc, port_obj, port, error);
    }
  yyjson_mut_val * out_ports_arr =
    yyjson_mut_obj_add_arr (doc, plugin_obj, "outPorts");
  for (int i = 0; i < plugin->num_out_ports; i++)
    {
      Port *           port = plugin->out_ports[i];
      yyjson_mut_val * port_obj = yyjson_mut_arr_add_obj (doc, out_ports_arr);
      port_serialize_to_json (doc, port_obj, port, error);
    }
  yyjson_mut_val * banks_arr = yyjson_mut_obj_add_arr (doc, plugin_obj, "banks");
  for (int i = 0; i < plugin->num_banks; i++)
    {
      PluginBank *     bank = plugin->banks[i];
      yyjson_mut_val * bank_obj = yyjson_mut_arr_add_obj (doc, banks_arr);
      plugin_bank_serialize_to_json (doc, bank_obj, bank, error);
    }
  yyjson_mut_val * selected_bank_obj =
    yyjson_mut_obj_add_obj (doc, plugin_obj, "selectedBank");
  plugin_preset_identifier_serialize_to_json (
    doc, selected_bank_obj, &plugin->selected_bank, error);
  yyjson_mut_val * selected_preset_obj =
    yyjson_mut_obj_add_obj (doc, plugin_obj, "selectedPreset");
  plugin_preset_identifier_serialize_to_json (
    doc, selected_preset_obj, &plugin->selected_preset, error);
  yyjson_mut_obj_add_bool (doc, plugin_obj, "visible", plugin->visible);
  if (plugin->state_dir)
    {
      yyjson_mut_obj_add_str (doc, plugin_obj, "stateDir", plugin->state_dir);
    }
  return true;
}

bool
plugin_identifier_deserialize_from_json (
  yyjson_doc *       doc,
  yyjson_val *       pid_obj,
  PluginIdentifier * pid,
  GError **          error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (pid_obj);
  pid->slot_type =
    (PluginSlotType) yyjson_get_int (yyjson_obj_iter_get (&it, "slotType"));
  pid->track_name_hash =
    yyjson_get_uint (yyjson_obj_iter_get (&it, "trackNameHash"));
  pid->slot = yyjson_get_int (yyjson_obj_iter_get (&it, "slot"));
  return true;
}

bool
plugin_descriptor_deserialize_from_json (
  yyjson_doc *       doc,
  yyjson_val *       pd_obj,
  PluginDescriptor * pd,
  GError **          error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (pd_obj);
  yyjson_val *    author_obj = yyjson_obj_iter_get (&it, "author");
  if (author_obj)
    {
      pd->author = g_strdup (yyjson_get_str (author_obj));
    }
  yyjson_val * name_obj = yyjson_obj_iter_get (&it, "name");
  if (name_obj)
    {
      pd->name = g_strdup (yyjson_get_str (name_obj));
    }
  yyjson_val * website_obj = yyjson_obj_iter_get (&it, "website");
  if (website_obj)
    {
      pd->website = g_strdup (yyjson_get_str (website_obj));
    }
  pd->category =
    (ZPluginCategory) yyjson_get_int (yyjson_obj_iter_get (&it, "cateogry"));
  yyjson_val * category_str_obj = yyjson_obj_iter_get (&it, "categoryStr");
  if (category_str_obj)
    {
      pd->category_str = g_strdup (yyjson_get_str (category_str_obj));
    }
  pd->num_audio_ins = yyjson_get_int (yyjson_obj_iter_get (&it, "numAudioIns"));
  pd->num_audio_outs =
    yyjson_get_int (yyjson_obj_iter_get (&it, "numAudioOuts"));
  pd->num_midi_ins = yyjson_get_int (yyjson_obj_iter_get (&it, "numMidiIns"));
  pd->num_midi_outs = yyjson_get_int (yyjson_obj_iter_get (&it, "numMidiOuts"));
  pd->num_ctrl_ins = yyjson_get_int (yyjson_obj_iter_get (&it, "numCtrlIns"));
  pd->num_ctrl_outs = yyjson_get_int (yyjson_obj_iter_get (&it, "numCtrlOuts"));
  pd->num_cv_ins = yyjson_get_int (yyjson_obj_iter_get (&it, "numCvIns"));
  pd->num_cv_outs = yyjson_get_int (yyjson_obj_iter_get (&it, "numCvOuts"));
  pd->unique_id = yyjson_get_int (yyjson_obj_iter_get (&it, "uniqueId"));
  pd->arch = (PluginArchitecture) yyjson_get_int (
    yyjson_obj_iter_get (&it, "architecture"));
  pd->protocol =
    (ZPluginProtocol) yyjson_get_int (yyjson_obj_iter_get (&it, "protocol"));
  yyjson_val * path_obj = yyjson_obj_iter_get (&it, "path");
  if (path_obj)
    {
      pd->path = g_strdup (yyjson_get_str (path_obj));
    }
  yyjson_val * uri_obj = yyjson_obj_iter_get (&it, "uri");
  if (uri_obj)
    {
      pd->uri = g_strdup (yyjson_get_str (uri_obj));
    }
  pd->min_bridge_mode = (CarlaBridgeMode) yyjson_get_int (
    yyjson_obj_iter_get (&it, "minBridgeMode"));
  pd->has_custom_ui = yyjson_get_bool (yyjson_obj_iter_get (&it, "hasCustomUI"));
  pd->ghash = yyjson_get_uint (yyjson_obj_iter_get (&it, "ghash"));
  return true;
}

bool
plugin_setting_deserialize_from_json (
  yyjson_doc *    doc,
  yyjson_val *    ps_obj,
  PluginSetting * ps,
  GError **       error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (ps_obj);
  yyjson_val *    pd_obj = yyjson_obj_iter_get (&it, "descriptor");
  ps->descr = object_new (PluginDescriptor);
  plugin_descriptor_deserialize_from_json (doc, pd_obj, ps->descr, error);
  ps->open_with_carla =
    yyjson_get_bool (yyjson_obj_iter_get (&it, "openWithCarla"));
  ps->force_generic_ui =
    yyjson_get_bool (yyjson_obj_iter_get (&it, "forceGenericUI"));
  ps->bridge_mode =
    (CarlaBridgeMode) yyjson_get_int (yyjson_obj_iter_get (&it, "bridgeMode"));
  yyjson_val * ui_uri_obj = yyjson_obj_iter_get (&it, "uiURI");
  if (ui_uri_obj)
    {
      ps->ui_uri = g_strdup (yyjson_get_str (ui_uri_obj));
    }
  return true;
}

bool
plugin_preset_identifier_deserialize_from_json (
  yyjson_doc *             doc,
  yyjson_val *             pid_obj,
  PluginPresetIdentifier * pid,
  GError **                error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (pid_obj);
  pid->idx = yyjson_get_int (yyjson_obj_iter_get (&it, "index"));
  pid->bank_idx = yyjson_get_int (yyjson_obj_iter_get (&it, "bankIndex"));
  yyjson_val * pl_id_obj = yyjson_obj_iter_get (&it, "pluginId");
  plugin_identifier_deserialize_from_json (
    doc, pl_id_obj, &pid->plugin_id, error);
  return true;
}

bool
plugin_preset_deserialize_from_json (
  yyjson_doc *   doc,
  yyjson_val *   pset_obj,
  PluginPreset * pset,
  GError **      error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (pset_obj);
  pset->name = g_strdup (yyjson_get_str (yyjson_obj_iter_get (&it, "name")));
  yyjson_val * uri_obj = yyjson_obj_iter_get (&it, "uri");
  if (uri_obj)
    {
      pset->uri = g_strdup (yyjson_get_str (uri_obj));
    }
  pset->carla_program =
    yyjson_get_int (yyjson_obj_iter_get (&it, "carlaProgram"));
  yyjson_val * pid_obj = yyjson_obj_iter_get (&it, "id");
  plugin_preset_identifier_deserialize_from_json (
    doc, pid_obj, &pset->id, error);
  return true;
}

bool
plugin_bank_deserialize_from_json (
  yyjson_doc * doc,
  yyjson_val * bank_obj,
  PluginBank * bank,
  GError **    error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (bank_obj);
  bank->name = g_strdup (yyjson_get_str (yyjson_obj_iter_get (&it, "name")));
  yyjson_val * psets_arr = yyjson_obj_iter_get (&it, "presets");
  bank->presets_size = yyjson_arr_size (psets_arr);
  if (bank->presets_size > 0)
    {
      bank->presets = g_malloc0_n (bank->presets_size, sizeof (PluginPreset *));
      yyjson_arr_iter pset_it = yyjson_arr_iter_with (psets_arr);
      yyjson_val *    pset_obj = NULL;
      while ((pset_obj = yyjson_arr_iter_next (&pset_it)))
        {
          PluginPreset * pset = object_new (PluginPreset);
          bank->presets[bank->num_presets++] = pset;
          plugin_preset_deserialize_from_json (doc, pset_obj, pset, error);
        }
    }
  yyjson_val * uri_obj = yyjson_obj_iter_get (&it, "uri");
  if (uri_obj)
    {
      bank->uri = g_strdup (yyjson_get_str (uri_obj));
    }
  yyjson_val * id_obj = yyjson_obj_iter_get (&it, "id");
  plugin_preset_identifier_deserialize_from_json (doc, id_obj, &bank->id, error);
  return true;
}

bool
plugin_deserialize_from_json (
  yyjson_doc * doc,
  yyjson_val * plugin_obj,
  Plugin *     plugin,
  GError **    error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (plugin_obj);
  yyjson_val *    pi_obj = yyjson_obj_iter_get (&it, "id");
  plugin_identifier_deserialize_from_json (doc, pi_obj, &plugin->id, error);
  yyjson_val * setting_obj = yyjson_obj_iter_get (&it, "setting");
  plugin->setting = object_new (PluginSetting);
  plugin_setting_deserialize_from_json (
    doc, setting_obj, plugin->setting, error);
  yyjson_val * in_ports_arr = yyjson_obj_iter_get (&it, "inPorts");
  plugin->in_ports_size = yyjson_arr_size (in_ports_arr);
  if (plugin->in_ports_size > 0)
    {
      plugin->in_ports = g_malloc0_n (plugin->in_ports_size, sizeof (Port *));
      yyjson_arr_iter port_it = yyjson_arr_iter_with (in_ports_arr);
      yyjson_val *    port_obj = NULL;
      while ((port_obj = yyjson_arr_iter_next (&port_it)))
        {
          Port * port = object_new (Port);
          plugin->in_ports[plugin->num_in_ports++] = port;
          port_deserialize_from_json (doc, port_obj, port, error);
        }
    }
  yyjson_val * out_ports_arr = yyjson_obj_iter_get (&it, "outPorts");
  plugin->out_ports_size = yyjson_arr_size (out_ports_arr);
  if (plugin->out_ports_size > 0)
    {
      plugin->out_ports = g_malloc0_n (plugin->out_ports_size, sizeof (Port *));
      yyjson_arr_iter port_it = yyjson_arr_iter_with (out_ports_arr);
      yyjson_val *    port_obj = NULL;
      while ((port_obj = yyjson_arr_iter_next (&port_it)))
        {
          Port * port = object_new (Port);
          plugin->out_ports[plugin->num_out_ports++] = port;
          port_deserialize_from_json (doc, port_obj, port, error);
        }
    }
  yyjson_val * banks_arr = yyjson_obj_iter_get (&it, "outPorts");
  plugin->banks_size = yyjson_arr_size (banks_arr);
  if (plugin->banks_size > 0)
    {
      plugin->banks = g_malloc0_n (plugin->banks_size, sizeof (PluginBank *));
      yyjson_arr_iter bank_it = yyjson_arr_iter_with (banks_arr);
      yyjson_val *    bank_obj = NULL;
      while ((bank_obj = yyjson_arr_iter_next (&bank_it)))
        {
          PluginBank * bank = object_new (PluginBank);
          plugin->banks[plugin->num_banks++] = bank;
          plugin_bank_deserialize_from_json (doc, bank_obj, bank, error);
        }
    }
  yyjson_val * selected_bank_obj = yyjson_obj_iter_get (&it, "selectedBank");
  plugin_preset_identifier_deserialize_from_json (
    doc, selected_bank_obj, &plugin->selected_bank, error);
  yyjson_val * selected_preset_obj = yyjson_obj_iter_get (&it, "selectedPreset");
  plugin_preset_identifier_deserialize_from_json (
    doc, selected_preset_obj, &plugin->selected_preset, error);
  plugin->visible = yyjson_get_bool (yyjson_obj_iter_get (&it, "visible"));
  yyjson_val * state_dir_obj = yyjson_obj_iter_get (&it, "stateDir");
  if (state_dir_obj)
    {
      plugin->state_dir = g_strdup (yyjson_get_str (state_dir_obj));
    }
  return true;
}
