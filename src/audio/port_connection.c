/*
 * Copyright (C) 2021-2022 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "audio/port_connection.h"
#include "audio/port_identifier.h"
#include "audio/tracklist.h"
#include "project.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "zrythm.h"

PortConnection *
port_connection_new (
  const PortIdentifier * src,
  const PortIdentifier * dest,
  float                  multiplier,
  bool                   locked,
  bool                   enabled)
{
  PortConnection * self =
    object_new (PortConnection);
  self->schema_version =
    PORT_CONNECTION_SCHEMA_VERSION;

  self->src_id = port_identifier_clone (src);
  self->dest_id = port_identifier_clone (dest);
  port_connection_update (
    self, multiplier, locked, enabled);

  return self;
}

void
port_connection_update (
  PortConnection * self,
  float            multiplier,
  bool             locked,
  bool             enabled)
{
  self->multiplier = multiplier;
  self->locked = locked;
  self->enabled = enabled;
}

bool
port_connection_is_send (
  const PortConnection * self)
{
  return self->src_id->owner_type
         == PORT_OWNER_TYPE_CHANNEL_SEND;
}

void
port_connection_print_to_str (
  const PortConnection * self,
  char *                 buf,
  size_t                 buf_sz)
{
  bool is_send = port_connection_is_send (self);
  const char * send_str = is_send ? " (send)" : "";
  if (
    ZRYTHM && PROJECT
    && port_connections_manager_contains_connection (
      PORT_CONNECTIONS_MGR, self))
    {
      Track * src_track =
        tracklist_find_track_by_name_hash (
          TRACKLIST, self->src_id->track_name_hash);
      Track * dest_track =
        tracklist_find_track_by_name_hash (
          TRACKLIST, self->dest_id->track_name_hash);
      snprintf (
        buf, buf_sz,
        "[%s (%u)] %s => [%s (%u)] %s%s",
        src_track ? src_track->name : "(none)",
        self->src_id->track_name_hash,
        self->src_id->label,
        dest_track ? dest_track->name : "(none)",
        self->dest_id->track_name_hash,
        self->dest_id->label, send_str);
    }
  else
    {
      snprintf (
        buf, buf_sz,
        "[track %u] %s => [track %u] %s%s",
        self->src_id->track_name_hash,
        self->src_id->label,
        self->dest_id->track_name_hash,
        self->dest_id->label, send_str);
    }
}

void
port_connection_print (const PortConnection * self)
{
  char buf[200];
  port_connection_print_to_str (self, buf, 200);
  g_message ("%s", buf);
}

/**
 * To be used during serialization.
 */
NONNULL
PortConnection *
port_connection_clone (const PortConnection * src)
{
  PortConnection * self = port_connection_new (
    src->src_id, src->dest_id, src->multiplier,
    src->locked, src->enabled);
  return self;
}

/**
 * Deletes port, doing required cleanup and updating counters.
 */
NONNULL
void
port_connection_free (PortConnection * self)
{
  port_identifier_free (self->src_id);
  port_identifier_free (self->dest_id);
  object_zero_and_free (self);
}
