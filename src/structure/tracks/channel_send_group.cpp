// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/channel_send_group.h"
#include "utils/logger.h"

namespace zrythm::structure::tracks
{

ChannelSendGroup::ChannelSendGroup (
  utils::IObjectRegistry &registry,
  dsp::PortType           signal_type,
  QObject *               parent)
    : QAbstractListModel (parent), registry_ (registry), signal_type_ (signal_type)
{
}

ChannelSendGroup::~ChannelSendGroup () = default;

// ============================================================================
// QML Interface
// ============================================================================

QHash<int, QByteArray>
ChannelSendGroup::roleNames () const
{
  static const QHash<int, QByteArray> roles = [] {
    QHash<int, QByteArray> r;
    r[static_cast<int> (Roles::SendRole)] = "send";
    return r;
  }();
  return roles;
}

int
ChannelSendGroup::rowCount (const QModelIndex &parent) const
{
  if (parent.isValid ())
    return 0;
  return static_cast<int> (sends_.size ());
}

QVariant
ChannelSendGroup::data (const QModelIndex &index, int role) const
{
  const auto row = index.row ();
  if (!index.isValid () || row >= static_cast<int> (sends_.size ()))
    return {};

  if (role == static_cast<int> (Roles::SendRole))
    {
      return QVariant::fromValue (sends_.at (row).get ());
    }

  return {};
}

// ============================================================================

ChannelSendUuidReference
ChannelSendGroup::append_send (bool is_prefader)
{
  auto send = utils::create_object<ChannelSend> (
    registry_, registry_, signal_type_, is_prefader);

  beginInsertRows ({}, rowCount (), rowCount ());
  sends_.emplace_back (send);
  endInsertRows ();

  return send;
}

ChannelSendUuidReference
ChannelSendGroup::insert_send (const ChannelSendUuidReference &send, int index)
{
  index = std::clamp (index, -1, rowCount ());

  if (index == -1)
    {
      beginInsertRows ({}, rowCount (), rowCount ());
      sends_.emplace_back (send);
    }
  else
    {
      beginInsertRows ({}, index, index);
      sends_.insert (std::ranges::next (sends_.begin (), index), send);
    }
  endInsertRows ();

  return send;
}

void
ChannelSendGroup::remove_send (const ChannelSend::Uuid &id)
{
  auto it = std::ranges::find_if (sends_, [&id] (const auto &send) {
    return send.id () == id;
  });
  if (it == sends_.end ())
    {
      throw std::invalid_argument ("Channel send UUID not found in group");
    }

  const auto index = std::ranges::distance (sends_.begin (), it);

  beginRemoveRows ({}, static_cast<int> (index), static_cast<int> (index));
  sends_.erase (it);
  endRemoveRows ();
}

ChannelSend *
ChannelSendGroup::send_at_index (int index) const
{
  if (index < 0 || index >= static_cast<int> (sends_.size ()))
    return nullptr;
  return sends_.at (index).get ();
}

void
to_json (nlohmann::json &j, const ChannelSendGroup &g)
{
  j[ChannelSendGroup::kSendsKey] = g.sends_;
}

void
from_json (const nlohmann::json &j, ChannelSendGroup &g)
{
  g.beginResetModel ();
  g.sends_.clear ();
  for (const auto &send_json : j.at (ChannelSendGroup::kSendsKey))
    {
      auto send_ref = ChannelSendUuidReference{ g.registry_ };
      from_json (send_json, send_ref);
      g.sends_.emplace_back (std::move (send_ref));
    }
  g.endResetModel ();
}

} // namespace zrythm::structure::tracks
