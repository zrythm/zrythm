// SPDX-FileCopyrightText: © 2021-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/port_connection.h"
#include "dsp/port_identifier.h"
#include "dsp/tracklist.h"
#include "project.h"
#include "utils/objects.h"
#include "zrythm.h"

PortConnection *
port_connection_new (
  const PortIdentifier * src,
  const PortIdentifier * dest,
  float                  multiplier,
  bool                   locked,
  bool                   enabled)
{
  PortConnection * self = object_new (PortConnection);

  self->src_id = new PortIdentifier (*src);
  self->dest_id = new PortIdentifier (*dest);
  port_connection_update (self, multiplier, locked, enabled);

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
port_connection_is_send (const PortConnection * self)
{
  return self->src_id->owner_type_ == PortIdentifier::OwnerType::CHANNEL_SEND;
}

void
port_connection_print_to_str (
  const PortConnection * self,
  char *                 buf,
  size_t                 buf_sz)
{
  bool         is_send = port_connection_is_send (self);
  const char * send_str = is_send ? " (send)" : "";
  if (
    gZrythm && PROJECT
    && port_connections_manager_contains_connection (PORT_CONNECTIONS_MGR, self))
    {
      Track * src_track = tracklist_find_track_by_name_hash (
        TRACKLIST, self->src_id->track_name_hash_);
      Track * dest_track = tracklist_find_track_by_name_hash (
        TRACKLIST, self->dest_id->track_name_hash_);
      snprintf (
        buf, buf_sz, "[%s (%u)] %s => [%s (%u)] %s%s",
        src_track ? src_track->name : "(none)", self->src_id->track_name_hash_,
        self->src_id->get_label_as_c_str (),
        dest_track ? dest_track->name : "(none)",
        self->dest_id->track_name_hash_, self->dest_id->get_label_as_c_str (),
        send_str);
    }
  else
    {
      snprintf (
        buf, buf_sz, "[track %u] %s => [track %u] %s%s",
        self->src_id->track_name_hash_, self->src_id->get_label_as_c_str (),
        self->dest_id->track_name_hash_, self->dest_id->get_label_as_c_str (),
        send_str);
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
NONNULL PortConnection *
port_connection_clone (const PortConnection * src)
{
  PortConnection * self = port_connection_new (
    src->src_id, src->dest_id, src->multiplier, src->locked, src->enabled);
  return self;
}

/**
 * Deletes port, doing required cleanup and updating counters.
 */
NONNULL void
port_connection_free (PortConnection * self)
{
  object_delete_and_null (self->src_id);
  object_delete_and_null (self->dest_id);
  object_zero_and_free (self);
}
