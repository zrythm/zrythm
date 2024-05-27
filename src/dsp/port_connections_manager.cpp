// SPDX-FileCopyrightText: © 2021-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/channel_send.h"
#include "dsp/port_connections_manager.h"
#include "dsp/port_identifier.h"
#include "dsp/track.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/objects.h"
#include "zrythm_app.h"

static void
free_connections (void * data)
{
  GPtrArray * arr = (GPtrArray *) data;
  g_ptr_array_unref (arr);
}

static void
add_or_replace_connection (
  GHashTable *           ht,
  const PortIdentifier * pi,
  PortConnection *       conn)
{
  /* prepare the connections */
  GPtrArray * connections = (GPtrArray *) g_hash_table_lookup (ht, pi);
  bool        replacing = false;
  if (connections)
    {
      replacing = true;
      GPtrArray * prev_connections = connections;
      connections = g_ptr_array_new ();
      g_ptr_array_extend (connections, prev_connections, NULL, NULL);
      /* the old array will be freed automatically
       * by the hash table when replacing */
    }
  else
    {
      connections = g_ptr_array_new ();
    }
  g_ptr_array_add (connections, conn);

  if (replacing)
    {
      PortIdentifier * pi_clone = new PortIdentifier (*pi);
      g_hash_table_replace (ht, pi_clone, connections);
    }
  else
    {
      PortIdentifier * pi_clone = new PortIdentifier (*pi);
      g_hash_table_insert (ht, pi_clone, connections);
    }
}

/**
 * Regenerates the hash tables.
 *
 * Must be called when a change is made in the
 * connections.
 */
void
port_connections_manager_regenerate_hashtables (PortConnectionsManager * self)
{
#if 0
  g_debug (
    "regenerating hashtables for port connections "
    "manager...");
#endif

  object_free_w_func_and_null (g_hash_table_destroy, self->src_ht);
  object_free_w_func_and_null (g_hash_table_destroy, self->dest_ht);

  /* FIXME the hashes returned are 32 bits but they should allow the minimum
   * size of 16 bits for guint used in the hash func. currently this doesn't
   * affect any major platform but it's not standards compliant */
  self->src_ht = g_hash_table_new_full (
    port_identifier_get_hash, port_identifier_is_equal_func,
    port_identifier_destroy_notify, free_connections);
  self->dest_ht = g_hash_table_new_full (
    port_identifier_get_hash, port_identifier_is_equal_func,
    port_identifier_destroy_notify, free_connections);

  for (int i = 0; i < self->num_connections; i++)
    {
      PortConnection * conn = self->connections[i];
      add_or_replace_connection (self->src_ht, conn->src_id, conn);
      add_or_replace_connection (self->dest_ht, conn->dest_id, conn);
    }

#if 0
  unsigned int srcs_size =
    g_hash_table_size (self->src_ht);
  unsigned int dests_size =
    g_hash_table_size (self->dest_ht);
  g_debug (
    "Sources hashtable: %u elements | "
    "Destinations hashtable: %u elements",
    srcs_size, dests_size);
#endif
}

void
port_connections_manager_init_loaded (PortConnectionsManager * self)
{
  self->connections_size = (size_t) self->num_connections;
  port_connections_manager_regenerate_hashtables (self);
}

PortConnectionsManager *
port_connections_manager_new (void)
{
  PortConnectionsManager * self = object_new (PortConnectionsManager);

  self->connections_size = 64;
  self->connections = object_new_n (self->connections_size, PortConnection *);

  port_connections_manager_regenerate_hashtables (self);

  return self;
}

/**
 * Returns whether the given connection is for the
 * given send.
 */
bool
port_connections_manager_predicate_is_send_of (
  const void * obj,
  const void * user_data)
{
  const PortConnection * conn = (const PortConnection *) obj;
  const ChannelSend *    send = (const ChannelSend *) user_data;

  Track * track = channel_send_get_track (send);
  g_return_val_if_fail (IS_TRACK_AND_NONNULL (track), false);

  if (track->out_signal_type == ZPortType::Z_PORT_TYPE_AUDIO)
    {
      StereoPorts * self_stereo =
        channel_send_is_prefader (send)
          ? track->channel->prefader->stereo_out
          : track->channel->fader->stereo_out;

      return conn->src_id->is_equal (self_stereo->l->id)
             || conn->src_id->is_equal (self_stereo->r->id);
    }
  else if (track->out_signal_type == ZPortType::Z_PORT_TYPE_EVENT)
    {
      Port * self_port =
        channel_send_is_prefader (send)
          ? track->channel->prefader->midi_out
          : track->channel->fader->midi_out;

      return conn->src_id->is_equal (self_port->id);
    }

  g_return_val_if_reached (false);
}

/**
 * Adds the sources/destinations of @ref id in the
 * given array.
 *
 * @param id The identifier of the port to look for.
 * @param arr Optional array to fill.
 * @param sources True to look for sources, false for
 *   destinations.
 *
 * @return The number of ports found.
 */
int
port_connections_manager_get_sources_or_dests (
  const PortConnectionsManager * self,
  GPtrArray *                    arr,
  const PortIdentifier *         id,
  bool                           sources)
{
  g_return_val_if_fail (self->dest_ht && self->src_ht, 0);
  g_return_val_if_fail (ZRYTHM_APP_IS_GTK_THREAD, 0);
  /* note: we look at the opposite hashtable */
  GHashTable * ht = sources ? self->dest_ht : self->src_ht;
  GPtrArray *  res = (GPtrArray *) g_hash_table_lookup (ht, id);

  if (!res)
    {
      return 0;
    }

  /* append to the given array */
  if (arr)
    {
      g_ptr_array_extend (arr, res, NULL, NULL);
    }

  /* return number of connections found */
  return (int) res->len;
}

/**
 * Adds the sources/destinations of @ref id in the
 * given array.
 *
 * The returned instances of PortConnection are owned
 * by @ref self and must not be free'd.
 *
 * @param id The identifier of the port to look for.
 * @param arr Optional array to fill.
 * @param sources True to look for sources, false for
 *   destinations.
 *
 * @return The number of ports found.
 */
NONNULL_ARGS (1, 3)
int port_connections_manager_get_unlocked_sources_or_dests (
  const PortConnectionsManager * self,
  GPtrArray *                    arr,
  const PortIdentifier *         id,
  bool                           sources)
{
  g_return_val_if_fail (self->dest_ht && self->src_ht, 0);
  g_return_val_if_fail (ZRYTHM_APP_IS_GTK_THREAD, 0);
  GPtrArray * res = (GPtrArray *) g_hash_table_lookup (
    /* note: we look at the opposite hashtable */
    (sources ? self->dest_ht : self->src_ht), id);

  if (!res)
    return 0;

  int ret = 0;
  for (size_t i = 0; i < res->len; i++)
    {
      PortConnection * conn = (PortConnection *) g_ptr_array_index (res, i);
      if (!conn->locked)
        ret++;

      /* append to the given array */
      if (arr)
        {
          g_ptr_array_add (arr, conn);
        }
    }

  /* return number of connections found */
  return (int) ret;
}

/**
 * Wrapper over
 * port_connections_manager_get_sources_or_dests()
 * that returns the first connection.
 *
 * It is a programming error to call this for ports
 * that are not expected to have exactly 1  matching
 * connection.
 */
PortConnection *
port_connections_manager_get_source_or_dest (
  const PortConnectionsManager * self,
  const PortIdentifier *         id,
  bool                           sources)
{
  GPtrArray * conns = g_ptr_array_new ();
  int         num_conns =
    port_connections_manager_get_sources_or_dests (self, conns, id, sources);
  if (num_conns != 1)
    {
      size_t sz = 2000;
      char   buf[sz];
      id->print_to_str (buf, sz);
      g_critical (
        "expected 1 %s, found %d "
        "connections for\n%s",
        sources ? "source" : "destination", num_conns, buf);
      return NULL;
    }

  PortConnection * conn = (PortConnection *) g_ptr_array_index (conns, 0);
  g_ptr_array_unref (conns);

  return conn;
}

PortConnection *
port_connections_manager_find_connection (
  const PortConnectionsManager * self,
  const PortIdentifier *         src,
  const PortIdentifier *         dest)
{
  for (int i = 0; i < self->num_connections; i++)
    {
      PortConnection * conn = self->connections[i];
      if (conn->src_id->is_equal (*src) && conn->dest_id->is_equal (*dest))
        {
          return conn;
        }
    }

  return NULL;
}

/**
 * Stores the connection for the given ports if
 * it doesn't exist, otherwise updates the existing
 * connection.
 *
 * @return Whether a new connection was made.
 */
const PortConnection *
port_connections_manager_ensure_connect (
  PortConnectionsManager * self,
  const PortIdentifier *   src,
  const PortIdentifier *   dest,
  float                    multiplier,
  bool                     locked,
  bool                     enabled)
{
  g_return_val_if_fail (ZRYTHM_APP_IS_GTK_THREAD, NULL);

  for (int i = 0; i < self->num_connections; i++)
    {
      PortConnection * conn = self->connections[i];
      if (conn->src_id->is_equal (*src) && conn->dest_id->is_equal (*dest))
        {
          port_connection_update (conn, multiplier, locked, enabled);
          port_connections_manager_regenerate_hashtables (self);
          return conn;
        }
    }

  array_double_size_if_full (
    self->connections, self->num_connections, self->connections_size,
    PortConnection *);
  PortConnection * conn =
    port_connection_new (src, dest, multiplier, locked, enabled);
  self->connections[self->num_connections++] = conn;

  if (self == PORT_CONNECTIONS_MGR)
    {
      size_t sz = 800;
      char   buf[sz];
      port_connection_print_to_str (conn, buf, sz);
      g_debug (
        "New connection: <%s>; "
        "have %d connections",
        buf, self->num_connections);
    }

  port_connections_manager_regenerate_hashtables (self);

  return conn;
}

static void
remove_connection (PortConnectionsManager * self, const int idx)
{
  PortConnection * conn = self->connections[idx];

  for (int i = idx; i < self->num_connections - 1; i++)
    {
      self->connections[i] = self->connections[i + 1];
    }
  self->num_connections--;

  if (self == PORT_CONNECTIONS_MGR)
    {
      size_t sz = 800;
      char   buf[sz];
      port_connection_print_to_str (conn, buf, sz);
      g_debug (
        "Disconnected <%s>; "
        "have %d connections",
        buf, self->num_connections);
    }

  port_connections_manager_regenerate_hashtables (self);

  object_free_w_func_and_null (port_connection_free, conn);
}

/**
 * Removes the connection for the given ports if
 * it exists.
 *
 * @return Whether a connection was removed.
 */
bool
port_connections_manager_ensure_disconnect (
  PortConnectionsManager * self,
  const PortIdentifier *   src,
  const PortIdentifier *   dest)
{
  g_return_val_if_fail (ZRYTHM_APP_IS_GTK_THREAD, NULL);

  for (int i = 0; i < self->num_connections; i++)
    {
      PortConnection * conn = self->connections[i];
      if (conn->src_id->is_equal (*src) && conn->dest_id->is_equal (*dest))
        {
          remove_connection (self, i);
          return true;
        }
    }

  return false;
}

/**
 * Disconnect all sources and dests of the given
 * port identifier.
 */
void
port_connections_manager_ensure_disconnect_all (
  PortConnectionsManager * self,
  const PortIdentifier *   pi)
{
  g_return_if_fail (ZRYTHM_APP_IS_GTK_THREAD);

  for (int i = 0; i < self->num_connections; i++)
    {
      PortConnection * conn = self->connections[i];
      if (conn->src_id->is_equal (*pi) || conn->dest_id->is_equal (*pi))
        {
          remove_connection (self, i);
        }
    }
}

bool
port_connections_manager_contains_connection (
  const PortConnectionsManager * self,
  const PortConnection * const   conn)
{
  for (int i = 0; i < self->num_connections; i++)
    {
      if (self->connections[i] == conn)
        return true;
    }
  return false;
}

static void
clear (PortConnectionsManager * self)
{
  for (int i = 0; i < self->num_connections; i++)
    {
      object_free_w_func_and_null (port_connection_free, self->connections[i]);
    }
  self->num_connections = 0;
}

/**
 * Removes all connections from @ref self.
 *
 * @param src If non-NULL, the connections are copied
 *   from this to @ref self.
 */
void
port_connections_manager_reset (
  PortConnectionsManager *       self,
  const PortConnectionsManager * src)
{
  clear (self);

  self->connections = (PortConnection **) g_realloc_n (
    self->connections, (size_t) src->num_connections, sizeof (PortConnection *));
  self->connections_size = (size_t) src->num_connections;
  for (int i = 0; i < src->num_connections; i++)
    {
      self->connections[i] = port_connection_clone (src->connections[i]);
    }
  self->num_connections = src->num_connections;

  port_connections_manager_regenerate_hashtables (self);
}

void
port_connections_manager_print_ht (GHashTable * ht)
{
  GHashTableIter iter;
  gpointer       key, value;
  g_hash_table_iter_init (&iter, ht);
  GString * gstr = g_string_new (NULL);
  size_t    sz = 800;
  char      buf[sz];
  while (g_hash_table_iter_next (&iter, &key, &value))
    {
      const PortIdentifier * pi = (const PortIdentifier *) key;
      g_string_append_printf (gstr, "%s:\n", pi->label);
      GPtrArray * conns = (GPtrArray *) value;
      for (size_t i = 0; i < conns->len; i++)
        {
          PortConnection * conn =
            (PortConnection *) g_ptr_array_index (conns, i);
          port_connection_print_to_str (conn, buf, sz);
          g_string_append_printf (gstr, "  %s\n", buf);
        }
    }
  char * str = g_string_free (gstr, false);
  g_message ("%s", str);
  g_free (str);
}

void
port_connections_manager_print (const PortConnectionsManager * self)
{
  GString * gstr = g_string_new (NULL);
  g_string_append_printf (gstr, "Port connections manager (%p):\n", self);
  for (int i = 0; i < self->num_connections; i++)
    {
      PortConnection * conn = self->connections[i];
      size_t           sz = 400;
      char             buf[sz];
      port_connection_print_to_str (conn, buf, sz);
      g_string_append_printf (gstr, "[%d] %s\n", i, buf);
    }
  char * str = g_string_free (gstr, false);
  g_message ("%s", str);
  g_free (str);
}

/**
 * To be used during serialization.
 */
PortConnectionsManager *
port_connections_manager_clone (const PortConnectionsManager * src)
{
  PortConnectionsManager * self = port_connections_manager_new ();

  self->connections = (PortConnection **) g_realloc_n (
    self->connections, (size_t) src->num_connections, sizeof (PortConnection *));
  self->connections_size = (size_t) src->num_connections;
  for (int i = 0; i < src->num_connections; i++)
    {
      self->connections[i] = port_connection_clone (src->connections[i]);
    }
  self->num_connections = src->num_connections;

  port_connections_manager_regenerate_hashtables (self);

  return self;
}

/**
 * Deletes port, doing required cleanup and updating counters.
 */
void
port_connections_manager_free (PortConnectionsManager * self)
{
  for (int i = 0; i < self->num_connections; i++)
    {
      object_free_w_func_and_null (port_connection_free, self->connections[i]);
    }

  object_free_w_func_and_null (g_hash_table_destroy, self->src_ht);
  object_free_w_func_and_null (g_hash_table_destroy, self->dest_ht);

  object_zero_and_free (self);
}
