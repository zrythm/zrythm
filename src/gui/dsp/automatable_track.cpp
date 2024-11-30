// SPDX-FileCopyrightText : © 2024 Alexandros Theodotou<alex @zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/zrythm.h"
#include "gui/dsp/automatable_track.h"
#include "gui/dsp/automation_track.h"
#include "gui/dsp/automation_tracklist.h"
#include "gui/dsp/channel_track.h"
#include "gui/dsp/modulator_track.h"
#include "gui/dsp/tempo_track.h"

#include "utils/gtest_wrapper.h"
#include "utils/rt_thread_id.h"

AutomatableTrack::AutomatableTrack ()
    : automation_tracklist_ (new AutomationTracklist ())
{
  automation_tracklist_->track_ = this;
}

void
AutomatableTrack::copy_members_from (const AutomatableTrack &other)
{
  automation_tracklist_ = other.automation_tracklist_->clone_raw_ptr ();
  if (auto * qobject = dynamic_cast<QObject *> (this))
    {
      automation_tracklist_->setParent (qobject);
    }
  automation_tracklist_->track_ = this;
  automation_visible_ = other.automation_visible_;
}

void
AutomatableTrack::init_loaded ()
{
  automation_tracklist_->init_loaded (this);

  std::vector<Port *> ports;
  append_ports (ports, true);
  unsigned int name_hash = get_name_hash ();
  for (auto &port : ports)
    {
      port->track_ = this;
      if (is_in_active_project ())
        {
          z_return_if_fail (port->id_->track_name_hash_ == name_hash);

          /* set automation tracks on ports */
          if (
            ENUM_BITSET_TEST (
              dsp::PortIdentifier::Flags, port->id_->flags_,
              dsp::PortIdentifier::Flags::Automatable))
            {
              auto *            ctrl = dynamic_cast<ControlPort *> (port);
              AutomationTrack * at =
                AutomationTrack::find_from_port (*ctrl, this, true);
              z_return_if_fail (at);
              ctrl->at_ = at;
            }
        }
    }
}

void
AutomatableTrack::generate_automation_tracks ()
{
  auto &atl = automation_tracklist_;
  auto  create_and_add_at = [&] (ControlPort &port) -> AutomationTrack  &{
    auto * at = atl->add_at (*new AutomationTrack (port));
    return *at;
  };

  if (has_channel ())
    {
      auto * track = dynamic_cast<ChannelTrack *> (this);
      auto  &ch = track->channel_;

      /* -- fader -- */

      /* volume */
      AutomationTrack &at = create_and_add_at (*ch->fader_->amp_);
      at.created_ = true;
      atl->set_at_visible (at, true);

      /* balance */
      create_and_add_at (*ch->fader_->balance_);

      /* mute */
      create_and_add_at (*ch->fader_->mute_);

      /* --- end fader --- */

      /* sends */
      for (auto &send : ch->sends_)
        {
          create_and_add_at (*send->amount_);
        }
    }

  if (has_piano_roll ())
    {
      auto &track = dynamic_cast<ProcessableTrack &> (*this);
      auto &processor = track.processor_;
      /* midi automatables */
      for (int i = 0; i < 16; i++)
        {
          for (int j = 0; j < 128; j++)
            {
              create_and_add_at (*processor->midi_cc_[i * 128 + j]);
            }

          create_and_add_at (*processor->pitch_bend_[i]);
          create_and_add_at (*processor->poly_key_pressure_[i]);
          create_and_add_at (*processor->channel_pressure_[i]);
        }
    }

  switch (type_)
    {
    case Track::Type::Tempo:
      {
        auto &track = dynamic_cast<TempoTrack &> (*this);
        /* create special BPM and time sig automation tracks for tempo track */
        auto &at = create_and_add_at (*track.bpm_port_);
        at.created_ = true;
        atl->set_at_visible (at, true);
        create_and_add_at (*track.beats_per_bar_port_);
        create_and_add_at (*track.beat_unit_port_);
        break;
      }
    case Track::Type::Modulator:
      {
        auto &track = dynamic_cast<ModulatorTrack &> (*this);
        for (auto &macro : track.modulator_macro_processors_)
          {
            auto &at = create_and_add_at (*macro->macro_);
            if (
              macro.get () == track.modulator_macro_processors_.front ().get ())
              {
                at.created_ = true;
                atl->set_at_visible (at, true);
              }
          }
      }
      break;
    case Track::Type::Audio:
      {
        auto &track = dynamic_cast<ProcessableTrack &> (*this);
        create_and_add_at (*track.processor_->output_gain_);
        break;
      }
    default:
      break;
    }

  z_debug ("generated automation tracks for '{}'", name_);
}

void
AutomatableTrack::set_automation_visible (const bool visible)
{
  automation_visible_ = visible;

  if (visible)
    {
      /* if no visible automation tracks, set the first one visible */
      auto &atl = get_automation_tracklist ();
      ;
      int num_visible = atl.get_num_visible ();

      if (num_visible == 0)
        {
          auto * at = atl.get_first_invisible_at ();
          if (at != nullptr)
            {
              at->created_ = true;
              atl.set_at_visible (*at, true);
            }
          else
            {
              z_info ("no automation tracks found for {}", name_);
            }
        }
    }

  // EVENTS_PUSH (EventType::ET_TRACK_AUTOMATION_VISIBILITY_CHANGED, this);
}

bool
AutomatableTrack::validate_base () const
{
  if (ZRYTHM_TESTING)
    {
      /* verify port identifiers */
      std::vector<Port *> ports;
      append_ports (ports, true);
      unsigned int name_hash = get_name_hash ();
      for (const auto * port : ports)
        {
          z_return_val_if_fail (port->id_->track_name_hash_ == name_hash, false);
          if (port->id_->owner_type_ == dsp::PortIdentifier::OwnerType::Plugin)
            {
              const auto &pid = port->id_->plugin_id_;
              z_return_val_if_fail (pid.track_name_hash_ == name_hash, false);
              zrythm::gui::old_dsp::plugins::Plugin * pl =
                zrythm::gui::old_dsp::plugins::Plugin::find (pid);
              z_return_val_if_fail (pl->id_.validate (), false);
              z_return_val_if_fail (pl->id_ == pid, false);
              if (pid.slot_type_ == zrythm::dsp::PluginSlotType::Instrument)
                {
                  const auto * channel_track =
                    dynamic_cast<const ChannelTrack *> (this);
                  z_return_val_if_fail (
                    pl == channel_track->channel_->instrument_.get (), false);
                }
            }

          /* check that the automation track is there */
          if (
            ENUM_BITSET_TEST (
              dsp::PortIdentifier::Flags, port->id_->flags_,
              dsp::PortIdentifier::Flags::Automatable))
            {
              const auto * ctrl = dynamic_cast<const ControlPort *> (port);
              const auto * at =
                AutomationTrack::find_from_port (*ctrl, this, true);
              if (at == nullptr)
                {
                  z_error (
                    "Could not find automation track for port '{}'",
                    port->get_full_designation ());
                  return false;
                }
              z_return_val_if_fail (
                AutomationTrack::find_from_port (*ctrl, this, false), false);
              z_return_val_if_fail (ctrl->at_ == at, false);

              z_return_val_if_fail (at->verify (), false);
            }
        }
    }

  /* verify tracklist identifiers */
  z_return_val_if_fail (automation_tracklist_->validate (), false);

  return true;
}
