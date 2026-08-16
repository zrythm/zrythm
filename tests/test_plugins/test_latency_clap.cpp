// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "clap_fixture_factory.h"

namespace zrythm_test_plugins
{

class TestLatencyClap final : public ClapFixturePluginBase
{
public:
  static constexpr uint32_t kLatencySamples = 256;

  explicit TestLatencyClap (const clap_host * host)
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

} // namespace zrythm_test_plugins

extern "C" {
CLAP_EXPORT const clap_plugin_entry clap_entry =
  zrythm_test_plugins::clap_fixture_entry<zrythm_test_plugins::TestLatencyClap>;
}
