// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/audio_port.h"
#include "plugins/internal_plugin_base.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"

namespace zrythm::plugins
{

/**
 * @brief Effect test double with a main stereo input and a stereo
 * sidechain input.
 */
class SidechainTestPlugin : public plugins::InternalPluginBase
{
  Q_OBJECT

public:
  explicit SidechainTestPlugin (utils::IObjectRegistry &registry)
      : plugins::InternalPluginBase (registry)
  {
    auto descriptor = std::make_unique<plugins::PluginDescriptor> ();
    descriptor->name_ = u8"Sidechain Test";
    descriptor->protocol_ = plugins::Protocol::ProtocolType::Internal;
    plugins::PluginConfiguration config;
    config.descr_ = std::move (descriptor);
    set_configuration (config);

    add_input_port (
      utils::create_object<dsp::AudioPort> (
        registry, u8"Audio In", dsp::PortFlow::Input,
        dsp::SpeakerArrangement::stereo ()));
    add_input_port (
      utils::create_object<dsp::AudioPort> (
        registry, u8"Sidechain In", dsp::PortFlow::Input,
        dsp::SpeakerArrangement::stereo (), dsp::AudioPort::Purpose::Sidechain));
    add_output_port (
      utils::create_object<dsp::AudioPort> (
        registry, u8"Audio Out", dsp::PortFlow::Output,
        dsp::SpeakerArrangement::stereo ()));
  }

private:
  void process_impl (
    dsp::graph::ProcessBlockInfo,
    const dsp::ITransport &,
    const dsp::TempoMap &) noexcept override
  {
  }
};

} // namespace zrythm::plugins
