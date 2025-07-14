// SPDX-FileCopyrightText: © 2019-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/audio_port.h"
#include "dsp/midi_event.h"
#include "dsp/panning.h"
#include "dsp/port.h"
#include "engine/device_io/engine.h"
#include "engine/session/control_room.h"
#include "gui/backend/backend/actions/tracklist_selections_action.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings_manager.h"
#include "gui/backend/backend/zrythm.h"
#include "structure/tracks/channel.h"
#include "structure/tracks/fader.h"
#include "structure/tracks/group_target_track.h"
#include "structure/tracks/master_track.h"
#include "structure/tracks/track.h"
#include "structure/tracks/tracklist.h"
#include "utils/debug.h"
#include "utils/dsp.h"
#include "utils/gtest_wrapper.h"
#include "utils/math.h"
#include "utils/midi.h"
#include "utils/rt_thread_id.h"

namespace zrythm::structure::tracks
{

Fader::Fader (
  dsp::PortRegistry                 &port_registry,
  dsp::ProcessorParameterRegistry   &param_registry,
  Type                               type,
  Track *                            track,
  engine::session::ControlRoom *     control_room,
  engine::session::SampleProcessor * sample_processor,
  QObject *                          parent)
    : QObject (parent),
      dsp::ProcessorBase (port_registry, param_registry, u8"Fader"),
      port_registry_ (port_registry), param_registry_ (param_registry),
      type_ (type), midi_mode_ (MidiFaderMode::MIDI_FADER_MODE_VEL_MULTIPLIER),
      track_ (track), control_room_ (control_room),
      sample_processor_ (sample_processor)
{
  const auto should_be_automatable =
    (type == Type::AudioChannel || type == Type::MidiChannel);
  {
    /* set volume */
    amp_id_ = param_registry.create_object<dsp::ProcessorParameter> (
      port_registry, dsp::ProcessorParameter::UniqueId (u8"fader_volume"),
      dsp::ParameterRange (
        dsp::ParameterRange::Type::GainAmplitude, 0.f, 2.f, 0.f, 1.f),
      utils::Utf8String::from_qstring (QObject::tr ("Fader Volume")));
    get_amp_param ().set_automatable (should_be_automatable);

    /* set pan */
    balance_id_ = param_registry.create_object<dsp::ProcessorParameter> (
      port_registry, dsp::ProcessorParameter::UniqueId (u8"fader_balance"),
      dsp::ParameterRange (
        dsp::ParameterRange::Type::Linear, 0.f, 1.f, 0.5f, 0.5f),
      utils::Utf8String::from_qstring (QObject::tr ("Fader Balance")));
    get_balance_param ().set_automatable (should_be_automatable);
  }

  {
    /* set mute */
    mute_id_ = param_registry.create_object<dsp::ProcessorParameter> (
      port_registry, dsp::ProcessorParameter::UniqueId (u8"fader_mute"),
      dsp::ParameterRange (dsp::ParameterRange::Type::Toggle, 0.f, 1.f, 0.f, 0.f),
      utils::Utf8String::from_qstring (QObject::tr ("Fader Mute")));
    get_mute_param ().set_automatable (should_be_automatable);
  }

  {
    /* set solo */
    solo_id_ = param_registry.create_object<dsp::ProcessorParameter> (
      port_registry, dsp::ProcessorParameter::UniqueId (u8"fader_solo"),
      dsp::ParameterRange (dsp::ParameterRange::Type::Toggle, 0.f, 1.f, 0.f, 0.f),
      utils::Utf8String::from_qstring (QObject::tr ("Fader Solo")));
    get_solo_param ().set_automatable (false);
  }

  {
    /* set listen */
    listen_id_ = param_registry.create_object<dsp::ProcessorParameter> (
      port_registry, dsp::ProcessorParameter::UniqueId (u8"fader_listen"),
      dsp::ParameterRange (dsp::ParameterRange::Type::Toggle, 0.f, 1.f, 0.f, 0.f),
      utils::Utf8String::from_qstring (QObject::tr ("Fader Listen")));
    get_listen_param ().set_automatable (false);
  }

  {
    /* set mono compat */
    mono_compat_enabled_id_ = param_registry.create_object<
      dsp::ProcessorParameter> (
      port_registry,
      dsp::ProcessorParameter::UniqueId (u8"fader_mono_compat_enabled"),
      dsp::ParameterRange (dsp::ParameterRange::Type::Toggle, 0.f, 1.f, 0.f, 0.f),
      utils::Utf8String::from_qstring (QObject::tr ("Fader Mono Compat")));
    get_mono_compat_enabled_param ().set_automatable (false);
  }

  {
    /* set swap phase */
    swap_phase_id_ = param_registry.create_object<dsp::ProcessorParameter> (
      port_registry, dsp::ProcessorParameter::UniqueId (u8"fader_swap_phase"),
      dsp::ParameterRange (dsp::ParameterRange::Type::Toggle, 0.f, 1.f, 0.f, 0.f),
      utils::Utf8String::from_qstring (QObject::tr ("Fader Swap Phase")));
    get_swap_phase_param ().set_automatable (false);
  }

  if (has_audio_ports ())
    {
      {
        utils::Utf8String name;
        utils::Utf8String sym;
        if (type == Type::AudioChannel)
          {
            name = utils::Utf8String::from_qstring (QObject::tr ("Ch Fader in"));
            sym = u8"ch_fader_in";
          }
        else if (type == Type::SampleProcessor)
          {
            name = utils::Utf8String::from_qstring (
              QObject::tr ("Sample Processor Fader in"));
            sym = u8"sample_processor_fader_in";
          }
        else
          {
            name = utils::Utf8String::from_qstring (
              QObject::tr ("Monitor Fader in"));
            sym = u8"monitor_fader_in";
          }

        /* stereo in */
        auto stereo_in_ports = dsp::StereoPorts::create_stereo_ports (
          port_registry_.value (), true, name, sym);
        add_input_port (stereo_in_ports.first);
        add_input_port (stereo_in_ports.second);
        auto &left_port = get_stereo_in_ports ().first;
        auto &right_port = get_stereo_in_ports ().second;
        left_port.set_full_designation_provider (this);
        right_port.set_full_designation_provider (this);
      }

      {
        utils::Utf8String name;
        utils::Utf8String sym;
        if (type == Type::AudioChannel)
          {
            name =
              utils::Utf8String::from_qstring (QObject::tr ("Ch Fader out"));
            sym = u8"ch_fader_out";
          }
        else if (type == Type::SampleProcessor)
          {
            name = utils::Utf8String::from_qstring (
              QObject::tr ("Sample Processor Fader out"));
            sym = u8"sample_processor_fader_out";
          }
        else
          {
            name = utils::Utf8String::from_qstring (
              QObject::tr ("Monitor Fader out"));
            sym = u8"monitor_fader_out";
          }

        {
          /* stereo out */
          auto stereo_out_ports = dsp::StereoPorts::create_stereo_ports (
            port_registry_.value (), false, name, sym);
          add_output_port (stereo_out_ports.first);
          add_output_port (stereo_out_ports.second);
          auto &left_port = get_stereo_out_ports ().first;
          auto &right_port = get_stereo_out_ports ().second;
          left_port.set_full_designation_provider (this);
          right_port.set_full_designation_provider (this);
        }
      }
    }

  if (has_midi_ports ())
    {
      {
        /* MIDI in */
        utils::Utf8String name;
        utils::Utf8String sym;
        name =
          utils::Utf8String::from_qstring (QObject::tr ("Ch MIDI Fader in"));
        sym = u8"ch_midi_fader_in";
        add_input_port (port_registry_->create_object<dsp::MidiPort> (
          name, dsp::PortFlow::Input));
        auto &midi_in_port = get_midi_in_port ();
        midi_in_port.set_full_designation_provider (this);
        midi_in_port.set_symbol (sym);
      }

      {
        utils::Utf8String name;
        utils::Utf8String sym;
        /* MIDI out */
        name =
          utils::Utf8String::from_qstring (QObject::tr ("Ch MIDI Fader out"));
        sym = u8"ch_midi_fader_out";
        add_output_port (port_registry_->create_object<dsp::MidiPort> (
          name, dsp::PortFlow::Output));
        auto &midi_out_port = get_midi_out_port ();
        midi_out_port.set_full_designation_provider (this);
        midi_out_port.set_symbol (sym);
      }
    }

  set_name ([&] () -> utils::Utf8String {
    if (type_ == Type::AudioChannel || type_ == Type::MidiChannel)
      {
        return utils::Utf8String::from_utf8_encoded_string (
          fmt::format ("{} {}", track->get_name (), "Fader"));
      }
    if (type_ == Type::Monitor)
      {
        return u8"Monitor Fader";
      }

    return u8"Fader";
  }());
}

void
Fader::init_loaded (
  dsp::PortRegistry                 &port_registry,
  dsp::ProcessorParameterRegistry   &param_registry,
  Track *                            track,
  engine::session::ControlRoom *     control_room,
  engine::session::SampleProcessor * sample_processor)
{
  port_registry_ = port_registry;
  param_registry_ = param_registry;
  track_ = track;
  control_room_ = control_room;
  sample_processor_ = sample_processor;

#if 0
  std::vector<dsp::Port *> ports;
  append_ports (ports);
  for (auto * port : ports)
    {
      port->set_full_designation_provider (this);
    }
#endif
}

utils::Utf8String
Fader::get_full_designation_for_port (const dsp::Port &port) const
{
  const auto port_label = port.get_label ();
  if (track_ != nullptr)
    {
      auto * tr = get_track ();
      return utils::Utf8String::from_utf8_encoded_string (
        fmt::format ("{}/{}", tr->get_name (), port_label));
    }
  return utils::Utf8String::from_utf8_encoded_string (
    fmt::format ("Engine/{}", port_label));
}

bool
Fader::get_implied_soloed () const
{
  /* only check channel faders */
  if (
    (type_ != Type::AudioChannel && type_ != Type::MidiChannel)
    || get_solo_param ().range ().is_toggled (get_solo_param ().currentValue ()))
    {
      return false;
    }

  auto track = get_track ();
  z_return_val_if_fail (track, false);

  /* check parents */
  auto out_track = track;
  do
    {
      auto soloed = std::visit (
        [&] (auto &&out_track_casted) -> bool {
          if constexpr (
            std::derived_from<
              base_type<decltype (out_track_casted)>, ChannelTrack>)
            {
              out_track = out_track_casted->channel_->get_output_track ();
              if (out_track && out_track->currently_soloed ())
                {
                  return true;
                }
            }
          else
            {
              out_track = nullptr;
            }
          return false;
        },
        convert_to_variant<TrackPtrVariant> (out_track));
      if (soloed)
        return true;
    }
  while (out_track != nullptr);

  /* check children */
  if (track->can_be_group_target ())
    {
      auto * group_target = dynamic_cast<GroupTargetTrack *> (track);
      for (const auto &child_id : group_target->children_)
        {
          auto child_track_var = TRACKLIST->get_track (child_id);

          auto child_soloed_or_implied_soloed =
            child_track_var.has_value ()
            && std::visit (
              [&] (auto &&child_track) {
                return static_cast<bool> (
                  child_track->currently_soloed ()
                  || child_track->get_implied_soloed ());
              },
              child_track_var.value ());
          if (child_soloed_or_implied_soloed)
            {
              return true;
            }
        }
    }

  return false;
}

std::string
Fader::db_string_getter () const
{
  return fmt::format (
    "{:.1f}",
    utils::math::amp_to_dbfs (
      get_amp_param ().range ().convert_from_0_to_1 (
        get_amp_param ().currentValue ())));
}

void
Fader::set_amp_with_action (float amp_from, float amp_to, bool skip_if_equal)
{
  auto track = get_track ();
  bool is_equal = utils::math::floats_near (amp_from, amp_to, 0.0001f);
  if (!skip_if_equal || !is_equal)
    {
      try
        {
          UNDO_MANAGER->perform (new gui::actions::SingleTrackFloatAction (
            gui::actions::TracklistSelectionsAction::EditType::Volume,
            convert_to_variant<TrackPtrVariant> (track), amp_from, amp_to,
            false));
        }
      catch (const ZrythmException &e)
        {
          e.handle (QObject::tr ("Failed to change volume"));
        }
    }
}

void
Fader::set_midi_mode (MidiFaderMode mode, bool with_action, bool fire_events)
{
  if (with_action)
    {
      auto track = get_track ();
      z_return_if_fail (track);

      try
        {
          UNDO_MANAGER->perform (new gui::actions::SingleTrackIntAction (
            gui::actions::TracklistSelectionsAction::EditType::MidiFaderMode,
            convert_to_variant<TrackPtrVariant> (track),
            static_cast<int> (mode), false));
        }
      catch (const ZrythmException &e)
        {
          e.handle (QObject::tr ("Failed to set MIDI mode"));
        }
    }
  else
    {
      midi_mode_ = mode;
    }

  if (fire_events)
    {
      // TODO: Implement event firing
    }
}

// TODO: attach to signals when this changes for the monitor/mute/listen/dim
// faders and update user settings to remember the values
#if 0
void
Fader::set_fader_val (float fader_val)
{
  fader_val_ = fader_val;
  float fader_amp = utils::math::get_amp_val_from_fader (fader_val);
  fader_amp = get_amp_port ().range_.clamp_to_range (fader_amp);
  set_amp (fader_amp);
  volume_ = utils::math::amp_to_dbfs (fader_amp);

  if (this == MONITOR_FADER.get ())
    {
      gui::SettingsManager::get_instance ()->set_monitorVolume (fader_amp);
    }
  else if (this == CONTROL_ROOM->mute_fader_.get ())
    {
      gui::SettingsManager::get_instance ()->set_monitorMuteVolume (fader_amp);
    }
  else if (this == CONTROL_ROOM->listen_fader_.get ())
    {
      gui::SettingsManager::get_instance ()->set_monitorListenVolume (fader_amp);
    }
  else if (this == CONTROL_ROOM->dim_fader_.get ())
    {
      gui::SettingsManager::get_instance ()->set_monitorDimVolume (fader_amp);
    }
}
#endif

void
Fader::set_fader_val_with_action_from_db (const std::string &str)
{
  bool  is_valid = false;
  float val;
  if (utils::math::is_string_valid_float (str, &val))
    {
      if (val <= 6.f)
        {
          is_valid = true;
        }
    }

  if (is_valid)
    {
      set_amp_with_action (
        get_current_amp (), utils::math::dbfs_to_amp (val), true);
    }
  else
    {
      // ui_show_error_message (QObject::tr ("Invalid Value"), QObject::tr
      // ("Invalid value"));
    }
}

auto
Fader::get_channel () const -> Channel *
{
  auto track = dynamic_cast<ChannelTrack *> (get_track ());
  z_return_val_if_fail (track, nullptr);

  return track->get_channel ();
}

Track *
Fader::get_track () const
{
  z_return_val_if_fail (track_, nullptr);
  return track_;
}

void
Fader::clear_buffers (std::size_t block_length)
{
  if (has_audio_ports ())
    {
      auto stereo_in = get_stereo_in_ports ();
      stereo_in.first.clear_buffer (block_length);
      stereo_in.second.clear_buffer (block_length);
      auto stereo_out = get_stereo_out_ports ();
      stereo_out.first.clear_buffer (block_length);
      stereo_out.second.clear_buffer (block_length);
    }
  else if (has_midi_ports ())
    {
      auto &midi_in = get_midi_in_port ();
      midi_in.clear_buffer (block_length);
      auto &midi_out = get_midi_out_port ();
      midi_out.clear_buffer (block_length);
    }
}

int
Fader::fade_frames_for_type (Type type)
{
  return type == Type::Monitor
           ? FADER_DEFAULT_FADE_FRAMES
           : FADER_DEFAULT_FADE_FRAMES_SHORT;
}

/**
 * Process the Fader.
 */
void
Fader::custom_process_block (const EngineProcessTimeInfo time_nfo)
{
  Track * track = nullptr;
  if (type_ == Type::AudioChannel)
    {
      track = get_track ();
      z_return_if_fail (track);
    }

  const int default_fade_frames = fade_frames_for_type (type_);

  bool effectively_muted =
      /* muted if any of the following is true:
       * 1. muted
       * 2. other track(s) is soloed and this isn't
       * 3. bounce mode and the track is set to BOUNCE_OFF */
        currently_muted()
        ||
        ((type_ == Type::AudioChannel || type_ == Type::MidiChannel)
         && TRACKLIST->get_track_span().has_soloed() && !currently_soloed() && !get_implied_soloed()
         && track != P_MASTER_TRACK)
        ||
        (AUDIO_ENGINE->bounce_mode_ == engine::device_io::BounceMode::On
         &&
         (type_ == Type::AudioChannel
          || type_ == Type::MidiChannel)
         && (track != nullptr)
         && !track->is_master()
         && !track->bounce_);

#if 0
      if (ZRYTHM_TESTING && track &&
          (self->type == Fader::Type::FADER_TYPE_AUDIO_CHANNEL ||
           self->type == Fader::Type::FADER_TYPE_MIDI_CHANNEL))
        {
          z_debug ("{} soloed {} implied soloed {} effectively muted {}",
            track->name, fader_currently_soloed (self),
            fader_get_implied_soloed (self),
            effectively_muted);
        }
#endif

  const auto &amp_param = get_amp_param ();
  const float amp =
    amp_param.range ().convert_from_0_to_1 (amp_param.currentValue ());

  if (has_audio_ports ())
    {
      auto stereo_in = get_stereo_in_ports ();
      auto stereo_out = get_stereo_out_ports ();
      /* copy the input to output */
      utils::float_ranges::copy (
        &stereo_out.first.buf_[time_nfo.local_offset_],
        &stereo_in.first.buf_[time_nfo.local_offset_], time_nfo.nframes_);
      utils::float_ranges::copy (
        &stereo_out.second.buf_[time_nfo.local_offset_],
        &stereo_in.second.buf_[time_nfo.local_offset_], time_nfo.nframes_);

      /* if monitor */
      float mute_amp;
      if (type_ == Fader::Type::Monitor)
        {
          mute_amp = AUDIO_ENGINE->denormal_prevention_val_;
          float dim_amp = CONTROL_ROOM->dim_fader_->get_current_amp ();

          /* if have listened tracks */
          if (TRACKLIST->get_track_span ().has_listened ())
            {
              /* dim signal */
              utils::float_ranges::mul_k2 (
                &stereo_out.first.buf_[time_nfo.local_offset_], dim_amp,
                time_nfo.nframes_);
              utils::float_ranges::mul_k2 (
                &stereo_out.second.buf_[time_nfo.local_offset_], dim_amp,
                time_nfo.nframes_);

              /* add listened signal */
              /* TODO add "listen" buffer on fader struct and add listened
               * tracks to it during processing instead of looping here */
              float listen_amp = CONTROL_ROOM->listen_fader_->get_current_amp ();
              for (const auto &cur_t : TRACKLIST->get_track_span ())
                {
                  std::visit (
                    [&] (auto &&t) {
                      using TrackT = base_type<decltype (t)>;
                      if constexpr (std::derived_from<TrackT, ChannelTrack>)
                        {
                          if (
                            t->get_output_signal_type () == dsp::PortType::Audio
                            && t->currently_listened ())
                            {
                              auto * f = t->get_channel ()->fader ();
                              utils::float_ranges::mix_product (
                                &stereo_out.first.buf_[time_nfo.local_offset_],
                                &f->get_stereo_out_ports ()
                                   .first.buf_[time_nfo.local_offset_],
                                listen_amp, time_nfo.nframes_);
                              utils::float_ranges::mix_product (
                                &stereo_out.second.buf_[time_nfo.local_offset_],
                                &f->get_stereo_out_ports ()
                                   .second.buf_[time_nfo.local_offset_],
                                listen_amp, time_nfo.nframes_);
                            }
                        }
                    },
                    cur_t);
                }
            } /* endif have listened tracks */

          /* apply dim if enabled */
          if (CONTROL_ROOM->dim_output_)
            {
              utils::float_ranges::mul_k2 (
                &stereo_out.first.buf_[time_nfo.local_offset_], dim_amp,
                time_nfo.nframes_);
              utils::float_ranges::mul_k2 (
                &stereo_out.second.buf_[time_nfo.local_offset_], dim_amp,
                time_nfo.nframes_);
            }
        } /* endif monitor fader */
      else
        {
          mute_amp = CONTROL_ROOM->mute_fader_->get_current_amp ();

          /* add fade if changed from muted to non-muted or
           * vice versa */
          if (effectively_muted && !was_effectively_muted_)
            {
              fade_out_samples_.store (default_fade_frames);
              fading_out_.store (true);
            }
          else if (!effectively_muted && was_effectively_muted_)
            {

              fading_out_.store (false);
              fade_in_samples_.store (default_fade_frames);
            }
        }

      /* handle fade in */
      int fade_in_samples = fade_in_samples_.load ();
      if (fade_in_samples > 0) [[unlikely]]
        {
          z_return_if_fail_cmp (default_fade_frames, >=, fade_in_samples);
#if 0
              z_debug (
                "fading in %d samples", fade_in_samples);
#endif
          utils::float_ranges::linear_fade_in_from (
            &stereo_out.first.buf_[time_nfo.local_offset_],
            default_fade_frames - fade_in_samples, default_fade_frames,
            time_nfo.nframes_, mute_amp);
          utils::float_ranges::linear_fade_in_from (
            &stereo_out.second.buf_[time_nfo.local_offset_],
            default_fade_frames - fade_in_samples, default_fade_frames,
            time_nfo.nframes_, mute_amp);
          fade_in_samples -= (int) time_nfo.nframes_;
          fade_in_samples = std::max (fade_in_samples, 0);
          fade_in_samples_.store (fade_in_samples);
        }

      /* handle fade out */
      size_t faded_out_frames = 0;
      if (fading_out_.load ()) [[unlikely]]
        {
          int fade_out_samples = fade_out_samples_.load ();
          int samples_to_process =
            std::max (0, std::min (fade_out_samples, (int) time_nfo.nframes_));
          if (fade_out_samples > 0)
            {
              z_return_if_fail_cmp (default_fade_frames, >=, fade_out_samples);

#if 0
                  z_debug (
                    "fading out %d frames",
                    samples_to_process);
#endif
              utils::float_ranges::linear_fade_out_to (
                &stereo_out.first.buf_[time_nfo.local_offset_],
                default_fade_frames - fade_out_samples, default_fade_frames,
                (size_t) samples_to_process, mute_amp);
              utils::float_ranges::linear_fade_out_to (
                &stereo_out.second.buf_[time_nfo.local_offset_],
                default_fade_frames - fade_out_samples, default_fade_frames,
                (size_t) samples_to_process, mute_amp);
              fade_out_samples -= samples_to_process;
              faded_out_frames += (size_t) samples_to_process;
              fade_out_samples_.store (fade_out_samples);
            }

          /* if still fading out and have no more fade out samples, silence */
          if (fade_out_samples == 0)
            {
              size_t remaining_frames =
                time_nfo.nframes_ - (size_t) samples_to_process;
#if 0
                  z_debug (
                    "silence for remaining %zu frames", remaining_frames);
#endif
              if (remaining_frames > 0)
                {
                  utils::float_ranges::mul_k2 (
                    &stereo_out.first
                       .buf_[time_nfo.local_offset_ + faded_out_frames],
                    mute_amp, remaining_frames);
                  utils::float_ranges::mul_k2 (
                    &stereo_out.second
                       .buf_[time_nfo.local_offset_ + faded_out_frames],
                    mute_amp, remaining_frames);
                  faded_out_frames += remaining_frames;
                }
            }
        }

      const auto &balance_param = get_balance_param ();
      const auto &mono_compat_param = get_mono_compat_enabled_param ();
      const auto &swap_phase_param = get_swap_phase_param ();
      const float pan = balance_param.range ().convert_from_0_to_1 (
        balance_param.currentValue ());
      const bool mono_compat_enabled = mono_compat_param.range ().is_toggled (
        mono_compat_param.currentValue ());
      const bool swap_phase =
        swap_phase_param.range ().is_toggled (swap_phase_param.currentValue ());

      auto [calc_l, calc_r] = dsp::calculate_balance_control (
        dsp::BalanceControlAlgorithm::Linear, pan);

      /* apply fader and pan */
      utils::float_ranges::mul_k2 (
        &stereo_out.first.buf_[time_nfo.local_offset_], amp * calc_l,
        time_nfo.nframes_);
      utils::float_ranges::mul_k2 (
        &stereo_out.second.buf_[time_nfo.local_offset_], amp * calc_r,
        time_nfo.nframes_);

      /* make mono if mono compat enabled */
      if (mono_compat_enabled)
        {
          utils::float_ranges::make_mono (
            &stereo_out.first.buf_[time_nfo.local_offset_],
            &stereo_out.second.buf_[time_nfo.local_offset_], time_nfo.nframes_,
            false);
        }

      /* swap phase if need */
      if (swap_phase)
        {
          utils::float_ranges::mul_k2 (
            &stereo_out.first.buf_[time_nfo.local_offset_], -1.f,
            time_nfo.nframes_);
          utils::float_ranges::mul_k2 (
            &stereo_out.second.buf_[time_nfo.local_offset_], -1.f,
            time_nfo.nframes_);
        }

      int fade_out_samples = fade_out_samples_.load ();
      if (
        effectively_muted && fade_out_samples == 0
        && time_nfo.nframes_ - faded_out_frames > 0)
        {
#if 0
              z_debug (
                "muting %zu frames",
                time_nfo->nframes - faded_out_frames);
#endif
          /* apply mute level */
          if (mute_amp < 0.00001f)
            {
              utils::float_ranges::fill (
                &stereo_out.first.buf_[time_nfo.local_offset_ + faded_out_frames],
                AUDIO_ENGINE->denormal_prevention_val_,
                time_nfo.nframes_ - faded_out_frames);
              utils::float_ranges::fill (
                &stereo_out.second
                   .buf_[time_nfo.local_offset_ + faded_out_frames],
                AUDIO_ENGINE->denormal_prevention_val_,
                time_nfo.nframes_ - faded_out_frames);
            }
          else
            {
              utils::float_ranges::mul_k2 (
                &stereo_out.first.buf_[time_nfo.local_offset_], mute_amp,
                time_nfo.nframes_ - faded_out_frames);
              utils::float_ranges::mul_k2 (
                &stereo_out.second
                   .buf_[time_nfo.local_offset_ + faded_out_frames],
                mute_amp, time_nfo.nframes_ - faded_out_frames);
            }
        }

      /* if master or monitor or sample processor, hard limit the output */
      if (
        (type_ == Type::AudioChannel && (track != nullptr) && track->is_master ())
        || type_ == Type::Monitor || type_ == Type::SampleProcessor)
        {
          utils::float_ranges::clip (
            &stereo_out.first.buf_[time_nfo.local_offset_], -2.f, 2.f,
            time_nfo.nframes_);
          utils::float_ranges::clip (
            &stereo_out.second.buf_[time_nfo.local_offset_], -2.f, 2.f,
            time_nfo.nframes_);
        }
    } /* fi monitor/audio fader */
  else if (type_ == Type::MidiChannel)
    {
      if (!effectively_muted)
        {
          auto &midi_in = get_midi_in_port ();
          auto &midi_out = get_midi_out_port ();
          midi_out.midi_events_.active_events_.append (
            midi_in.midi_events_.active_events_, time_nfo.local_offset_,
            time_nfo.nframes_);

          // also apply volume changes
          for (auto &ev : midi_out.midi_events_.active_events_)
            {
              if (
                midi_mode_ == MidiFaderMode::MIDI_FADER_MODE_VEL_MULTIPLIER
                && utils::midi::midi_is_note_on (ev.raw_buffer_))
                {
                  const midi_byte_t prev_vel =
                    utils::midi::midi_get_velocity (ev.raw_buffer_);
                  const auto new_vel = (midi_byte_t) ((float) prev_vel * amp);
                  ev.set_velocity (
                    std::min (new_vel, static_cast<midi_byte_t> (127)));
                }
            }

          if (
            midi_mode_ == MidiFaderMode::MIDI_FADER_MODE_CC_VOLUME
            && !utils::math::floats_equal (last_cc_volume_, amp))
            {
              /* TODO add volume event on each channel */
            }
        }
    }

  was_effectively_muted_ = effectively_muted;
}

void
init_from (Fader &obj, const Fader &other, utils::ObjectCloneType clone_type)
{
  obj.port_registry_ = other.port_registry_;

  obj.type_ = other.type_;
  obj.phase_ = other.phase_;
  obj.midi_mode_ = other.midi_mode_;

  if (clone_type == utils::ObjectCloneType::Snapshot)
    {
      obj.amp_id_ = other.amp_id_;
      obj.balance_id_ = other.balance_id_;
      obj.mute_id_ = other.mute_id_;
      obj.solo_id_ = other.solo_id_;
      obj.listen_id_ = other.listen_id_;
      obj.mono_compat_enabled_id_ = other.mono_compat_enabled_id_;
      obj.swap_phase_id_ = other.swap_phase_id_;
    }
  else if (clone_type == utils::ObjectCloneType::NewIdentity)
    {
// TODO
#if 0
      auto deep_clone_port = [&] (auto &own_port_id, const auto &other_port_id) {
        if (!other_port_id.has_value ())
          return;

        auto other_amp_port = other_port_id->get_object ();
        std::visit (
          [&] (auto &&other_port) {
            own_port_id = obj.port_registry_->clone_object (*other_port);
          },
          other_amp_port);
      };
      deep_clone_port (obj.amp_id_, other.amp_id_);
      deep_clone_port (obj.balance_id_, other.balance_id_);
      deep_clone_port (obj.mute_id_, other.mute_id_);
      deep_clone_port (obj.solo_id_, other.solo_id_);
      deep_clone_port (obj.listen_id_, other.listen_id_);
      deep_clone_port (
        obj.mono_compat_enabled_id_, other.mono_compat_enabled_id_);
      deep_clone_port (obj.swap_phase_id_, other.swap_phase_id_);

      /* set owner */
      std::vector<dsp::Port *> ports;
      obj.append_ports (ports);
      for (auto &port : ports)
        {
          port->set_full_designation_provider (&obj);
        }
#endif
    }
}

void
from_json (const nlohmann::json &j, Fader &fader)
{
  from_json (j, static_cast<dsp::ProcessorBase &> (fader));
  j.at (Fader::kTypeKey).get_to (fader.type_);
  fader.amp_id_ = { *fader.param_registry_ };
  j.at (Fader::kAmpKey).get_to (*fader.amp_id_);
  j.at (Fader::kPhaseKey).get_to (fader.phase_);
  fader.balance_id_ = { *fader.param_registry_ };
  j.at (Fader::kBalanceKey).get_to (*fader.balance_id_);
  fader.mute_id_ = { *fader.param_registry_ };
  j.at (Fader::kMuteKey).get_to (*fader.mute_id_);
  fader.solo_id_ = { *fader.param_registry_ };
  j.at (Fader::kSoloKey).get_to (*fader.solo_id_);
  fader.listen_id_ = { *fader.param_registry_ };
  j.at (Fader::kListenKey).get_to (*fader.listen_id_);
  fader.mono_compat_enabled_id_ = { *fader.param_registry_ };
  j.at (Fader::kMonoCompatEnabledKey).get_to (*fader.mono_compat_enabled_id_);
  fader.swap_phase_id_ = { *fader.param_registry_ };
  j.at (Fader::kSwapPhaseKey).get_to (*fader.swap_phase_id_);
  j.at (Fader::kMidiModeKey).get_to (fader.midi_mode_);
}
}
