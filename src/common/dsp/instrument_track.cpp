// SPDX-FileCopyrightText: © 2018-2019, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "common/dsp/instrument_track.h"
#include "common/dsp/position.h"
#include "common/dsp/region.h"
#include "common/dsp/track.h"
#include "common/utils/rt_thread_id.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"

InstrumentTrack::InstrumentTrack (const std::string &name, int pos)
    : Track (Track::Type::Instrument, name, pos, PortType::Event, PortType::Audio)
{
  color_ = Color ("#FF9616");
  icon_name_ = _ ("instrument");
}

bool
InstrumentTrack::initialize ()
{
  init_channel ();
  generate_automation_tracks ();

  return true;
}

bool
InstrumentTrack::validate () const
{
  return Track::validate_base () && ChannelTrack::validate_base ()
         && AutomatableTrack::validate_base () && LanedTrackImpl::validate_base ()
         && GroupTargetTrack::validate_base ();
}

zrythm::plugins::Plugin *
InstrumentTrack::get_instrument ()
{
  auto &plugin = channel_->instrument_;
  z_return_val_if_fail (plugin, nullptr);
  return plugin.get ();
}

const zrythm::plugins::Plugin *
InstrumentTrack::get_instrument () const
{
  auto &plugin = channel_->instrument_;
  z_return_val_if_fail (plugin, nullptr);
  return plugin.get ();
}

bool
InstrumentTrack::is_plugin_visible () const
{
  const auto &plugin = get_instrument ();
  z_return_val_if_fail (plugin, false);
  return plugin->visible_;
}

void
InstrumentTrack::toggle_plugin_visible ()
{
  zrythm::plugins::Plugin * plugin = get_instrument ();
  z_return_if_fail (plugin);
  plugin->visible_ = !plugin->visible_;

  // EVENTS_PUSH (EventType::ET_PLUGIN_VISIBILITY_CHANGED, plugin);
}

void
InstrumentTrack::init_after_cloning (const InstrumentTrack &other)
{
  Track::copy_members_from (other);
  AutomatableTrack::copy_members_from (other);
  ProcessableTrack::copy_members_from (other);
  ChannelTrack::copy_members_from (other);
  GroupTargetTrack::copy_members_from (other);
  RecordableTrack::copy_members_from (other);
  LanedTrackImpl<MidiRegion>::copy_members_from (other);
  PianoRollTrack::copy_members_from (other);
}

void
InstrumentTrack::init_loaded ()
{
  // ChannelTrack must be initialized before AutomatableTrack
  ChannelTrack::init_loaded ();
  AutomatableTrack::init_loaded ();
  ProcessableTrack::init_loaded ();
  RecordableTrack::init_loaded ();
  LanedTrackImpl<MidiRegion>::init_loaded ();
  PianoRollTrack::init_loaded ();
}