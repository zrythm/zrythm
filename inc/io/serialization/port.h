// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Port serialization.
 */

#ifndef __IO_PORT_H__
#define __IO_PORT_H__

#include "utils/types.h"

#include <glib.h>

#include <yyjson.h>

TYPEDEF_STRUCT (PortIdentifier);
TYPEDEF_STRUCT (Port);
TYPEDEF_STRUCT (StereoPorts);
TYPEDEF_STRUCT (ExtPort);
TYPEDEF_STRUCT (PortConnection);
TYPEDEF_STRUCT (PortConnectionsManager);

bool
port_identifier_serialize_to_json (
  yyjson_mut_doc *       doc,
  yyjson_mut_val *       pi_obj,
  const PortIdentifier * pi,
  GError **              error);

bool
port_serialize_to_json (
  yyjson_mut_doc * doc,
  yyjson_mut_val * port_obj,
  const Port *     port,
  GError **        error);

bool
stereo_ports_serialize_to_json (
  yyjson_mut_doc *    doc,
  yyjson_mut_val *    sp_obj,
  const StereoPorts * sp,
  GError **           error);

bool
ext_port_serialize_to_json (
  yyjson_mut_doc * doc,
  yyjson_mut_val * ep_obj,
  const ExtPort *  ep,
  GError **        error);

bool
port_connection_serialize_to_json (
  yyjson_mut_doc *       doc,
  yyjson_mut_val *       conn_obj,
  const PortConnection * conn,
  GError **              error);

bool
port_connections_manager_serialize_to_json (
  yyjson_mut_doc *               doc,
  yyjson_mut_val *               mgr_obj,
  const PortConnectionsManager * mgr,
  GError **                      error);

#endif // __IO_PORT_H__
