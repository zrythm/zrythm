// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "clap_fixture_factory.h"
#include "gain_dsp.h"
#include <nlohmann/json.hpp>

namespace zrythm_test_plugins
{

class TestGainClap final : public ClapFixturePluginBase
{
public:
  static constexpr clap_id kLevelParamId = 0;

  explicit TestGainClap (const clap_host * host)
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
      .id = "org.zrythm.TestGain",
      .name = "Test Gain",
      .vendor = "Zrythm",
      .url = "https://zrythm.org",
      .manual_url = "https://manual.zrythm.org",
      .support_url = "https://gitlab.zrythm.org/zrythm/zrythm/-/issues",
      .version = "1.0.0",
      .description = "Minimal gain plugin used as a test fixture",
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
    *value = gain_.load ();
    return true;
  }
  bool paramsValueToText (
    clap_id  paramId,
    double   value,
    char *   display,
    uint32_t size) noexcept override
  {
    if (paramId != kLevelParamId)
      return false;
    std::snprintf (display, size, "%.3f", value);
    return true;
  }
  bool paramsTextToValue (
    clap_id      paramId,
    const char * display,
    double *     value) noexcept override
  {
    if (paramId != kLevelParamId)
      return false;
    char *       end = nullptr;
    const double v = std::strtod (display, &end);
    if (end == display)
      return false;
    *value = std::clamp (v, 0.0, 1.0);
    return true;
  }
  void paramsFlush (
    const clap_input_events * in,
    const clap_output_events * /*out*/) noexcept override
  {
    apply_events (in);
  }

  // state
  bool implementsState () const noexcept override { return true; }
  bool stateSave (const clap_ostream * stream) noexcept override
  {
    const nlohmann::json j{
      { "gain", gain_.load () }
    };
    const auto json_text = j.dump ();
    const auto text_size = json_text.size ();
    return stream->write (stream, json_text.data (), text_size)
           == static_cast<int64_t> (text_size);
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
    if (j.is_discarded () || !j.contains ("gain"))
      return false;
    gain_.store (std::clamp (j["gain"].get<double> (), 0.0, 1.0));
    return true;
  }

  clap_process_status process (const clap_process * process) noexcept override
  {
    apply_events (process->in_events);

    const auto num_frames = process->frames_count;
    if (process->audio_inputs_count < 1 || process->audio_outputs_count < 1)
      return CLAP_PROCESS_CONTINUE;
    for (uint32_t ch = 0; ch < 2; ++ch)
      {
        apply_gain (
          process->audio_inputs[0].data32[ch],
          process->audio_outputs[0].data32[ch], num_frames, gain_.load ());
      }
    return CLAP_PROCESS_CONTINUE;
  }

private:
  void apply_events (const clap_input_events * in) noexcept
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
              gain_.store (std::clamp (ev->value, 0.0, 1.0));
          }
      }
  }

  std::atomic<double> gain_{ 1.0 };
};

} // namespace zrythm_test_plugins

extern "C" {
CLAP_EXPORT const clap_plugin_entry clap_entry =
  zrythm_test_plugins::clap_fixture_entry<zrythm_test_plugins::TestGainClap>;
}
