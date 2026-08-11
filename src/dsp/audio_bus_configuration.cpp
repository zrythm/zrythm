// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <ranges>
#include <vector>

#include "dsp/audio_bus_configuration.h"
#include "utils/registry_utils.h"
#include "utils/views.h"

namespace zrythm::dsp
{

bool
reconcile_audio_bus_configuration (
  utils::IObjectRegistry         &registry,
  ProcessorBase                  &processor,
  PortFlow                        flow,
  std::span<const AudioBusConfig> desired)
{
  const bool  is_input = flow == PortFlow::Input;
  const auto &all_ports =
    is_input
      ? processor.get_all_input_ports ()
      : processor.get_all_output_ports ();

  // Bus index = position among the audio ports
  const auto audio_ports =
    all_ports | std::views::transform (&dsp::PortUuidReference::get)
    | utils::views::qobject_cast_and_filter<AudioPort>
    | std::ranges::to<std::vector> ();

  bool changed = false;

  for (const auto &[port, config] : std::views::zip (audio_ports, desired))
    {
      if (port->arrangement () != config.arrangement)
        {
          port->set_arrangement (config.arrangement);
          changed = true;
        }
      if (port->get_label () != config.name)
        {
          port->set_label (config.name);
          changed = true;
        }
      const bool want_detached = !config.active;
      if (port->detached () != want_detached)
        {
          port->set_detached (want_detached);
          changed = true;
        }
    }

  // Buses removed: detach the remaining ports
  for (
    auto * port : audio_ports | std::views::drop (std::ranges::ssize (desired)))
    {
      if (!port->detached ())
        {
          port->set_detached (true);
          changed = true;
        }
    }

  // Buses added: create new ports
  for (
    const auto &config :
    desired | std::views::drop (std::ranges::ssize (audio_ports)))
    {
      auto port_ref = utils::create_object<AudioPort> (
        registry, config.name, flow, config.arrangement, config.purpose);
      auto * port = port_ref.get ();
      if (!config.active)
        {
          port->set_detached (true);
        }
      if (is_input)
        {
          processor.add_input_port (port_ref);
        }
      else
        {
          processor.add_output_port (port_ref);
        }
      changed = true;
    }

  return changed;
}

} // namespace zrythm::dsp
