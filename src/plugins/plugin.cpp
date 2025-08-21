// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <utility>

#include "plugins/plugin.h"

namespace zrythm::plugins
{

Plugin::Plugin (
  dsp::ProcessorBase::ProcessorBaseDependencies dependencies,
  StateDirectoryParentPathProvider              state_path_provider,
  QObject *                                     parent)
    : QObject (parent), zrythm::dsp::ProcessorBase (dependencies, u8"Plugin"),
      state_dir_parent_path_provider_ (std::move (state_path_provider))
{
  QObject::connect (
    this, &Plugin::instantiationFinished, this, [this] (bool successful) {
      instantiation_status_ =
        successful ? InstantiationStatus::Successful : InstantiationStatus::Failed;
      Q_EMIT instantiationStatusChanged (instantiation_status_);
    });
}

dsp::ProcessorParameterUuidReference
Plugin::generate_default_bypass_param () const
{
  auto bypass_id =
    dependencies ().param_registry_.create_object<dsp::ProcessorParameter> (
      dependencies ().port_registry_,
      dsp::ProcessorParameter::UniqueId (u8"/zrythm-bypass"),
      dsp::ParameterRange{ dsp::ParameterRange::Type::Toggle, 0.f, 1.f, 0.f, 0.f },
      utils::Utf8String::from_qstring (QObject::tr ("Bypass")));
  bypass_id.get_object_as<dsp::ProcessorParameter> ()->set_description (
    utils::Utf8String::from_qstring (
      QObject::tr ("Enables or disables the plugin")));
  return bypass_id;
}

dsp::ProcessorParameterUuidReference
Plugin::generate_default_gain_param () const
{
  auto gain_id =
    dependencies ().param_registry_.create_object<dsp::ProcessorParameter> (
      dependencies ().port_registry_,
      dsp::ProcessorParameter::UniqueId (u8"/zrythm-gain"),
      dsp::ParameterRange{
        dsp::ParameterRange::Type::GainAmplitude, 0.f, 8.f, 0.f, 1.f },
      utils::Utf8String::from_qstring (QObject::tr ("Gain")));
  return gain_id;
}

void
Plugin::set_configuration (const PluginConfiguration &setting)
{
  assert (!set_configuration_called_);
  set_configuration_called_ = true;

  configuration_ = utils::clone_unique (setting);
  const auto &descr = configuration_->descr_;

  z_debug (
    "setting setting for plugin: {} ({})", descr->name_,
    ENUM_NAME (descr->protocol_));

  z_debug ("{} ({})", get_name (), ENUM_NAME (get_protocol ()));

  set_name (get_name ());

  Q_EMIT configurationChanged (configuration_.get ());
}

// ============================================================================
// IProcessable Interface
// ============================================================================

void
Plugin::custom_prepare_for_processing (
  sample_rate_t sample_rate,
  nframes_t     max_block_length)
{
  init_param_caches ();
  prepare_for_processing_impl (sample_rate, max_block_length);
}

void
Plugin::custom_process_block (const EngineProcessTimeInfo time_nfo) noexcept
{
  if (instantiation_failed_)
    return;

  if (!currently_enabled ())
    {
      process_passthrough_impl (time_nfo);
      return;
    }

  process_impl (time_nfo);
}

void
Plugin::custom_release_resources ()
{
  release_resources_impl ();
}

void
Plugin::process_passthrough_impl (const EngineProcessTimeInfo time_nfo) noexcept
{
  // ProcessorBase's processing logic does passthrough
  dsp::ProcessorBase::custom_process_block (time_nfo);
}

// ============================================================================

void
Plugin::init_param_caches ()
{
  audio_in_ports_.clear ();
  cv_in_ports_.clear ();
  audio_out_ports_.clear ();

  for (const auto &port_var : get_input_ports ())
    {
      std::visit (
        [&] (auto &&port) {
          using PortT = base_type<decltype (port)>;
          if constexpr (std::is_same_v<PortT, dsp::AudioPort>)
            audio_in_ports_.push_back (port);
          else if constexpr (std::is_same_v<PortT, dsp::CVPort>)
            cv_in_ports_.push_back (port);
          else if constexpr (std::is_same_v<PortT, dsp::MidiPort>)
            midi_in_port_ = port;
        },
        port_var.get_object ());
    }

  for (const auto &port_var : get_output_ports ())
    {
      std::visit (
        [&] (auto &&port) {
          using PortT = base_type<decltype (port)>;
          if constexpr (std::is_same_v<PortT, dsp::AudioPort>)
            audio_out_ports_.push_back (port);
          else if constexpr (std::is_same_v<PortT, dsp::MidiPort>)
            midi_out_port_ = port;
        },
        port_var.get_object ());
    }
}

void
from_json (const nlohmann::json &j, Plugin &p)
{
  from_json (j, static_cast<Plugin::UuidIdentifiableObject &> (p));
  from_json (j, static_cast<dsp::ProcessorBase &> (p));
  j.at (Plugin::kSettingKey).get_to (p.configuration_);
  j.at (Plugin::kProgramIndexKey).get_to (p.program_index_);
  j.at (Plugin::kVisibleKey).get_to (p.visible_);

  if (p.configuration_)
    {
      p.set_configuration (*p.configuration_);
    }

  p.gain_id_.reset ();
  p.bypass_id_.reset ();
  for (const auto &param_ref : p.get_parameters ())
    {
      const auto * param = param_ref.get_object_as<dsp::ProcessorParameter> ();
      if (
        param->get_unique_id ()
        == dsp::ProcessorParameter::UniqueId (u8"/zrythm-gain"))
        {
          p.gain_id_ = param->get_uuid ();
        }
      else if (
        param->get_unique_id ()
        == dsp::ProcessorParameter::UniqueId (u8"/zrythm-bypass"))
        {
          p.bypass_id_ = param->get_uuid ();
        }

      if (p.gain_id_.has_value () && p.bypass_id_.has_value ())
        break;
    }
}

} // namespace zrythm::gui::old_dsp::plugins
