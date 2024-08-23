// SPDX-FileCopyrightText: © 2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_PORT_CONNECTIONS_MANAGER_H__
#define __AUDIO_PORT_CONNECTIONS_MANAGER_H__

#include "zrythm-config.h"

#include "dsp/port_connection.h"
#include "utils/icloneable.h"
#include "utils/types.h"

#include <glib.h>

class Port;
class PortIdentifier;
class PortConnection;

/**
 * @addtogroup dsp
 *
 * @{
 */

#define PORT_CONNECTIONS_MANAGER_SCHEMA_VERSION 1

#define PORT_CONNECTIONS_MGR (PROJECT->port_connections_manager_)

/**
 * Port connections manager.
 */
class PortConnectionsManager final
    : public ICloneable<PortConnectionsManager>,
      public ISerializable<PortConnectionsManager>
{
  using ConnectionHashTableValueType = std::vector<PortConnection *>;
  /**
   * Hashtable to speedup lookups by port identifier.
   *
   * Key: Port identifier
   * Value: A vector of PortConnection references from @ref connections_.
   */
  using ConnectionHashTable = std::
    map<PortIdentifier, ConnectionHashTableValueType, PortIdentifier::Compare>;

public:
  /**
   * Regenerates the hash tables.
   *
   * Must be called when a change is made in the connections.
   */
  void regenerate_hashtables ();

  /**
   * Adds the sources/destinations of @ref id in the given array.
   *
   * The returned instances of PortConnection are owned by this and must
   * not be free'd.
   *
   * @param id The identifier of the port to look for.
   * @param arr Optional array to fill.
   * @param sources True to look for sources, false for destinations.
   *
   * @return The number of ports found.
   */
  int get_sources_or_dests (
    std::vector<PortConnection *> * arr,
    const PortIdentifier           &id,
    bool                            sources) const;

  /**
   * Adds the sources/destinations of @ref id in the given array.
   *
   * The returned instances of PortConnection are owned by this and must
   * not be free'd.
   *
   * @param id The identifier of the port to look for.
   * @param arr Optional array to fill.
   * @param sources True to look for sources, false for destinations.
   *
   * @return The number of ports found.
   */
  int get_unlocked_sources_or_dests (
    std::vector<PortConnection *> * arr,
    const PortIdentifier           &id,
    bool                            sources) const;

  /**
   * Wrapper over @ref get_sources_or_dests() that returns the first connection.
   *
   * It is a programming error to call this for ports that are not expected to
   * have exactly 1  matching connection.
   */
  PortConnection *
  get_source_or_dest (const PortIdentifier &id, bool sources) const;

  PortConnection *
  find_connection (const PortIdentifier &src, const PortIdentifier &dest) const;

  /**
   * Adds the connections matching the given predicate to the given array (if
   * given).
   *
   * @param arr Optional array to fill.
   *
   * @return The number of connections found.
   */
  int find (std::vector<PortConnection *> * arr, GenericPredicateFunc predicate)
    const;

  /**
   * Stores the connection for the given ports if it doesn't exist, otherwise
   * updates the existing connection.
   *
   * @return The connection.
   */
  const PortConnection * ensure_connect (
    const PortIdentifier &src,
    const PortIdentifier &dest,
    float                 multiplier,
    bool                  locked,
    bool                  enabled);

  const PortConnection *
  ensure_connect_from_connection (const PortConnection &conn)
  {
    return ensure_connect (
      conn.src_id_, conn.dest_id_, conn.multiplier_, conn.locked_,
      conn.enabled_);
  }

  /**
   * Removes the connection for the given ports if it exists.
   *
   * @return Whether a connection was removed.
   */
  bool
  ensure_disconnect (const PortIdentifier &src, const PortIdentifier &dest);

  /**
   * Disconnect all sources and dests of the given port identifier.
   */
  void ensure_disconnect_all (const PortIdentifier &pi);

  /**
   * Removes all connections from this.
   *
   * @param src If non-nullptr, the connections are copied from this to this.
   */
  void reset (const PortConnectionsManager * other);

  bool contains_connection (const PortConnection &conn) const;

  static void print_ht (const ConnectionHashTable &ht);

  void print () const;

  void init_after_cloning (const PortConnectionsManager &other) override;

  DECLARE_DEFINE_FIELDS_METHOD ();

private:
  void add_or_replace_connection (
    ConnectionHashTable  &ht,
    const PortIdentifier &id,
    PortConnection       &conn);

  void remove_connection (const size_t idx);

  void clear_connections () { connections_.clear (); }

public:
  /** Connections. */
  std::vector<PortConnection> connections_;

  /**
   * Hashtable to speedup lookup by source port identifier.
   *
   * Key: Port identifier
   * Value: A reference to a PortConnection from @ref connections_.
   */
  ConnectionHashTable src_ht_;

  /**
   * Hashtable to speedup lookup by destination port identifier.
   *
   * Key: Destination port identifier
   * Value: A reference to a PortConnection from @ref connections_.
   */
  ConnectionHashTable dest_ht_;
};

/**
 * @}
 */

#endif /* __AUDIO_PORT_CONNECTIONS_MANAGER_H__ */
