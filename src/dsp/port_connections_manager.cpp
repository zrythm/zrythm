// SPDX-FileCopyrightText: © 2021-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/channel_send.h"
#include "dsp/port_connections_manager.h"
#include "dsp/port_identifier.h"
#include "project.h"
#include "utils/rt_thread_id.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <fmt/format.h>

void
PortConnectionsManager::add_or_replace_connection (
  ConnectionHashTable  &ht,
  const PortIdentifier &id,
  PortConnection       &conn)
{
  auto it = ht.find (id);
  if (it != ht.end ())
    {
      it->second.push_back (&conn);
    }
  else
    {
      ht.emplace (id, ConnectionHashTableValueType{ &conn });
    }
}

void
PortConnectionsManager::regenerate_hashtables ()
{
  src_ht_.clear ();
  dest_ht_.clear ();

  for (auto &conn : connections_)
    {
      add_or_replace_connection (src_ht_, conn.src_id_, conn);
      add_or_replace_connection (dest_ht_, conn.dest_id_, conn);
    }

#if 0
  unsigned int srcs_size = src_ht_.size ();
  unsigned int dests_size = dest_ht_.size ();
  z_debug (
    "Sources hashtable: %u elements | "
    "Destinations hashtable: %u elements",
    srcs_size, dests_size);
#endif
}
int
PortConnectionsManager::get_sources_or_dests (
  std::vector<PortConnection *> * arr,
  const PortIdentifier           &id,
  bool                            sources) const
{
  /* note: we look at the opposite hashtable */
  const ConnectionHashTable &ht = sources ? dest_ht_ : src_ht_;
  auto                       it = ht.find (id);

  if (it == ht.end ())
    {
      return 0;
    }

  /* append to the given array */
  if (arr)
    {
      for (auto &conn : it->second)
        {
          arr->push_back (conn);
        }
    }

  /* return number of connections found */
  return (int) it->second.size ();
}

int
PortConnectionsManager::get_unlocked_sources_or_dests (
  std::vector<PortConnection *> * arr,
  const PortIdentifier                 &id,
  bool                                  sources) const
{
  const ConnectionHashTable &ht = sources ? dest_ht_ : src_ht_;
  auto                       it = ht.find (id);

  if (it == ht.end ())
    return 0;

  int ret = 0;
  for (auto &conn : it->second)
    {
      if (!conn->locked_)
        ret++;

      /* append to the given array */
      if (arr)
        {
          arr->push_back (conn);
        }
    }

  /* return number of connections found */
  return (int) ret;
}

PortConnection *
PortConnectionsManager::get_source_or_dest (
  const PortIdentifier &id,
  bool                  sources) const
{
  std::vector<PortConnection *> conns;
  int num_conns = get_sources_or_dests (&conns, id, sources);
  if (num_conns != 1)
    {
      auto buf = id.print_to_str ();
      z_error (
        "expected 1 %s, found %d "
        "connections for\n%s",
        sources ? "source" : "destination", num_conns, buf);
      return nullptr;
    }

  return const_cast<PortConnection *> (conns[0]);
}
PortConnection *
PortConnectionsManager::find_connection (
  const PortIdentifier &src,
  const PortIdentifier &dest) const
{
  for (const auto &conn : connections_)
    {
      if (conn.src_id_ == src && conn.dest_id_ == dest)
        {
          return const_cast<PortConnection *> (&conn);
        }
    }

  return nullptr;
}

const PortConnection *
PortConnectionsManager::ensure_connect (
  const PortIdentifier &src,
  const PortIdentifier &dest,
  float                 multiplier,
  bool                  locked,
  bool                  enabled)
{
  z_return_val_if_fail (ZRYTHM_APP_IS_GTK_THREAD, nullptr);

  for (auto &conn : connections_)
    {
      if (conn.src_id_ == src && conn.dest_id_ == dest)
        {
          conn.update (multiplier, locked, enabled);
          regenerate_hashtables ();
          return &conn;
        }
    }

  connections_.emplace_back (src, dest, multiplier, locked, enabled);
  const auto &conn = connections_.back ();

  if (this == PORT_CONNECTIONS_MGR.get())
    {
      z_debug (
        "New connection: <%s>; "
        "have %zu connections",
        conn.print_to_str ().c_str (), connections_.size ());
    }

  regenerate_hashtables ();

  return &conn;
}

void
PortConnectionsManager::remove_connection (const size_t idx)
{
  const auto &conn = connections_[idx];

  connections_.erase (connections_.begin () + idx);

  if (this == PORT_CONNECTIONS_MGR.get())
    {
      z_debug (
        "Disconnected <%s>; "
        "have %zu connections",
        conn.print_to_str ().c_str (), connections_.size ());
    }

  regenerate_hashtables ();
}

bool
PortConnectionsManager::ensure_disconnect (
  const PortIdentifier &src,
  const PortIdentifier &dest)
{
  z_return_val_if_fail (ZRYTHM_APP_IS_GTK_THREAD, false);

  for (size_t i = 0; i < connections_.size (); i++)
    {
      const auto &conn = connections_[i];
      if (conn.src_id_ == src && conn.dest_id_ == dest)
        {
          remove_connection (i);
          return true;
        }
    }

  return false;
}

void
PortConnectionsManager::ensure_disconnect_all (const PortIdentifier &pi)
{
  z_return_if_fail (ZRYTHM_APP_IS_GTK_THREAD);

  for (size_t i = 0; i < connections_.size (); i++)
    {
      const auto &conn = connections_[i];
      if (conn.src_id_ == pi || conn.dest_id_ == pi)
        {
          remove_connection (i);
          i--;
        }
    }
}
bool
PortConnectionsManager::contains_connection (const PortConnection &conn) const
{
  return std::find (connections_.begin (), connections_.end (), conn)
         != connections_.end ();
}

void
PortConnectionsManager::reset (const PortConnectionsManager * other)
{
  clear_connections ();

  if (other)
    {
      connections_ = other->connections_;
      regenerate_hashtables ();
    }
}

void
PortConnectionsManager::print_ht (ConnectionHashTable &ht)
{
  std::string str;
  for (const auto &[key, value] : ht)
    {
      str += fmt::format ("{}\n", key.get_label ());
      for (const auto &conn : value)
        {
          str += fmt::format ("  {}\n", conn->print_to_str ());
        }
    }
  z_info ("%s", str.c_str ());
}

void
PortConnectionsManager::print () const
{
  std::string str =
    fmt::format ("Port connections manager ({}):\n", (void *) this);
  for (size_t i = 0; i < connections_.size (); i++)
    {
      str += fmt::format ("[{}] {}\n", i, connections_[i].print_to_str ());
    }
  z_info ("%s", str.c_str ());
}