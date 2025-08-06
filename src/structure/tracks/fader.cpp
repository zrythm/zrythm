// SPDX-FileCopyrightText: © 2019-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <utility>

#include "dsp/audio_port.h"
#include "dsp/midi_event.h"
#include "dsp/panning.h"
#include "dsp/port.h"
#include "structure/tracks/fader.h"
#include "utils/dsp.h"
#include "utils/math.h"
#include "utils/midi.h"

namespace zrythm::structure::tracks
{

static constexpr auto amp_id_str = "volume"sv;
static constexpr auto balance_id_str = "balance"sv;
static constexpr auto mute_id_str = "mute"sv;
static constexpr auto solo_id_str = "solo"sv;
static constexpr auto listen_id_str = "listen"sv;
static constexpr auto mono_compat_enabled_id_str = "mono_compat_enabled"sv;
static constexpr auto swap_phase_id_str = "swap_phase"sv;

Fader::Fader (
  dsp::ProcessorBase::ProcessorBaseDependencies      dependencies,
  dsp::PortType                                      signal_type,
  bool                                               hard_limit_output,
  bool                                               make_params_automatable,
  std::optional<std::function<utils::Utf8String ()>> owner_name_provider,
  ShouldBeMutedCallback                              should_be_muted_cb,
  QObject *                                          parent)
    : QObject (parent), dsp::ProcessorBase (dependencies, u8"Fader"),
      signal_type_ (signal_type), hard_limit_output_ (hard_limit_output),
      should_be_muted_cb_ (std::move (should_be_muted_cb))
{
  {
    /* set volume */
    auto amp_id = dependencies.param_registry_.create_object<
      dsp::ProcessorParameter> (
      dependencies.port_registry_,
      dsp::ProcessorParameter::UniqueId (
        utils::Utf8String::from_utf8_encoded_string (amp_id_str)),
      dsp::ParameterRange (
        dsp::ParameterRange::Type::GainAmplitude, 0.f, 2.f, 0.f, 1.f),
      utils::Utf8String::from_qstring (QObject::tr ("Fader Volume")));
    add_parameter (amp_id);
    amp_id_ = amp_id.id ();
    get_amp_param ().set_automatable (make_params_automatable);

    /* set pan */
    auto balance_id = dependencies.param_registry_.create_object<
      dsp::ProcessorParameter> (
      dependencies.port_registry_,
      dsp::ProcessorParameter::UniqueId (
        utils::Utf8String::from_utf8_encoded_string (balance_id_str)),
      dsp::ParameterRange (
        dsp::ParameterRange::Type::Linear, 0.f, 1.f, 0.5f, 0.5f),
      utils::Utf8String::from_qstring (QObject::tr ("Fader Balance")));
    add_parameter (balance_id);
    balance_id_ = balance_id.id ();
    get_balance_param ().set_automatable (make_params_automatable);
  }

  {
    /* set mute */
    auto mute_id = dependencies.param_registry_.create_object<
      dsp::ProcessorParameter> (
      dependencies.port_registry_,
      dsp::ProcessorParameter::UniqueId (
        utils::Utf8String::from_utf8_encoded_string (mute_id_str)),
      dsp::ParameterRange (dsp::ParameterRange::Type::Toggle, 0.f, 1.f, 0.f, 0.f),
      utils::Utf8String::from_qstring (QObject::tr ("Fader Mute")));
    add_parameter (mute_id);
    mute_id_ = mute_id.id ();
    get_mute_param ().set_automatable (make_params_automatable);
  }

  {
    /* set solo */
    auto solo_id = dependencies.param_registry_.create_object<
      dsp::ProcessorParameter> (
      dependencies.port_registry_,
      dsp::ProcessorParameter::UniqueId (
        utils::Utf8String::from_utf8_encoded_string (solo_id_str)),
      dsp::ParameterRange (dsp::ParameterRange::Type::Toggle, 0.f, 1.f, 0.f, 0.f),
      utils::Utf8String::from_qstring (QObject::tr ("Fader Solo")));
    add_parameter (solo_id);
    solo_id_ = solo_id.id ();
    get_solo_param ().set_automatable (false);
  }

  {
    /* set listen */
    auto listen_id = dependencies.param_registry_.create_object<
      dsp::ProcessorParameter> (
      dependencies.port_registry_,
      dsp::ProcessorParameter::UniqueId (
        utils::Utf8String::from_utf8_encoded_string (listen_id_str)),
      dsp::ParameterRange (dsp::ParameterRange::Type::Toggle, 0.f, 1.f, 0.f, 0.f),
      utils::Utf8String::from_qstring (QObject::tr ("Fader Listen")));
    add_parameter (listen_id);
    listen_id_ = listen_id.id ();
    get_listen_param ().set_automatable (false);
  }

  if (is_audio ())
    {
      {
        /* set mono compat */
        auto mono_compat_enabled_id = dependencies.param_registry_.create_object<
          dsp::ProcessorParameter> (
          dependencies.port_registry_,
          dsp::ProcessorParameter::UniqueId (
            utils::Utf8String::from_utf8_encoded_string (
              mono_compat_enabled_id_str)),
          dsp::ParameterRange (
            dsp::ParameterRange::Type::Toggle, 0.f, 1.f, 0.f, 0.f),
          utils::Utf8String::from_qstring (QObject::tr ("Fader Mono Compat")));
        add_parameter (mono_compat_enabled_id);
        mono_compat_enabled_id_ = mono_compat_enabled_id.id ();
        get_mono_compat_enabled_param ().set_automatable (false);
      }

      {
        /* set swap phase */
        auto swap_phase_id = dependencies.param_registry_.create_object<
          dsp::ProcessorParameter> (
          dependencies.port_registry_,
          dsp::ProcessorParameter::UniqueId (
            utils::Utf8String::from_utf8_encoded_string (swap_phase_id_str)),
          dsp::ParameterRange (
            dsp::ParameterRange::Type::Toggle, 0.f, 1.f, 0.f, 0.f),
          utils::Utf8String::from_qstring (QObject::tr ("Fader Swap Phase")));
        add_parameter (swap_phase_id);
        swap_phase_id_ = swap_phase_id.id ();
        get_swap_phase_param ().set_automatable (false);
      }
    }

  if (is_audio ())
    {
      {
        utils::Utf8String name;
        utils::Utf8String sym;
        name = utils::Utf8String::from_qstring (QObject::tr ("Fader input"));
        sym = u8"fader_in";

        /* stereo in */
        auto stereo_in_ports = dsp::StereoPorts::create_stereo_ports (
          dependencies.port_registry_, true, name, sym);
        add_input_port (stereo_in_ports.first);
        add_input_port (stereo_in_ports.second);
      }

      {
        utils::Utf8String name;
        utils::Utf8String sym;
        name = utils::Utf8String::from_qstring (QObject::tr ("Fader output"));
        sym = u8"fader_output";

        {
          /* stereo out */
          auto stereo_out_ports = dsp::StereoPorts::create_stereo_ports (
            dependencies.port_registry_, false, name, sym);
          add_output_port (stereo_out_ports.first);
          add_output_port (stereo_out_ports.second);
        }
      }
    }
  else if (is_midi ())
    {
      {
        /* MIDI in */
        utils::Utf8String name;
        utils::Utf8String sym;
        name =
          utils::Utf8String::from_qstring (QObject::tr ("Ch MIDI Fader in"));
        sym = u8"ch_midi_fader_in";
        add_input_port (dependencies.port_registry_.create_object<dsp::MidiPort> (
          name, dsp::PortFlow::Input));
        auto &midi_in_port = get_midi_in_port ();
        midi_in_port.set_symbol (sym);
      }

      {
        utils::Utf8String name;
        utils::Utf8String sym;
        /* MIDI out */
        name =
          utils::Utf8String::from_qstring (QObject::tr ("Ch MIDI Fader out"));
        sym = u8"ch_midi_fader_out";
        add_output_port (dependencies.port_registry_.create_object<dsp::MidiPort> (
          name, dsp::PortFlow::Output));
        auto &midi_out_port = get_midi_out_port ();
        midi_out_port.set_symbol (sym);
      }
    }

  set_name ([&] () -> utils::Utf8String {
    if (owner_name_provider.has_value ())
      {
        return std::invoke (owner_name_provider.value ()) + u8" Fader";
      }

    return u8"Fader";
  }());

  init_param_caches ();
}

void
Fader::init_param_caches ()
{
  for (const auto &param_ref : get_parameters ())
    {
      const auto * param = param_ref.get_object_as<dsp::ProcessorParameter> ();

      const auto &id_str = type_safe::get (param->get_unique_id ()).view ();

      if (id_str == amp_id_str)
        {
          amp_id_ = param_ref.id ();
        }
      else if (id_str == balance_id_str)
        {
          balance_id_ = param_ref.id ();
        }
      else if (id_str == mute_id_str)
        {
          mute_id_ = param_ref.id ();
        }
      else if (id_str == solo_id_str)
        {
          solo_id_ = param_ref.id ();
        }
      else if (id_str == listen_id_str)
        {
          listen_id_ = param_ref.id ();
        }
      else if (id_str == mono_compat_enabled_id_str)
        {
          mono_compat_enabled_id_ = param_ref.id ();
        }
      else if (id_str == swap_phase_id_str)
        {
          swap_phase_id_ = param_ref.id ();
        }
    }
}

std::string
Fader::db_string_getter () const
{
  return fmt::format (
    "{:.1f}",
    utils::math::amp_to_dbfs (
      get_amp_param ().range ().convertFrom0To1 (
        get_amp_param ().currentValue ())));
}

float
Fader::calculate_target_gain () const
{
  if (effectively_muted ())
    return mute_gain_cb_ ();

  const auto &amp_param = get_amp_param ();
  const auto  gain_from_param =
    amp_param.range ().convertFrom0To1 (amp_param.currentValue ());
  return gain_from_param;
}

// ============================================================================
// IProcessable Interface
// ============================================================================

void
Fader::custom_prepare_for_processing (
  sample_rate_t sample_rate,
  nframes_t     max_block_length)
{
  current_gain_.reset (static_cast<double> (sample_rate), 0.01);
}

void
Fader::custom_process_block (const EngineProcessTimeInfo time_nfo) noexcept
{
  current_gain_.setTargetValue (calculate_target_gain ());

  if (is_audio ())
    {
      auto stereo_in = get_stereo_in_ports ();
      auto stereo_out = get_stereo_out_ports ();

      // First, copy the input to output
      utils::float_ranges::copy (
        &stereo_out.first.buf_[time_nfo.local_offset_],
        &stereo_in.first.buf_[time_nfo.local_offset_], time_nfo.nframes_);
      utils::float_ranges::copy (
        &stereo_out.second.buf_[time_nfo.local_offset_],
        &stereo_in.second.buf_[time_nfo.local_offset_], time_nfo.nframes_);

      if (preprocess_audio_cb_.has_value ())
        {
          std::invoke (
            preprocess_audio_cb_.value (),
            std::make_pair (
              std::span (stereo_out.first.buf_),
              std::span (stereo_out.second.buf_)),
            time_nfo);
        }

      const auto &balance_param = get_balance_param ();
      const auto &mono_compat_param = get_mono_compat_enabled_param ();
      const auto &swap_phase_param = get_swap_phase_param ();
      const float pan =
        balance_param.range ().convertFrom0To1 (balance_param.currentValue ());
      const bool mono_compat_enabled = mono_compat_param.range ().is_toggled (
        mono_compat_param.currentValue ());
      const bool swap_phase =
        swap_phase_param.range ().is_toggled (swap_phase_param.currentValue ());

      // apply gain (TODO: use SIMD)
      for (size_t i = 0; i < time_nfo.nframes_; i++)
        {
          stereo_out.first.buf_[time_nfo.local_offset_ + i] *=
            current_gain_.getCurrentValue ();
          stereo_out.second.buf_[time_nfo.local_offset_ + i] *=
            current_gain_.getCurrentValue ();
          current_gain_.skip (1);
        }

      auto [calc_l, calc_r] = dsp::calculate_balance_control (
        dsp::BalanceControlAlgorithm::Linear, pan);

      // apply pan
      utils::float_ranges::mul_k2 (
        &stereo_out.first.buf_[time_nfo.local_offset_], calc_l,
        time_nfo.nframes_);
      utils::float_ranges::mul_k2 (
        &stereo_out.second.buf_[time_nfo.local_offset_], calc_r,
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

      // hard-limit output if requested
      if (hard_limit_output_)
        {
          utils::float_ranges::clip (
            &stereo_out.first.buf_[time_nfo.local_offset_], -2.f, 2.f,
            time_nfo.nframes_);
          utils::float_ranges::clip (
            &stereo_out.second.buf_[time_nfo.local_offset_], -2.f, 2.f,
            time_nfo.nframes_);
        }
    } // endif is_audio()
  else if (is_midi ())
    {
      if (!effectively_muted ())
        {
          auto &midi_in = get_midi_in_port ();
          auto &midi_out = get_midi_out_port ();
          auto &target_events = midi_out.midi_events_.queued_events_;
          target_events.append (
            midi_in.midi_events_.active_events_, time_nfo.local_offset_,
            time_nfo.nframes_);

          // also apply volume changes
          for (auto &ev : target_events)
            {
              if (
                midi_mode_ == MidiFaderMode::MIDI_FADER_MODE_VEL_MULTIPLIER
                && utils::midi::midi_is_note_on (ev.raw_buffer_))
                {
                  const midi_byte_t prev_vel =
                    utils::midi::midi_get_velocity (ev.raw_buffer_);
                  const auto new_vel =
                    (midi_byte_t) ((float) prev_vel
                                   * current_gain_.getCurrentValue ());
                  ev.set_velocity (
                    std::min (new_vel, static_cast<midi_byte_t> (127)));
                }
            }

          if (
            midi_mode_ == MidiFaderMode::MIDI_FADER_MODE_CC_VOLUME
            && !utils::math::floats_equal (
              last_cc_volume_, current_gain_.getCurrentValue ()))
            {
              /* TODO add volume event on each channel */
            }
        }

      // Note: Currently for MIDI, we are using the gain at the start of the
      // cycle for the whole cycle. We should interpolate instead
      current_gain_.skip (static_cast<int> (time_nfo.nframes_));
    }
}

// ============================================================================

void
init_from (Fader &obj, const Fader &other, utils::ObjectCloneType clone_type)
{
  // TODO
  // obj.type_ = other.type_;
  // obj.phase_ = other.phase_;
  // obj.midi_mode_ = other.midi_mode_;
}

void
from_json (const nlohmann::json &j, Fader &fader)
{
  from_json (j, static_cast<dsp::ProcessorBase &> (fader));
  j.at (Fader::kMidiModeKey).get_to (fader.midi_mode_);
  fader.init_param_caches ();
}
}
