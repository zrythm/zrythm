// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string_view>

#include "sine_synth.h"
#include <clap/all.h>
#include <clap/helpers/plugin.hh>
#include <clap/helpers/plugin.hxx>

namespace zrythm_test_plugins
{

using ClapPluginBase = clap::helpers::Plugin<
  clap::helpers::MisbehaviourHandler::Terminate,
  clap::helpers::CheckingLevel::Maximal>;

/**
 * Test synth exposed in two variants: one that only accepts the CLAP note
 * dialect and one that only accepts the MIDI dialect. Events of other
 * dialects are ignored so that tests fail if a host sends the wrong dialect.
 */
template <uint32_t SupportedDialects>
class TestSynthClap final : public ClapPluginBase
{
public:
  static constexpr clap_id kLevelParamId = 0;

  explicit TestSynthClap (const clap_host * host)
      : ClapPluginBase (descriptor (), host)
  {
  }

  static const clap_plugin_descriptor * descriptor ()
  {
    static constexpr const char * const features[] = {
      CLAP_PLUGIN_FEATURE_INSTRUMENT, CLAP_PLUGIN_FEATURE_STEREO, nullptr
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
      .description = "Minimal sine synth used as a test fixture",
      .features = features,
    };
    return &desc;
  }

  // string_views of literals - .data() is null-terminated, as the CLAP ABI
  // expects
  static constexpr std::string_view kPluginId =
    SupportedDialects == CLAP_NOTE_DIALECT_MIDI
      ? "org.zrythm.TestSynthMidi"
      : "org.zrythm.TestSynth";
  static constexpr std::string_view kPluginName =
    SupportedDialects == CLAP_NOTE_DIALECT_MIDI ? "Test Synth MIDI" : "Test Synth";

  static const clap_plugin * createInstance (const clap_host * host) noexcept
  {
    auto * p = new TestSynthClap (host);
    return p->clapPlugin ();
  }

  bool activate (
    double sampleRate,
    uint32_t /*minFrameCount*/,
    uint32_t /*maxFrameCount*/) noexcept override
  {
    synth_.set_sample_rate (sampleRate);
    return true;
  }

  // audio ports
  bool     implementsAudioPorts () const noexcept override { return true; }
  uint32_t audioPortsCount (bool isInput) const noexcept override
  {
    return isInput ? 0 : 1;
  }
  bool
  audioPortsInfo (uint32_t index, bool isInput, clap_audio_port_info * info)
    const noexcept override
  {
    if (isInput || index != 0)
      return false;
    info->id = 0;
    std::snprintf (info->name, sizeof (info->name), "%s", "Output");
    info->channel_count = 2;
    info->flags = CLAP_AUDIO_PORT_IS_MAIN;
    info->port_type = CLAP_PORT_STEREO;
    info->in_place_pair = CLAP_INVALID_ID;
    return true;
  }

  // note ports
  bool     implementsNotePorts () const noexcept override { return true; }
  uint32_t notePortsCount (bool isInput) const noexcept override
  {
    // Each variant additionally echoes incoming note events (in its own
    // dialect) on an output port so hosts can verify note-output forwarding
    return 1;
  }
  bool notePortsInfo (uint32_t index, bool isInput, clap_note_port_info * info)
    const noexcept override
  {
    if (index != 0)
      return false;
    info->id = 0;
    info->supported_dialects = SupportedDialects;
    info->preferred_dialect = SupportedDialects;
    std::snprintf (
      info->name, sizeof (info->name), "%s",
      isInput ? "Note Input" : "Note Output");
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
    const double v = gain_.load ();
    return stream->write (stream, &v, sizeof (v)) == sizeof (v);
  }
  bool stateLoad (const clap_istream * stream) noexcept override
  {
    double v = 0.0;
    if (stream->read (stream, &v, sizeof (v)) != sizeof (v))
      return false;
    gain_.store (std::clamp (v, 0.0, 1.0));
    return true;
  }

  clap_process_status process (const clap_process * process) noexcept override
  {
    apply_events (process->in_events);
    echo_note_events (
      process->in_events, process->out_events, process->frames_count);

    const auto num_frames = process->frames_count;
    if (process->audio_outputs_count < 1)
      return CLAP_PROCESS_CONTINUE;

    auto * left = process->audio_outputs[0].data32[0];
    auto * right = process->audio_outputs[0].data32[1];
    std::fill_n (left, num_frames, 0.0f);
    std::fill_n (right, num_frames, 0.0f);
    synth_.process (left, right, num_frames, gain_.load ());
    return CLAP_PROCESS_CONTINUE;
  }

private:
  static void echo_note_events (
    const clap_input_events *  in,
    const clap_output_events * out,
    uint32_t                   frames_count) noexcept
  {
    const auto num_events = in->size (in);
    for (uint32_t i = 0; i < num_events; ++i)
      {
        const auto * header = in->get (in, i);
        if (header->space_id != CLAP_CORE_EVENT_SPACE_ID)
          continue;
        if constexpr (SupportedDialects == CLAP_NOTE_DIALECT_CLAP)
          {
            if (
              header->type != CLAP_EVENT_NOTE_ON
              && header->type != CLAP_EVENT_NOTE_OFF)
              continue;
            const auto * note =
              reinterpret_cast<const clap_event_note *> (header);
            // Key 127 requests a deliberately out-of-block echo so hosts
            // can verify output event time validation
            if (note->key == 127)
              {
                auto bad = *note;
                bad.header.time = frames_count;
                out->try_push (out, &bad.header);
                continue;
              }
            out->try_push (out, header);
          }
        else
          {
            if (header->type != CLAP_EVENT_MIDI)
              continue;
            const auto * midi =
              reinterpret_cast<const clap_event_midi *> (header);
            // Key 127 requests a deliberately out-of-block echo so hosts
            // can verify output event time validation
            if ((midi->data[0] & 0xF0) == 0x90 && midi->data[1] == 127)
              {
                auto bad = *midi;
                bad.header.time = frames_count;
                out->try_push (out, &bad.header);
                continue;
              }
            out->try_push (out, header);
          }
      }
  }

  void apply_events (const clap_input_events * in) noexcept
  {
    const auto num_events = in->size (in);
    for (uint32_t i = 0; i < num_events; ++i)
      {
        const auto * header = in->get (in, i);
        if (header->space_id != CLAP_CORE_EVENT_SPACE_ID)
          continue;
        if (header->type == CLAP_EVENT_PARAM_VALUE)
          {
            const auto * ev =
              reinterpret_cast<const clap_event_param_value *> (header);
            if (ev->param_id == kLevelParamId)
              gain_.store (std::clamp (ev->value, 0.0, 1.0));
          }

        if constexpr (SupportedDialects == CLAP_NOTE_DIALECT_CLAP)
          {
            if (header->type == CLAP_EVENT_NOTE_ON)
              {
                const auto * ev =
                  reinterpret_cast<const clap_event_note *> (header);
                synth_.note_on (ev->key, ev->velocity);
              }
            else if (header->type == CLAP_EVENT_NOTE_OFF)
              {
                const auto * ev =
                  reinterpret_cast<const clap_event_note *> (header);
                synth_.note_off (ev->key);
              }
          }
        else
          {
            if (header->type == CLAP_EVENT_MIDI)
              {
                const auto * ev =
                  reinterpret_cast<const clap_event_midi *> (header);
                const auto status = ev->data[0] & 0xF0;
                if (status == 0x90 && ev->data[2] != 0)
                  synth_.note_on (
                    static_cast<int16_t> (ev->data[1]), ev->data[2] / 127.0);
                else if (status == 0x80 || (status == 0x90 && ev->data[2] == 0))
                  synth_.note_off (static_cast<int16_t> (ev->data[1]));
              }
          }
      }
  }

  SineSynth           synth_;
  std::atomic<double> gain_{ 1.0 };
};

using TestSynthClapNotes = TestSynthClap<CLAP_NOTE_DIALECT_CLAP>;
using TestSynthClapMidi = TestSynthClap<CLAP_NOTE_DIALECT_MIDI>;

static const std::array<const clap_plugin_descriptor *, 2> kDescriptors = {
  TestSynthClapNotes::descriptor (), TestSynthClapMidi::descriptor ()
};

static const clap_plugin_factory plugin_factory = {
  .get_plugin_count = [] (const clap_plugin_factory *) -> uint32_t {
    return kDescriptors.size ();
  },
  .get_plugin_descriptor = [] (const clap_plugin_factory *, uint32_t index)
    -> const clap_plugin_descriptor * {
    return index < kDescriptors.size () ? kDescriptors[index] : nullptr;
  },
  .create_plugin =
    [] (const clap_plugin_factory *, const clap_host * host, const char * plugin_id)
    -> const clap_plugin * {
    if (host == nullptr || !clap_version_is_compatible (host->clap_version))
      return nullptr;
    if (TestSynthClapNotes::kPluginId == plugin_id)
      return TestSynthClapNotes::createInstance (host);
    if (TestSynthClapMidi::kPluginId == plugin_id)
      return TestSynthClapMidi::createInstance (host);
    return nullptr;
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
