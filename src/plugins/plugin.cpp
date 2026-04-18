// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <utility>

#include "plugins/plugin.h"

#include <QTimer>

namespace zrythm::plugins
{

Plugin::Plugin (
  dsp::ProcessorBase::ProcessorBaseDependencies dependencies,
  QObject *                                     parent)
    : QObject (parent), zrythm::dsp::ProcessorBase (dependencies, u8"Plugin"),
      param_flush_timer_ (utils::make_qobject_unique<QTimer> (this))
{
  QObject::connect (
    param_flush_timer_.get (), &QTimer::timeout, this,
    &Plugin::flush_plugin_values);

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

  /* If ports/params were already restored (e.g., from JSON deserialization),
   * tell handlers to skip generation and only reinitialize the underlying
   * plugin instance. */
  const bool generate_new =
    get_input_ports ().empty () && get_output_ports ().empty ();
  Q_EMIT configurationChanged (configuration_.get (), generate_new);
}

// ============================================================================
// IProcessable Interface
// ============================================================================

void
Plugin::custom_prepare_for_processing (
  const dsp::graph::GraphNode * node,
  units::sample_rate_t          sample_rate,
  units::sample_u32_t           max_block_length)
{
  init_param_caches ();
  param_sync_.prepare (get_parameters ().size ());
  prepare_for_processing_impl (sample_rate, max_block_length);
  param_flush_timer_->start (std::chrono::milliseconds (20));
}

void
Plugin::custom_process_block (
  const dsp::graph::EngineProcessTimeInfo time_nfo,
  const dsp::ITransport                  &transport,
  const dsp::TempoMap                    &tempo_map) noexcept
{
  if (instantiation_failed_)
    return;

  if (!currently_enabled_rt ())
    {
      process_passthrough_impl (time_nfo, transport, tempo_map);
      return;
    }

  process_impl (time_nfo);
}

void
Plugin::custom_release_resources ()
{
  param_flush_timer_->stop ();
  param_sync_.entries.clear ();
  release_resources_impl ();
}

void
Plugin::process_passthrough_impl (
  const dsp::graph::EngineProcessTimeInfo time_nfo,
  const dsp::ITransport                  &transport,
  const dsp::TempoMap                    &tempo_map) noexcept
{
  // ProcessorBase's processing logic does passthrough
  dsp::ProcessorBase::custom_process_block (time_nfo, transport, tempo_map);
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

  bypass_param_rt_ = bypassParameter ();
}

std::string
Plugin::save_state () const
{
  return save_state_impl ();
}

void
Plugin::load_state (const std::string &base64_state)
{
  load_state_impl (base64_state);
}

void
Plugin::flush_plugin_values ()
{
  for (
    size_t i = 0;
    i < param_sync_.entries.size () && i < get_parameters ().size (); ++i)
    {
      auto &entry = param_sync_.entries[i];
      float val = entry.pending_value.exchange (-1.f, std::memory_order_acq_rel);
      if (val >= 0.f)
        {
          const auto &param_ref = get_parameters ()[i];
          auto * param = param_ref.get_object_as<dsp::ProcessorParameter> ();
          param->setBaseValue (val);
        }
    }
}
void
to_json (nlohmann::json &j, const Plugin &p)
{
  to_json (j, static_cast<const Plugin::UuidIdentifiableObject &> (p));
  to_json (j, static_cast<const dsp::ProcessorBase &> (p));
  j[Plugin::kConfigurationKey] = p.configuration_;
  if (p.program_index_.has_value ())
    {
      j[Plugin::kProgramIndexKey] = *p.program_index_;
    }
  j[Plugin::kProtocolKey] = p.get_protocol ();
  j[Plugin::kVisibleKey] = p.visible_;
}

void
from_json (const nlohmann::json &j, Plugin &p)
{
  from_json (j, static_cast<Plugin::UuidIdentifiableObject &> (p));
  from_json (j, static_cast<dsp::ProcessorBase &> (p));
  j.at (Plugin::kConfigurationKey).get_to (p.configuration_);
  if (j.contains (Plugin::kProgramIndexKey))
    {
      p.program_index_ = j[Plugin::kProgramIndexKey].get<int> ();
    }
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

Plugin::~Plugin () = default;

} // namespace zrythm::plugins
