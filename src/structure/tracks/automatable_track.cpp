// SPDX-FileCopyrightText : © 2024 Alexandros Theodotou<alex @zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/zrythm.h"
#include "structure/tracks/automatable_track.h"
#include "structure/tracks/automation_track.h"
#include "structure/tracks/automation_tracklist.h"
#include "structure/tracks/channel_track.h"
#include "structure/tracks/modulator_track.h"
#include "utils/gtest_wrapper.h"
#include "utils/rt_thread_id.h"

namespace zrythm::structure::tracks
{
AutomatableTrack::AutomatableTrack (
  dsp::FileAudioSourceRegistry    &file_audio_source_registry,
  dsp::PortRegistry               &port_registry,
  dsp::ProcessorParameterRegistry &param_registry,
  bool                             new_identity)
    : file_audio_source_registry_ (file_audio_source_registry)
{
  // initialized here because we use base class members
  automation_tracklist_ = utils::make_qobject_unique<AutomationTracklist> (
    file_audio_source_registry_, port_registry_, param_registry,
    object_registry_, *this);
}

void
init_from (
  AutomatableTrack       &obj,
  const AutomatableTrack &other,
  utils::ObjectCloneType  clone_type)
{
  obj.automation_tracklist_ = utils::clone_qobject (
    *other.automation_tracklist_, dynamic_cast<QObject *> (&obj),
    utils::ObjectCloneType::Snapshot, obj.file_audio_source_registry_,
    obj.port_registry_, obj.param_registry_,
    PROJECT->get_arranger_object_registry (), obj);
  obj.automation_visible_ = other.automation_visible_;
}

void
AutomatableTrack::init_loaded (
  PluginRegistry                  &plugin_registry,
  dsp::PortRegistry               &port_registry,
  dsp::ProcessorParameterRegistry &param_registry)
{
  automation_tracklist_->init_loaded ();
}

void
AutomatableTrack::generate_automation_tracks ()
{
  std::visit (
    [&] (auto &&self) {
      using TrackT = base_type<decltype (self)>;
      auto &atl = automation_tracklist_;
      auto  create_and_add_at =
        [&] (dsp::ProcessorParameter &port) -> AutomationTrack & {
        auto * at = atl->add_automation_track (*new AutomationTrack (
          file_audio_source_registry_, object_registry_,
          [self] () { return self; },
          dsp::ProcessorParameterUuidReference (
            port.get_uuid (), param_registry_)));
        return *at;
      };

      if constexpr (std::derived_from<TrackT, ChannelTrack>)
        {
          auto &ch = self->channel_;

          /* -- fader -- */

          /* volume */
          AutomationTrack &at = create_and_add_at (ch->fader_->get_amp_param ());
          at.created_ = true;
          atl->set_at_visible (at, true);

          /* balance */
          create_and_add_at (ch->fader_->get_balance_param ());

          /* mute */
          create_and_add_at (ch->fader_->get_mute_param ());

          /* --- end fader --- */

          /* sends */
          for (auto &send : ch->sends_)
            {
              create_and_add_at (send->get_amount_param ());
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
                    processor->get_midi_cc_param (channel_index, cc_index));
                }

              create_and_add_at (
                processor->get_pitch_bend_param (channel_index));
              create_and_add_at (
                processor->get_poly_key_pressure_param (channel_index));
              create_and_add_at (
                processor->get_channel_pressure_param (channel_index));
            }
        }

      if constexpr (std::is_same_v<TrackT, ModulatorTrack>)
        {
          const auto processors = self->get_modulator_macro_processors ();
          for (const auto &macro : processors)
            {
              auto &at = create_and_add_at (macro->get_macro_param ());
              if (macro.get () == processors.front ().get ())
                {
                  at.created_ = true;
                  atl->set_at_visible (at, true);
                }
            }
        }
      else if constexpr (std::is_same_v<TrackT, AudioTrack>)
        {
          create_and_add_at (self->processor_->get_output_gain_param ());
        }

      z_debug ("generated automation tracks for '{}'", name_);
    },
    convert_to_variant<AutomatableTrackPtrVariant> (this));
}

void
AutomatableTrack::generate_automation_tracks_for_plugin (
  const Plugin::Uuid &plugin_id)
{
  auto pl_var = get_plugin_registry ().find_by_id_or_throw (plugin_id);
  std::visit (
    [&] (auto &&plugin) {
      z_debug ("generating automation tracks for {}...", plugin->get_name ());

      auto &atl = get_automation_tracklist ();
      for (auto port_var : plugin->params_)
        {
          const auto port =
            port_var.template get_object_as<dsp::ProcessorParameter> ();
          if (!port->automatable ())
            continue;

          auto * at = new AutomationTrack (
            file_audio_source_registry_, get_object_registry (),
            [&] () { return convert_to_variant<TrackPtrVariant> (this); },
            { port->get_uuid (), param_registry_ });
          atl.add_automation_track (*at);
        }
    },
    pl_var);
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

}
