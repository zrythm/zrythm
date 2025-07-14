// SPDX-FileCopyrightText: © 2021, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/port_connection.h"
#include "utils/icloneable.h"

namespace zrythm::dsp
{

/**
 * Port connections manager.
 */
class PortConnectionsManager final : public QObject
{
  Q_OBJECT
  QML_ELEMENT

public:
  using PortConnection = dsp::PortConnection;
  using ConnectionsVector = std::vector<PortConnection *>;

  explicit PortConnectionsManager (QObject * parent = nullptr);

private:
  /**
   * Hashtable to speedup lookups by port identifier.
   *
   * Key: PortUuid
   * Value: A vector of PortConnection references from @ref connections_.
   */
  using ConnectionHashTable = std::unordered_map<PortUuid, ConnectionsVector>;

  static const auto &connection_ref_predicate (const PortConnection * conn)
  {
    return *conn;
  }

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
  int
  get_sources_or_dests (ConnectionsVector * arr, const PortUuid &id, bool sources)
    const;

  int get_sources (ConnectionsVector * arr, const PortUuid &id) const
  {
    return get_sources_or_dests (arr, id, true);
  }

  int get_dests (ConnectionsVector * arr, const PortUuid &id) const
  {
    return get_sources_or_dests (arr, id, false);
  }

  int get_num_sources (const PortUuid &id) const
  {
    return get_sources (nullptr, id);
  }
  int get_num_dests (const PortUuid &id) const
  {
    return get_dests (nullptr, id);
  }

  auto get_connection_count () const noexcept { return connections_.size (); }

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
    ConnectionsVector * arr,
    const PortUuid     &id,
    bool                sources) const;

  /**
   * Wrapper over @ref get_sources_or_dests() that returns the first connection.
   *
   * It is a programming error to call this for ports that are not expected to
   * have exactly 1  matching connection.
   *
   * @return The connection, owned by this, or null.
   */
  PortConnection * get_source_or_dest (const PortUuid &id, bool sources) const;

  /**
   * @brief
   *
   * @param src
   * @param dest
   * @return The connection, owned by this, or null.
   */
  PortConnection *
  get_connection (const PortUuid &src, const PortUuid &dest) const;

  bool connection_exists (const PortUuid &src, const PortUuid &dest) const
  {
    return get_connection (src, dest) != nullptr;
  }

  /**
   * @brief Replaces the given @p before connection with @p after.
   *
   * @return Whether the connection was replaced.
   */
  bool
  update_connection (const PortConnection &before, const PortConnection &after);

  bool update_connection (
    const PortUuid &src,
    const PortUuid &dest,
    float           multiplier,
    bool            locked,
    bool            enabled);

  /**
   * Stores the connection for the given ports if it doesn't exist, otherwise
   * updates the existing connection.
   *
   * @note To be called from the main thread only.
   *
   * @return The connection.
   */
  const PortConnection * add_connection (
    const PortUuid &src,
    const PortUuid &dest,
    float           multiplier,
    bool            locked,
    bool            enabled);

  /**
   * @brief Overload for default settings (multiplier = 1.0, enabled = true).
   */
  const PortConnection *
  add_default_connection (const PortUuid &src, const PortUuid &dest, bool locked)
  {
    return add_connection (src, dest, 1.0f, locked, true);
  }

  const PortConnection * add_connection (const PortConnection &conn)
  {
    return add_connection (
      conn.src_id_, conn.dest_id_, conn.multiplier_, conn.locked_,
      conn.enabled_);
  }

  /**
   * Removes the connection for the given ports if it exists.
   *
   * @note To be called from the main thread only.
   *
   * @return Whether a connection was removed.
   */
  bool remove_connection (const PortUuid &src, const PortUuid &dest);

  /**
   * Disconnect all sources and dests of the given port identifier.
   *
   * @note To be called from the main thread only.
   */
  void remove_all_connections (const PortUuid &pi);

  /**
   * Removes all connections from this.
   *
   * @param src If non-nullptr, the connections are copied from this to this.
   */
  void reset_connections_from_other (const PortConnectionsManager * other);

  bool contains_connection (const PortConnection &conn) const;

  void print_ht (const ConnectionHashTable &ht);

  void print () const;

  friend void init_from (
    PortConnectionsManager       &obj,
    const PortConnectionsManager &other,
    utils::ObjectCloneType        clone_type);

  void clear_all ()
  {
    connections_.clear ();
    regenerate_hashtables ();
  }

  auto &get_connections () const { return connections_; }

private:
  static constexpr auto kConnectionsKey = "connections"sv;
  friend void to_json (nlohmann::json &j, const PortConnectionsManager &pcm)
  {
    j[kConnectionsKey] = pcm.connections_;
  }
  friend void from_json (const nlohmann::json &j, PortConnectionsManager &pcm)
  {
    for (const auto &conn_json : j.at (kConnectionsKey))
      {
        auto * conn = new PortConnection (&pcm);
        from_json (conn_json, *conn);
        pcm.connections_.push_back (conn);
      }
    pcm.regenerate_hashtables ();
  }

  void add_or_replace_connection (
    ConnectionHashTable  &ht,
    const PortUuid       &id,
    const PortConnection &conn);

  void remove_connection (size_t idx);

private:
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

} // namespace zrythm::dsp
