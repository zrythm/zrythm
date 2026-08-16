// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * Plugin that reports two output buses with the same stable id, which makes
 * its port report untrustworthy, for testing that the host refuses to
 * activate it — and keeps refusing it on repeated prepares.
 *
 * Has a single MIDI output so that hosts requiring at least one port can
 * still instantiate it.
 */

#include <cstdio>
#include <string_view>

#include "clap_fixture_factory.h"

namespace zrythm_test_plugins
{

class TestBadPortsClap final : public ClapFixturePluginBase
{
public:
  explicit TestBadPortsClap (const clap_host * host)
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
      .description = "Reports duplicate audio port ids",
      .features = features,
    };
    return &desc;
  }

  // string_view of a literal - .data() is null-terminated, as the CLAP ABI
  // expects
  static constexpr std::string_view kPluginId = "org.zrythm.TestBadPorts";
  static constexpr std::string_view kPluginName = "Test Bad Ports";

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
    std::snprintf (info->name, sizeof (info->name), "%s", "Event Out");
    info->supported_dialects = CLAP_NOTE_DIALECT_MIDI;
    info->preferred_dialect = CLAP_NOTE_DIALECT_MIDI;
    return true;
  }

  // audio ports: two output buses, both with id 0 — a report the host must
  // not trust
  bool     implementsAudioPorts () const noexcept override { return true; }
  uint32_t audioPortsCount (bool isInput) const noexcept override
  {
    return isInput ? 0 : 2;
  }
  bool
  audioPortsInfo (uint32_t index, bool isInput, clap_audio_port_info * info)
    const noexcept override
  {
    if (isInput || index >= 2)
      return false;
    info->id = 0;
    std::snprintf (
      info->name, sizeof (info->name), "Out %u",
      static_cast<unsigned> (index + 1));
    info->channel_count = 2;
    info->flags = index == 0 ? CLAP_AUDIO_PORT_IS_MAIN : 0;
    info->port_type = CLAP_PORT_STEREO;
    info->in_place_pair = CLAP_INVALID_ID;
    return true;
  }
};

} // namespace zrythm_test_plugins

extern "C" {
CLAP_EXPORT const clap_plugin_entry clap_entry =
  zrythm_test_plugins::clap_fixture_entry<zrythm_test_plugins::TestBadPortsClap>;
}
