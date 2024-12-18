// SPDX-FileCopyrightText: © 2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_PORT_CONNECTIONS_MANAGER_H__
#define __AUDIO_PORT_CONNECTIONS_MANAGER_H__

#include "zrythm-config.h"

#include "gui/dsp/port_connection.h"

#include "utils/icloneable.h"
#include "utils/types.h"

class Port;

/**
 * @addtogroup dsp
 *
 * @{
 */

#define PORT_CONNECTIONS_MGR (PortConnectionsManager::get_active_instance ())

/**
 * Port connections manager.
 */
class PortConnectionsManager final
    : public QObject,
      public ICloneable<PortConnectionsManager>,
      public zrythm::utils::serialization::ISerializable<PortConnectionsManager>
{
  Q_OBJECT
  QML_ELEMENT

public:
  using PortIdentifier = zrythm::dsp::PortIdentifier;
  using ConnectionsVector = std::vector<PortConnection *>;

  explicit PortConnectionsManager (QObject * parent = nullptr);

private:
  /**
   * Hashtable to speedup lookups by port identifier.
   *
   * Key: Port identifier hash
   * Value: A vector of PortConnection references from @ref connections_.
   */
  using ConnectionHashTable = std::unordered_map<uint32_t, ConnectionsVector>;

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
    ConnectionsVector *   arr,
    const PortIdentifier &id,
    bool                  sources) const;

  int get_sources (ConnectionsVector * arr, const PortIdentifier &id) const
  {
    return get_sources_or_dests (arr, id, true);
  }

  int get_dests (ConnectionsVector * arr, const PortIdentifier &id) const
  {
    return get_sources_or_dests (arr, id, false);
  }

  static PortConnectionsManager * get_active_instance ();

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
    ConnectionsVector *   arr,
    const PortIdentifier &id,
    bool                  sources) const;

  /**
   * Wrapper over @ref get_sources_or_dests() that returns the first connection.
   *
   * It is a programming error to call this for ports that are not expected to
   * have exactly 1  matching connection.
   *
   * @return The connection, owned by this, or null.
   */
  PortConnection *
  get_source_or_dest (const PortIdentifier &id, bool sources) const;

  /**
   * @brief
   *
   * @param src
   * @param dest
   * @return The connection, owned by this, or null.
   */
  PortConnection *
  find_connection (const PortIdentifier &src, const PortIdentifier &dest) const;

  bool
  are_ports_connected (const PortIdentifier &src, const PortIdentifier &dest) const
  {
    return find_connection (src, dest) != nullptr;
  }

  /**
   * @brief Replaces the given @p before connection with @p after.
   *
   * @return Whether the connection was replaced.
   */
  bool
  replace_connection (const PortConnection &before, const PortConnection &after);

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

  /**
   * @brief Overload for default settings (multiplier = 1.0, enabled = true).
   */
  const PortConnection * ensure_connect_default (
    const PortIdentifier &src,
    const PortIdentifier &dest,
    bool                  locked)
  {
    return ensure_connect (src, dest, 1.0f, locked, true);
  }

  const PortConnection *
  ensure_connect_from_connection (const PortConnection &conn)
  {
    return ensure_connect (
      *conn.src_id_, *conn.dest_id_, conn.multiplier_, conn.locked_,
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
   * @brief Disconnects all the given ports
   *
   * @param ports Ports to delete all connections for.
   * @param deleting Whether to set the `deleting` flag on the ports.
   */
  void disconnect_port_collection (std::vector<Port *> &ports, bool deleting);

  /**
   * Removes all connections from this.
   *
   * @param src If non-nullptr, the connections are copied from this to this.
   */
  void reset_connections (const PortConnectionsManager * other);

  bool contains_connection (const PortConnection &conn) const;

  void print_ht (const ConnectionHashTable &ht);

  void print () const;

  void init_after_cloning (const PortConnectionsManager &other) override;

  DECLARE_DEFINE_FIELDS_METHOD ();

private:
  void add_or_replace_connection (
    ConnectionHashTable  &ht,
    const PortIdentifier &id,
    const PortConnection &conn);

  void remove_connection (size_t idx);

  void clear_connections () { connections_.clear (); }

public:
  /** Connections (owned pointers). */
  std::vector<PortConnection *> connections_;

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
