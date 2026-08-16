// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cstdio>
#include <string_view>

#include "clap_fixture_factory.h"

namespace zrythm_test_plugins
{

/**
 * Plugin that asks the host to rescan audio ports without implementing the
 * audio ports extension, for testing that the host rejects the request
 * instead of dereferencing a missing extension.
 *
 * Has a single MIDI output so that hosts requiring at least one port can
 * still instantiate it.
 */
class TestRescanMisuseClap final : public ClapFixturePluginBase
{
public:
  explicit TestRescanMisuseClap (const clap_host * host)
      : ClapFixturePluginBase (descriptor (), host)
  {
  }

  static const clap_plugin_descriptor * descriptor ()
  {
    static constexpr const char * const features[] = {
      CLAP_PLUGIN_FEATURE_UTILITY, nullptr
    };
    static const clap_plugin_descriptor desc = {
      .clap_version = CLAP_VERSION,
      .id = kPluginId.data (),
      .name = kPluginName.data (),
      .vendor = "Zrythm",
      .url = "https://zrythm.org",
      .manual_url = "https://manual.zrythm.org",
      .support_url = "https://gitlab.zrythm.org/zrythm/zrythm/-/issues",
      .version = "1.0.0",
      .description =
        "Requests an audio ports rescan without implementing audio ports",
      .features = features,
    };
    return &desc;
  }

  // string_view of a literal - .data() is null-terminated, as the CLAP ABI
  // expects
  static constexpr std::string_view kPluginId = "org.zrythm.TestRescanMisuse";
  static constexpr std::string_view kPluginName = "Test Rescan Misuse";

  // note ports
  bool     implementsNotePorts () const noexcept override { return true; }
  uint32_t notePortsCount (bool isInput) const noexcept override
  {
    return isInput ? 0 : 1;
  }
  bool notePortsInfo (uint32_t index, bool isInput, clap_note_port_info * info)
    const noexcept override
  {
    if (isInput || index != 0)
      return false;
    info->id = 0;
    info->supported_dialects = CLAP_NOTE_DIALECT_MIDI;
    info->preferred_dialect = CLAP_NOTE_DIALECT_MIDI;
    std::snprintf (info->name, sizeof (info->name), "%s", "MIDI Out");
    return true;
  }

  bool activate (
    double   sampleRate,
    uint32_t minFrameCount,
    uint32_t maxFrameCount) noexcept override
  {
    // Spec violation on purpose: this plugin does not implement the audio
    // ports extension, so it may not ask for an audio ports rescan
    _host.audioPortsRescan (CLAP_AUDIO_PORTS_RESCAN_LIST);
    return ClapFixturePluginBase::activate (
      sampleRate, minFrameCount, maxFrameCount);
  }

  clap_process_status process (const clap_process *) noexcept override
  {
    return CLAP_PROCESS_CONTINUE;
  }
};

} // namespace zrythm_test_plugins

extern "C" {
CLAP_EXPORT const clap_plugin_entry clap_entry = zrythm_test_plugins::
  clap_fixture_entry<zrythm_test_plugins::TestRescanMisuseClap>;
}
