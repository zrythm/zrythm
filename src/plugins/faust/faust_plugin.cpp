// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <charconv>

#include "plugins/faust/faust_plugin.h"
#include "plugins/faust/faust_synth_voice.h"
#include "plugins/plugin_configuration.h"
#include "utils/float_ranges.h"
#include "utils/logger.h"
#include "utils/views.h"

#include <scn/scan.h>

namespace zrythm::plugins
{

namespace
{
dsp::ParameterRange::Unit
faust_unit_to_param_unit (const utils::Utf8String &unit)
{
  if (unit == u8"Hz")
    return dsp::ParameterRange::Unit::Hz;
  if (unit == u8"s")
    return dsp::ParameterRange::Unit::Seconds;
  if (unit == u8"ms")
    return dsp::ParameterRange::Unit::Ms;
  if (unit == u8"dB")
    return dsp::ParameterRange::Unit::Db;
  return dsp::ParameterRange::Unit::None;
}
}

FaustPlugin::FaustPlugin (utils::IObjectRegistry &registry, QObject * parent)
    : InternalPluginBase (registry, parent)
{
}

FaustPlugin::~FaustPlugin () = default;

void
FaustPlugin::on_configuration_changed (
  PluginConfiguration * configuration,
  bool                  generateNewPluginPortsAndParams)
{
  const auto * descr = configuration->descriptor ();
  const auto * key_ptr = std::get_if<utils::Utf8String> (&descr->path_or_id_);
  const auto * info =
    key_ptr != nullptr ? faust::find_faust_plugin_by_key (*key_ptr) : nullptr;

  if (info == nullptr)
    {
      // Not a bundled Faust plugin - act as a pass-through
      z_debug ("configuration changed (no matching Faust plugin)");
      InternalPluginBase::on_configuration_changed (
        configuration, generateNewPluginPortsAndParams);
      return;
    }

  z_debug ("configuration changed: {}", info->key_.str ());

  info_ = info;
  setup_faust (*info, generateNewPluginPortsAndParams);

  Q_EMIT instantiationFinished (true, {});
}

void
FaustPlugin::setup_faust (const faust::FaustPluginInfo &info, bool create_ports)
{
  // Reset any previous Faust DSP state
  voice_manager_.clear_voices ();
  dsp_.reset ();
  controls_.clear ();
  param_to_control_.clear ();

  dsp_ = info.create_ ();
  is_instrument_ = info.is_instrument_;

  faust::FaustControlEnumerator enumerator;
  dsp_->buildUserInterface (&enumerator);
  controls_ = std::ranges::to<decltype (controls_)> (enumerator.controls ());

  if (create_ports)
    {
      create_ports_and_params (
        is_instrument_, dsp_->getNumInputs (), dsp_->getNumOutputs ());
    }

  if (is_instrument_)
    {
      faust::FaustMetaReader meta;
      dsp_->metadata (&meta);

      int nvoices = 16;
      if (const auto sv = meta.get ("nvoices").str (); !sv.empty ())
        {
          auto [ptr, ec] =
            std::from_chars (sv.data (), sv.data () + sv.size (), nvoices);
          if (ec != std::errc{})
            {
              z_warning ("invalid nvoices metadata '{}', using default 16", sv);
              nvoices = 16;
            }
        }

      // Optional release-tail overrides (defaults: 2s release, -80 dBFS RMS).
      if (
        const auto sv = meta.get ("zrythm_release_seconds").str (); !sv.empty ())
        {
          if (auto r = scn::scan_value<float> (sv); r && r->value () > 0.f)
            voice_release_seconds_ = r->value ();
          else
            z_warning ("invalid zrythm_release_seconds '{}', using default", sv);
        }
      if (
        const auto sv = meta.get ("zrythm_silence_threshold_db").str ();
        !sv.empty ())
        {
          if (auto r = scn::scan_value<float> (sv); r)
            voice_silence_threshold_db_ = r->value ();
          else
            z_warning (
              "invalid zrythm_silence_threshold_db '{}', using default", sv);
        }

      for ([[maybe_unused]] const auto i : std::views::iota (0, nvoices))
        {
          voice_manager_.add_voice (
            std::make_unique<faust::FaustSynthVoice> (info.create_ ()));
        }
    }

  // Parameters exist on both paths now (freshly created, or restored from a
  // project before configuration). Map them to their controls and push the
  // current values so restored values take effect immediately.
  rebuild_param_mapping ();
  push_all_params_to_zones ();
}

void
FaustPlugin::create_ports_and_params (bool instrument, int num_ins, int num_outs)
{
  if (num_ins > 0)
    {
      auto port_ref = utils::create_object<dsp::AudioPort> (
        registry (), u8"audio_in", dsp::PortFlow::Input,
        num_ins == 1
          ? dsp::AudioPort::BusLayout::Mono
          : dsp::AudioPort::BusLayout::Stereo,
        static_cast<uint8_t> (num_ins));
      add_input_port (port_ref);
    }
  if (num_outs > 0)
    {
      auto port_ref = utils::create_object<dsp::AudioPort> (
        registry (), u8"audio_out", dsp::PortFlow::Output,
        num_outs == 1
          ? dsp::AudioPort::BusLayout::Mono
          : dsp::AudioPort::BusLayout::Stereo,
        static_cast<uint8_t> (num_outs));
      add_output_port (port_ref);
    }

  if (instrument)
    {
      auto port_ref = utils::create_object<dsp::MidiPort> (
        registry (), u8"midi_in", dsp::PortFlow::Input);
      add_input_port (port_ref);
    }

  for (const auto &control : controls_)
    {
      if (control.widget == faust::FaustControl::Widget::Bargraph)
        continue;
      if (
        instrument && faust::FaustSynthVoice::is_voice_managed_control (control))
        continue;

      add_faust_param (control);
    }
}

void
FaustPlugin::add_faust_param (const faust::FaustControl &control)
{
  dsp::ParameterRange range{};
  const auto          is_button =
    control.widget == faust::FaustControl::Widget::Button
    || control.widget == faust::FaustControl::Widget::CheckButton;
  if (is_button)
    {
      range = dsp::ParameterRange::make_toggle (control.init > 0.5f);
    }
  else
    {
      auto type = dsp::ParameterRange::Type::Linear;
      if (control.scale_log)
        {
          type = dsp::ParameterRange::Type::Logarithmic;
        }
      else if (
        control.step == 1.f && control.min == std::floor (control.min)
        && control.max == std::floor (control.max))
        {
          type = dsp::ParameterRange::Type::Integer;
        }
      range = dsp::ParameterRange{
        type, control.min, control.max, 0.f, control.init
      };
    }
  range.unit_ = faust_unit_to_param_unit (control.unit);

  auto param_ref = utils::create_object<dsp::ProcessorParameter> (
    registry (), registry (), dsp::ProcessorParameter::UniqueId (control.path),
    range, control.label);
  add_parameter (param_ref);
}

void
FaustPlugin::rebuild_param_mapping ()
{
  const auto &params = get_parameters ();
  param_to_control_.assign (params.size (), kNoControl);
  for (const auto &[param_idx, param_ref] : utils::views::enumerate (params))
    {
      const auto &unique_id = param_ref.get ()->get_unique_id ();
      for (
        const auto &[control_idx, control] : utils::views::enumerate (controls_))
        {
          if (unique_id == dsp::ProcessorParameter::UniqueId (control.path))
            {
              param_to_control_[param_idx] = control_idx;
              break;
            }
        }
    }
}

void
FaustPlugin::push_all_params_to_zones ()
{
  const auto &params = get_parameters ();
  for (
    const auto &[param_idx, control_idx] :
    utils::views::enumerate (param_to_control_))
    {
      if (control_idx == kNoControl)
        continue;
      auto *      param = params[param_idx].get ();
      const float real_value =
        param->range ().convertFrom0To1 (param->baseValue ());
      write_control_value (control_idx, real_value);
    }
}

void
FaustPlugin::prepare_plugin_for_processing (
  units::sample_rate_t sample_rate,
  units::sample_u32_t  max_block_length)
{
  if (dsp_ == nullptr)
    return;

  const auto sr = sample_rate.in (units::sample_rate);
  const auto max_block = max_block_length.in<int> (units::samples);

  info_->class_init_ (sr);
  dsp_->instanceInit (sr);

  in_ptrs_.resize (static_cast<size_t> (dsp_->getNumInputs ()));
  out_ptrs_.resize (static_cast<size_t> (dsp_->getNumOutputs ()));

  // Validate that the port channel counts match the dsp's expectations, so
  // process_fx never passes nullptr channel pointers to compute().
  [[maybe_unused]] int in_ch = 0;
  for (const auto * port : audio_in_ports_)
    in_ch += static_cast<int> (port->num_channels ());
  [[maybe_unused]] int out_ch = 0;
  for (const auto * port : audio_out_ports_)
    out_ch += static_cast<int> (port->num_channels ());
  assert (in_ch == dsp_->getNumInputs ());
  assert (out_ch == dsp_->getNumOutputs ());

  if (is_instrument_)
    {
      synth_buffer_.setSize (dsp_->getNumOutputs (), max_block);
      for (const auto &voice : voice_manager_.voices ())
        {
          static_cast<faust::FaustSynthVoice *> (voice.get ())
            ->prepare (
              sr, max_block, voice_release_seconds_, voice_silence_threshold_db_);
        }
      voice_manager_.all_notes_off ();
    }
}

void
FaustPlugin::write_control_value (size_t control_index, float real_value) noexcept
{
  if (is_instrument_)
    {
      for (const auto &voice : voice_manager_.voices ())
        {
          static_cast<faust::FaustSynthVoice *> (voice.get ())
            ->set_control_value (control_index, real_value);
        }
    }
  else
    {
      *controls_[control_index].zone = real_value;
    }
}

void
FaustPlugin::sync_params_to_zones () noexcept
{
  for (const auto &change : change_tracker ().changes ())
    {
      if (change.index >= param_to_control_.size ())
        continue;

      const size_t control_idx = param_to_control_[change.index];
      if (control_idx == kNoControl)
        continue;

      const float real_value =
        change.param->range ().convertFrom0To1 (change.modulated_value);
      write_control_value (control_idx, real_value);
    }
}

void
FaustPlugin::process_impl (dsp::graph::ProcessBlockInfo time_info) noexcept
{
  if (dsp_ == nullptr)
    return;

  sync_params_to_zones ();

  if (is_instrument_)
    {
      process_instrument (time_info);
    }
  else
    {
      process_fx (time_info);
    }
}

void
FaustPlugin::process_fx (dsp::graph::ProcessBlockInfo time_info) noexcept
{
  const auto local_offset = time_info.buffer_offset_.in<int> (units::samples);
  const auto nframes = time_info.nframes_.in<int> (units::samples);
  const int  num_ins = dsp_->getNumInputs ();
  const int  num_outs = dsp_->getNumOutputs ();

  int ch = 0;
  for (auto * port : audio_in_ports_)
    {
      for (
        int i = 0; i < static_cast<int> (port->num_channels ()) && ch < num_ins;
        ++i, ++ch)
        {
          // Faust DSP contract: inputs are read-only; the const_cast is
          // required only to satisfy the FAUSTFLOAT** parameter type.
          in_ptrs_[ch] = const_cast<float *> (
            port->buffers ()->getReadPointer (i, local_offset));
        }
    }
  ch = 0;
  for (auto * port : audio_out_ports_)
    {
      for (
        int i = 0;
        i < static_cast<int> (port->num_channels ()) && ch < num_outs; ++i, ++ch)
        {
          out_ptrs_[ch] = port->buffers ()->getWritePointer (i, local_offset);
        }
    }

  dsp_->compute (nframes, in_ptrs_.data (), out_ptrs_.data ());
}

void
FaustPlugin::process_instrument (dsp::graph::ProcessBlockInfo time_info) noexcept
{
  const auto local_offset = time_info.buffer_offset_.in<int> (units::samples);
  const auto nframes = time_info.nframes_.in<int> (units::samples);

  synth_buffer_.clear (local_offset, nframes);
  const auto &midi_events =
    midi_in_ports_.empty () ? empty_midi_buffer_ : midi_in_ports_.front ()->buffer_;
  voice_manager_.process (
    synth_buffer_, midi_events, time_info.buffer_offset_, time_info.nframes_);

  int ch = 0;
  for (auto * port : audio_out_ports_)
    {
      for (
        int i = 0;
        i < static_cast<int> (port->num_channels ())
        && ch < synth_buffer_.getNumChannels ();
        ++i, ++ch)
        {
          utils::float_ranges::copy (
            { port->buffers ()->getWritePointer (i, local_offset),
              static_cast<size_t> (nframes) },
            { synth_buffer_.getReadPointer (ch) + local_offset,
              static_cast<size_t> (nframes) });
        }
    }
}

} // namespace zrythm::plugins
