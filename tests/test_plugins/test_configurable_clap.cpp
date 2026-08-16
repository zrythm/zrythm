// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <array>
#include <cstdio>
#include <ranges>
#include <string_view>

#include "clap_fixture_factory.h"

namespace zrythm_test_plugins
{

/**
 * Effect whose main output bus can be reconfigured between stereo and 5.1
 * via the configurable audio ports extension, for testing the host's
 * load-time configuration push.
 *
 * Output layout: bus 0 is the main output (stereo, 5.1, or discrete with an
 * arbitrary channel count — starting as 5.1), buses 1 and 2 are plain stereo
 * side outputs. The bus changes the plugin accepts are validated in
 * canApplyConfiguration, including reading port_details for surround and
 * ambisonic requests. The 5.1 wire order is deliberately non-canonical (FC
 * first), and every output channel is filled with a value identifying its
 * wire index, so hosts can verify their wire-order permutation.
 */
class TestConfigurableClap final : public ClapFixturePluginBase
{
public:
  explicit TestConfigurableClap (const clap_host * host)
      : ClapFixturePluginBase (descriptor (), host)
  {
  }

  static const clap_plugin_descriptor * descriptor ()
  {
    static constexpr const char * const features[] = {
      CLAP_PLUGIN_FEATURE_AUDIO_EFFECT, CLAP_PLUGIN_FEATURE_SURROUND, nullptr
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
      .description = "Effect with a configurable main output bus",
      .features = features,
    };
    return &desc;
  }

  // string_view of a literal - .data() is null-terminated, as the CLAP ABI
  // expects
  static constexpr std::string_view kPluginId = "org.zrythm.TestConfigurable";
  static constexpr std::string_view kPluginName = "Test Configurable";

  // audio ports
  bool     implementsAudioPorts () const noexcept override { return true; }
  uint32_t audioPortsCount (bool isInput) const noexcept override
  {
    return isInput ? 1 : 3;
  }
  bool
  audioPortsInfo (uint32_t index, bool isInput, clap_audio_port_info * info)
    const noexcept override
  {
    if (isInput)
      {
        if (index != 0)
          return false;
        info->id = 0;
        std::snprintf (info->name, sizeof (info->name), "%s", "Input");
        info->channel_count = 2;
        info->flags = CLAP_AUDIO_PORT_IS_MAIN;
        info->port_type = CLAP_PORT_STEREO;
        info->in_place_pair = CLAP_INVALID_ID;
        return true;
      }
    if (index >= 3)
      return false;
    info->id = index;
    if (index == 0)
      {
        std::snprintf (info->name, sizeof (info->name), "%s", "Main Out");
        info->flags = CLAP_AUDIO_PORT_IS_MAIN;
        switch (main_out_mode_)
          {
          case MainOutMode::Stereo:
            info->channel_count = 2;
            info->port_type = CLAP_PORT_STEREO;
            break;
          case MainOutMode::FivePointOne:
            info->channel_count = 6;
            info->port_type = CLAP_PORT_SURROUND;
            break;
          case MainOutMode::Discrete:
            info->channel_count = main_out_discrete_channels_;
            info->port_type = nullptr;
            break;
          }
      }
    else
      {
        std::snprintf (info->name, sizeof (info->name), "Out %u", index + 1);
        info->channel_count = 2;
        info->flags = 0;
        info->port_type = CLAP_PORT_STEREO;
      }
    info->in_place_pair = CLAP_INVALID_ID;
    return true;
  }

  // surround
  bool implementsSurround () const noexcept override { return true; }
  bool isChannelMaskSupported (uint64_t channel_mask) const noexcept override
  {
    static constexpr uint64_t kStereoMask =
      (uint64_t{ 1 } << CLAP_SURROUND_FL) | (uint64_t{ 1 } << CLAP_SURROUND_FR);
    static constexpr uint64_t kFivePointOneMask =
      kStereoMask | (uint64_t{ 1 } << CLAP_SURROUND_FC)
      | (uint64_t{ 1 } << CLAP_SURROUND_LFE)
      | (uint64_t{ 1 } << CLAP_SURROUND_SL)
      | (uint64_t{ 1 } << CLAP_SURROUND_SR);
    return channel_mask == kStereoMask || channel_mask == kFivePointOneMask;
  }
  uint32_t getChannelMap (
    bool      is_input,
    uint32_t  port_index,
    uint8_t * channel_map,
    uint32_t  channel_map_capacity) const noexcept override
  {
    if (
      is_input || port_index != 0 || main_out_mode_ != MainOutMode::FivePointOne)
      return 0;
    if (channel_map_capacity < kWireFivePointOneMap.size ())
      return 0;
    std::copy_n (
      kWireFivePointOneMap.begin (), kWireFivePointOneMap.size (), channel_map);
    return static_cast<uint32_t> (kWireFivePointOneMap.size ());
  }

  // configurable audio ports
  bool implementsConfigurableAudioPorts () const noexcept override
  {
    return true;
  }
  bool configurableAudioPortsCanApplyConfiguration (
    const clap_audio_port_configuration_request * requests,
    uint32_t request_count) const noexcept override
  {
    for (const auto i : std::views::iota (0u, request_count))
      {
        const auto &req = requests[i];
        if (req.is_input)
          {
            // The single input is fixed stereo
            if (req.port_index != 0 || req.channel_count != 2)
              return false;
            continue;
          }
        if (req.port_index >= 3)
          return false;
        if (req.port_type == nullptr)
          {
            // Unspecified (discrete): any channel count
            if (req.channel_count < 1)
              return false;
          }
        else if (std::string_view (req.port_type) == CLAP_PORT_SURROUND)
          {
            // Only 5.1 is supported; the channel map must be readable
            if (req.channel_count != 6 || req.port_details == nullptr)
              return false;
            const auto * map = static_cast<const uint8_t *> (req.port_details);
            for (const auto ch : std::views::iota (0u, req.channel_count))
              {
                if (map[ch] != kCanonicalFivePointOneMap.at (ch))
                  return false;
              }
          }
        else if (std::string_view (req.port_type) == CLAP_PORT_AMBISONIC)
          {
            // Only FuMa is supported; the config must be readable
            if (req.port_details == nullptr)
              return false;
            const auto * config =
              static_cast<const clap_ambisonic_config *> (req.port_details);
            if (
              config->ordering != CLAP_AMBISONIC_ORDERING_FUMA
              || config->normalization != CLAP_AMBISONIC_NORMALIZATION_MAXN)
              return false;
          }
        else if (req.channel_count < 1 || req.channel_count > 2)
          {
            return false;
          }
      }
    return true;
  }
  bool configurableAudioPortsApplyConfiguration (
    const clap_audio_port_configuration_request * requests,
    uint32_t request_count) noexcept override
  {
    for (const auto i : std::views::iota (0u, request_count))
      {
        const auto &req = requests[i];
        if (req.is_input || req.port_index != 0)
          continue;
        if (req.port_type == nullptr)
          {
            main_out_mode_ = MainOutMode::Discrete;
            main_out_discrete_channels_ = req.channel_count;
          }
        else if (std::string_view (req.port_type) == CLAP_PORT_SURROUND)
          {
            main_out_mode_ = MainOutMode::FivePointOne;
          }
        else if (std::string_view (req.port_type) == CLAP_PORT_STEREO)
          {
            main_out_mode_ = MainOutMode::Stereo;
          }
        else if (std::string_view (req.port_type) == CLAP_PORT_MONO)
          {
            // No dedicated mono mode: reported back as a 1-channel discrete
            // bus
            main_out_mode_ = MainOutMode::Discrete;
            main_out_discrete_channels_ = 1;
          }
      }
    return true;
  }

  clap_process_status process (const clap_process * process) noexcept override
  {
    // fill every output channel with a value identifying its wire index
    // (channel 0 -> 1.0, channel 1 -> 2.0, ...), so hosts can verify their
    // wire-order permutation
    for (const auto port : std::views::iota (0u, process->audio_outputs_count))
      {
        const auto &buf = process->audio_outputs[port];
        for (const auto ch : std::views::iota (0u, buf.channel_count))
          {
            std::fill_n (
              buf.data32[ch], process->frames_count,
              static_cast<float> (ch + 1));
          }
      }
    return CLAP_PROCESS_CONTINUE;
  }

private:
  enum class MainOutMode
  {
    Stereo,
    FivePointOne,
    Discrete,
  };

  // The wire order reported to the host, deliberately non-canonical (FC
  // first) so hosts exercising the wire-order permutation don't see the
  // identity mapping
  static constexpr std::array<uint8_t, 6> kWireFivePointOneMap{
    CLAP_SURROUND_FC,  CLAP_SURROUND_FL, CLAP_SURROUND_FR,
    CLAP_SURROUND_LFE, CLAP_SURROUND_SL, CLAP_SURROUND_SR
  };
  // Configuration requests carry canonical-order maps
  static constexpr std::array<uint8_t, 6> kCanonicalFivePointOneMap{
    CLAP_SURROUND_FL,  CLAP_SURROUND_FR, CLAP_SURROUND_FC,
    CLAP_SURROUND_LFE, CLAP_SURROUND_SL, CLAP_SURROUND_SR
  };

  MainOutMode main_out_mode_{ MainOutMode::FivePointOne };
  uint32_t    main_out_discrete_channels_{ 2 };
};

} // namespace zrythm_test_plugins

extern "C" {
CLAP_EXPORT const clap_plugin_entry clap_entry = zrythm_test_plugins::
  clap_fixture_entry<zrythm_test_plugins::TestConfigurableClap>;
}
