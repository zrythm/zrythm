// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cstring>

#include <clap/all.h>
#include <clap/helpers/plugin.hh>
#include <clap/helpers/plugin.hxx>

namespace zrythm_test_plugins
{

using ClapPluginBase = clap::helpers::Plugin<
  clap::helpers::MisbehaviourHandler::Terminate,
  clap::helpers::CheckingLevel::Maximal>;

class TestLatencyClap final : public ClapPluginBase
{
public:
  static constexpr uint32_t kLatencySamples = 256;

  explicit TestLatencyClap (const clap_host * host)
      : ClapPluginBase (descriptor (), host)
  {
  }

  static const clap_plugin_descriptor * descriptor ()
  {
    static constexpr const char * const features[] = {
      CLAP_PLUGIN_FEATURE_AUDIO_EFFECT, CLAP_PLUGIN_FEATURE_STEREO, nullptr
    };
    static const clap_plugin_descriptor desc = {
      .clap_version = CLAP_VERSION,
      .id = "org.zrythm.TestLatency",
      .name = "Test Latency",
      .vendor = "Zrythm",
      .url = "https://zrythm.org",
      .manual_url = "https://manual.zrythm.org",
      .support_url = "https://gitlab.zrythm.org/zrythm/zrythm/-/issues",
      .version = "1.0.0",
      .description = "Latency-reporting plugin used as a test fixture",
      .features = features,
    };
    return &desc;
  }

  static const clap_plugin * createInstance (const clap_host * host) noexcept
  {
    auto * p = new TestLatencyClap (host);
    return p->clapPlugin ();
  }

  bool activate (double, uint32_t, uint32_t) noexcept override
  {
    // Report the latency from within activate(), as plug-ins are allowed
    // to per the latency extension contract ([main-thread &
    // being-activated])
    if (_host.canUseLatency ())
      _host.latencyChanged ();
    return true;
  }

  // latency
  bool     implementsLatency () const noexcept override { return true; }
  uint32_t latencyGet () const noexcept override { return kLatencySamples; }

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

  clap_process_status process (const clap_process *) noexcept override
  {
    return CLAP_PROCESS_CONTINUE;
  }
};

static const clap_plugin_factory plugin_factory = {
  .get_plugin_count = [] (const clap_plugin_factory *) -> uint32_t { return 1; },
  .get_plugin_descriptor = [] (const clap_plugin_factory *, uint32_t index)
    -> const clap_plugin_descriptor * {
    return index == 0 ? TestLatencyClap::descriptor () : nullptr;
  },
  .create_plugin =
    [] (const clap_plugin_factory *, const clap_host * host, const char * plugin_id)
    -> const clap_plugin * {
    if (host == nullptr || !clap_version_is_compatible (host->clap_version))
      return nullptr;
    if (std::strcmp (plugin_id, TestLatencyClap::descriptor ()->id) != 0)
      return nullptr;
    return TestLatencyClap::createInstance (host);
  },
};

} // namespace zrythm_test_plugins

extern "C" {
CLAP_EXPORT const clap_plugin_entry clap_entry = {
  .clap_version = CLAP_VERSION,
  .init = [] (const char *) -> bool { return true; },
  .deinit = [] () { },
  .get_factory = [] (const char * factory_id) -> const void * {
    return std::strcmp (factory_id, CLAP_PLUGIN_FACTORY_ID) == 0
             ? static_cast<const void *> (&zrythm_test_plugins::plugin_factory)
             : nullptr;
  },
};
}
