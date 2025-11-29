// SPDX-FileCopyrightText: © 2019-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <utility>

#include "dsp/audio_port.h"
#include "dsp/fader.h"
#include "dsp/midi_event.h"
#include "dsp/panning.h"
#include "dsp/port.h"
#include "utils/dsp.h"
#include "utils/math.h"
#include "utils/midi.h"

namespace zrythm::dsp
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
      utils::Utf8String::from_qstring (QObject::tr ("Fader Gain")));
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
        auto port = dependencies.port_registry_.create_object<dsp::AudioPort> (
          name, dsp::PortFlow::Input, dsp::AudioPort::BusLayout::Stereo, 2);
        port.get_object_as<dsp::AudioPort> ()->set_symbol (sym);
        add_input_port (port);
      }

      {
        utils::Utf8String name;
        utils::Utf8String sym;
        name = utils::Utf8String::from_qstring (QObject::tr ("Fader output"));
        sym = u8"fader_output";

        {
          /* stereo out */
          auto port = dependencies.port_registry_.create_object<dsp::AudioPort> (
            name, dsp::PortFlow::Output, dsp::AudioPort::BusLayout::Stereo, 2);
          port.get_object_as<dsp::AudioPort> ()->set_symbol (sym);
          add_output_port (port);
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
Fader::calculate_target_gain_rt () const
{
  if (effectively_muted_rt ())
    return mute_gain_cb_ ();

  const auto gain_from_param =
    processing_caches_->amp_param_->range ().convertFrom0To1 (
      processing_caches_->amp_param_->currentValue ());
  return gain_from_param;
}

// ============================================================================
// ProcessorBase Interface
// ============================================================================

void
Fader::custom_prepare_for_processing (
  const graph::GraphNode * node,
  units::sample_rate_t     sample_rate,
  nframes_t                max_block_length)
{
  processing_caches_ = std::make_unique<FaderProcessingCaches> ();

  current_gain_.reset (sample_rate.in<double> (units::sample_rate), 0.01);

  processing_caches_->amp_param_ =
    amp_id_.has_value () ? &get_amp_param () : nullptr;
  processing_caches_->balance_param_ =
    balance_id_.has_value () ? &get_balance_param () : nullptr;
  processing_caches_->mute_param_ =
    mute_id_.has_value () ? &get_mute_param () : nullptr;
  processing_caches_->solo_param_ =
    solo_id_.has_value () ? &get_solo_param () : nullptr;
  processing_caches_->listen_param_ =
    listen_id_.has_value () ? &get_listen_param () : nullptr;
  processing_caches_->mono_compat_enabled_param_ =
    mono_compat_enabled_id_.has_value ()
      ? &get_mono_compat_enabled_param ()
      : nullptr;
  processing_caches_->swap_phase_param_ =
    swap_phase_id_.has_value () ? &get_swap_phase_param () : nullptr;

  processing_caches_->audio_ins_rt_.clear ();
  processing_caches_->audio_outs_rt_.clear ();
  processing_caches_->midi_in_rt_ = {};
  processing_caches_->midi_out_rt_ = {};
  if (is_audio ())
    {
      auto &stereo_in = get_stereo_in_port ();
      processing_caches_->audio_ins_rt_.push_back (&stereo_in);
      auto &stereo_out = get_stereo_out_port ();
      processing_caches_->audio_outs_rt_.push_back (&stereo_out);
    }
  else if (is_midi ())
    {
      processing_caches_->midi_in_rt_ = &get_midi_in_port ();
      processing_caches_->midi_out_rt_ = &get_midi_out_port ();
    }
}

void
Fader::custom_release_resources ()
{
  processing_caches_.reset ();
}

void
Fader::custom_process_block (
  EngineProcessTimeInfo  time_nfo,
  const dsp::ITransport &transport) noexcept
{
  current_gain_.setTargetValue (calculate_target_gain_rt ());

  if (is_audio ())
    {
      // First, copy the input to output
      for (
        const auto &[out, in] : std::views::zip (
          processing_caches_->audio_outs_rt_, processing_caches_->audio_ins_rt_))
        {
          out->copy_source_rt (*in, time_nfo);
        }

      if (preprocess_audio_cb_.has_value ())
        {
          const auto &out_buf =
            processing_caches_->audio_outs_rt_.front ()->buffers ();
          std::invoke (
            preprocess_audio_cb_.value (),
            std::make_pair (
              std::span (out_buf->getWritePointer (0), out_buf->getNumSamples ()),
              std::span (out_buf->getWritePointer (1), out_buf->getNumSamples ())),
            time_nfo);
        }

      const auto &balance_param = processing_caches_->balance_param_;
      const auto &mono_compat_param =
        processing_caches_->mono_compat_enabled_param_;
      const auto &swap_phase_param = processing_caches_->swap_phase_param_;
      const float pan = balance_param->range ().convertFrom0To1 (
        balance_param->currentValue ());
      const bool mono_compat_enabled = mono_compat_param->range ().is_toggled (
        mono_compat_param->currentValue ());
      const bool swap_phase = swap_phase_param->range ().is_toggled (
        swap_phase_param->currentValue ());

      // apply gain
      const auto &out_buf =
        processing_caches_->audio_outs_rt_.front ()->buffers ();
      {
        for (
          const auto i : std::views::iota (
            static_cast<int> (time_nfo.local_offset_), out_buf->getNumSamples ()))
          {
            const auto gain = current_gain_.getCurrentValue ();
            for (
              const auto ch : std::views::iota (0, out_buf->getNumChannels ()))
              {
                out_buf->applyGain (ch, i, 1, gain);
              }
            current_gain_.skip (1);
          }
      }

      // apply pan
      {
        auto [calc_l, calc_r] = dsp::calculate_balance_control (
          dsp::BalanceControlAlgorithm::Linear, pan);
        out_buf->applyGain (
          0, static_cast<int> (time_nfo.local_offset_),
          static_cast<int> (time_nfo.nframes_), calc_l);
        out_buf->applyGain (
          1, static_cast<int> (time_nfo.local_offset_),
          static_cast<int> (time_nfo.nframes_), calc_r);
      }

      /* make mono if mono compat enabled */
      if (mono_compat_enabled)
        {
          utils::float_ranges::make_mono (
            out_buf->getWritePointer (
              0, static_cast<int> (time_nfo.local_offset_)),
            out_buf->getWritePointer (
              1, static_cast<int> (time_nfo.local_offset_)),
            time_nfo.nframes_, false);
        }

      /* swap phase if need */
      if (swap_phase)
        {
          utils::float_ranges::mul_k2 (
            out_buf->getWritePointer (
              0, static_cast<int> (time_nfo.local_offset_)),
            -1.f, time_nfo.nframes_);
          utils::float_ranges::mul_k2 (
            out_buf->getWritePointer (
              1, static_cast<int> (time_nfo.local_offset_)),
            -1.f, time_nfo.nframes_);
        }

      // hard-limit output if requested
      if (hard_limit_output_)
        {
          for (const auto ch : std::views::iota (0, out_buf->getNumChannels ()))
            {
              utils::float_ranges::clip (
                out_buf->getWritePointer (
                  ch, static_cast<int> (time_nfo.local_offset_)),
                -2.f, 2.f, time_nfo.nframes_);
            }
        }
    } // endif is_audio()
  else if (is_midi ())
    {
      if (!effectively_muted_rt ())
        {
          auto &target_events =
            processing_caches_->midi_out_rt_->midi_events_.queued_events_;
          target_events.append (
            processing_caches_->midi_in_rt_->midi_events_.active_events_,
            time_nfo.local_offset_, time_nfo.nframes_);

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
