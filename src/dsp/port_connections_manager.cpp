// SPDX-FileCopyrightText: © 2021-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>

#include "dsp/port_connections_manager.h"

#include <fmt/format.h>

namespace zrythm::dsp
{

PortConnectionsManager::PortConnectionsManager (QObject * parent)
    : QObject (parent)
{
}

void
PortConnectionsManager::init_after_cloning (
  const PortConnectionsManager &other,
  ObjectCloneType               clone_type)
{
  connections_.reserve (other.connections_.size ());
  for (const auto &conn : other.connections_)
    {
      connections_.push_back (conn->clone_raw_ptr ());
      connections_.back ()->setParent (this);
    }
  regenerate_hashtables ();
}

#if 0
void
PortConnectionsManager::disconnect_port_collection (
  std::vector<Port *> &ports,
  bool                 deleting)
{
  /* can only be called from the gtk thread */
  z_return_if_fail (ZRYTHM_IS_QT_THREAD);

  /* go through each port */
  for (auto * port : ports)
    {
      remove_all_connections (port->get_uuid ());
      port->srcs_.clear ();
      port->dests_.clear ();
      port->deleting_ = deleting;
    }
}
#endif

void
PortConnectionsManager::add_or_replace_connection (
  ConnectionHashTable  &ht,
  const PortUuid       &id,
  const PortConnection &conn)
{
  auto   it = ht.find (id);
  auto * clone_conn = conn.clone_raw_ptr ();
  clone_conn->setParent (this);
  if (it != ht.end ())
    {
      z_return_if_fail (it->first == id);
      it->second.push_back (clone_conn);
    }
  else
    {
      ht.emplace (id, ConnectionsVector{ clone_conn });
    }
}

bool
PortConnectionsManager::update_connection (
  const PortConnection &before,
  const PortConnection &after)
{
  auto it = std::ranges::find (connections_, before, connection_ref_predicate);
  if (it == connections_.end ())
    {
      z_return_val_if_reached (false);
    }

  (*it)->setParent (nullptr);
  (*it)->deleteLater ();
  *it = after.clone_qobject (this);
  regenerate_hashtables ();

  return true;
}

bool
PortConnectionsManager::update_connection (
  const PortUuid &src,
  const PortUuid &dest,
  float           multiplier,
  bool            locked,
  bool            enabled)
{
  auto * conn = get_connection (src, dest);
  if (conn == nullptr)
    {
      return false;
    }
  conn->update (multiplier, locked, enabled);
  return true;
}

void
PortConnectionsManager::regenerate_hashtables ()
{
  src_ht_.clear ();
  dest_ht_.clear ();

  for (auto &conn : connections_)
    {
      add_or_replace_connection (src_ht_, conn->src_id_, *conn);
      add_or_replace_connection (dest_ht_, conn->dest_id_, *conn);
    }

#if 0
  unsigned int srcs_size = src_ht_.size ();
  unsigned int dests_size = dest_ht_.size ();
  z_debug (
    "Sources hashtable: {} elements | "
    "Destinations hashtable: {} elements",
    srcs_size, dests_size);
#endif
}

#if 0
void
PortConnectionsManager::update_connections (
  const dsp::PortIdentifier &prev_id,
  const dsp::PortIdentifier &new_id)
{
  /* update in all sources */
  PortConnectionsManager::ConnectionsVector srcs;
  get_sources_or_dests (&srcs, prev_id, true);
  for (const auto &conn : srcs)
    {
      if (*conn->dest_id_ != new_id)
        {
          auto new_conn = conn->clone_unique ();
          new_conn->dest_id_ = new_id.clone_unique ();
          replace_connection (conn, *new_conn);
        }
    }

  /* update in all dests */
  PortConnectionsManager::ConnectionsVector dests;
  get_sources_or_dests (&dests, prev_id, false);
  for (const auto &conn : dests)
    {
      if (conn->src_id_ != new_id)
        {
          auto new_conn = conn->clone_unique ();
          new_conn->src_id_ = new_id.clone_unique ();
          replace_connection (conn, *new_conn);
        }
    }
}
#endif

int
PortConnectionsManager::get_sources_or_dests (
  ConnectionsVector * arr,
  const PortUuid     &id,
  bool                sources) const
{
  /* note: we look at the opposite hashtable */
  const ConnectionHashTable &ht = sources ? dest_ht_ : src_ht_;
  auto                       it = ht.find (id);

  if (it == ht.end ())
    {
      return 0;
    }

  /* append to the given array */
  if (arr != nullptr)
    {
      for (const auto &conn : it->second)
        {
          arr->push_back (conn);
        }
    }

  /* return number of connections found */
  return (int) it->second.size ();
}

int
PortConnectionsManager::get_unlocked_sources_or_dests (
  ConnectionsVector * arr,
  const PortUuid     &id,
  bool                sources) const
{
  const ConnectionHashTable &ht = sources ? dest_ht_ : src_ht_;
  auto                       it = ht.find (id);

  if (it == ht.end ())
    return 0;

  int ret = 0;
  for (const auto &conn : it->second)
    {
      if (!conn->locked_)
        ret++;

      /* append to the given array */
      if (arr != nullptr)
        {
          arr->push_back (conn);
        }
    }

  /* return number of connections found */
  return ret;
}

PortConnection *
PortConnectionsManager::get_source_or_dest (const PortUuid &id, bool sources) const
{
  ConnectionsVector conns;
  int               num_conns = get_sources_or_dests (&conns, id, sources);
  if (num_conns != 1)
    {
      z_error (
        "expected 1 {}, found {} connections for {}",
        sources ? "source" : "destination", num_conns, id);
      return nullptr;
    }

  return conns.front ();
}

PortConnection *
PortConnectionsManager::get_connection (
  const PortUuid &src,
  const PortUuid &dest) const
{
  auto it = std::ranges::find_if (connections_, [&] (const auto &conn) {
    return conn->src_id_ == src && conn->dest_id_ == dest;
  });
  return it != connections_.end () ? (*it) : nullptr;
}

const PortConnection *
PortConnectionsManager::add_connection (
  const PortUuid &src,
  const PortUuid &dest,
  float           multiplier,
  bool            locked,
  bool            enabled)
{
  for (auto &conn : connections_)
    {
      if (conn->src_id_ == src && conn->dest_id_ == dest)
        {
          conn->update (multiplier, locked, enabled);
          regenerate_hashtables ();
          return conn;
        }
    }

  connections_.push_back (
    new PortConnection (src, dest, multiplier, locked, enabled, this));
  const auto &conn = connections_.back ();

  z_debug (
    "New connection: <{}>; have {} connections", *conn, connections_.size ());

  regenerate_hashtables ();

  return conn;
}

void
PortConnectionsManager::remove_connection (const size_t idx)
{
  auto * const conn = connections_[idx];

  connections_.erase (std::next (connections_.begin (), (int) idx));

  z_debug (
    "Disconnected <{}>; have {} connections", *conn, connections_.size ());

  regenerate_hashtables ();
}

bool
PortConnectionsManager::remove_connection (
  const PortUuid &src,
  const PortUuid &dest)
{
  for (size_t i = 0; i < connections_.size (); i++)
    {
      const auto &conn = connections_[i];
      if (conn->src_id_ == src && conn->dest_id_ == dest)
        {
          remove_connection (i);
          return true;
        }
    }

  return false;
}

void
PortConnectionsManager::remove_all_connections (const PortUuid &pi)
{
  for (size_t i = 0; i < connections_.size (); i++)
    {
      const auto &conn = connections_[i];
      if (conn->src_id_ == pi || conn->dest_id_ == pi)
        {
          remove_connection (i);
          i--;
        }
    }
}
bool
PortConnectionsManager::contains_connection (const PortConnection &conn) const
{
  return std::ranges::contains (connections_, conn, connection_ref_predicate);
}

void
PortConnectionsManager::reset_connections_from_other (
  const PortConnectionsManager * other)
{
  clear_all ();

  if (other)
    {
      connections_ = other->connections_;
      regenerate_hashtables ();
    }
}

void
PortConnectionsManager::print_ht (const ConnectionHashTable &ht)
{
  std::string str;
  z_trace ("ht size: {}", ht.size ());
  for (const auto &[key, value] : ht)
    {
      for (const auto &conn : value)
        {
          const auto &id = &ht == &src_ht_ ? conn->dest_id_ : conn->src_id_;
          str += fmt::format ("{}\n  {}\n", id, *conn);
        }
    }
  z_info ("{}", str.c_str ());
}

void
PortConnectionsManager::print () const
{
  std::string str =
    fmt::format ("Port connections manager ({}):\n", (void *) this);
  for (size_t i = 0; i < connections_.size (); i++)
    {
      str += fmt::format ("[{}] {}\n", i, *connections_[i]);
    }
  z_info ("{}", str.c_str ());
}

} // namespace zrythm::dsp
