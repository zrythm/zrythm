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

AutomatableTrack::AutomatableTrack (const DeserializationDependencyHolder &dh)
    : AutomatableTrack (dh.get<std::reference_wrapper<PortRegistry>> ().get (), false)
{
}

AutomatableTrack::AutomatableTrack (
  PortRegistry &port_registry,
  bool          new_identity)
{
  // initialized here because we use base class members
  automation_tracklist_ =
    new AutomationTracklist (port_registry_, object_registry_, *this);
}

void
AutomatableTrack::copy_members_from (
  const AutomatableTrack &other,
  ObjectCloneType         clone_type)
{
  automation_tracklist_ = other.automation_tracklist_->clone_raw_ptr (
    ObjectCloneType::Snapshot, tracklist_->port_registry_.value (),
    PROJECT->get_arranger_object_registry (), *this);
  if (auto * qobject = dynamic_cast<QObject *> (this))
    {
      automation_tracklist_->setParent (qobject);
    }
  automation_visible_ = other.automation_visible_;
}

void
AutomatableTrack::init_loaded (
  PluginRegistry &plugin_registry,
  PortRegistry   &port_registry)
{
  automation_tracklist_->init_loaded ();

  std::vector<Port *> ports;
  append_ports (ports, true);
  auto this_id = get_uuid ();
  for (auto &port : ports)
    {
      if (is_in_active_project ())
        {
          z_return_if_fail (port->id_->get_track_id ().value () == this_id);

          /* set automation tracks on ports */
          if (ENUM_BITSET_TEST (
                port->id_->flags_, dsp::PortIdentifier::Flags::Automatable))
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
  std::visit (
    [&] (auto &&self) {
      using TrackT = base_type<decltype (self)>;
      auto &atl = automation_tracklist_;
      auto  create_and_add_at = [&] (ControlPort &port) -> AutomationTrack  &{
        auto * at = atl->add_automation_track (*new AutomationTrack (
          port_registry_, object_registry_, [self] () { return self; },
          port.get_uuid ()));
        return *at;
      };

      if constexpr (std::derived_from<TrackT, ChannelTrack>)
        {
          auto &ch = self->channel_;

          /* -- fader -- */

          /* volume */
          AutomationTrack &at = create_and_add_at (ch->fader_->get_amp_port ());
          at.created_ = true;
          atl->set_at_visible (at, true);

          /* balance */
          create_and_add_at (ch->fader_->get_balance_port ());

          /* mute */
          create_and_add_at (ch->fader_->get_mute_port ());

          /* --- end fader --- */

          /* sends */
          for (auto &send : ch->sends_)
            {
              create_and_add_at (send->get_amount_port ());
            }
        }

      if constexpr (std::derived_from<TrackT, PianoRollTrack>)
        {
          auto &processor = self->processor_;
          /* midi automatables */
          for (const auto channel_index : std::views::iota (0, 16))
            {
              for (const auto cc_index : std::views::iota (0, 128))
                {
                  create_and_add_at (
                    processor->get_midi_cc_port (channel_index, cc_index));
                }

              create_and_add_at (processor->get_pitch_bend_port (channel_index));
              create_and_add_at (
                processor->get_poly_key_pressure_port (channel_index));
              create_and_add_at (
                processor->get_channel_pressure_port (channel_index));
            }
        }

      if constexpr (std::is_same_v<TrackT, TempoTrack>)
        {
          /* create special BPM and time sig automation tracks for tempo track
           */
          auto &at = create_and_add_at (self->get_bpm_port ());
          at.created_ = true;
          atl->set_at_visible (at, true);
          create_and_add_at (self->get_beats_per_bar_port ());
          create_and_add_at (self->get_beat_unit_port ());
        }
      else if constexpr (std::is_same_v<TrackT, ModulatorTrack>)
        {
          const auto processors = self->get_modulator_macro_processors ();
          for (const auto &macro : processors)
            {
              auto &at = create_and_add_at (macro->get_macro_port ());
              if (macro.get () == processors.front ().get ())
                {
                  at.created_ = true;
                  atl->set_at_visible (at, true);
                }
            }
        }
      else if constexpr (std::is_same_v<TrackT, AudioTrack>)
        {
          create_and_add_at (self->processor_->get_output_gain_port ());
        }

      z_debug ("generated automation tracks for '{}'", name_);
    },
    convert_to_variant<AutomatableTrackPtrVariant> (this));
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
      auto this_id = get_uuid ();
      for (const auto * port : ports)
        {
          z_return_val_if_fail (port->id_->get_track_id () == this_id, false);
          if (port->id_->owner_type_ == dsp::PortIdentifier::OwnerType::Plugin)
            {
              const auto &pid_opt = port->id_->plugin_id_;
              z_return_val_if_fail (pid_opt.has_value (), false);
              const auto &pid = pid_opt.value ();
              auto        pl_var = PROJECT->find_plugin_by_id (pid);
              z_return_val_if_fail (pl_var.has_value (), false);
              auto pl_valid = std::visit (
                [&] (auto &&pl) {
                  z_return_val_if_fail (pl->id_.track_id_ == this_id, false);
                  z_return_val_if_fail (pl->id_.validate (), false);
                  if (
                    !pl->id_.slot_.has_slot_index ()
                    && pl->id_.slot_.get_slot_type_only ()
                         == dsp::PluginSlotType::Instrument)
                    {
                      const auto * channel_track =
                        dynamic_cast<const ChannelTrack *> (this);
                      z_return_val_if_fail (
                        pl->get_uuid ()
                          == channel_track->channel_->instrument_.value (),
                        false);
                    }
                  return true;
                },
                pl_var.value ());
              z_return_val_if_fail (pl_valid, false);
            }

          /* check that the automation track is there */
          if (ENUM_BITSET_TEST (
                port->id_->flags_, dsp::PortIdentifier::Flags::Automatable))
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
