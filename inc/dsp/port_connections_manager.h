// SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Port connections manager.
 */

#ifndef __AUDIO_PORT_CONNECTIONS_MANAGER_H__
#define __AUDIO_PORT_CONNECTIONS_MANAGER_H__

#include "zrythm-config.h"

#include "dsp/port_connection.h"
#include "utils/types.h"
#include "utils/yaml.h"

#include <glib.h>

class Port;
struct PortIdentifier;
typedef struct PortConnection PortConnection;

/**
 * @addtogroup dsp
 *
 * @{
 */

#define PORT_CONNECTIONS_MANAGER_SCHEMA_VERSION 1

#define PORT_CONNECTIONS_MGR (PROJECT->port_connections_manager)

/**
 * Port connections manager.
 */
typedef struct PortConnectionsManager
{
  /** Connections. */
  PortConnection ** connections;
  int               num_connections;
  size_t            connections_size;

  /**
   * Hashtable to speedup lookup by source port identifier.
   *
   * Key: hash of source port identifier
   * Value: A pointer to a PortConnection from
   *   PortConnectionsManager.connections.
   */
  GHashTable * src_ht;

  /**
   * Hashtable to speedup lookup by destination port identifier.
   *
   * Key: hash of destination port identifier
   * Value: A pointer to a PortConnection from
   *   PortConnectionsManager.connections.
   */
  GHashTable * dest_ht;
} PortConnectionsManager;

NONNULL void
port_connections_manager_init_loaded (PortConnectionsManager * self);

PortConnectionsManager *
port_connections_manager_new (void);

/**
 * Regenerates the hash tables.
 *
 * Must be called when a change is made in the
 * connections.
 */
void
port_connections_manager_regenerate_hashtables (PortConnectionsManager * self);

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
int port_connections_manager_get_sources_or_dests (
  const PortConnectionsManager * self,
  GPtrArray *                    arr,
  const PortIdentifier *         id,
  bool                           sources);

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
  bool                           sources);

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
  bool                           sources);

PortConnection *
port_connections_manager_find_connection (
  const PortConnectionsManager * self,
  const PortIdentifier *         src,
  const PortIdentifier *         dest);

/**
 * Returns whether the given connection is for the
 * given send.
 */
bool
port_connections_manager_predicate_is_send_of (
  const void * obj,
  const void * user_data);

/**
 * Adds the connections matching the given predicate
 * to the given array (if given).
 *
 * @param arr Optional array to fill.
 *
 * @return The number of connections found.
 */
int
port_connections_manager_find (
  const PortConnectionsManager * self,
  GPtrArray *                    arr,
  GenericPredicateFunc           predicate);

/**
 * Stores the connection for the given ports if
 * it doesn't exist, otherwise updates the existing
 * connection.
 *
 * @return The connection.
 */
const PortConnection *
port_connections_manager_ensure_connect (
  PortConnectionsManager * self,
  const PortIdentifier *   src,
  const PortIdentifier *   dest,
  float                    multiplier,
  bool                     locked,
  bool                     enabled);

#define port_connections_manager_ensure_connect_from_connection(self, conn) \
  port_connections_manager_ensure_connect ( \
    self, conn->src_id, conn->dest_id, conn->multiplier, conn->locked, \
    conn->enabled)

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
  const PortIdentifier *   dest);

/**
 * Disconnect all sources and dests of the given
 * port identifier.
 */
void
port_connections_manager_ensure_disconnect_all (
  PortConnectionsManager * self,
  const PortIdentifier *   pi);

/**
 * Removes all connections from @ref self.
 *
 * @param src If non-NULL, the connections are copied
 *   from this to @ref self.
 */
void
port_connections_manager_reset (
  PortConnectionsManager *       self,
  const PortConnectionsManager * src);

bool
port_connections_manager_contains_connection (
  const PortConnectionsManager * self,
  const PortConnection * const   conn);

void
port_connections_manager_print_ht (GHashTable * ht);

void
port_connections_manager_print (const PortConnectionsManager * self);

/**
 * To be used during serialization.
 */
NONNULL PortConnectionsManager *
port_connections_manager_clone (const PortConnectionsManager * src);

/**
 * Deletes port, doing required cleanup and updating counters.
 */
NONNULL void
port_connections_manager_free (PortConnectionsManager * self);

/**
 * @}
 */

#endif /* __AUDIO_PORT_CONNECTIONS_MANAGER_H__ */
