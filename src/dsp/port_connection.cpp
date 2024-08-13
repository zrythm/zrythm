// SPDX-FileCopyrightText: © 2021-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/port_connection.h"
#include "dsp/port_identifier.h"
#include "dsp/tracklist.h"
#include "project.h"
#include "utils/logger.h"
#include "zrythm.h"

#include <fmt/format.h>

bool
PortConnection::is_send () const
{
  return src_id_.owner_type_ == PortIdentifier::OwnerType::ChannelSend;
}

std::string
PortConnection::print_to_str () const
{
  bool         is_send = this->is_send ();
  const char * send_str = is_send ? " (send)" : "";
  if (gZrythm && PROJECT && PORT_CONNECTIONS_MGR->contains_connection (*this))
    {
      Track * src_track =
        TRACKLIST->find_track_by_name_hash (src_id_.track_name_hash_);
      Track * dest_track =
        TRACKLIST->find_track_by_name_hash (dest_id_.track_name_hash_);
      return fmt::format (
        "[%s ({})] %s => [%s ({})] %s%s",
        src_track ? src_track->name_ : "(none)", src_id_.track_name_hash_,
        src_id_.get_label (), dest_track ? dest_track->name_ : "(none)",
        dest_id_.track_name_hash_, dest_id_.get_label (), send_str);
    }
  else
    {
      return fmt::format (
        "[track {}] %s => [track {}] %s%s", src_id_.track_name_hash_,
        src_id_.get_label (), dest_id_.track_name_hash_, dest_id_.get_label (),
        send_str);
    }
}

void
PortConnection::print () const
{
  z_debug ("{}", print_to_str ());
}
