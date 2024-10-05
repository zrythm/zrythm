// SPDX-FileCopyrightText: © 2018-2019, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "common/dsp/folder_track.h"

FolderTrack::FolderTrack (const std::string &name, int pos)
    : Track (Track::Type::Folder, name, pos, PortType::Unknown, PortType::Unknown)
{
  color_ = Color ("#865E3C");
  icon_name_ = "fluentui-folder-regular";
}

bool
FolderTrack::initialize ()
{
  init_channel ();
  generate_automation_tracks ();

  return true;
}

bool
FolderTrack::validate () const
{
  return Track::validate_base () && ChannelTrack::validate_base ()
         && AutomatableTrack::validate_base ();
}