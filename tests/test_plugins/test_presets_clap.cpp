// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <array>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

#include "clap_fixture_factory.h"
#include <clap/ext/preset-load.h>
#include <nlohmann/json.hpp>

namespace zrythm_test_plugins
{

/**
 * Fixture exercising the clap.preset-load and preset-discovery
 * interfaces.
 *
 * Embedded presets (location kind PLUGIN, keyed by load_key):
 * - embedded-a (level 0.1), embedded-b (0.2), embedded-noid (0.15) and
 *   embedded-broken apply to the fixture plugin
 * - embedded-other declares a different plugin id and must be filtered
 *   out by the host
 * - embedded-hijack applies embedded-b's value and reports
 *   embedded-b as loaded, standing in for a plugin that switches
 *   presets from its own browser
 * - embedded-ghost applies a level and reports a load key that is not
 *   in the preset list
 * - embedded-broken-hijack reports embedded-b as loaded and then fails
 *   the load it was asked to perform
 *
 * FILE-kind locations are read from the directory named by the
 * ZRYTHM_TEST_PRESET_DIR environment variable; preset files are JSON
 * containers of the form
 * {"presets":[{"name":...,"key":...,"level":...,"pluginId":...?}]}.
 */
class TestPresetsClap final : public ClapFixturePluginBase
{
public:
  static constexpr clap_id kLevelParamId = 0;

  explicit TestPresetsClap (const clap_host * host)
      : ClapFixturePluginBase (descriptor (), host)
  {
  }

  static const clap_plugin_descriptor * descriptor ()
  {
    static constexpr const char * const features[] = {
      CLAP_PLUGIN_FEATURE_AUDIO_EFFECT, CLAP_PLUGIN_FEATURE_STEREO, nullptr
    };
    static const clap_plugin_descriptor desc = {
      .clap_version = CLAP_VERSION,
      .id = "org.zrythm.TestPresets",
      .name = "Test Presets",
      .vendor = "Zrythm",
      .url = "https://zrythm.org",
      .manual_url = "https://manual.zrythm.org",
      .support_url = "https://gitlab.zrythm.org/zrythm/zrythm/-/issues",
      .version = "1.0.0",
      .description = "Preset discovery and loading fixture",
      .features = features,
    };
    return &desc;
  }

  // audio ports
  bool     implementsAudioPorts () const noexcept override { return true; }
  uint32_t audioPortsCount (bool isInput) const noexcept override { return 1; }
  bool
  audioPortsInfo (uint32_t index, bool isInput, clap_audio_port_info * info)
    const noexcept override
  {
    if (index != 0)
      return false;
    info->id = 0;
    std::snprintf (
      info->name, sizeof (info->name), "%s", isInput ? "Input" : "Output");
    info->channel_count = 2;
    info->flags = CLAP_AUDIO_PORT_IS_MAIN;
    info->port_type = CLAP_PORT_STEREO;
    info->in_place_pair = CLAP_INVALID_ID;
    return true;
  }

  // params
  bool     implementsParams () const noexcept override { return true; }
  uint32_t paramsCount () const noexcept override { return 1; }
  bool
  paramsInfo (uint32_t paramIndex, clap_param_info * info) const noexcept override
  {
    if (paramIndex != 0)
      return false;
    info->id = kLevelParamId;
    info->flags = CLAP_PARAM_IS_AUTOMATABLE;
    info->cookie = nullptr;
    std::snprintf (info->name, sizeof (info->name), "%s", "Level");
    info->module[0] = '\0';
    info->min_value = 0.0;
    info->max_value = 1.0;
    info->default_value = 1.0;
    return true;
  }
  bool paramsValue (clap_id paramId, double * value) noexcept override
  {
    if (paramId != kLevelParamId)
      return false;
    *value = level_.load ();
    return true;
  }
  void paramsFlush (
    const clap_input_events * in,
    const clap_output_events * /*out*/) noexcept override
  {
    const auto num_events = in->size (in);
    for (uint32_t i = 0; i < num_events; ++i)
      {
        const auto * header = in->get (in, i);
        if (
          header->space_id == CLAP_CORE_EVENT_SPACE_ID
          && header->type == CLAP_EVENT_PARAM_VALUE)
          {
            const auto * ev =
              reinterpret_cast<const clap_event_param_value *> (header);
            if (ev->param_id == kLevelParamId)
              level_.store (ev->value);
          }
      }
  }

  // state
  bool implementsState () const noexcept override { return true; }
  bool stateSave (const clap_ostream * stream) noexcept override
  {
    const nlohmann::json j{
      { "level", level_.load () }
    };
    const auto json_text = j.dump ();
    return stream->write (stream, json_text.data (), json_text.size ())
           == static_cast<int64_t> (json_text.size ());
  }
  bool stateLoad (const clap_istream * stream) noexcept override
  {
    std::string           json_text;
    std::array<char, 256> chunk{};
    while (true)
      {
        const auto bytes = stream->read (stream, chunk.data (), chunk.size ());
        if (bytes <= 0)
          break;
        json_text.append (chunk.data (), static_cast<size_t> (bytes));
      }
    const auto j = nlohmann::json::parse (json_text, nullptr, false);
    if (j.is_discarded () || !j.contains ("level"))
      return false;
    level_.store (j["level"].get<double> ());
    return true;
  }

  // preset load
  bool implementsPresetLoad () const noexcept override { return true; }
  bool presetLoadFromLocation (
    uint32_t     location_kind,
    const char * location,
    const char * load_key) noexcept override
  {
    if (load_key == nullptr)
      return false;

    if (std::strcmp (load_key, "embedded-broken-hijack") == 0)
      {
        // Reports a preset as loaded and then fails the load it was
        // asked to perform
        report_loaded_to_host (location_kind, location, "embedded-b");
        return false;
      }

    double level = 0.0;
    // What to report to the host as loaded (hijack-style keys report
    // another preset, like a plugin-side browser switch)
    const char * reported_key = load_key;
    switch (location_kind)
      {
      case CLAP_PRESET_DISCOVERY_LOCATION_PLUGIN:
        if (std::strcmp (load_key, "embedded-a") == 0)
          level = 0.1;
        else if (std::strcmp (load_key, "embedded-b") == 0)
          level = 0.2;
        else if (std::strcmp (load_key, "embedded-noid") == 0)
          level = 0.15;
        else if (std::strcmp (load_key, "embedded-hijack") == 0)
          {
            level = 0.2;
            reported_key = "embedded-b";
          }
        else if (std::strcmp (load_key, "embedded-ghost") == 0)
          {
            level = 0.25;
            reported_key = "embedded-unlisted";
          }
        else
          return false;
        break;

      case CLAP_PRESET_DISCOVERY_LOCATION_FILE:
        {
          if (location == nullptr)
            return false;
          std::ifstream file (location);
          if (!file.is_open ())
            return false;
          std::stringstream buffer;
          buffer << file.rdbuf ();
          const auto j = nlohmann::json::parse (buffer.str (), nullptr, false);
          if (j.is_discarded () || !j.contains ("presets"))
            return false;
          bool found = false;
          for (const auto &preset : j["presets"])
            {
              if (
                preset.contains ("key") && preset.contains ("level")
                && preset["key"].get<std::string> () == load_key)
                {
                  level = preset["level"].get<double> ();
                  found = true;
                  break;
                }
            }
          if (!found)
            return false;
          break;
        }

      default:
        return false;
      }

    level_.store (level);

    report_loaded_to_host (location_kind, location, reported_key);
    return true;
  }

private:
  /** Reports a preset load to the host's preset-load extension. */
  void report_loaded_to_host (
    uint32_t     location_kind,
    const char * location,
    const char * load_key) const noexcept
  {
    if (const auto * host = _host.host ())
      {
        if (
          const auto * host_preset_load =
            static_cast<const clap_host_preset_load *> (
              host->get_extension (host, CLAP_EXT_PRESET_LOAD)))
          {
            host_preset_load->loaded (host, location_kind, location, load_key);
          }
      }
  }

  std::atomic<double> level_{ 1.0 };
};

// ---------------------------------------------------------------------------
// Preset discovery provider
// ---------------------------------------------------------------------------

namespace
{

struct ProviderState
{
  const clap_preset_discovery_indexer * indexer = nullptr;
};

// Global so the static factory's create() can reach it; single-crawl
// at a time by design (the host drives providers sequentially)
ProviderState provider_state;

const clap_universal_plugin_id_t
  kThisPluginId = { .abi = "clap", .id = "org.zrythm.TestPresets" };

const clap_universal_plugin_id_t
  kOtherPluginId = { .abi = "clap", .id = "org.other.thing" };

bool
begin_preset (
  const clap_preset_discovery_metadata_receiver * receiver,
  const char *                                    name,
  const char *                                    load_key)
{
  return receiver->begin_preset (receiver, name, load_key);
}

void
add_plugin_id (
  const clap_preset_discovery_metadata_receiver * receiver,
  const clap_universal_plugin_id_t *              id)
{
  receiver->add_plugin_id (receiver, id);
}

bool
report_embedded_presets (
  const clap_preset_discovery_metadata_receiver * receiver)
{
  if (!begin_preset (receiver, "Embedded A", "embedded-a"))
    return false;
  add_plugin_id (receiver, &kThisPluginId);

  if (!begin_preset (receiver, "Embedded B", "embedded-b"))
    return false;
  add_plugin_id (receiver, &kThisPluginId);

  if (!begin_preset (receiver, "Embedded Broken", "embedded-broken"))
    return false;
  add_plugin_id (receiver, &kThisPluginId);

  if (
    !begin_preset (receiver, "Embedded Broken Hijack", "embedded-broken-hijack"))
    return false;
  add_plugin_id (receiver, &kThisPluginId);

  if (!begin_preset (receiver, "Embedded Ghost", "embedded-ghost"))
    return false;
  add_plugin_id (receiver, &kThisPluginId);

  if (!begin_preset (receiver, "Embedded Hijack", "embedded-hijack"))
    return false;
  add_plugin_id (receiver, &kThisPluginId);

  if (!begin_preset (receiver, "Embedded For Other", "embedded-other"))
    return false;
  add_plugin_id (receiver, &kOtherPluginId);

  if (!begin_preset (receiver, "Embedded No Id", "embedded-noid"))
    return false;

  return true;
}

bool
report_file_presets (
  const clap_preset_discovery_metadata_receiver * receiver,
  const char *                                    location)
{
  std::ifstream file (location);
  if (!file.is_open ())
    return false;
  std::stringstream buffer;
  buffer << file.rdbuf ();
  const auto j = nlohmann::json::parse (buffer.str (), nullptr, false);
  if (j.is_discarded () || !j.contains ("presets"))
    return false;
  for (const auto &preset : j["presets"])
    {
      if (!preset.contains ("name") || !preset.contains ("key"))
        continue;
      const auto name = preset["name"].get<std::string> ();
      const auto key = preset["key"].get<std::string> ();
      if (!begin_preset (receiver, name.c_str (), key.c_str ()))
        return false;
      if (preset.contains ("pluginId"))
        {
          const auto plugin_id = preset["pluginId"].get<std::string> ();
          const clap_universal_plugin_id_t id = {
            .abi = "clap", .id = plugin_id.c_str ()
          };
          add_plugin_id (receiver, &id);
        }
    }
  return true;
}

const clap_preset_discovery_provider_descriptor_t kProviderDescriptor = {
  .clap_version = CLAP_VERSION,
  .id = "org.zrythm.TestPresets.provider",
  .name = "Test Presets Provider",
  .vendor = "Zrythm",
};

const clap_preset_discovery_filetype_t kPresetFiletype = {
  .name = "Test Preset Container",
  .description = "JSON preset container",
  .file_extension = "zpreset",
};

const clap_preset_discovery_location_t kEmbeddedLocation = {
  .flags = CLAP_PRESET_DISCOVERY_IS_FACTORY_CONTENT,
  .name = "Embedded",
  .kind = CLAP_PRESET_DISCOVERY_LOCATION_PLUGIN,
  .location = nullptr,
};

const clap_preset_discovery_provider_t kPresetProvider = {
  .desc = &kProviderDescriptor,
  .provider_data = &provider_state,
  .init = +[] (const clap_preset_discovery_provider * provider) -> bool {
    const auto * state =
      static_cast<const ProviderState *> (provider->provider_data);
    const auto * indexer = state->indexer;
    if (indexer == nullptr)
      return false;
    if (!indexer->declare_filetype (indexer, &kPresetFiletype))
      return false;
    if (!indexer->declare_location (indexer, &kEmbeddedLocation))
      return false;

    // The FILE location is only declared when the test pointed the
    // provider at a directory
    if (const char * dir = std::getenv ("ZRYTHM_TEST_PRESET_DIR"))
      {
        static std::string file_location_dir;
        file_location_dir = dir;
        const clap_preset_discovery_location_t file_location = {
          .flags = CLAP_PRESET_DISCOVERY_IS_FACTORY_CONTENT,
          .name = "Factory Files",
          .kind = CLAP_PRESET_DISCOVERY_LOCATION_FILE,
          .location = file_location_dir.c_str (),
        };
        if (!indexer->declare_location (indexer, &file_location))
          return false;
      }
    return true;
  },
  .destroy = +[] (const clap_preset_discovery_provider *) { },
  .get_metadata =
    +[] (
       const clap_preset_discovery_provider *,
       uint32_t                                        location_kind,
       const char *                                    location,
       const clap_preset_discovery_metadata_receiver * receiver) -> bool {
    if (location_kind == CLAP_PRESET_DISCOVERY_LOCATION_PLUGIN)
      return report_embedded_presets (receiver);
    if (location_kind == CLAP_PRESET_DISCOVERY_LOCATION_FILE)
      return report_file_presets (receiver, location);
    return false;
  },
  .get_extension = +[] (const clap_preset_discovery_provider *, const char *)
    -> const void * { return nullptr; },
};

const clap_preset_discovery_factory_t kPresetDiscoveryFactory = {
  .count = +[] (const clap_preset_discovery_factory *) -> uint32_t { return 1; },
  .get_descriptor = +[] (const clap_preset_discovery_factory *, uint32_t index)
    -> const clap_preset_discovery_provider_descriptor_t * {
    return index == 0 ? &kProviderDescriptor : nullptr;
  },
  .create =
    +[] (
       const clap_preset_discovery_factory *,
       const clap_preset_discovery_indexer * indexer,
       const char * provider_id) -> const clap_preset_discovery_provider_t * {
    if (std::strcmp (provider_id, kProviderDescriptor.id) != 0)
      return nullptr;
    provider_state.indexer = indexer;
    return &kPresetProvider;
  },
};

} // namespace

} // namespace zrythm_test_plugins

extern "C" {
CLAP_EXPORT const clap_plugin_entry clap_entry = {
  .clap_version = CLAP_VERSION,
  .init = [] (const char *) -> bool { return true; },
  .deinit = [] () { },
  .get_factory = [] (const char * factory_id) -> const void * {
    if (std::strcmp (factory_id, CLAP_PLUGIN_FACTORY_ID) == 0)
      {
        return static_cast<const void *> (
          &zrythm_test_plugins::clap_fixture_factory<
            zrythm_test_plugins::TestPresetsClap>);
      }
    if (std::strcmp (factory_id, CLAP_PRESET_DISCOVERY_FACTORY_ID) == 0)
      {
        return static_cast<const void *> (
          &zrythm_test_plugins::kPresetDiscoveryFactory);
      }
    return nullptr;
  },
};
}
