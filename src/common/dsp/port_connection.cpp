// SPDX-FileCopyrightText: © 2021-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/port_connection.h"
#include "common/dsp/port_identifier.h"
#include "common/dsp/tracklist.h"
#include "common/utils/logger.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"

#include <fmt/format.h>

PortConnection::PortConnection (QObject * parent) : QObject (parent) { }

PortConnection::PortConnection (
  const PortIdentifier &src,
  const PortIdentifier &dest,
  float                 multiplier,
  bool                  locked,
  bool                  enabled,
  QObject *             parent)
    : QObject (parent), src_id_ (src.clone_raw_ptr ()),
      dest_id_ (dest.clone_raw_ptr ()), multiplier_ (multiplier),
      locked_ (locked), enabled_ (enabled)
{
  src_id_->setParent (this);
  dest_id_->setParent (this);
}

void
PortConnection::init_after_cloning (const PortConnection &other)
{
  src_id_ = other.src_id_->clone_raw_ptr ();
  dest_id_ = other.dest_id_->clone_raw_ptr ();
  multiplier_ = other.multiplier_;
  locked_ = other.locked_;
  enabled_ = other.enabled_;
  base_value_ = other.base_value_;
  src_id_->setParent (this);
  dest_id_->setParent (this);
}

bool
PortConnection::is_send () const
{
  return src_id_->owner_type_ == PortIdentifier::OwnerType::ChannelSend;
}

std::string
PortConnection::print_to_str () const
{
  bool         is_send = this->is_send ();
  const char * send_str = is_send ? " (send)" : "";
  if (
    gZrythm && PROJECT && PORT_CONNECTIONS_MGR
    && PORT_CONNECTIONS_MGR->contains_connection (*this))
    {
      Track * src_track =
        TRACKLIST->find_track_by_name_hash (src_id_->track_name_hash_);
      Track * dest_track =
        TRACKLIST->find_track_by_name_hash (dest_id_->track_name_hash_);
      return fmt::format (
        "[{} ({})] {} => [{} ({})] {}{}",
        (src_track != nullptr) ? src_track->name_ : "(none)",
        src_id_->track_name_hash_, src_id_->get_label (),
        dest_track ? dest_track->name_ : "(none)", dest_id_->track_name_hash_,
        dest_id_->get_label (), send_str);
    }
  else
    {
      return fmt::format (
        "[track {}] {} => [track {}] {}{}", src_id_->track_name_hash_,
        src_id_->get_label (), dest_id_->track_name_hash_,
        dest_id_->get_label (), send_str);
    }
}

void
PortConnection::print () const
{
  z_debug ("{}", print_to_str ());
}

bool
operator== (const PortConnection &lhs, const PortConnection &rhs)
{
  return lhs.src_id_ == rhs.src_id_ && lhs.dest_id_ == rhs.dest_id_
         && math_floats_equal (lhs.multiplier_, rhs.multiplier_)
         && lhs.locked_ == rhs.locked_ && lhs.enabled_ == rhs.enabled_
         && math_floats_equal (lhs.base_value_, rhs.base_value_);
}