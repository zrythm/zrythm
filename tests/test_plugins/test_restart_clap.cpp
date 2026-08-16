// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <ranges>
#include <span>
#include <string_view>

#include "clap_fixture_factory.h"

namespace zrythm_test_plugins
{

/**
 * Effect that changes its audio output ports at runtime, for testing the
 * host's audio ports rescan handling.
 *
 * Toggle params request a port change: the plugin defers it to its
 * deactivate() (per the spec, ports may only change while deactivated),
 * calling the host's audio ports rescan from there. Grow/Shrink add or
 * remove a second stereo output bus; Grow Input adds a second stereo input
 * bus; Widen switches the main output between stereo and 6 channels with no
 * port type (discrete); Swap exchanges the enumeration order of the two
 * output buses. Rename toggles the main output's name and requests a
 * names-only rescan from the main thread — the one rescan a plugin may
 * legally request while active, so no restart is involved. Each output bus
 * is filled with a constant level identifying
 * its enumeration index (0.5 for bus 0, 1.0 for bus 1) so hosts can verify
 * they route by stable port id. If any sample on input bus 1 is nonzero,
 * output bus 0 is overwritten with 9.75 instead, so hosts can verify they
 * feed silence to a bus that has no port yet.
 */
class TestRestartClap final : public ClapFixturePluginBase
{
public:
  static constexpr clap_id kGrowOutputParamId = 0;
  static constexpr clap_id kShrinkOutputParamId = 1;
  static constexpr clap_id kWidenOutputParamId = 2;
  static constexpr clap_id kSwapOutputsParamId = 3;
  static constexpr clap_id kGrowInputParamId = 4;
  static constexpr clap_id kRenameOutputParamId = 5;

  /// Level written to output bus 0 when input bus 1 carries a nonzero sample
  static constexpr float kNonzeroInputSentinel = 9.75f;

  explicit TestRestartClap (const clap_host * host)
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
      .id = kPluginId.data (),
      .name = kPluginName.data (),
      .vendor = "Zrythm",
      .url = "https://zrythm.org",
      .manual_url = "https://manual.zrythm.org",
      .support_url = "https://gitlab.zrythm.org/zrythm/zrythm/-/issues",
      .version = "1.0.0",
      .description = "Minimal effect that changes its audio ports at runtime",
      .features = features,
    };
    return &desc;
  }

  // string_view of a literal - .data() is null-terminated, as the CLAP ABI
  // expects
  static constexpr std::string_view kPluginId = "org.zrythm.TestRestart";
  static constexpr std::string_view kPluginName = "Test Restart";

  // audio ports
  bool     implementsAudioPorts () const noexcept override { return true; }
  uint32_t audioPortsCount (bool isInput) const noexcept override
  {
    if (isInput)
      return extra_input_.load (std::memory_order_acquire) ? 2 : 1;
    return extra_output_.load (std::memory_order_acquire) ? 2 : 1;
  }
  bool
  audioPortsInfo (uint32_t index, bool isInput, clap_audio_port_info * info)
    const noexcept override
  {
    if (isInput)
      {
        if (index == 1)
          {
            if (!extra_input_.load (std::memory_order_acquire))
              return false;
            info->id = 1;
            std::snprintf (info->name, sizeof (info->name), "%s", "In 2");
            info->channel_count = 2;
            info->flags = 0;
            info->port_type = CLAP_PORT_STEREO;
            info->in_place_pair = CLAP_INVALID_ID;
            return true;
          }
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
    {
      const bool extra_output = extra_output_.load (std::memory_order_acquire);
      const bool wide_output = wide_output_.load (std::memory_order_acquire);
      const bool swapped = swapped_.load (std::memory_order_acquire);
      // Which bus sits at this enumeration index; the two buses exchange
      // places when swapped
      const uint32_t bus =
        (swapped && extra_output) ? (index == 0 ? 1u : 0u) : index;
      if (bus == 1)
        {
          if (!extra_output)
            return false;
          info->id = 1;
          std::snprintf (info->name, sizeof (info->name), "%s", "Out 2");
          info->channel_count = 2;
          // The main port must be at index 0, so the flag follows the
          // enumeration index, not the bus
          info->flags = index == 0 ? CLAP_AUDIO_PORT_IS_MAIN : 0;
          info->port_type = CLAP_PORT_STEREO;
          info->in_place_pair = CLAP_INVALID_ID;
          return true;
        }
      if (bus != 0)
        return false;
      info->id = 0;
      std::snprintf (
        info->name, sizeof (info->name), "%s",
        output_renamed_ ? "Renamed Out" : "Output");
      info->channel_count = wide_output ? 6 : 2;
      info->flags = index == 0 ? CLAP_AUDIO_PORT_IS_MAIN : 0;
      info->port_type = wide_output ? nullptr : CLAP_PORT_STEREO;
      info->in_place_pair = CLAP_INVALID_ID;
      return true;
    }
  }

  // params
  bool     implementsParams () const noexcept override { return true; }
  uint32_t paramsCount () const noexcept override { return 6; }
  bool
  paramsInfo (uint32_t paramIndex, clap_param_info * info) const noexcept override
  {
    switch (paramIndex)
      {
      case 0:
        info->id = kGrowOutputParamId;
        std::snprintf (info->name, sizeof (info->name), "%s", "Grow Output");
        break;
      case 1:
        info->id = kShrinkOutputParamId;
        std::snprintf (info->name, sizeof (info->name), "%s", "Shrink Output");
        break;
      case 2:
        info->id = kWidenOutputParamId;
        std::snprintf (info->name, sizeof (info->name), "%s", "Widen Output");
        break;
      case 3:
        info->id = kSwapOutputsParamId;
        std::snprintf (info->name, sizeof (info->name), "%s", "Swap Outputs");
        break;
      case 4:
        info->id = kGrowInputParamId;
        std::snprintf (info->name, sizeof (info->name), "%s", "Grow Input");
        break;
      case 5:
        info->id = kRenameOutputParamId;
        std::snprintf (info->name, sizeof (info->name), "%s", "Rename Output");
        break;
      default:
        return false;
      }
    info->flags = CLAP_PARAM_IS_AUTOMATABLE;
    info->cookie = nullptr;
    info->module[0] = '\0';
    info->min_value = 0.0;
    info->max_value = 1.0;
    info->default_value = 0.0;
    return true;
  }
  bool paramsValue (clap_id paramId, double * value) noexcept override
  {
    if (
      paramId != kGrowOutputParamId && paramId != kShrinkOutputParamId
      && paramId != kWidenOutputParamId && paramId != kSwapOutputsParamId
      && paramId != kGrowInputParamId && paramId != kRenameOutputParamId)
      return false;
    *value = 0.0;
    return true;
  }
  bool paramsValueToText (
    clap_id  paramId,
    double   value,
    char *   display,
    uint32_t size) noexcept override
  {
    if (
      paramId != kGrowOutputParamId && paramId != kShrinkOutputParamId
      && paramId != kWidenOutputParamId && paramId != kSwapOutputsParamId
      && paramId != kGrowInputParamId && paramId != kRenameOutputParamId)
      return false;
    std::snprintf (display, size, "%.3f", value);
    return true;
  }
  bool paramsTextToValue (
    clap_id      paramId,
    const char * display,
    double *     value) noexcept override
  {
    if (
      paramId != kGrowOutputParamId && paramId != kShrinkOutputParamId
      && paramId != kWidenOutputParamId && paramId != kSwapOutputsParamId
      && paramId != kGrowInputParamId && paramId != kRenameOutputParamId)
      return false;
    char * end = nullptr;
    *value = std::strtod (display, &end);
    return end != display;
  }
  void paramsFlush (
    const clap_input_events * in,
    const clap_output_events * /*out*/) noexcept override
  {
    apply_events (in);
  }

  void deactivate () noexcept override
  {
    // Ports may only change while deactivated; the host restart that follows
    // requestRestart() deactivates us, so apply the pending change here and
    // notify the host
    apply_pending_change ();
    ClapFixturePluginBase::deactivate ();
  }

  void onMainThread () noexcept override
  {
    ClapFixturePluginBase::onMainThread ();
    if (rename_pending_.exchange (false, std::memory_order_acq_rel))
      {
        output_renamed_ = !output_renamed_;
        _host.audioPortsRescan (CLAP_AUDIO_PORTS_RESCAN_NAMES);
      }
  }

  clap_process_status process (const clap_process * process) noexcept override
  {
    apply_events (process->in_events);

    // A nonzero sample on input bus 1 means the host fed us something other
    // than silence on a bus that has no port yet; report it via the output
    const bool nonzero_input =
      process->audio_inputs_count >= 2
      && std::ranges::any_of (
        std::views::iota (0u, process->audio_inputs[1].channel_count),
        [&] (uint32_t ch) {
          const auto &buf = process->audio_inputs[1];
          return std::ranges::any_of (
            std::span{ buf.data32[ch], process->frames_count },
            [] (float sample) { return sample != 0.f; });
        });

    // fill every output channel with a constant identifying the bus's
    // enumeration index (0.5 for bus 0, 1.0 for bus 1)
    for (const auto port : std::views::iota (0u, process->audio_outputs_count))
      {
        const auto &buf = process->audio_outputs[port];
        const float level =
          (nonzero_input && port == 0)
            ? kNonzeroInputSentinel
            : 0.5f * static_cast<float> (port + 1);
        for (const auto ch : std::views::iota (0u, buf.channel_count))
          {
            std::fill_n (buf.data32[ch], process->frames_count, level);
          }
      }
    return CLAP_PROCESS_CONTINUE;
  }

private:
  void apply_events (const clap_input_events * in) noexcept
  {
    const auto num_events = in->size (in);
    for (const auto i : std::views::iota (0u, num_events))
      {
        const auto * header = in->get (in, i);
        if (
          header->space_id != CLAP_CORE_EVENT_SPACE_ID
          || header->type != CLAP_EVENT_PARAM_VALUE)
          continue;
        const auto * ev =
          reinterpret_cast<const clap_event_param_value *> (header);
        if (ev->value <= 0.5)
          continue;
        const bool extra_output = extra_output_.load (std::memory_order_acquire);
        const bool extra_input = extra_input_.load (std::memory_order_acquire);
        const bool want_grow =
          ev->param_id == kGrowOutputParamId && !extra_output;
        const bool want_shrink =
          ev->param_id == kShrinkOutputParamId && extra_output;
        const bool want_widen = ev->param_id == kWidenOutputParamId;
        const bool want_swap =
          ev->param_id == kSwapOutputsParamId && extra_output;
        const bool want_grow_input =
          ev->param_id == kGrowInputParamId && !extra_input;
        const bool want_rename = ev->param_id == kRenameOutputParamId;
        if (want_rename)
          {
            // A rename needs no restart: names may change while the plugin
            // is active, but the rescan must be requested from the main
            // thread
            rename_pending_.store (true, std::memory_order_release);
            _host.requestCallback ();
            continue;
          }
        if (
          !want_grow && !want_shrink && !want_widen && !want_swap
          && !want_grow_input)
          continue;
        grow_pending_.store (want_grow, std::memory_order_release);
        shrink_pending_.store (want_shrink, std::memory_order_release);
        widen_pending_.store (want_widen, std::memory_order_release);
        swap_pending_.store (want_swap, std::memory_order_release);
        grow_input_pending_.store (want_grow_input, std::memory_order_release);
        if (isActive ())
          {
            // The host deactivates us for the restart; deactivate() applies
            // the pending change and notifies the rescan
            _host.requestRestart ();
          }
        else
          {
            apply_pending_change ();
          }
      }
  }

  void apply_pending_change () noexcept
  {
    if (grow_pending_.exchange (false, std::memory_order_acq_rel))
      {
        extra_output_.store (true, std::memory_order_release);
        _host.audioPortsRescan (CLAP_AUDIO_PORTS_RESCAN_LIST);
      }
    else if (shrink_pending_.exchange (false, std::memory_order_acq_rel))
      {
        extra_output_.store (false, std::memory_order_release);
        _host.audioPortsRescan (CLAP_AUDIO_PORTS_RESCAN_LIST);
      }
    else if (widen_pending_.exchange (false, std::memory_order_acq_rel))
      {
        const bool wide = !wide_output_.load (std::memory_order_acquire);
        wide_output_.store (wide, std::memory_order_release);
        _host.audioPortsRescan (
          CLAP_AUDIO_PORTS_RESCAN_CHANNEL_COUNT
          | CLAP_AUDIO_PORTS_RESCAN_PORT_TYPE);
      }
    else if (swap_pending_.exchange (false, std::memory_order_acq_rel))
      {
        const bool swapped = !swapped_.load (std::memory_order_acquire);
        swapped_.store (swapped, std::memory_order_release);
        _host.audioPortsRescan (CLAP_AUDIO_PORTS_RESCAN_LIST);
      }
    else if (grow_input_pending_.exchange (false, std::memory_order_acq_rel))
      {
        extra_input_.store (true, std::memory_order_release);
        _host.audioPortsRescan (CLAP_AUDIO_PORTS_RESCAN_LIST);
      }
  }

  std::atomic<bool> extra_output_{ false };
  std::atomic<bool> extra_input_{ false };
  std::atomic<bool> wide_output_{ false };
  std::atomic<bool> swapped_{ false };
  std::atomic<bool> grow_pending_{ false };
  std::atomic<bool> shrink_pending_{ false };
  std::atomic<bool> widen_pending_{ false };
  std::atomic<bool> swap_pending_{ false };
  std::atomic<bool> grow_input_pending_{ false };
  std::atomic<bool> rename_pending_{ false };
  /// Main-thread only: toggled in onMainThread, read in audioPortsInfo
  bool output_renamed_ = false;
};

} // namespace zrythm_test_plugins

extern "C" {
CLAP_EXPORT const clap_plugin_entry clap_entry =
  zrythm_test_plugins::clap_fixture_entry<zrythm_test_plugins::TestRestartClap>;
}
