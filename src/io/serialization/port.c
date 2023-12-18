// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/ext_port.h"
#include "dsp/port.h"
#include "dsp/port_connection.h"
#include "dsp/port_connections_manager.h"
#include "io/serialization/plugin.h"
#include "io/serialization/port.h"
#include "utils/objects.h"

#include <yyjson.h>

typedef enum
{
  Z_IO_SERIALIZATION_PORT_ERROR_FAILED,
} ZIOSerializationPortError;

#define Z_IO_SERIALIZATION_PORT_ERROR z_io_serialization_port_error_quark ()
GQuark
z_io_serialization_port_error_quark (void);
G_DEFINE_QUARK (z - io - serialization - port - error - quark, z_io_serialization_port_error)

bool
port_identifier_serialize_to_json (
  yyjson_mut_doc *       doc,
  yyjson_mut_val *       pi_obj,
  const PortIdentifier * pi,
  GError **              error)
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
  plugin_identifier_serialize_to_json (
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

bool
port_serialize_to_json (
  yyjson_mut_doc * doc,
  yyjson_mut_val * port_obj,
  const Port *     port,
  GError **        error)
{
  yyjson_mut_val * pi_id_obj = yyjson_mut_obj_add_obj (doc, port_obj, "id");
  port_identifier_serialize_to_json (doc, pi_id_obj, &port->id, error);
  yyjson_mut_obj_add_bool (
    doc, port_obj, "exposedToBackend", port->exposed_to_backend);
  yyjson_mut_obj_add_real (doc, port_obj, "control", port->control);
  yyjson_mut_obj_add_real (doc, port_obj, "minf", port->minf);
  yyjson_mut_obj_add_real (doc, port_obj, "maxf", port->maxf);
  yyjson_mut_obj_add_real (doc, port_obj, "zerof", port->zerof);
  yyjson_mut_obj_add_real (doc, port_obj, "deff", port->deff);
  yyjson_mut_obj_add_int (
    doc, port_obj, "carlaParameterId", port->carla_param_id);
  yyjson_mut_obj_add_real (doc, port_obj, "baseValue", port->base_value);
  return true;
}

bool
stereo_ports_serialize_to_json (
  yyjson_mut_doc *    doc,
  yyjson_mut_val *    sp_obj,
  const StereoPorts * sp,
  GError **           error)
{
  yyjson_mut_val * l_obj = yyjson_mut_obj_add_obj (doc, sp_obj, "l");
  port_serialize_to_json (doc, l_obj, sp->l, error);
  yyjson_mut_val * r_obj = yyjson_mut_obj_add_obj (doc, sp_obj, "r");
  port_serialize_to_json (doc, r_obj, sp->r, error);
  return true;
}

bool
ext_port_serialize_to_json (
  yyjson_mut_doc * doc,
  yyjson_mut_val * ep_obj,
  const ExtPort *  ep,
  GError **        error)
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

bool
port_connection_serialize_to_json (
  yyjson_mut_doc *       doc,
  yyjson_mut_val *       conn_obj,
  const PortConnection * conn,
  GError **              error)
{
  yyjson_mut_val * src_id_obj = yyjson_mut_obj_add_obj (doc, conn_obj, "srcId");
  port_identifier_serialize_to_json (doc, src_id_obj, conn->src_id, error);
  yyjson_mut_val * dest_id_obj =
    yyjson_mut_obj_add_obj (doc, conn_obj, "destId");
  port_identifier_serialize_to_json (doc, dest_id_obj, conn->dest_id, error);
  yyjson_mut_obj_add_real (doc, conn_obj, "multiplier", conn->multiplier);
  yyjson_mut_obj_add_bool (doc, conn_obj, "enabled", conn->enabled);
  yyjson_mut_obj_add_bool (doc, conn_obj, "locked", conn->locked);
  yyjson_mut_obj_add_real (doc, conn_obj, "baseValue", conn->base_value);
  return true;
}

bool
port_connections_manager_serialize_to_json (
  yyjson_mut_doc *               doc,
  yyjson_mut_val *               mgr_obj,
  const PortConnectionsManager * mgr,
  GError **                      error)
{
  if (mgr->connections)
    {
      yyjson_mut_val * conns_arr =
        yyjson_mut_obj_add_arr (doc, mgr_obj, "connections");
      for (int i = 0; i < mgr->num_connections; i++)
        {
          PortConnection * conn = mgr->connections[i];
          yyjson_mut_val * conn_obj = yyjson_mut_arr_add_obj (doc, conns_arr);
          port_connection_serialize_to_json (doc, conn_obj, conn, error);
        }
    }
  return true;
}

bool
port_identifier_deserialize_from_json (
  yyjson_doc *     doc,
  yyjson_val *     pi_obj,
  PortIdentifier * pi,
  GError **        error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (pi_obj);
  yyjson_val *    label_obj = yyjson_obj_iter_get (&it, "label");
  if (label_obj)
    {
      pi->label = g_strdup (yyjson_get_str (label_obj));
    }
  yyjson_val * sym_obj = yyjson_obj_iter_get (&it, "symbol");
  if (sym_obj)
    {
      pi->sym = g_strdup (yyjson_get_str (sym_obj));
    }
  yyjson_val * uri_obj = yyjson_obj_iter_get (&it, "uri");
  if (uri_obj)
    {
      pi->uri = g_strdup (yyjson_get_str (uri_obj));
    }
  yyjson_val * comment_obj = yyjson_obj_iter_get (&it, "comment");
  if (comment_obj)
    {
      pi->comment = g_strdup (yyjson_get_str (comment_obj));
    }
  pi->owner_type =
    (PortOwnerType) yyjson_get_int (yyjson_obj_iter_get (&it, "ownerType"));
  pi->type = (PortType) yyjson_get_int (yyjson_obj_iter_get (&it, "type"));
  pi->flow = (PortFlow) yyjson_get_int (yyjson_obj_iter_get (&it, "flow"));
  pi->unit = (PortUnit) yyjson_get_int (yyjson_obj_iter_get (&it, "unit"));
  pi->flags = (PortFlags) yyjson_get_int (yyjson_obj_iter_get (&it, "flags"));
  pi->flags2 = (PortFlags2) yyjson_get_int (yyjson_obj_iter_get (&it, "flags2"));
  pi->track_name_hash =
    yyjson_get_uint (yyjson_obj_iter_get (&it, "trackNameHash"));
  yyjson_val * plugin_id_obj = yyjson_obj_iter_get (&it, "pluginId");
  plugin_identifier_deserialize_from_json (
    doc, plugin_id_obj, &pi->plugin_id, error);
  yyjson_val * port_group_obj = yyjson_obj_iter_get (&it, "portGroup");
  if (port_group_obj)
    {
      pi->port_group = g_strdup (yyjson_get_str (port_group_obj));
    }
  yyjson_val * ext_port_id_obj = yyjson_obj_iter_get (&it, "externalPortId");
  if (ext_port_id_obj)
    {
      pi->ext_port_id = g_strdup (yyjson_get_str (ext_port_id_obj));
    }
  pi->port_index = yyjson_get_int (yyjson_obj_iter_get (&it, "portIndex"));
  return true;
}

bool
port_deserialize_from_json (
  yyjson_doc * doc,
  yyjson_val * port_obj,
  Port *       port,
  GError **    error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (port_obj);
  yyjson_val *    id_obj = yyjson_obj_iter_get (&it, "id");
  port_identifier_deserialize_from_json (doc, id_obj, &port->id, error);
  port->exposed_to_backend =
    yyjson_get_bool (yyjson_obj_iter_get (&it, "exposedToBackend"));
  port->control = (float) yyjson_get_real (yyjson_obj_iter_get (&it, "control"));
  port->minf = (float) yyjson_get_real (yyjson_obj_iter_get (&it, "minf"));
  port->maxf = (float) yyjson_get_real (yyjson_obj_iter_get (&it, "maxf"));
  port->zerof = (float) yyjson_get_real (yyjson_obj_iter_get (&it, "zerof"));
  port->deff = (float) yyjson_get_real (yyjson_obj_iter_get (&it, "deff"));
  port->carla_param_id =
    yyjson_get_int (yyjson_obj_iter_get (&it, "carlaParameterId"));
  yyjson_val * base_val_obj = yyjson_obj_iter_get (&it, "baseValue");
  if (base_val_obj) /* added in 1.7 */
    {
      port->base_value = (float) yyjson_get_real (base_val_obj);
    }
  port->magic = PORT_MAGIC;
  return true;
}

bool
stereo_ports_deserialize_from_json (
  yyjson_doc *  doc,
  yyjson_val *  sp_obj,
  StereoPorts * sp,
  GError **     error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (sp_obj);
  yyjson_val *    l_obj = yyjson_obj_iter_get (&it, "l");
  sp->l = object_new (Port);
  port_deserialize_from_json (doc, l_obj, sp->l, error);
  yyjson_val * r_obj = yyjson_obj_iter_get (&it, "r");
  sp->r = object_new (Port);
  port_deserialize_from_json (doc, r_obj, sp->r, error);
  return true;
}

bool
ext_port_deserialize_from_json (
  yyjson_doc * doc,
  yyjson_val * ep_obj,
  ExtPort *    ep,
  GError **    error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (ep_obj);
  ep->full_name =
    g_strdup (yyjson_get_str (yyjson_obj_iter_get (&it, "fullName")));
  yyjson_val * short_name_obj = yyjson_obj_iter_get (&it, "shortName");
  if (short_name_obj)
    {
      ep->short_name = g_strdup (yyjson_get_str (short_name_obj));
    }
  yyjson_val * alias1_obj = yyjson_obj_iter_get (&it, "alias1");
  if (alias1_obj)
    {
      ep->alias1 = g_strdup (yyjson_get_str (alias1_obj));
    }
  yyjson_val * alias2_obj = yyjson_obj_iter_get (&it, "alias2");
  if (alias2_obj)
    {
      ep->alias2 = g_strdup (yyjson_get_str (alias2_obj));
    }
  yyjson_val * rtaudio_dev_name_obj =
    yyjson_obj_iter_get (&it, "rtAudioDeviceName");
  if (rtaudio_dev_name_obj)
    {
      ep->rtaudio_dev_name = g_strdup (yyjson_get_str (rtaudio_dev_name_obj));
    }
  ep->num_aliases = yyjson_get_int (yyjson_obj_iter_get (&it, "numAliases"));
  ep->is_midi = yyjson_get_bool (yyjson_obj_iter_get (&it, "isMidi"));
  ep->type = (ExtPortType) yyjson_get_int (yyjson_obj_iter_get (&it, "type"));
  ep->rtaudio_channel_idx =
    yyjson_get_uint (yyjson_obj_iter_get (&it, "rtAudioChannelIndex"));
  return true;
}

bool
port_connection_deserialize_from_json (
  yyjson_doc *     doc,
  yyjson_val *     conn_obj,
  PortConnection * conn,
  GError **        error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (conn_obj);
  yyjson_val *    src_id_obj = yyjson_obj_iter_get (&it, "srcId");
  conn->src_id = object_new (PortIdentifier);
  port_identifier_deserialize_from_json (doc, src_id_obj, conn->src_id, error);
  yyjson_val * dest_id_obj = yyjson_obj_iter_get (&it, "destId");
  conn->dest_id = object_new (PortIdentifier);
  port_identifier_deserialize_from_json (doc, dest_id_obj, conn->dest_id, error);
  conn->multiplier =
    (float) yyjson_get_real (yyjson_obj_iter_get (&it, "multiplier"));
  conn->enabled = yyjson_get_bool (yyjson_obj_iter_get (&it, "enabled"));
  conn->locked = yyjson_get_bool (yyjson_obj_iter_get (&it, "locked"));
  conn->base_value =
    (float) yyjson_get_real (yyjson_obj_iter_get (&it, "baseValue"));
  return true;
}

bool
port_connections_manager_deserialize_from_json (
  yyjson_doc *             doc,
  yyjson_val *             mgr_obj,
  PortConnectionsManager * mgr,
  GError **                error)
{
  yyjson_obj_iter it = yyjson_obj_iter_with (mgr_obj);
  yyjson_val *    conns_arr = yyjson_obj_iter_get (&it, "connections");
  if (conns_arr)
    {
      mgr->connections_size = yyjson_arr_size (conns_arr);
      if (mgr->connections_size > 0)
        {
          mgr->connections =
            g_malloc0_n (mgr->connections_size, sizeof (PortConnection *));
          yyjson_arr_iter conn_it = yyjson_arr_iter_with (conns_arr);
          yyjson_val *    conn_obj = NULL;
          while ((conn_obj = yyjson_arr_iter_next (&conn_it)))
            {
              PortConnection * conn = object_new (PortConnection);
              mgr->connections[mgr->num_connections++] = conn;
              port_connection_deserialize_from_json (doc, conn_obj, conn, error);
            }
        }
    }
  return true;
}
