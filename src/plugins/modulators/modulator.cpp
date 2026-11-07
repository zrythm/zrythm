// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <cassert>
#include <ranges>

#include "plugins/modulators/engines/macro_engine.h"
#include "plugins/modulators/modulator.h"
#include "plugins/modulators/modulator_registry.h"
#include "plugins/modulators/modulator_runtime_context.h"
#include "utils/logger.h"
#include "utils/registry_utils.h"
#include "utils/utf8_string.h"

namespace zrythm::plugins::modulators
{

namespace
{

bool
parameter_matches (const Modulator &modulator, const ModulatorParameterSpec &spec)
{
  return std::ranges::any_of (
    modulator.get_parameters (), [&spec] (const auto &param_ref) {
      const auto * param = param_ref.get ();
      return param != nullptr
             && param->get_unique_id ()
                  == dsp::ProcessorParameter::UniqueId (spec.unique_id_);
    });
}

dsp::Port *
find_layout_port (
  const Modulator         &modulator,
  const ModulatorPortSpec &spec,
  dsp::PortFlow            flow)
{
  const auto ports =
    flow == dsp::PortFlow::Input
      ? modulator.get_all_input_ports ()
      : modulator.get_all_output_ports ();
  const auto found = std::ranges::find_if (ports, [&spec] (const auto &ref) {
    auto * port = ref.get ();
    return port != nullptr && port->get_symbol () == spec.symbol_;
  });
  return found != ports.end () ? found->get () : nullptr;
}

bool
port_matches (
  const Modulator         &modulator,
  const ModulatorPortSpec &spec,
  dsp::PortFlow            flow)
{
  auto * port = find_layout_port (modulator, spec, flow);
  if (port == nullptr || port->type () != spec.type_)
    return false;
  if (spec.type_ != dsp::PortType::Audio)
    return true;
  auto * audio_port = qobject_cast<dsp::AudioPort *> (port);
  return audio_port != nullptr
         && audio_port->arrangement () == spec.audio_arrangement_;
}

bool
layout_matches (const Modulator &modulator, const ModulatorLayout &layout)
{
  const auto parameters_match =
    std::ranges::all_of (layout.parameters_, [&modulator] (const auto &spec) {
      return parameter_matches (modulator, spec);
    });
  if (!parameters_match)
    return false;

  const auto inputs_match =
    std::ranges::all_of (layout.input_ports_, [&modulator] (const auto &spec) {
      return port_matches (modulator, spec, dsp::PortFlow::Input);
    });
  if (!inputs_match)
    return false;

  return std::ranges::all_of (
    layout.output_ports_, [&modulator] (const auto &spec) {
      return port_matches (modulator, spec, dsp::PortFlow::Output);
    });
}

} // namespace

struct Modulator::Impl
{
  std::unique_ptr<ModulatorEngine> engine_;
  ModulatorLayout                  layout_;
};

std::vector<std::string_view>
Modulator::available_descriptor_ids ()
{
  return modulator_registrations ()
         | std::views::transform (&ModulatorRegistration::descriptor_id_)
         | std::ranges::to<std::vector<std::string_view>> ();
}

std::unique_ptr<PluginDescriptor>
Modulator::make_descriptor (std::string_view descriptor_id)
{
  const auto * registration = find_modulator_registration (descriptor_id);
  return registration != nullptr ? registration->make_descriptor_ () : nullptr;
}

bool
Modulator::descriptor_is_native_modulator (const PluginDescriptor &descriptor)
{
  if (descriptor.protocol_ != Protocol::ProtocolType::Internal)
    return false;
  const auto * descriptor_id =
    std::get_if<utils::Utf8String> (&descriptor.path_or_id_);
  return descriptor_id != nullptr
         && find_modulator_registration (descriptor_id->view ()) != nullptr;
}

Modulator::Modulator (utils::IObjectRegistry &registry, QObject * parent)
    : InternalPluginBase (registry, parent)
{
}

Modulator::~Modulator () = default;

void
Modulator::on_configuration_changed (
  PluginConfiguration * configuration,
  bool                  generateNewPluginPortsAndParams)
{
  const auto * descriptor = configuration->descriptor ();
  assert (descriptor != nullptr);
  const auto * descriptor_id =
    std::get_if<utils::Utf8String> (&descriptor->path_or_id_);
  const auto * registration =
    descriptor_id != nullptr
      ? find_modulator_registration (descriptor_id->view ())
      : nullptr;
  if (registration == nullptr)
    {
      instantiation_failed_ = true;
      const auto error =
        QObject::tr ("%1 is not a registered native modulator")
          .arg (descriptor->name ());
      z_warning ("{}", utils::Utf8String::from_qstring (error));
      Q_EMIT instantiationFinished (false, error);
      return;
    }

  impl_ = std::make_unique<Impl> ();
  impl_->engine_ = registration->make_engine_ ();
  impl_->layout_ = impl_->engine_->layout ();

  if (generateNewPluginPortsAndParams)
    {
      const auto add_ports = [this] (const auto &specs, dsp::PortFlow flow) {
        for (const auto &spec : specs)
          {
            switch (spec.type_)
              {
              case dsp::PortType::Audio:
                {
                  auto port_ref = utils::create_object<dsp::AudioPort> (
                    registry (), spec.label_, flow, spec.audio_arrangement_);
                  port_ref.get ()->set_symbol (spec.symbol_);
                  if (flow == dsp::PortFlow::Input)
                    add_input_port (port_ref);
                  else
                    add_output_port (port_ref);
                  break;
                }
              case dsp::PortType::Midi:
                {
                  auto port_ref = utils::create_object<dsp::MidiPort> (
                    registry (), spec.label_, flow);
                  port_ref.get ()->set_symbol (spec.symbol_);
                  if (flow == dsp::PortFlow::Input)
                    add_input_port (port_ref);
                  else
                    add_output_port (port_ref);
                  break;
                }
              case dsp::PortType::CV:
                {
                  auto port_ref = utils::create_object<dsp::CVPort> (
                    registry (), spec.label_, flow);
                  port_ref.get ()->set_symbol (spec.symbol_);
                  if (flow == dsp::PortFlow::Input)
                    add_input_port (port_ref);
                  else
                    add_output_port (port_ref);
                  break;
                }
              }
          }
      };

      for (const auto &spec : impl_->layout_.parameters_)
        {
          add_parameter (
            utils::create_object<dsp::ProcessorParameter> (
              registry (), registry (),
              dsp::ProcessorParameter::UniqueId (spec.unique_id_), spec.range_,
              spec.label_));
        }
      add_ports (impl_->layout_.input_ports_, dsp::PortFlow::Input);
      add_ports (impl_->layout_.output_ports_, dsp::PortFlow::Output);
    }
  else if (!layout_matches (*this, impl_->layout_))
    {
      // Drop the engine so the failed modulator stays inert: the graph
      // still adds rack members that failed to instantiate, and
      // prepare/process only run when an engine is present
      impl_.reset ();
      instantiation_failed_ = true;
      const auto error =
        QObject::tr (
          "The saved %1 modulator layout does not match its descriptor")
          .arg (descriptor->name ());
      z_warning ("{}", utils::Utf8String::from_qstring (error));
      Q_EMIT instantiationFinished (false, error);
      return;
    }

  InternalPluginBase::on_configuration_changed (
    configuration, generateNewPluginPortsAndParams);
}

void
Modulator::prepare_plugin_for_processing (
  units::sample_rate_t sample_rate,
  units::sample_u32_t)
{
  if (impl_ == nullptr || impl_->engine_ == nullptr)
    return;

  assert (layout_matches (*this, impl_->layout_));
  ModulatorRuntimeContext context;
  context.sample_rate_ = sample_rate;
  for (const auto &spec : impl_->layout_.parameters_)
    {
      const auto found = std::ranges::find_if (
        get_parameters (), [&spec] (const auto &param_ref) {
          const auto * param = param_ref.get ();
          return param != nullptr
                 && param->get_unique_id ()
                      == dsp::ProcessorParameter::UniqueId (spec.unique_id_);
        });
      assert (found != get_parameters ().end ());
      context.parameters_.push_back (found->get ());
    }

  for (const auto &spec : impl_->layout_.input_ports_)
    {
      auto * port = find_layout_port (*this, spec, dsp::PortFlow::Input);
      assert (port != nullptr);
      switch (spec.type_)
        {
        case dsp::PortType::Audio:
          context.audio_inputs_.push_back (
            qobject_cast<dsp::AudioPort *> (port));
          break;
        case dsp::PortType::Midi:
          context.midi_inputs_.push_back (qobject_cast<dsp::MidiPort *> (port));
          break;
        case dsp::PortType::CV:
          context.cv_inputs_.push_back (qobject_cast<dsp::CVPort *> (port));
          break;
        }
    }
  for (const auto &spec : impl_->layout_.output_ports_)
    {
      assert (spec.type_ == dsp::PortType::CV);
      auto * port = find_layout_port (*this, spec, dsp::PortFlow::Output);
      assert (port != nullptr);
      context.cv_outputs_.push_back (qobject_cast<dsp::CVPort *> (port));
    }

  impl_->engine_->prepare (context);
}

void
Modulator::process_impl (
  dsp::graph::ProcessBlockInfo time_info,
  const dsp::ITransport       &transport,
  const dsp::TempoMap         &tempo_map) noexcept
{
  if (impl_ == nullptr || impl_->engine_ == nullptr)
    return;
  impl_->engine_->process (time_info, transport, tempo_map);
}

void
Modulator::release_resources_impl ()
{
  if (impl_ != nullptr && impl_->engine_ != nullptr)
    {
      impl_->engine_->release_resources ();
    }
}

} // namespace zrythm::plugins::modulators
