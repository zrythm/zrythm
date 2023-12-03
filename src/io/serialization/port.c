// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/ext_port.h"
#include "dsp/port.h"
#include "dsp/port_connection.h"
#include "dsp/port_connections_manager.h"
#include "io/serialization/plugin.h"
#include "io/serialization/port.h"

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
      yyjson_mut_obj_add_str (doc, pi_obj, "URI", pi->uri);
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
