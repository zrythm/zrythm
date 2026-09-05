// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <ranges>
#include <string_view>
#include <vector>

#include "plugins/lv2_discovery.h"
#include "plugins/lv2_world.h"

#include <fmt/format.h>
#include <lv2/atom/atom.h>
#include <lv2/core/lv2.h>
#include <lv2/midi/midi.h>

namespace zrythm::plugins
{

namespace
{
/** Returns the fragment part of a class URI, e.g. "InstrumentPlugin". */
utils::Utf8String
class_uri_tail (const LilvPluginClass * plugin_class)
{
  const auto uri = node_to_utf8 (lilv_plugin_class_get_uri (plugin_class));
  const auto str = uri.view ();
  const auto hash_pos = str.find_last_of ('#');
  return hash_pos == std::string_view::npos
           ? uri
           : utils::Utf8String::from_utf8_encoded_string (
               str.substr (hash_pos + 1));
}
} // namespace

std::vector<Lv2PluginInfo>
get_plugins_in_bundle (Lv2World &world, const std::filesystem::path &bundle_dir)
{
  auto * lilv_world = world.raw ();
  // (bundle_dir / "") forces a trailing slash: lilv requires directories
  // to be passed with one
  const LilvNodeUPtr bundle_uri{ lilv_new_file_uri (
    lilv_world, nullptr, (bundle_dir / "").string ().c_str ()) };
  if (bundle_uri == nullptr)
    {
      z_warning ("Failed to create file URI for LV2 bundle '{}'", bundle_dir);
      return {};
    }

  lilv_world_load_bundle (lilv_world, bundle_uri.get ());

  const LilvNodeUPtr input_port{
    lilv_new_uri (lilv_world, LV2_CORE__InputPort)
  };
  const LilvNodeUPtr audio_port{
    lilv_new_uri (lilv_world, LV2_CORE__AudioPort)
  };
  const LilvNodeUPtr control_port{
    lilv_new_uri (lilv_world, LV2_CORE__ControlPort)
  };
  const LilvNodeUPtr cv_port{ lilv_new_uri (lilv_world, LV2_CORE__CVPort) };
  const LilvNodeUPtr atom_port{ lilv_new_uri (lilv_world, LV2_ATOM__AtomPort) };
  const LilvNodeUPtr midi_event{
    lilv_new_uri (lilv_world, LV2_MIDI__MidiEvent)
  };
  const LilvNodeUPtr minor_version{
    lilv_new_uri (lilv_world, LV2_CORE__minorVersion)
  };
  const LilvNodeUPtr micro_version{
    lilv_new_uri (lilv_world, LV2_CORE__microVersion)
  };

  const auto * all_plugins = lilv_world_get_all_plugins (lilv_world);

  std::vector<Lv2PluginInfo> result;
  LILV_FOREACH (plugins, iter, all_plugins)
    {
      const auto * plugin = lilv_plugins_get (all_plugins, iter);

      if (!lilv_node_equals (
            lilv_plugin_get_bundle_uri (plugin), bundle_uri.get ()))
        continue;

      auto &info = result.emplace_back ();
      info.uri_ = node_to_utf8 (lilv_plugin_get_uri (plugin));

      if (
        const LilvNodeUPtr name{ lilv_plugin_get_name (plugin) };
        name != nullptr)
        {
          info.name_ = node_to_utf8 (name.get ());
        }
      if (
        const LilvNodeUPtr author{ lilv_plugin_get_author_name (plugin) };
        author != nullptr)
        {
          info.author_ = node_to_utf8 (author.get ());
        }
      const LilvNodesUPtr minor{
        lilv_plugin_get_value (plugin, minor_version.get ())
      };
      const LilvNodesUPtr micro{
        lilv_plugin_get_value (plugin, micro_version.get ())
      };
      if (minor != nullptr && micro != nullptr)
        {
          info.version_ = utils::Utf8String::from_utf8_encoded_string (
            fmt::format (
              "{}.{}", lilv_node_as_int (lilv_nodes_get_first (minor.get ())),
              lilv_node_as_int (lilv_nodes_get_first (micro.get ()))));
        }

      const auto * plugin_class = lilv_plugin_get_class (plugin);
      auto         class_tail = class_uri_tail (plugin_class);
      info.is_instrument_ = class_tail == "InstrumentPlugin";
      // the base class means the plugin declared no more specific class
      if (class_tail != "Plugin")
        info.category_str_ = std::move (class_tail);

      // Classify ports by type and direction
      const auto num_ports = lilv_plugin_get_num_ports (plugin);
      for (const auto port_index : std::views::iota (0u, num_ports))
        {
          const auto * port = lilv_plugin_get_port_by_index (plugin, port_index);
          const bool is_input = lilv_port_is_a (plugin, port, input_port.get ());

          if (lilv_port_is_a (plugin, port, audio_port.get ()))
            {
              ++(is_input ? info.num_audio_ins_ : info.num_audio_outs_);
            }
          else if (lilv_port_is_a (plugin, port, control_port.get ()))
            {
              ++(is_input ? info.num_ctrl_ins_ : info.num_ctrl_outs_);
            }
          else if (lilv_port_is_a (plugin, port, cv_port.get ()))
            {
              ++(is_input ? info.num_cv_ins_ : info.num_cv_outs_);
            }
          else if (
            lilv_port_is_a (plugin, port, atom_port.get ())
            && lilv_port_supports_event (plugin, port, midi_event.get ()))
            {
              // only atom ports announcing midi:MidiEvent are counted as
              // MIDI; ports using the legacy lv2:event extension or other
              // event types are left uncounted
              ++(is_input ? info.num_midi_ins_ : info.num_midi_outs_);
            }
        }

      const LilvUIsUPtr uis{ lilv_plugin_get_uis (plugin) };
      info.has_custom_ui_ = uis != nullptr && lilv_uis_size (uis.get ()) > 0;
    }

  return result;
}

} // namespace zrythm::plugins
