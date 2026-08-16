// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <ranges>
#include <vector>

#include "dsp/audio_bus_configuration.h"
#include "utils/logger.h"
#include "utils/registry_utils.h"
#include "utils/views.h"

namespace zrythm::dsp
{

AudioBusReconcileResult
reconcile_audio_bus_configuration (
  utils::IObjectRegistry         &registry,
  ProcessorBase                  &processor,
  PortFlow                        flow,
  std::span<const AudioBusConfig> desired)
{
  const bool is_input = flow == PortFlow::Input;

  // Audio ports only; MIDI and CV ports are not counted and never touched
  const auto audio_ports = processor.get_all_audio_ports (flow);

  std::vector<bool>       matched (audio_ports.size (), false);
  AudioBusReconcileResult result{
    .graph_changed = false, .metadata_changed = false
  };

  const auto sync_port =
    [&result] (AudioPort &port, const AudioBusConfig &config) {
      if (port.arrangement () != config.arrangement)
        {
          port.set_arrangement (config.arrangement);
          result.graph_changed = true;
        }
      if (port.get_label () != config.name)
        {
          port.set_label (config.name);
          result.metadata_changed = true;
        }
      // Purpose feeds graph wiring, so a change needs a recalculation like
      // an arrangement change does
      if (port.purpose () != config.purpose)
        {
          port.set_purpose (config.purpose);
          result.graph_changed = true;
        }
      const bool want_detached = !config.active;
      if (port.detached () != want_detached)
        {
          port.set_detached (want_detached);
          result.graph_changed = true;
        }
      // An id-carrying port only ever matches the config carrying its id,
      // so the write below only ever fires when the port had no id: the
      // port adopts its config's id. The id is metadata: adopting one does
      // not affect buffers or the graph
      if (port.external_port_id () != config.external_id)
        {
          port.set_external_port_id (config.external_id);
          result.metadata_changed = true;
        }
    };

  // Claims the first unmatched port satisfying the predicate
  const auto claim = [&] (auto pred) -> AudioPort * {
    for (const auto &[i, port] : utils::views::enumerate (audio_ports))
      {
        if (!matched[i] && pred (*port))
          {
            matched[i] = true;
            return port;
          }
      }
    return nullptr;
  };

  bool any_id_matched = false;

  for (const auto &config : desired)
    {
      // Stable-id match re-associates the bus with its port even when a
      // removal shifted enumeration indices; the fallback claims ports that
      // carry no stable id (e.g. saved before ids were recorded)
      AudioPort * match =
        config.external_id.has_value ()
          ? claim ([&] (const AudioPort &port) {
              return port.external_port_id () == config.external_id;
            })
          : nullptr;
      if (match != nullptr && config.external_id.has_value ())
        {
          any_id_matched = true;
        }
      if (match == nullptr)
        {
          match = claim ([] (const AudioPort &port) {
            return !port.external_port_id ().has_value ();
          });
        }

      if (match != nullptr)
        {
          sync_port (*match, config);
          continue;
        }

      // A plugin regenerating its stable ids on each scan invalidates every
      // id it previously reported: no config matches an existing port by id
      // in such a pass, and the old id-carrying ports stay detached forever
      // while new ones accumulate in the project. An ordinary churn pass
      // (one bus swapped for another) still matches the surviving buses by
      // id, so it stays quiet
      if (config.external_id.has_value () && !any_id_matched)
        {
          const bool detached_ids_remain = std::ranges::any_of (
            utils::views::enumerate (audio_ports), [&] (const auto &indexed) {
              const auto &[i, existing] = indexed;
              return !matched[i] && existing->external_port_id ().has_value ();
            });
          if (detached_ids_remain)
            {
              z_warning (
                "Creating a port for bus '{}' (external id {}) while no "
                "reported id matched an existing port; if the plugin "
                "regenerates its stable ids on each scan, ports accumulate "
                "in the project",
                config.name, *config.external_id);
            }
        }

      // New bus: create a port (fresh UUID; detached when the bus is
      // inactive)
      auto port_ref = utils::create_object<AudioPort> (
        registry, config.name, flow, config.arrangement, config.purpose);
      auto * port = port_ref.get ();
      port->set_external_port_id (config.external_id);
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
      result.graph_changed = true;
    }

  // Buses removed: detach the ports nothing matched
  for (const auto &[i, port] : utils::views::enumerate (audio_ports))
    {
      if (!matched[i] && !port->detached ())
        {
          port->set_detached (true);
          result.graph_changed = true;
        }
    }

  return result;
}

} // namespace zrythm::dsp
