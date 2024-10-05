// SPDX-FileCopyrightText: © 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/utils/objects.h"
#include "gui/cpp/backend/cyaml_schemas/project.h"

#include <yyjson.h>

typedef enum
{
  Z_SCHEMAS_PROJECT_ERROR_FAILED,
} ZSchemasProjectError;

#define Z_SCHEMAS_PROJECT_ERROR z_schemas_project_error_quark ()
GQuark
z_schemas_project_error_quark (void);
G_DEFINE_QUARK (z - schemas - project - error - quark, z_schemas_project_error)

static bool
gdk_rgba_serialize_to_json (
  yyjson_mut_doc * doc,
  yyjson_mut_val * rgba_obj,
  const GdkRGBA *  rgba,
  GError **        error)
{
  yyjson_mut_obj_add_real (doc, rgba_obj, "red", rgba->red);
  yyjson_mut_obj_add_real (doc, rgba_obj, "green", rgba->green);
  yyjson_mut_obj_add_real (doc, rgba_obj, "blue", rgba->blue);
  yyjson_mut_obj_add_real (doc, rgba_obj, "alpha", rgba->alpha);
  return true;
}

static bool
position_v1_serialize_to_json (
  yyjson_mut_doc *    doc,
  yyjson_mut_val *    pos_obj,
  const Position_v1 * pos,
  GError **           error)
{
  yyjson_mut_obj_add_real (doc, pos_obj, "ticks", pos->ticks);
  yyjson_mut_obj_add_int (doc, pos_obj, "frames", pos->frames);
  return true;
}

static bool
curve_options_v1_serialize_to_json (
  yyjson_mut_doc *        doc,
  yyjson_mut_val *        opts_obj,
  const CurveOptions_v1 * opts,
  GError **               error)
{
  yyjson_mut_obj_add_int (doc, opts_obj, "algorithm", opts->algo);
  yyjson_mut_obj_add_real (doc, opts_obj, "curviness", opts->curviness);
  return true;
}

static bool
chord_descriptor_v2_serialize_to_json (
  yyjson_mut_doc *           doc,
  yyjson_mut_val *           descr_obj,
  const ChordDescriptor_v2 * descr,
  GError **                  error)
{
  yyjson_mut_obj_add_bool (doc, descr_obj, "hasBass", descr->has_bass);
  yyjson_mut_obj_add_int (doc, descr_obj, "rootNote", descr->root_note);
  yyjson_mut_obj_add_int (doc, descr_obj, "bassNote", descr->bass_note);
  yyjson_mut_obj_add_int (doc, descr_obj, "type", descr->type);
  yyjson_mut_obj_add_int (doc, descr_obj, "accent", descr->accent);
  yyjson_mut_val * notes_arr = yyjson_mut_obj_add_arr (doc, descr_obj, "notes");
  for (int i = 0; i < 48; i++)
    {
      yyjson_mut_arr_add_bool (doc, notes_arr, descr->notes[i]);
    }
  yyjson_mut_obj_add_int (doc, descr_obj, "inversion", descr->inversion);
  return true;
}

static bool
plugin_identifier_v1_serialize_to_json (
  yyjson_mut_doc *            doc,
  yyjson_mut_val *            pi_obj,
  const PluginIdentifier_v1 * pi,
  GError **                   error)
{
  yyjson_mut_obj_add_int (doc, pi_obj, "slotType", pi->slot_type);
  yyjson_mut_obj_add_uint (doc, pi_obj, "trackNameHash", pi->track_name_hash);
  yyjson_mut_obj_add_int (doc, pi_obj, "slot", pi->slot);
  return true;
}

static bool
plugin_descriptor_v1_serialize_to_json (
  yyjson_mut_doc *            doc,
  yyjson_mut_val *            pd_obj,
  const PluginDescriptor_v1 * pd,
  GError **                   error)
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

static bool
plugin_setting_v1_serialize_to_json (
  yyjson_mut_doc *         doc,
  yyjson_mut_val *         ps_obj,
  const PluginSetting_v1 * ps,
  GError **                error)
{
  yyjson_mut_val * pd_obj = yyjson_mut_obj_add_obj (doc, ps_obj, "descriptor");
  plugin_descriptor_v1_serialize_to_json (doc, pd_obj, ps->descr, error);
  yyjson_mut_obj_add_bool (doc, ps_obj, "openWithCarla", ps->open_with_carla);
  yyjson_mut_obj_add_bool (doc, ps_obj, "forceGenericUI", ps->force_generic_ui);
  yyjson_mut_obj_add_int (doc, ps_obj, "bridgeMode", ps->bridge_mode);
  if (ps->ui_uri)
    {
      yyjson_mut_obj_add_str (doc, ps_obj, "uiURI", ps->ui_uri);
    }
  return true;
}

static bool
plugin_preset_identifier_v1_serialize_to_json (
  yyjson_mut_doc *                  doc,
  yyjson_mut_val *                  pid_obj,
  const PluginPresetIdentifier_v1 * pid,
  GError **                         error)
{
  yyjson_mut_obj_add_int (doc, pid_obj, "index", pid->idx);
  yyjson_mut_obj_add_int (doc, pid_obj, "bankIndex", pid->bank_idx);
  yyjson_mut_val * pl_id_obj = yyjson_mut_obj_add_obj (doc, pid_obj, "pluginId");
  plugin_identifier_v1_serialize_to_json (
    doc, pl_id_obj, &pid->plugin_id, error);
  return true;
}

static bool
plugin_preset_v1_serialize_to_json (
  yyjson_mut_doc *        doc,
  yyjson_mut_val *        pset_obj,
  const PluginPreset_v1 * pset,
  GError **               error)
{
  yyjson_mut_obj_add_str (doc, pset_obj, "name", pset->name);
  if (pset->uri)
    {
      yyjson_mut_obj_add_str (doc, pset_obj, "uri", pset->uri);
    }
  yyjson_mut_obj_add_int (doc, pset_obj, "carlaProgram", pset->carla_program);
  yyjson_mut_val * pid_obj = yyjson_mut_obj_add_obj (doc, pset_obj, "id");
  plugin_preset_identifier_v1_serialize_to_json (doc, pid_obj, &pset->id, error);
  return true;
}

static bool
plugin_bank_v1_serialize_to_json (
  yyjson_mut_doc *      doc,
  yyjson_mut_val *      bank_obj,
  const PluginBank_v1 * bank,
  GError **             error)
{
  yyjson_mut_obj_add_str (doc, bank_obj, "name", bank->name);
  yyjson_mut_val * psets_arr = yyjson_mut_obj_add_arr (doc, bank_obj, "presets");
  for (int i = 0; i < bank->num_presets; i++)
    {
      PluginPreset_v1 * pset = bank->presets[i];
      yyjson_mut_val *  pset_obj = yyjson_mut_arr_add_obj (doc, psets_arr);
      plugin_preset_v1_serialize_to_json (doc, pset_obj, pset, error);
    }
  if (bank->uri)
    {
      yyjson_mut_obj_add_str (doc, bank_obj, "uri", bank->uri);
    }
  yyjson_mut_val * id_obj = yyjson_mut_obj_add_obj (doc, bank_obj, "id");
  plugin_preset_identifier_v1_serialize_to_json (doc, id_obj, &bank->id, error);
  return true;
}

static bool
port_identifier_v1_serialize_to_json (
  yyjson_mut_doc *          doc,
  yyjson_mut_val *          pi_obj,
  const PortIdentifier_v1 * pi,
  GError **                 error)
{
  if (pi->label)
    {
      yyjson_mut_obj_add_str (doc, pi_obj, "label", pi->label);
    }
  if (pi->sym)
    {
      yyjson_mut_obj_add_str (doc, pi_obj, "symbol", pi->sym);
    }
  if (pi->uri)
    {
      yyjson_mut_obj_add_str (doc, pi_obj, "uri", pi->uri);
    }
  if (pi->comment)
    {
      yyjson_mut_obj_add_str (doc, pi_obj, "comment", pi->comment);
    }
  yyjson_mut_obj_add_int (doc, pi_obj, "ownerType", pi->owner_type);
  yyjson_mut_obj_add_int (doc, pi_obj, "type", pi->type);
  yyjson_mut_obj_add_int (doc, pi_obj, "flow", pi->flow);
  yyjson_mut_obj_add_int (doc, pi_obj, "unit", pi->unit);
  yyjson_mut_obj_add_int (doc, pi_obj, "flags", pi->flags);
  yyjson_mut_obj_add_int (doc, pi_obj, "flags2", pi->flags2);
  yyjson_mut_obj_add_uint (doc, pi_obj, "trackNameHash", pi->track_name_hash);
  yyjson_mut_val * plugin_id_obj =
    yyjson_mut_obj_add_obj (doc, pi_obj, "pluginId");
  plugin_identifier_v1_serialize_to_json (
    doc, plugin_id_obj, &pi->plugin_id, error);
  if (pi->port_group)
    {
      yyjson_mut_obj_add_str (doc, pi_obj, "portGroup", pi->port_group);
    }
  if (pi->ext_port_id)
    {
      yyjson_mut_obj_add_str (doc, pi_obj, "externalPortId", pi->ext_port_id);
    }
  yyjson_mut_obj_add_int (doc, pi_obj, "portIndex", pi->port_index);
  return true;
}

static bool
port_v1_serialize_to_json (
  yyjson_mut_doc * doc,
  yyjson_mut_val * port_obj,
  const Port_v1 *  port,
  GError **        error)
{
  yyjson_mut_val * pi_id_obj = yyjson_mut_obj_add_obj (doc, port_obj, "id");
  port_identifier_v1_serialize_to_json (doc, pi_id_obj, &port->id, error);
  yyjson_mut_obj_add_bool (
    doc, port_obj, "exposedToBackend", port->exposed_to_backend);
  yyjson_mut_obj_add_real (doc, port_obj, "control", port->control);
  yyjson_mut_obj_add_real (doc, port_obj, "minf", port->minf);
  yyjson_mut_obj_add_real (doc, port_obj, "maxf", port->maxf);
  yyjson_mut_obj_add_real (doc, port_obj, "zerof", port->zerof);
  yyjson_mut_obj_add_real (doc, port_obj, "deff", port->deff);
  yyjson_mut_obj_add_int (
    doc, port_obj, "carlaParameterId", port->carla_param_id);
  return true;
}

static bool
stereo_ports_v1_serialize_to_json (
  yyjson_mut_doc *       doc,
  yyjson_mut_val *       sp_obj,
  const StereoPorts_v1 * sp,
  GError **              error)
{
  yyjson_mut_val * l_obj = yyjson_mut_obj_add_obj (doc, sp_obj, "l");
  port_v1_serialize_to_json (doc, l_obj, sp->l, error);
  yyjson_mut_val * r_obj = yyjson_mut_obj_add_obj (doc, sp_obj, "r");
  port_v1_serialize_to_json (doc, r_obj, sp->r, error);
  return true;
}

static bool
plugin_v1_serialize_to_json (
  yyjson_mut_doc *  doc,
  yyjson_mut_val *  plugin_obj,
  const Plugin_v1 * plugin,
  GError **         error)
{
  yyjson_mut_val * pi_obj = yyjson_mut_obj_add_obj (doc, plugin_obj, "id");
  plugin_identifier_v1_serialize_to_json (doc, pi_obj, &plugin->id, error);
  yyjson_mut_val * ps_obj = yyjson_mut_obj_add_obj (doc, plugin_obj, "setting");
  plugin_setting_v1_serialize_to_json (doc, ps_obj, plugin->setting, error);
  yyjson_mut_val * in_ports_arr =
    yyjson_mut_obj_add_arr (doc, plugin_obj, "inPorts");
  for (int i = 0; i < plugin->num_in_ports; i++)
    {
      Port_v1 *        port = plugin->in_ports[i];
      yyjson_mut_val * port_obj = yyjson_mut_arr_add_obj (doc, in_ports_arr);
      port_v1_serialize_to_json (doc, port_obj, port, error);
    }
  yyjson_mut_val * out_ports_arr =
    yyjson_mut_obj_add_arr (doc, plugin_obj, "outPorts");
  for (int i = 0; i < plugin->num_out_ports; i++)
    {
      Port_v1 *        port = plugin->out_ports[i];
      yyjson_mut_val * port_obj = yyjson_mut_arr_add_obj (doc, out_ports_arr);
      port_v1_serialize_to_json (doc, port_obj, port, error);
    }
  yyjson_mut_val * banks_arr = yyjson_mut_obj_add_arr (doc, plugin_obj, "banks");
  for (int i = 0; i < plugin->num_banks; i++)
    {
      PluginBank_v1 *  bank = plugin->banks[i];
      yyjson_mut_val * bank_obj = yyjson_mut_arr_add_obj (doc, banks_arr);
      plugin_bank_v1_serialize_to_json (doc, bank_obj, bank, error);
    }
  yyjson_mut_val * selected_bank_obj =
    yyjson_mut_obj_add_obj (doc, plugin_obj, "selectedBank");
  plugin_preset_identifier_v1_serialize_to_json (
    doc, selected_bank_obj, &plugin->selected_bank, error);
  yyjson_mut_val * selected_preset_obj =
    yyjson_mut_obj_add_obj (doc, plugin_obj, "selectedPreset");
  plugin_preset_identifier_v1_serialize_to_json (
    doc, selected_preset_obj, &plugin->selected_preset, error);
  yyjson_mut_obj_add_bool (doc, plugin_obj, "visible", plugin->visible);
  if (plugin->state_dir)
    {
      yyjson_mut_obj_add_str (doc, plugin_obj, "stateDir", plugin->state_dir);
    }
  return true;
}

static bool
ext_port_v1_serialize_to_json (
  yyjson_mut_doc *   doc,
  yyjson_mut_val *   ep_obj,
  const ExtPort_v1 * ep,
  GError **          error)
{
  yyjson_mut_obj_add_str (doc, ep_obj, "fullName", ep->full_name);
  if (ep->short_name)
    {
      yyjson_mut_obj_add_str (doc, ep_obj, "shortName", ep->short_name);
    }
  if (ep->alias1)
    {
      yyjson_mut_obj_add_str (doc, ep_obj, "alias1", ep->alias1);
    }
  if (ep->alias2)
    {
      yyjson_mut_obj_add_str (doc, ep_obj, "alias2", ep->alias2);
    }
  if (ep->rtaudio_dev_name)
    {
      yyjson_mut_obj_add_str (
        doc, ep_obj, "rtAudioDeviceName", ep->rtaudio_dev_name);
    }
  yyjson_mut_obj_add_int (doc, ep_obj, "numAliases", ep->num_aliases);
  yyjson_mut_obj_add_bool (doc, ep_obj, "isMidi", ep->is_midi);
  yyjson_mut_obj_add_int (doc, ep_obj, "type", ep->type);
  yyjson_mut_obj_add_uint (
    doc, ep_obj, "rtAudioChannelIndex", ep->rtaudio_channel_idx);
  return true;
}

static bool
port_connection_v1_serialize_to_json (
  yyjson_mut_doc *          doc,
  yyjson_mut_val *          conn_obj,
  const PortConnection_v1 * conn,
  GError **                 error)
{
  yyjson_mut_val * src_id_obj = yyjson_mut_obj_add_obj (doc, conn_obj, "srcId");
  port_identifier_v1_serialize_to_json (doc, src_id_obj, conn->src_id, error);
  yyjson_mut_val * dest_id_obj =
    yyjson_mut_obj_add_obj (doc, conn_obj, "destId");
  port_identifier_v1_serialize_to_json (doc, dest_id_obj, conn->dest_id, error);
  yyjson_mut_obj_add_real (doc, conn_obj, "multiplier", conn->multiplier);
  yyjson_mut_obj_add_bool (doc, conn_obj, "enabled", conn->enabled);
  yyjson_mut_obj_add_bool (doc, conn_obj, "locked", conn->locked);
  yyjson_mut_obj_add_real (doc, conn_obj, "baseValue", conn->base_value);
  return true;
}

static bool
port_connections_manager_v1_serialize_to_json (
  yyjson_mut_doc *                  doc,
  yyjson_mut_val *                  mgr_obj,
  const PortConnectionsManager_v1 * mgr,
  GError **                         error)
{
  if (mgr->connections)
    {
      yyjson_mut_val * conns_arr =
        yyjson_mut_obj_add_arr (doc, mgr_obj, "connections");
      for (int i = 0; i < mgr->num_connections; i++)
        {
          PortConnection_v1 * conn = mgr->connections[i];
          yyjson_mut_val * conn_obj = yyjson_mut_arr_add_obj (doc, conns_arr);
          port_connection_v1_serialize_to_json (doc, conn_obj, conn, error);
        }
    }
  return true;
}

static bool
modulator_macro_processor_v1_serialize_to_json (
  yyjson_mut_doc *                   doc,
  yyjson_mut_val *                   mmp_obj,
  const ModulatorMacroProcessor_v1 * mmp,
  GError **                          error)
{
  yyjson_mut_obj_add_str (doc, mmp_obj, "name", mmp->name);
  yyjson_mut_val * cv_in_obj = yyjson_mut_obj_add_obj (doc, mmp_obj, "cvIn");
  port_v1_serialize_to_json (doc, cv_in_obj, mmp->cv_in, error);
  yyjson_mut_val * cv_out_obj = yyjson_mut_obj_add_obj (doc, mmp_obj, "cvOut");
  port_v1_serialize_to_json (doc, cv_out_obj, mmp->cv_out, error);
  yyjson_mut_val * macro_obj = yyjson_mut_obj_add_obj (doc, mmp_obj, "macro");
  port_v1_serialize_to_json (doc, macro_obj, mmp->macro, error);
  return true;
}

static bool
track_processor_v1_serialize_to_json (
  yyjson_mut_doc *          doc,
  yyjson_mut_val *          tp_obj,
  const TrackProcessor_v1 * tp,
  GError **                 error)
{
  if (tp->mono)
    {
      yyjson_mut_val * mono_obj = yyjson_mut_obj_add_obj (doc, tp_obj, "mono");
      port_v1_serialize_to_json (doc, mono_obj, tp->mono, error);
    }
  if (tp->input_gain)
    {
      yyjson_mut_val * input_gain_obj =
        yyjson_mut_obj_add_obj (doc, tp_obj, "inputGain");
      port_v1_serialize_to_json (doc, input_gain_obj, tp->input_gain, error);
    }
  if (tp->output_gain)
    {
      yyjson_mut_val * output_gain_obj =
        yyjson_mut_obj_add_obj (doc, tp_obj, "outputGain");
      port_v1_serialize_to_json (doc, output_gain_obj, tp->output_gain, error);
    }
  if (tp->midi_in)
    {
      yyjson_mut_val * midi_in_obj =
        yyjson_mut_obj_add_obj (doc, tp_obj, "midiIn");
      port_v1_serialize_to_json (doc, midi_in_obj, tp->midi_in, error);
    }
  if (tp->midi_out)
    {
      yyjson_mut_val * midi_out_obj =
        yyjson_mut_obj_add_obj (doc, tp_obj, "midiOut");
      port_v1_serialize_to_json (doc, midi_out_obj, tp->midi_out, error);
    }
  if (tp->piano_roll)
    {
      yyjson_mut_val * piano_roll_obj =
        yyjson_mut_obj_add_obj (doc, tp_obj, "pianoRoll");
      port_v1_serialize_to_json (doc, piano_roll_obj, tp->piano_roll, error);
    }
  if (tp->monitor_audio)
    {
      yyjson_mut_val * monitor_audio_obj =
        yyjson_mut_obj_add_obj (doc, tp_obj, "monitorAudio");
      port_v1_serialize_to_json (
        doc, monitor_audio_obj, tp->monitor_audio, error);
    }
  if (tp->stereo_in)
    {
      yyjson_mut_val * stereo_in_obj =
        yyjson_mut_obj_add_obj (doc, tp_obj, "stereoIn");
      stereo_ports_v1_serialize_to_json (
        doc, stereo_in_obj, tp->stereo_in, error);
    }
  if (tp->stereo_out)
    {
      yyjson_mut_val * stereo_out_obj =
        yyjson_mut_obj_add_obj (doc, tp_obj, "stereoOut");
      stereo_ports_v1_serialize_to_json (
        doc, stereo_out_obj, tp->stereo_out, error);
    }
  if (tp->midi_cc[0])
    {
      yyjson_mut_val * midi_cc_arr =
        yyjson_mut_obj_add_arr (doc, tp_obj, "midiCc");
      for (int j = 0; j < 128 * 16; j++)
        {
          Port_v1 *        port = tp->midi_cc[j];
          yyjson_mut_val * port_obj = yyjson_mut_arr_add_obj (doc, midi_cc_arr);
          port_v1_serialize_to_json (doc, port_obj, port, error);
        }
      yyjson_mut_val * pitch_bend_arr =
        yyjson_mut_obj_add_arr (doc, tp_obj, "pitchBend");
      for (int j = 0; j < 16; j++)
        {
          Port_v1 *        port = tp->pitch_bend[j];
          yyjson_mut_val * port_obj =
            yyjson_mut_arr_add_obj (doc, pitch_bend_arr);
          port_v1_serialize_to_json (doc, port_obj, port, error);
        }
      yyjson_mut_val * poly_key_pressure_arr =
        yyjson_mut_obj_add_arr (doc, tp_obj, "polyKeyPressure");
      for (int j = 0; j < 16; j++)
        {
          Port_v1 *        port = tp->poly_key_pressure[j];
          yyjson_mut_val * port_obj =
            yyjson_mut_arr_add_obj (doc, poly_key_pressure_arr);
          port_v1_serialize_to_json (doc, port_obj, port, error);
        }
      yyjson_mut_val * channel_pressure_arr =
        yyjson_mut_obj_add_arr (doc, tp_obj, "channelPressure");
      for (int j = 0; j < 16; j++)
        {
          Port_v1 *        port = tp->channel_pressure[j];
          yyjson_mut_val * port_obj =
            yyjson_mut_arr_add_obj (doc, channel_pressure_arr);
          port_v1_serialize_to_json (doc, port_obj, port, error);
        }
    }
  return true;
}

static bool
region_identifier_v1_serialize_to_json (
  yyjson_mut_doc *            doc,
  yyjson_mut_val *            id_obj,
  const RegionIdentifier_v1 * id,
  GError **                   error)
{
  yyjson_mut_obj_add_int (doc, id_obj, "type", id->type);
  yyjson_mut_obj_add_int (doc, id_obj, "linkGroup", id->link_group);
  yyjson_mut_obj_add_uint (doc, id_obj, "trackNameHash", id->track_name_hash);
  yyjson_mut_obj_add_int (doc, id_obj, "lanePos", id->lane_pos);
  yyjson_mut_obj_add_int (doc, id_obj, "automationTrackIndex", id->at_idx);
  yyjson_mut_obj_add_int (doc, id_obj, "index", id->idx);
  return true;
}

static bool
arranger_object_v1_serialize_to_json (
  yyjson_mut_doc *          doc,
  yyjson_mut_val *          ao_obj,
  const ArrangerObject_v1 * ao,
  GError **                 error)
{
  yyjson_mut_obj_add_int (doc, ao_obj, "type", ao->type);
  yyjson_mut_obj_add_int (doc, ao_obj, "flags", ao->flags);
  yyjson_mut_obj_add_bool (doc, ao_obj, "muted", ao->muted);
  yyjson_mut_val * pos_obj = yyjson_mut_obj_add_obj (doc, ao_obj, "pos");
  position_v1_serialize_to_json (doc, pos_obj, &ao->pos, error);
  yyjson_mut_val * end_pos_obj = yyjson_mut_obj_add_obj (doc, ao_obj, "endPos");
  position_v1_serialize_to_json (doc, end_pos_obj, &ao->end_pos, error);
  yyjson_mut_val * clip_start_pos_obj =
    yyjson_mut_obj_add_obj (doc, ao_obj, "clipStartPos");
  position_v1_serialize_to_json (
    doc, clip_start_pos_obj, &ao->clip_start_pos, error);
  yyjson_mut_val * loop_start_pos_obj =
    yyjson_mut_obj_add_obj (doc, ao_obj, "loopStartPos");
  position_v1_serialize_to_json (
    doc, loop_start_pos_obj, &ao->loop_start_pos, error);
  yyjson_mut_val * loop_end_pos_obj =
    yyjson_mut_obj_add_obj (doc, ao_obj, "loopEndPos");
  position_v1_serialize_to_json (
    doc, loop_end_pos_obj, &ao->loop_end_pos, error);
  yyjson_mut_val * fade_in_pos_obj =
    yyjson_mut_obj_add_obj (doc, ao_obj, "fadeInPos");
  position_v1_serialize_to_json (doc, fade_in_pos_obj, &ao->fade_in_pos, error);
  yyjson_mut_val * fade_out_pos_obj =
    yyjson_mut_obj_add_obj (doc, ao_obj, "fadeOutPos");
  position_v1_serialize_to_json (
    doc, fade_out_pos_obj, &ao->fade_out_pos, error);
  yyjson_mut_val * fade_in_opts_obj =
    yyjson_mut_obj_add_obj (doc, ao_obj, "fadeInOpts");
  curve_options_v1_serialize_to_json (
    doc, fade_in_opts_obj, &ao->fade_in_opts, error);
  yyjson_mut_val * fade_out_opts_obj =
    yyjson_mut_obj_add_obj (doc, ao_obj, "fadeOutOpts");
  curve_options_v1_serialize_to_json (
    doc, fade_out_opts_obj, &ao->fade_out_opts, error);
  yyjson_mut_val * region_id_obj =
    yyjson_mut_obj_add_obj (doc, ao_obj, "regionId");
  region_identifier_v1_serialize_to_json (
    doc, region_id_obj, &ao->region_id, error);
  return true;
}

static bool
velocity_v1_serialize_to_json (
  yyjson_mut_doc *    doc,
  yyjson_mut_val *    vel_obj,
  const Velocity_v1 * vel,
  GError **           error)
{
  yyjson_mut_val * base_obj = yyjson_mut_obj_add_obj (doc, vel_obj, "base");
  arranger_object_v1_serialize_to_json (doc, base_obj, &vel->base, error);
  yyjson_mut_obj_add_uint (doc, vel_obj, "velocity", vel->vel);
  return true;
}

static bool
midi_note_v1_serialize_to_json (
  yyjson_mut_doc *    doc,
  yyjson_mut_val *    mn_obj,
  const MidiNote_v1 * mn,
  GError **           error)
{
  yyjson_mut_val * base_obj = yyjson_mut_obj_add_obj (doc, mn_obj, "base");
  arranger_object_v1_serialize_to_json (doc, base_obj, &mn->base, error);
  yyjson_mut_val * vel_obj = yyjson_mut_obj_add_obj (doc, mn_obj, "velocity");
  velocity_v1_serialize_to_json (doc, vel_obj, mn->vel, error);
  yyjson_mut_obj_add_uint (doc, mn_obj, "value", mn->val);
  yyjson_mut_obj_add_bool (doc, mn_obj, "muted", mn->muted);
  yyjson_mut_obj_add_int (doc, mn_obj, "pos", mn->pos);
  return true;
}

static bool
automation_point_v1_serialize_to_json (
  yyjson_mut_doc *           doc,
  yyjson_mut_val *           ap_obj,
  const AutomationPoint_v1 * ap,
  GError **                  error)
{
  yyjson_mut_val * base_obj = yyjson_mut_obj_add_obj (doc, ap_obj, "base");
  arranger_object_v1_serialize_to_json (doc, base_obj, &ap->base, error);
  yyjson_mut_obj_add_real (doc, ap_obj, "fValue", ap->fvalue);
  yyjson_mut_obj_add_real (doc, ap_obj, "normalizedValue", ap->normalized_val);
  yyjson_mut_obj_add_int (doc, ap_obj, "index", ap->index);
  yyjson_mut_val * curve_opts_obj =
    yyjson_mut_obj_add_obj (doc, ap_obj, "curveOpts");
  curve_options_v1_serialize_to_json (
    doc, curve_opts_obj, &ap->curve_opts, error);
  return true;
}

static bool
chord_object_v1_serialize_to_json (
  yyjson_mut_doc *       doc,
  yyjson_mut_val *       co_obj,
  const ChordObject_v1 * co,
  GError **              error)
{
  yyjson_mut_val * base_obj = yyjson_mut_obj_add_obj (doc, co_obj, "base");
  arranger_object_v1_serialize_to_json (doc, base_obj, &co->base, error);
  yyjson_mut_obj_add_int (doc, co_obj, "index", co->index);
  return true;
}

static bool
region_v1_serialize_to_json (
  yyjson_mut_doc *   doc,
  yyjson_mut_val *   r_obj,
  const ZRegion_v1 * r,
  GError **          error)
{
  yyjson_mut_val * base_obj = yyjson_mut_obj_add_obj (doc, r_obj, "base");
  arranger_object_v1_serialize_to_json (doc, base_obj, &r->base, error);
  yyjson_mut_val * id_obj = yyjson_mut_obj_add_obj (doc, r_obj, "id");
  region_identifier_v1_serialize_to_json (doc, id_obj, &r->id, error);
  yyjson_mut_obj_add_str (doc, r_obj, "name", r->name);
  yyjson_mut_obj_add_int (doc, r_obj, "poolId", r->pool_id);
  yyjson_mut_obj_add_real (doc, r_obj, "gain", r->gain);
  yyjson_mut_val * color_obj = yyjson_mut_obj_add_obj (doc, r_obj, "color");
  GdkRGBA          color = { 0.0, 0.0, 0.0, 0.0 };
  gdk_rgba_serialize_to_json (doc, color_obj, &color, error);
  yyjson_mut_obj_add_false (doc, r_obj, "useColor");
  switch (r->id.type)
    {
    case REGION_TYPE_AUDIO_v1:
      break;
    case REGION_TYPE_MIDI_v1:
      {
        yyjson_mut_val * midi_notes_arr =
          yyjson_mut_obj_add_arr (doc, r_obj, "midiNotes");
        for (int i = 0; i < r->num_midi_notes; i++)
          {
            MidiNote_v1 *    mn = r->midi_notes[i];
            yyjson_mut_val * mn_obj =
              yyjson_mut_arr_add_obj (doc, midi_notes_arr);
            midi_note_v1_serialize_to_json (doc, mn_obj, mn, error);
          }
      }
      break;
    case REGION_TYPE_CHORD_v1:
      {
        yyjson_mut_val * chord_objects_arr =
          yyjson_mut_obj_add_arr (doc, r_obj, "chordObjects");
        for (int i = 0; i < r->num_chord_objects; i++)
          {
            ChordObject_v1 * co = r->chord_objects[i];
            yyjson_mut_val * co_obj =
              yyjson_mut_arr_add_obj (doc, chord_objects_arr);
            chord_object_v1_serialize_to_json (doc, co_obj, co, error);
          }
      }
      break;
    case REGION_TYPE_AUTOMATION_v1:
      {
        yyjson_mut_val * automation_points_arr =
          yyjson_mut_obj_add_arr (doc, r_obj, "automationPoints");
        for (int i = 0; i < r->num_aps; i++)
          {
            AutomationPoint_v1 * ap = r->aps[i];
            yyjson_mut_val *     ap_obj =
              yyjson_mut_arr_add_obj (doc, automation_points_arr);
            automation_point_v1_serialize_to_json (doc, ap_obj, ap, error);
          }
      }
      break;
    }
  return true;
}

static bool
musical_scale_v2_serialize_to_json (
  yyjson_mut_doc *        doc,
  yyjson_mut_val *        scale_obj,
  const MusicalScale_v2 * scale,
  GError **               error)
{
  yyjson_mut_obj_add_int (doc, scale_obj, "type", scale->type);
  yyjson_mut_obj_add_int (doc, scale_obj, "rootKey", scale->root_key);
  return true;
}

static bool
scale_object_v1_serialize_to_json (
  yyjson_mut_doc *       doc,
  yyjson_mut_val *       so_obj,
  const ScaleObject_v1 * so,
  GError **              error)
{
  yyjson_mut_val * base_obj = yyjson_mut_obj_add_obj (doc, so_obj, "base");
  arranger_object_v1_serialize_to_json (doc, base_obj, &so->base, error);
  yyjson_mut_obj_add_int (doc, so_obj, "index", so->index);
  yyjson_mut_val * scale_obj = yyjson_mut_obj_add_obj (doc, so_obj, "scale");
  musical_scale_v2_serialize_to_json (doc, scale_obj, so->scale, error);
  return true;
}

static bool
marker_v1_serialize_to_json (
  yyjson_mut_doc *  doc,
  yyjson_mut_val *  m_obj,
  const Marker_v1 * m,
  GError **         error)
{
  yyjson_mut_val * base_obj = yyjson_mut_obj_add_obj (doc, m_obj, "base");
  arranger_object_v1_serialize_to_json (doc, base_obj, &m->base, error);
  yyjson_mut_obj_add_str (doc, m_obj, "name", m->name);
  yyjson_mut_obj_add_uint (doc, m_obj, "trackNameHash", m->track_name_hash);
  yyjson_mut_obj_add_int (doc, m_obj, "index", m->index);
  yyjson_mut_obj_add_int (doc, m_obj, "type", m->type);
  return true;
}

static bool
automation_track_v1_serialize_to_json (
  yyjson_mut_doc *           doc,
  yyjson_mut_val *           at_obj,
  const AutomationTrack_v1 * at,
  GError **                  error)
{
  yyjson_mut_obj_add_int (doc, at_obj, "index", at->index);
  yyjson_mut_val * port_id_obj = yyjson_mut_obj_add_obj (doc, at_obj, "portId");
  port_identifier_v1_serialize_to_json (doc, port_id_obj, &at->port_id, error);
  if (at->regions)
    {
      yyjson_mut_val * regions_arr =
        yyjson_mut_obj_add_arr (doc, at_obj, "regions");
      for (int i = 0; i < at->num_regions; i++)
        {
          ZRegion_v1 *     r = at->regions[i];
          yyjson_mut_val * r_obj = yyjson_mut_arr_add_obj (doc, regions_arr);
          region_v1_serialize_to_json (doc, r_obj, r, error);
        }
    }
  yyjson_mut_obj_add_bool (doc, at_obj, "created", at->created);
  yyjson_mut_obj_add_int (doc, at_obj, "automationMode", at->automation_mode);
  yyjson_mut_obj_add_int (doc, at_obj, "recordMode", at->record_mode);
  yyjson_mut_obj_add_bool (doc, at_obj, "visible", at->visible);
  yyjson_mut_obj_add_real (doc, at_obj, "height", at->height);
  return true;
}

static bool
channel_send_v1_serialize_to_json (
  yyjson_mut_doc *       doc,
  yyjson_mut_val *       cs_obj,
  const ChannelSend_v1 * cs,
  GError **              error)
{
  yyjson_mut_obj_add_int (doc, cs_obj, "slot", cs->slot);
  yyjson_mut_val * amount_obj = yyjson_mut_obj_add_obj (doc, cs_obj, "amount");
  port_v1_serialize_to_json (doc, amount_obj, cs->amount, error);
  yyjson_mut_val * enabled_obj = yyjson_mut_obj_add_obj (doc, cs_obj, "enabled");
  port_v1_serialize_to_json (doc, enabled_obj, cs->enabled, error);
  yyjson_mut_obj_add_bool (doc, cs_obj, "isSidechain", cs->is_sidechain);
  if (cs->midi_in)
    {
      yyjson_mut_val * midi_in_obj =
        yyjson_mut_obj_add_obj (doc, cs_obj, "midiIn");
      port_v1_serialize_to_json (doc, midi_in_obj, cs->midi_in, error);
    }
  if (cs->stereo_in)
    {
      yyjson_mut_val * stereo_in_obj =
        yyjson_mut_obj_add_obj (doc, cs_obj, "stereoIn");
      stereo_ports_v1_serialize_to_json (
        doc, stereo_in_obj, cs->stereo_in, error);
    }
  if (cs->midi_out)
    {
      yyjson_mut_val * midi_out_obj =
        yyjson_mut_obj_add_obj (doc, cs_obj, "midiOut");
      port_v1_serialize_to_json (doc, midi_out_obj, cs->midi_out, error);
    }
  if (cs->stereo_out)
    {
      yyjson_mut_val * stereo_out_obj =
        yyjson_mut_obj_add_obj (doc, cs_obj, "stereoOut");
      stereo_ports_v1_serialize_to_json (
        doc, stereo_out_obj, cs->stereo_out, error);
    }
  yyjson_mut_obj_add_uint (doc, cs_obj, "trackNameHash", cs->track_name_hash);
  return true;
}

static bool
fader_v2_serialize_to_json (
  yyjson_mut_doc * doc,
  yyjson_mut_val * f_obj,
  const Fader_v2 * f,
  GError **        error)
{
  yyjson_mut_obj_add_int (doc, f_obj, "type", f->type);
  yyjson_mut_obj_add_real (doc, f_obj, "volume", f->volume);
  yyjson_mut_val * amp_obj = yyjson_mut_obj_add_obj (doc, f_obj, "amp");
  port_v1_serialize_to_json (doc, amp_obj, f->amp, error);
  yyjson_mut_obj_add_real (doc, f_obj, "phase", f->phase);
  yyjson_mut_val * balance_obj = yyjson_mut_obj_add_obj (doc, f_obj, "balance");
  port_v1_serialize_to_json (doc, balance_obj, f->balance, error);
  yyjson_mut_val * mute_obj = yyjson_mut_obj_add_obj (doc, f_obj, "mute");
  port_v1_serialize_to_json (doc, mute_obj, f->mute, error);
  yyjson_mut_val * solo_obj = yyjson_mut_obj_add_obj (doc, f_obj, "solo");
  port_v1_serialize_to_json (doc, solo_obj, f->solo, error);
  yyjson_mut_val * listen_obj = yyjson_mut_obj_add_obj (doc, f_obj, "listen");
  port_v1_serialize_to_json (doc, listen_obj, f->listen, error);
  yyjson_mut_val * mono_compat_enabled_obj =
    yyjson_mut_obj_add_obj (doc, f_obj, "monoCompatEnabled");
  port_v1_serialize_to_json (
    doc, mono_compat_enabled_obj, f->mono_compat_enabled, error);
  yyjson_mut_val * swap_phase_obj =
    yyjson_mut_obj_add_obj (doc, f_obj, "swapPhase");
  port_v1_serialize_to_json (doc, swap_phase_obj, f->swap_phase, error);
  if (f->midi_in)
    {
      yyjson_mut_val * midi_in_obj =
        yyjson_mut_obj_add_obj (doc, f_obj, "midiIn");
      port_v1_serialize_to_json (doc, midi_in_obj, f->midi_in, error);
    }
  if (f->midi_out)
    {
      yyjson_mut_val * midi_out_obj =
        yyjson_mut_obj_add_obj (doc, f_obj, "midiOut");
      port_v1_serialize_to_json (doc, midi_out_obj, f->midi_out, error);
    }
  if (f->stereo_in)
    {
      yyjson_mut_val * stereo_in_obj =
        yyjson_mut_obj_add_obj (doc, f_obj, "stereoIn");
      stereo_ports_v1_serialize_to_json (
        doc, stereo_in_obj, f->stereo_in, error);
    }
  if (f->stereo_out)
    {
      yyjson_mut_val * stereo_out_obj =
        yyjson_mut_obj_add_obj (doc, f_obj, "stereoOut");
      stereo_ports_v1_serialize_to_json (
        doc, stereo_out_obj, f->stereo_out, error);
    }
  yyjson_mut_obj_add_int (doc, f_obj, "midiMode", f->midi_mode);
  yyjson_mut_obj_add_bool (doc, f_obj, "passthrough", f->passthrough);
  return true;
}

static bool
channel_v2_serialize_to_json (
  yyjson_mut_doc *   doc,
  yyjson_mut_val *   ch_obj,
  const Channel_v2 * ch,
  GError **          error)
{
  yyjson_mut_val * midi_fx_arr = yyjson_mut_obj_add_arr (doc, ch_obj, "midiFx");
  for (auto pl : ch->midi_fx)
    {
      if (pl)
        {
          yyjson_mut_val * pl_obj = yyjson_mut_arr_add_obj (doc, midi_fx_arr);
          plugin_v1_serialize_to_json (doc, pl_obj, pl, error);
        }
      else
        {
          yyjson_mut_arr_add_null (doc, midi_fx_arr);
        }
    }
  yyjson_mut_val * inserts_arr = yyjson_mut_obj_add_arr (doc, ch_obj, "inserts");
  for (auto pl : ch->inserts)
    {
      if (pl)
        {
          yyjson_mut_val * pl_obj = yyjson_mut_arr_add_obj (doc, inserts_arr);
          plugin_v1_serialize_to_json (doc, pl_obj, pl, error);
        }
      else
        {
          yyjson_mut_arr_add_null (doc, inserts_arr);
        }
    }
  yyjson_mut_val * sends_arr = yyjson_mut_obj_add_arr (doc, ch_obj, "sends");
  for (auto cs : ch->sends)
    {
      g_return_val_if_fail (cs, false);
      yyjson_mut_val * cs_obj = yyjson_mut_arr_add_obj (doc, sends_arr);
      channel_send_v1_serialize_to_json (doc, cs_obj, cs, error);
    }
  if (ch->instrument)
    {
      yyjson_mut_val * pl_obj =
        yyjson_mut_obj_add_obj (doc, ch_obj, "instrument");
      plugin_v1_serialize_to_json (doc, pl_obj, ch->instrument, error);
    }
  yyjson_mut_val * prefader_obj =
    yyjson_mut_obj_add_obj (doc, ch_obj, "prefader");
  fader_v2_serialize_to_json (doc, prefader_obj, ch->prefader, error);
  yyjson_mut_val * fader_obj = yyjson_mut_obj_add_obj (doc, ch_obj, "fader");
  fader_v2_serialize_to_json (doc, fader_obj, ch->fader, error);
  if (ch->midi_out)
    {
      yyjson_mut_val * port_obj =
        yyjson_mut_obj_add_obj (doc, ch_obj, "midiOut");
      port_v1_serialize_to_json (doc, port_obj, ch->midi_out, error);
    }
  if (ch->stereo_out)
    {
      yyjson_mut_val * port_obj =
        yyjson_mut_obj_add_obj (doc, ch_obj, "stereoOut");
      stereo_ports_v1_serialize_to_json (doc, port_obj, ch->stereo_out, error);
    }
  yyjson_mut_obj_add_bool (doc, ch_obj, "hasOutput", ch->has_output);
  yyjson_mut_obj_add_uint (doc, ch_obj, "outputNameHash", ch->output_name_hash);
  yyjson_mut_obj_add_int (doc, ch_obj, "trackPos", ch->track_pos);
  if (!ch->all_midi_ins)
    {
      yyjson_mut_val * ext_midi_ins_arr =
        yyjson_mut_obj_add_arr (doc, ch_obj, "extMidiIns");
      for (int i = 0; i < ch->num_ext_midi_ins; i++)
        {
          ExtPort_v1 *     ep = ch->ext_midi_ins[i];
          yyjson_mut_val * ep_obj =
            yyjson_mut_arr_add_obj (doc, ext_midi_ins_arr);
          ext_port_v1_serialize_to_json (doc, ep_obj, ep, error);
        }
    }
  yyjson_mut_obj_add_bool (doc, ch_obj, "allMidiIns", ch->all_midi_ins);
  if (!ch->all_midi_channels)
    {
      yyjson_mut_val * midi_channels_arr =
        yyjson_mut_obj_add_arr (doc, ch_obj, "midiChannels");
      for (int i = 0; i < 16; i++)
        {
          yyjson_mut_arr_add_bool (doc, midi_channels_arr, ch->midi_channels[i]);
        }
    }
  yyjson_mut_obj_add_bool (
    doc, ch_obj, "allMidiChannels", ch->all_midi_channels);
  if (!ch->all_stereo_l_ins)
    {
      yyjson_mut_val * ext_stereo_l_ins_arr =
        yyjson_mut_obj_add_arr (doc, ch_obj, "extStereoLIns");
      for (int i = 0; i < ch->num_ext_stereo_l_ins; i++)
        {
          ExtPort_v1 *     ep = ch->ext_stereo_l_ins[i];
          yyjson_mut_val * ep_obj =
            yyjson_mut_arr_add_obj (doc, ext_stereo_l_ins_arr);
          ext_port_v1_serialize_to_json (doc, ep_obj, ep, error);
        }
    }
  yyjson_mut_obj_add_bool (doc, ch_obj, "allStereoLIns", ch->all_stereo_l_ins);
  if (!ch->all_stereo_r_ins)
    {
      yyjson_mut_val * ext_stereo_r_ins_arr =
        yyjson_mut_obj_add_arr (doc, ch_obj, "extStereoRIns");
      for (int i = 0; i < ch->num_ext_stereo_r_ins; i++)
        {
          ExtPort_v1 *     ep = ch->ext_stereo_r_ins[i];
          yyjson_mut_val * ep_obj =
            yyjson_mut_arr_add_obj (doc, ext_stereo_r_ins_arr);
          ext_port_v1_serialize_to_json (doc, ep_obj, ep, error);
        }
    }
  yyjson_mut_obj_add_bool (doc, ch_obj, "allStereoRIns", ch->all_stereo_r_ins);
  yyjson_mut_obj_add_int (doc, ch_obj, "width", ch->width);
  return true;
}

static bool
track_v2_serialize_to_json (
  yyjson_mut_doc * doc,
  yyjson_mut_val * track_obj,
  const Track_v2 * track,
  GError **        error)
{
  yyjson_mut_obj_add_str (doc, track_obj, "name", track->name);
  yyjson_mut_obj_add_str (doc, track_obj, "iconName", track->icon_name);
  yyjson_mut_obj_add_int (doc, track_obj, "type", track->type);
  yyjson_mut_obj_add_int (doc, track_obj, "pos", track->pos);
  yyjson_mut_obj_add_bool (doc, track_obj, "lanesVisible", track->lanes_visible);
  yyjson_mut_obj_add_bool (
    doc, track_obj, "automationVisible", track->automation_visible);
  yyjson_mut_obj_add_bool (doc, track_obj, "visible", track->visible);
  yyjson_mut_obj_add_real (doc, track_obj, "mainHeight", track->main_height);
  yyjson_mut_obj_add_bool (
    doc, track_obj, "passthroughMidiInput", track->passthrough_midi_input);
  if (track->recording)
    {
      yyjson_mut_val * recording_obj =
        yyjson_mut_obj_add_obj (doc, track_obj, "recording");
      port_v1_serialize_to_json (doc, recording_obj, track->recording, error);
    }
  yyjson_mut_obj_add_bool (doc, track_obj, "enabled", track->enabled);
  yyjson_mut_val * color_obj = yyjson_mut_obj_add_obj (doc, track_obj, "color");
  gdk_rgba_serialize_to_json (doc, color_obj, &track->color, error);
  yyjson_mut_val * lanes_arr = yyjson_mut_obj_add_arr (doc, track_obj, "lanes");
  for (int j = 0; j < track->num_lanes; j++)
    {
      TrackLane_v1 *   lane = track->lanes[j];
      yyjson_mut_val * lane_obj = yyjson_mut_arr_add_obj (doc, lanes_arr);
      yyjson_mut_obj_add_int (doc, lane_obj, "pos", lane->pos);
      yyjson_mut_obj_add_str (doc, lane_obj, "name", lane->name);
      yyjson_mut_obj_add_real (doc, lane_obj, "height", lane->height);
      yyjson_mut_obj_add_bool (doc, lane_obj, "mute", lane->mute);
      yyjson_mut_obj_add_bool (doc, lane_obj, "solo", lane->solo);
      yyjson_mut_val * regions_arr =
        yyjson_mut_obj_add_arr (doc, lane_obj, "regions");
      for (int k = 0; k < lane->num_regions; k++)
        {
          ZRegion_v1 *     region = lane->regions[k];
          yyjson_mut_val * region_obj =
            yyjson_mut_arr_add_obj (doc, regions_arr);
          region_v1_serialize_to_json (doc, region_obj, region, error);
        }
      yyjson_mut_obj_add_uint (doc, lane_obj, "midiCh", lane->midi_ch);
    }
  if (track->type == TRACK_TYPE_CHORD_v1)
    {
      yyjson_mut_val * chord_regions_arr =
        yyjson_mut_obj_add_arr (doc, track_obj, "chordRegions");
      for (int j = 0; j < track->num_chord_regions; j++)
        {
          ZRegion_v1 *     r = track->chord_regions[j];
          yyjson_mut_val * r_obj =
            yyjson_mut_arr_add_obj (doc, chord_regions_arr);
          region_v1_serialize_to_json (doc, r_obj, r, error);
        }
      yyjson_mut_val * scales_arr =
        yyjson_mut_obj_add_arr (doc, track_obj, "scaleObjects");
      for (int j = 0; j < track->num_scales; j++)
        {
          ScaleObject_v1 * so = track->scales[j];
          yyjson_mut_val * so_obj = yyjson_mut_arr_add_obj (doc, scales_arr);
          scale_object_v1_serialize_to_json (doc, so_obj, so, error);
        }
    }
  if (track->type == TRACK_TYPE_MARKER_v1)
    {
      yyjson_mut_val * markers_arr =
        yyjson_mut_obj_add_arr (doc, track_obj, "markers");
      for (int j = 0; j < track->num_markers; j++)
        {
          Marker_v1 *      m = track->markers[j];
          yyjson_mut_val * m_obj = yyjson_mut_arr_add_obj (doc, markers_arr);
          marker_v1_serialize_to_json (doc, m_obj, m, error);
        }
    }
  if (track->channel)
    {
      yyjson_mut_val * ch_obj =
        yyjson_mut_obj_add_obj (doc, track_obj, "channel");
      channel_v2_serialize_to_json (doc, ch_obj, track->channel, error);
    }
  if (track->type == TRACK_TYPE_TEMPO_v1)
    {
      yyjson_mut_val * port_obj =
        yyjson_mut_obj_add_obj (doc, track_obj, "bpmPort");
      port_v1_serialize_to_json (doc, port_obj, track->bpm_port, error);
      port_obj = yyjson_mut_obj_add_obj (doc, track_obj, "beatsPerBarPort");
      port_v1_serialize_to_json (
        doc, port_obj, track->beats_per_bar_port, error);
      port_obj = yyjson_mut_obj_add_obj (doc, track_obj, "beatUnitPort");
      port_v1_serialize_to_json (doc, port_obj, track->beat_unit_port, error);
    }
  if (track->modulators)
    {
      yyjson_mut_val * modulators_arr =
        yyjson_mut_obj_add_arr (doc, track_obj, "modulators");
      for (int j = 0; j < track->num_modulators; j++)
        {
          Plugin_v1 * pl = track->modulators[j];
          yyjson_mut_val * pl_obj = yyjson_mut_arr_add_obj (doc, modulators_arr);
          plugin_v1_serialize_to_json (doc, pl_obj, pl, error);
        }
    }
  yyjson_mut_val * modulator_macros_arr =
    yyjson_mut_obj_add_arr (doc, track_obj, "modulatorMacros");
  for (int j = 0; j < track->num_modulator_macros; j++)
    {
      ModulatorMacroProcessor_v1 * mmp = track->modulator_macros[j];
      yyjson_mut_val *             mmp_obj =
        yyjson_mut_arr_add_obj (doc, modulator_macros_arr);
      modulator_macro_processor_v1_serialize_to_json (doc, mmp_obj, mmp, error);
    }
  yyjson_mut_obj_add_int (
    doc, track_obj, "numVisibleModulatorMacros",
    track->num_visible_modulator_macros);
  yyjson_mut_val * tp_obj = yyjson_mut_obj_add_obj (doc, track_obj, "processor");
  track_processor_v1_serialize_to_json (doc, tp_obj, track->processor, error);
  yyjson_mut_val * atl_obj =
    yyjson_mut_obj_add_obj (doc, track_obj, "automationTracklist");
  yyjson_mut_val * ats_arr =
    yyjson_mut_obj_add_arr (doc, atl_obj, "automationTracks");
  for (int j = 0; j < track->automation_tracklist.num_ats; j++)
    {
      AutomationTrack_v1 * at = track->automation_tracklist.ats[j];
      yyjson_mut_val *     at_obj = yyjson_mut_arr_add_obj (doc, ats_arr);
      automation_track_v1_serialize_to_json (doc, at_obj, at, error);
    }
  yyjson_mut_obj_add_int (doc, track_obj, "inSignalType", track->in_signal_type);
  yyjson_mut_obj_add_int (
    doc, track_obj, "outSignalType", track->out_signal_type);
  yyjson_mut_obj_add_uint (doc, track_obj, "midiCh", track->midi_ch);
  yyjson_mut_obj_add_str (doc, track_obj, "comment", track->comment);
  yyjson_mut_val * children_arr =
    yyjson_mut_obj_add_arr (doc, track_obj, "children");
  for (int j = 0; j < track->num_children; j++)
    {
      yyjson_mut_arr_add_uint (doc, children_arr, track->children[j]);
    }
  yyjson_mut_obj_add_bool (doc, track_obj, "frozen", track->frozen);
  yyjson_mut_obj_add_int (doc, track_obj, "poolId", track->pool_id);
  yyjson_mut_obj_add_int (doc, track_obj, "size", track->size);
  yyjson_mut_obj_add_bool (doc, track_obj, "folded", track->folded);
  yyjson_mut_obj_add_bool (
    doc, track_obj, "recordSetAutomatically", track->record_set_automatically);
  yyjson_mut_obj_add_bool (doc, track_obj, "drumMode", track->drum_mode);
  return true;
}

static bool
editor_settings_v1_serialize_to_json (
  yyjson_mut_doc *          doc,
  yyjson_mut_val *          settings_obj,
  const EditorSettings_v1 * settings,
  GError **                 error)
{
  yyjson_mut_obj_add_int (
    doc, settings_obj, "scrollStartX", settings->scroll_start_x);
  yyjson_mut_obj_add_int (
    doc, settings_obj, "scrollStartY", settings->scroll_start_y);
  yyjson_mut_obj_add_real (
    doc, settings_obj, "horizontalZoomLevel", settings->hzoom_level);
  return true;
}

static bool
piano_roll_v1_serialize_to_json (
  yyjson_mut_doc *     doc,
  yyjson_mut_val *     pr_obj,
  const PianoRoll_v1 * pr,
  GError **            error)
{
  yyjson_mut_obj_add_real (doc, pr_obj, "notesZoom", pr->notes_zoom);
  yyjson_mut_obj_add_int (doc, pr_obj, "midiModifier", pr->midi_modifier);
  yyjson_mut_val * editor_settings_obj =
    yyjson_mut_obj_add_obj (doc, pr_obj, "editorSettings");
  editor_settings_v1_serialize_to_json (
    doc, editor_settings_obj, &pr->editor_settings, error);
  return true;
}

static bool
automation_editor_v1_serialize_to_json (
  yyjson_mut_doc *            doc,
  yyjson_mut_val *            ae_obj,
  const AutomationEditor_v1 * ae,
  GError **                   error)
{
  yyjson_mut_val * editor_settings_obj =
    yyjson_mut_obj_add_obj (doc, ae_obj, "editorSettings");
  editor_settings_v1_serialize_to_json (
    doc, editor_settings_obj, &ae->editor_settings, error);
  return true;
}

static bool
chord_editor_v1_serialize_to_json (
  yyjson_mut_doc *       doc,
  yyjson_mut_val *       ce_obj,
  const ChordEditor_v1 * ce,
  GError **              error)
{
  yyjson_mut_val * chords_arr = yyjson_mut_obj_add_arr (doc, ce_obj, "chords");
  for (int i = 0; i < ce->num_chords; i++)
    {
      const ChordDescriptor_v2 * chord = ce->chords[i];
      yyjson_mut_val * chord_obj = yyjson_mut_arr_add_obj (doc, chords_arr);
      chord_descriptor_v2_serialize_to_json (doc, chord_obj, chord, error);
    }
  yyjson_mut_val * editor_settings_obj =
    yyjson_mut_obj_add_obj (doc, ce_obj, "editorSettings");
  editor_settings_v1_serialize_to_json (
    doc, editor_settings_obj, &ce->editor_settings, error);
  return true;
}

static bool
audio_clip_editor_v1_serialize_to_json (
  yyjson_mut_doc *           doc,
  yyjson_mut_val *           editor_obj,
  const AudioClipEditor_v1 * editor,
  GError **                  error)
{
  yyjson_mut_val * editor_settings_obj =
    yyjson_mut_obj_add_obj (doc, editor_obj, "editorSettings");
  editor_settings_v1_serialize_to_json (
    doc, editor_settings_obj, &editor->editor_settings, error);
  return true;
}

static bool
clip_editor_v1_serialize_to_json (
  yyjson_mut_doc *      doc,
  yyjson_mut_val *      ce_obj,
  const ClipEditor_v1 * ce,
  GError **             error)
{
  yyjson_mut_val * region_id_obj =
    yyjson_mut_obj_add_obj (doc, ce_obj, "regionId");
  region_identifier_v1_serialize_to_json (
    doc, region_id_obj, &ce->region_id, error);
  yyjson_mut_obj_add_bool (doc, ce_obj, "hasRegion", ce->has_region);
  yyjson_mut_val * piano_roll_obj =
    yyjson_mut_obj_add_obj (doc, ce_obj, "pianoRoll");
  piano_roll_v1_serialize_to_json (doc, piano_roll_obj, ce->piano_roll, error);
  yyjson_mut_val * automation_editor_obj =
    yyjson_mut_obj_add_obj (doc, ce_obj, "automationEditor");
  automation_editor_v1_serialize_to_json (
    doc, automation_editor_obj, ce->automation_editor, error);
  yyjson_mut_val * chord_editor_obj =
    yyjson_mut_obj_add_obj (doc, ce_obj, "chordEditor");
  chord_editor_v1_serialize_to_json (
    doc, chord_editor_obj, ce->chord_editor, error);
  yyjson_mut_val * audio_clip_editor_obj =
    yyjson_mut_obj_add_obj (doc, ce_obj, "audioClipEditor");
  audio_clip_editor_v1_serialize_to_json (
    doc, audio_clip_editor_obj, ce->audio_clip_editor, error);
  return true;
}

static bool
timeline_v1_serialize_to_json (
  yyjson_mut_doc *    doc,
  yyjson_mut_val *    t_obj,
  const Timeline_v1 * t,
  GError **           error)
{
  yyjson_mut_val * editor_settings_obj =
    yyjson_mut_obj_add_obj (doc, t_obj, "editorSettings");
  editor_settings_v1_serialize_to_json (
    doc, editor_settings_obj, &t->editor_settings, error);
  return true;
}

static bool
snap_grid_v1_serialize_to_json (
  yyjson_mut_doc *    doc,
  yyjson_mut_val *    sg_obj,
  const SnapGrid_v1 * sg,
  GError **           error)
{
  yyjson_mut_obj_add_int (doc, sg_obj, "type", sg->type);
  yyjson_mut_obj_add_int (doc, sg_obj, "snapNoteLength", sg->snap_note_length);
  yyjson_mut_obj_add_int (doc, sg_obj, "snapNoteType", sg->snap_note_type);
  yyjson_mut_obj_add_bool (doc, sg_obj, "snapAdaptive", sg->snap_adaptive);
  yyjson_mut_obj_add_int (
    doc, sg_obj, "defaultNoteLength", sg->default_note_length);
  yyjson_mut_obj_add_int (doc, sg_obj, "defaultNoteType", sg->default_note_type);
  yyjson_mut_obj_add_bool (doc, sg_obj, "defaultAdaptive", sg->default_adaptive);
  yyjson_mut_obj_add_int (doc, sg_obj, "lengthType", sg->length_type);
  yyjson_mut_obj_add_bool (doc, sg_obj, "snapToGrid", sg->snap_to_grid);
  yyjson_mut_obj_add_bool (
    doc, sg_obj, "keepOffset", sg->snap_to_grid_keep_offset);
  yyjson_mut_obj_add_bool (doc, sg_obj, "snapToEvents", sg->snap_to_events);
  return true;
}

static bool
quantize_options_v1_serialize_to_json (
  yyjson_mut_doc *           doc,
  yyjson_mut_val *           qo_obj,
  const QuantizeOptions_v1 * qo,
  GError **                  error)
{
  yyjson_mut_obj_add_int (doc, qo_obj, "noteLength", qo->note_length);
  yyjson_mut_obj_add_int (doc, qo_obj, "noteType", qo->note_type);
  yyjson_mut_obj_add_real (doc, qo_obj, "amount", qo->amount);
  yyjson_mut_obj_add_int (doc, qo_obj, "adjStart", qo->adj_start);
  yyjson_mut_obj_add_int (doc, qo_obj, "adjEnd", qo->adj_end);
  yyjson_mut_obj_add_real (doc, qo_obj, "swing", qo->swing);
  yyjson_mut_obj_add_real (doc, qo_obj, "randTicks", qo->rand_ticks);
  return true;
}

static bool
tracklist_selections_v2_serialize_to_json (
  yyjson_mut_doc *               doc,
  yyjson_mut_val *               sel_obj,
  const TracklistSelections_v2 * sel,
  GError **                      error)
{
  yyjson_mut_obj_add_bool (doc, sel_obj, "isProject", sel->is_project);
  yyjson_mut_val * tracks_arr = yyjson_mut_obj_add_arr (doc, sel_obj, "tracks");
  for (int i = 0; i < sel->num_tracks; i++)
    {
      Track_v2 *       track = sel->tracks[i];
      yyjson_mut_val * track_obj = yyjson_mut_arr_add_obj (doc, tracks_arr);
      track_v2_serialize_to_json (doc, track_obj, track, error);
    }
  return true;
}

static bool
transport_v1_serialize_to_json (
  yyjson_mut_doc *     doc,
  yyjson_mut_val *     transport_obj,
  const Transport_v1 * transport,
  GError **            error)
{
  yyjson_mut_obj_add_int (
    doc, transport_obj, "totalBars", transport->total_bars);
  yyjson_mut_val * playhead_pos_obj =
    yyjson_mut_obj_add_obj (doc, transport_obj, "playheadPos");
  position_v1_serialize_to_json (
    doc, playhead_pos_obj, &transport->playhead_pos, error);
  yyjson_mut_val * cue_pos_obj =
    yyjson_mut_obj_add_obj (doc, transport_obj, "cuePos");
  position_v1_serialize_to_json (doc, cue_pos_obj, &transport->cue_pos, error);
  yyjson_mut_val * loop_start_pos_obj =
    yyjson_mut_obj_add_obj (doc, transport_obj, "loopStartPos");
  position_v1_serialize_to_json (
    doc, loop_start_pos_obj, &transport->loop_start_pos, error);
  yyjson_mut_val * loop_end_pos_obj =
    yyjson_mut_obj_add_obj (doc, transport_obj, "loopEndPos");
  position_v1_serialize_to_json (
    doc, loop_end_pos_obj, &transport->loop_end_pos, error);
  yyjson_mut_val * punch_in_pos_obj =
    yyjson_mut_obj_add_obj (doc, transport_obj, "punchInPos");
  position_v1_serialize_to_json (
    doc, punch_in_pos_obj, &transport->punch_in_pos, error);
  yyjson_mut_val * punch_out_pos_obj =
    yyjson_mut_obj_add_obj (doc, transport_obj, "punchOutPos");
  position_v1_serialize_to_json (
    doc, punch_out_pos_obj, &transport->punch_out_pos, error);
  yyjson_mut_val * range_1_obj =
    yyjson_mut_obj_add_obj (doc, transport_obj, "range1");
  position_v1_serialize_to_json (doc, range_1_obj, &transport->range_1, error);
  yyjson_mut_val * range_2_obj =
    yyjson_mut_obj_add_obj (doc, transport_obj, "range2");
  position_v1_serialize_to_json (doc, range_2_obj, &transport->range_2, error);
  yyjson_mut_obj_add_bool (doc, transport_obj, "hasRange", transport->has_range);
  yyjson_mut_obj_add_uint (doc, transport_obj, "position", transport->position);
  yyjson_mut_val * roll_obj =
    yyjson_mut_obj_add_obj (doc, transport_obj, "roll");
  port_v1_serialize_to_json (doc, roll_obj, transport->roll, error);
  yyjson_mut_val * stop_obj =
    yyjson_mut_obj_add_obj (doc, transport_obj, "stop");
  port_v1_serialize_to_json (doc, stop_obj, transport->stop, error);
  yyjson_mut_val * backward_obj =
    yyjson_mut_obj_add_obj (doc, transport_obj, "backward");
  port_v1_serialize_to_json (doc, backward_obj, transport->backward, error);
  yyjson_mut_val * forward_obj =
    yyjson_mut_obj_add_obj (doc, transport_obj, "forward");
  port_v1_serialize_to_json (doc, forward_obj, transport->forward, error);
  yyjson_mut_val * loop_toggle_obj =
    yyjson_mut_obj_add_obj (doc, transport_obj, "loopToggle");
  port_v1_serialize_to_json (
    doc, loop_toggle_obj, transport->loop_toggle, error);
  yyjson_mut_val * rec_toggle_obj =
    yyjson_mut_obj_add_obj (doc, transport_obj, "recToggle");
  port_v1_serialize_to_json (doc, rec_toggle_obj, transport->rec_toggle, error);
  return true;
}

static bool
audio_clip_v1_serialize_to_json (
  yyjson_mut_doc *     doc,
  yyjson_mut_val *     clip_obj,
  const AudioClip_v1 * clip,
  GError **            error)
{
  yyjson_mut_obj_add_str (doc, clip_obj, "name", clip->name);
  if (clip->file_hash)
    {
      yyjson_mut_obj_add_str (doc, clip_obj, "fileHash", clip->file_hash);
    }
  yyjson_mut_obj_add_real (doc, clip_obj, "bpm", clip->bpm);
  yyjson_mut_obj_add_int (doc, clip_obj, "bitDepth", clip->bit_depth);
  yyjson_mut_obj_add_bool (doc, clip_obj, "useFlac", clip->use_flac);
  yyjson_mut_obj_add_int (doc, clip_obj, "samplerate", clip->samplerate);
  yyjson_mut_obj_add_int (doc, clip_obj, "poolId", clip->pool_id);
  return true;
}

static bool
audio_pool_v1_serialize_to_json (
  yyjson_mut_doc *     doc,
  yyjson_mut_val *     pool_obj,
  const AudioPool_v1 * pool,
  GError **            error)
{
  yyjson_mut_val * clips_arr = yyjson_mut_obj_add_arr (doc, pool_obj, "clips");
  for (int i = 0; i < pool->num_clips; i++)
    {
      AudioClip_v1 * clip = pool->clips[i];
      if (clip)
        {
          yyjson_mut_val * clip_obj = yyjson_mut_arr_add_obj (doc, clips_arr);
          audio_clip_v1_serialize_to_json (doc, clip_obj, clip, error);
        }
      else
        {
          yyjson_mut_arr_add_null (doc, clips_arr);
        }
    }
  return true;
}

static bool
control_room_v2_serialize_to_json (
  yyjson_mut_doc *       doc,
  yyjson_mut_val *       cr_obj,
  const ControlRoom_v2 * cr,
  GError **              error)
{
  yyjson_mut_val * monitor_fader_obj =
    yyjson_mut_obj_add_obj (doc, cr_obj, "monitorFader");
  fader_v2_serialize_to_json (doc, monitor_fader_obj, cr->monitor_fader, error);
  return true;
}

static bool
sample_processor_v2_serialize_to_json (
  yyjson_mut_doc *           doc,
  yyjson_mut_val *           sp_obj,
  const SampleProcessor_v2 * sp,
  GError **                  error)
{
  yyjson_mut_val * fader_obj = yyjson_mut_obj_add_obj (doc, sp_obj, "fader");
  fader_v2_serialize_to_json (doc, fader_obj, sp->fader, error);
  return true;
}

static bool
hardware_processor_v1_serialize_to_json (
  yyjson_mut_doc *             doc,
  yyjson_mut_val *             hp_obj,
  const HardwareProcessor_v1 * hp,
  GError **                    error)
{
  yyjson_mut_obj_add_bool (doc, hp_obj, "isInput", hp->is_input);
  if (hp->ext_audio_ports)
    {
      yyjson_mut_val * ext_audio_ports_arr =
        yyjson_mut_obj_add_arr (doc, hp_obj, "extAudioPorts");
      for (int i = 0; i < hp->num_ext_audio_ports; i++)
        {
          ExtPort_v1 *     port = hp->ext_audio_ports[i];
          yyjson_mut_val * port_obj =
            yyjson_mut_arr_add_obj (doc, ext_audio_ports_arr);
          ext_port_v1_serialize_to_json (doc, port_obj, port, error);
        }
    }
  if (hp->ext_midi_ports)
    {
      yyjson_mut_val * ext_midi_ports_arr =
        yyjson_mut_obj_add_arr (doc, hp_obj, "extMidiPorts");
      for (int i = 0; i < hp->num_ext_midi_ports; i++)
        {
          ExtPort_v1 *     port = hp->ext_midi_ports[i];
          yyjson_mut_val * port_obj =
            yyjson_mut_arr_add_obj (doc, ext_midi_ports_arr);
          ext_port_v1_serialize_to_json (doc, port_obj, port, error);
        }
    }
  if (hp->audio_ports)
    {
      yyjson_mut_val * audio_ports_arr =
        yyjson_mut_obj_add_arr (doc, hp_obj, "audioPorts");
      for (int i = 0; i < hp->num_audio_ports; i++)
        {
          Port_v1 *        port = hp->audio_ports[i];
          yyjson_mut_val * port_obj =
            yyjson_mut_arr_add_obj (doc, audio_ports_arr);
          port_v1_serialize_to_json (doc, port_obj, port, error);
        }
    }
  if (hp->midi_ports)
    {
      yyjson_mut_val * midi_ports_arr =
        yyjson_mut_obj_add_arr (doc, hp_obj, "midiPorts");
      for (int i = 0; i < hp->num_midi_ports; i++)
        {
          Port_v1 *        port = hp->midi_ports[i];
          yyjson_mut_val * port_obj =
            yyjson_mut_arr_add_obj (doc, midi_ports_arr);
          port_v1_serialize_to_json (doc, port_obj, port, error);
        }
    }
  return true;
}

static bool
audio_engine_v2_serialize_to_json (
  yyjson_mut_doc *       doc,
  yyjson_mut_val *       engine_obj,
  const AudioEngine_v2 * engine,
  GError **              error)
{
  yyjson_mut_obj_add_int (
    doc, engine_obj, "transportType", engine->transport_type);
  yyjson_mut_obj_add_uint (doc, engine_obj, "sampleRate", engine->sample_rate);
  yyjson_mut_obj_add_real (
    doc, engine_obj, "framesPerTick", engine->frames_per_tick);
  yyjson_mut_val * monitor_out_obj =
    yyjson_mut_obj_add_obj (doc, engine_obj, "monitorOut");
  stereo_ports_v1_serialize_to_json (
    doc, monitor_out_obj, engine->monitor_out, error);
  yyjson_mut_val * midi_editor_manual_press_obj =
    yyjson_mut_obj_add_obj (doc, engine_obj, "midiEditorManualPress");
  port_v1_serialize_to_json (
    doc, midi_editor_manual_press_obj, engine->midi_editor_manual_press, error);
  yyjson_mut_val * midi_in_obj =
    yyjson_mut_obj_add_obj (doc, engine_obj, "midiIn");
  port_v1_serialize_to_json (doc, midi_in_obj, engine->midi_in, error);
  yyjson_mut_val * transport_obj =
    yyjson_mut_obj_add_obj (doc, engine_obj, "transport");
  transport_v1_serialize_to_json (doc, transport_obj, engine->transport, error);
  yyjson_mut_val * pool_obj = yyjson_mut_obj_add_obj (doc, engine_obj, "pool");
  audio_pool_v1_serialize_to_json (doc, pool_obj, engine->pool, error);
  yyjson_mut_val * control_room_obj =
    yyjson_mut_obj_add_obj (doc, engine_obj, "controlRoom");
  control_room_v2_serialize_to_json (
    doc, control_room_obj, engine->control_room, error);
  yyjson_mut_val * sample_processor_obj =
    yyjson_mut_obj_add_obj (doc, engine_obj, "sampleProcessor");
  sample_processor_v2_serialize_to_json (
    doc, sample_processor_obj, engine->sample_processor, error);
  yyjson_mut_val * hw_in_processor_obj =
    yyjson_mut_obj_add_obj (doc, engine_obj, "hwInProcessor");
  hardware_processor_v1_serialize_to_json (
    doc, hw_in_processor_obj, engine->hw_in_processor, error);
  yyjson_mut_val * hw_out_processor_obj =
    yyjson_mut_obj_add_obj (doc, engine_obj, "hwOutProcessor");
  hardware_processor_v1_serialize_to_json (
    doc, hw_out_processor_obj, engine->hw_out_processor, error);
  return true;
}

static bool
region_link_group_manager_v1_serialize_to_json (
  yyjson_mut_doc *                  doc,
  yyjson_mut_val *                  mgr_obj,
  const RegionLinkGroupManager_v1 * mgr,
  GError **                         error)
{
  if (mgr->groups)
    {
      yyjson_mut_val * groups_arr =
        yyjson_mut_obj_add_arr (doc, mgr_obj, "groups");
      for (int i = 0; i < mgr->num_groups; i++)
        {
          RegionLinkGroup_v1 * group = mgr->groups[i];
          yyjson_mut_val * group_obj = yyjson_mut_arr_add_obj (doc, groups_arr);
          yyjson_mut_val * ids_arr =
            yyjson_mut_obj_add_arr (doc, group_obj, "ids");
          for (int j = 0; j < group->num_ids; j++)
            {
              RegionIdentifier_v1 * id = &group->ids[j];
              yyjson_mut_val * id_obj = yyjson_mut_arr_add_obj (doc, ids_arr);
              region_identifier_v1_serialize_to_json (doc, id_obj, id, error);
            }
          yyjson_mut_obj_add_int (doc, group_obj, "groupIdx", group->group_idx);
        }
    }
  return true;
}

static bool
midi_mappings_v1_serialize_to_json (
  yyjson_mut_doc *        doc,
  yyjson_mut_val *        mm_obj,
  const MidiMappings_v1 * mm,
  GError **               error)
{
  if (mm->mappings)
    {
      yyjson_mut_val * mappings_arr =
        yyjson_mut_obj_add_arr (doc, mm_obj, "mappings");
      for (int i = 0; i < mm->num_mappings; i++)
        {
          MidiMapping_v1 * m = mm->mappings[i];
          yyjson_mut_val * m_obj = yyjson_mut_arr_add_obj (doc, mappings_arr);
          yyjson_mut_val * key_obj = yyjson_mut_arr_with_uint8 (doc, m->key, 3);
          yyjson_mut_obj_add_val (doc, m_obj, "key", key_obj);
          if (m->device_port)
            {
              yyjson_mut_val * device_port_obj =
                yyjson_mut_obj_add_obj (doc, m_obj, "devicePort");
              ext_port_v1_serialize_to_json (
                doc, device_port_obj, m->device_port, error);
            }
          yyjson_mut_val * dest_id_obj =
            yyjson_mut_obj_add_obj (doc, m_obj, "destId");
          port_identifier_v1_serialize_to_json (
            doc, dest_id_obj, &m->dest_id, error);
          yyjson_mut_obj_add_bool (doc, m_obj, "enabled", m->enabled);
        }
    }
  return true;
}

char *
project_v5_serialize_to_json_str (const Project_v5 * prj, GError ** error)
{
  g_return_val_if_fail (prj, NULL);

  yyjson_mut_doc * doc = yyjson_mut_doc_new (NULL);
  yyjson_mut_val * root = yyjson_mut_obj (doc);
  if (!root)
    {
      g_set_error_literal (
        error, Z_SCHEMAS_PROJECT_ERROR, Z_SCHEMAS_PROJECT_ERROR_FAILED,
        "Failed to create root obj");
      return NULL;
    }
  yyjson_mut_doc_set_root (doc, root);

  yyjson_mut_obj_add_int (doc, root, "formatMajor", 1);
  yyjson_mut_obj_add_int (doc, root, "formatMinor", 6);
  yyjson_mut_obj_add_str (doc, root, "type", "ZrythmProject");

  yyjson_mut_obj_add_str (doc, root, "title", prj->title);
  yyjson_mut_obj_add_str (doc, root, "datetime", prj->datetime_str);
  yyjson_mut_obj_add_str (doc, root, "version", prj->version);

  /* tracklist */
  yyjson_mut_val * tracklist_obj =
    yyjson_mut_obj_add_obj (doc, root, "tracklist");
  yyjson_mut_obj_add_int (
    doc, tracklist_obj, "pinnedTracksCutoff",
    prj->tracklist->pinned_tracks_cutoff);
  yyjson_mut_val * tracks_arr =
    yyjson_mut_obj_add_arr (doc, tracklist_obj, "tracks");
  for (int i = 0; i < prj->tracklist->num_tracks; i++)
    {
      Track_v2 *       track = prj->tracklist->tracks[i];
      yyjson_mut_val * track_obj = yyjson_mut_arr_add_obj (doc, tracks_arr);
      track_v2_serialize_to_json (doc, track_obj, track, error);
    }
  yyjson_mut_val * clip_editor_obj =
    yyjson_mut_obj_add_obj (doc, root, "clipEditor");
  clip_editor_v1_serialize_to_json (
    doc, clip_editor_obj, prj->clip_editor, error);
  yyjson_mut_val * timeline_obj = yyjson_mut_obj_add_obj (doc, root, "timeline");
  timeline_v1_serialize_to_json (doc, timeline_obj, prj->timeline, error);
  yyjson_mut_val * snap_grid_tl_obj =
    yyjson_mut_obj_add_obj (doc, root, "snapGridTimeline");
  snap_grid_v1_serialize_to_json (
    doc, snap_grid_tl_obj, prj->snap_grid_timeline, error);
  yyjson_mut_val * snap_grid_editor_obj =
    yyjson_mut_obj_add_obj (doc, root, "snapGridEditor");
  snap_grid_v1_serialize_to_json (
    doc, snap_grid_editor_obj, prj->snap_grid_editor, error);
  yyjson_mut_val * quantize_options_tl_obj =
    yyjson_mut_obj_add_obj (doc, root, "quantizeOptsTimeline");
  quantize_options_v1_serialize_to_json (
    doc, quantize_options_tl_obj, prj->quantize_opts_timeline, error);
  yyjson_mut_val * quantize_options_editor_obj =
    yyjson_mut_obj_add_obj (doc, root, "quantizeOptsEditor");
  quantize_options_v1_serialize_to_json (
    doc, quantize_options_editor_obj, prj->quantize_opts_editor, error);
  yyjson_mut_val * audio_engine_obj =
    yyjson_mut_obj_add_obj (doc, root, "audioEngine");
  audio_engine_v2_serialize_to_json (
    doc, audio_engine_obj, prj->audio_engine, error);
  yyjson_mut_val * tracklist_selections_obj =
    yyjson_mut_obj_add_obj (doc, root, "tracklistSelections");
  tracklist_selections_v2_serialize_to_json (
    doc, tracklist_selections_obj, prj->tracklist_selections, error);
  yyjson_mut_val * region_link_group_mgr_obj =
    yyjson_mut_obj_add_obj (doc, root, "regionLinkGroupManager");
  region_link_group_manager_v1_serialize_to_json (
    doc, region_link_group_mgr_obj, prj->region_link_group_manager, error);
  yyjson_mut_val * port_connections_mgr_obj =
    yyjson_mut_obj_add_obj (doc, root, "portConnectionsManager");
  port_connections_manager_v1_serialize_to_json (
    doc, port_connections_mgr_obj, prj->port_connections_manager, error);
  yyjson_mut_val * midi_mappings_obj =
    yyjson_mut_obj_add_obj (doc, root, "midiMappings");
  midi_mappings_v1_serialize_to_json (
    doc, midi_mappings_obj, prj->midi_mappings, error);
  yyjson_mut_obj_add_int (doc, root, "lastSelection", prj->last_selection);

  /* to string - minified */
  char * json = yyjson_mut_write (
    doc, YYJSON_WRITE_NOFLAG /* YYJSON_WRITE_PRETTY_TWO_SPACES */, NULL);

  yyjson_mut_doc_free (doc);

  if (!json)
    {
      g_set_error_literal (
        error, Z_SCHEMAS_PROJECT_ERROR, Z_SCHEMAS_PROJECT_ERROR_FAILED,
        "Failed to write JSON");
      return NULL;
    }

  return json;
}
