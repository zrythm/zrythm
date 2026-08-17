// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>

#include "clap_fixture_factory.h"
#include "sine_synth.h"
#include <nlohmann/json.hpp>

namespace zrythm_test_plugins
{

/**
 * Test synth exposed in two variants: one that only accepts the CLAP note
 * dialect and one that only accepts the MIDI dialect. Events of other
 * dialects are ignored so that tests fail if a host sends the wrong dialect.
 */
template <uint32_t SupportedDialects>
class TestSynthClap final : public ClapFixturePluginBase
{
public:
  static constexpr clap_id kLevelParamId = 0;

  explicit TestSynthClap (const clap_host * host)
      : ClapFixturePluginBase (descriptor (), host)
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
    // Structured fixture state: the gain plus the last received transport,
    // so hosts can verify transport delivery
    const nlohmann::json j{
      { "gain",      gain_.load () },
      { "transport",
       nlohmann::json{
          { "present", ctx_present_.load () != 0.0 },
          { "flags", static_cast<uint32_t> (ctx_flags_.load ()) },
          { "tempo", ctx_tempo_.load () },
          { "songPosBeats", ctx_song_pos_beats_.load () },
          { "songPosSeconds", ctx_song_pos_seconds_.load () },
          { "barStartBeats", ctx_bar_start_beats_.load () },
          { "barNumber", static_cast<int> (ctx_bar_number_.load ()) },
          { "loopStartBeats", ctx_loop_start_beats_.load () },
          { "loopEndBeats", ctx_loop_end_beats_.load () },
          { "timeSigNum", static_cast<int> (ctx_time_sig_num_.load ()) },
          { "timeSigDenom", static_cast<int> (ctx_time_sig_denom_.load ()) },
        }                          },
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
    echo_note_events (
      process->in_events, process->out_events, process->frames_count);

    // Capture the received transport so hosts can verify transport delivery
    // via the state
    if (process->transport != nullptr)
      {
        const auto &transport = *process->transport;
        const auto  beats = [] (clap_beattime t) {
          return static_cast<double> (t)
                 / static_cast<double> (CLAP_BEATTIME_FACTOR);
        };
        const auto secs = [] (clap_sectime t) {
          return static_cast<double> (t)
                 / static_cast<double> (CLAP_SECTIME_FACTOR);
        };
        ctx_present_.store (1.0);
        ctx_flags_.store (static_cast<double> (transport.flags));
        ctx_tempo_.store (transport.tempo);
        ctx_song_pos_beats_.store (beats (transport.song_pos_beats));
        ctx_song_pos_seconds_.store (secs (transport.song_pos_seconds));
        ctx_bar_start_beats_.store (beats (transport.bar_start));
        ctx_bar_number_.store (static_cast<double> (transport.bar_number));
        ctx_loop_start_beats_.store (beats (transport.loop_start_beats));
        ctx_loop_end_beats_.store (beats (transport.loop_end_beats));
        ctx_time_sig_num_.store (static_cast<double> (transport.tsig_num));
        ctx_time_sig_denom_.store (static_cast<double> (transport.tsig_denom));
      }
    else
      {
        ctx_present_.store (0.0);
      }

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

  // Last received transport, captured during process() and serialized into
  // the state so hosts can verify transport delivery
  std::atomic<double> ctx_present_{ 0.0 };
  std::atomic<double> ctx_flags_{ 0.0 };
  std::atomic<double> ctx_tempo_{ 0.0 };
  std::atomic<double> ctx_song_pos_beats_{ 0.0 };
  std::atomic<double> ctx_song_pos_seconds_{ 0.0 };
  std::atomic<double> ctx_bar_start_beats_{ 0.0 };
  std::atomic<double> ctx_bar_number_{ 0.0 };
  std::atomic<double> ctx_loop_start_beats_{ 0.0 };
  std::atomic<double> ctx_loop_end_beats_{ 0.0 };
  std::atomic<double> ctx_time_sig_num_{ 0.0 };
  std::atomic<double> ctx_time_sig_denom_{ 0.0 };
};

using TestSynthClapNotes = TestSynthClap<CLAP_NOTE_DIALECT_CLAP>;
using TestSynthClapMidi = TestSynthClap<CLAP_NOTE_DIALECT_MIDI>;

} // namespace zrythm_test_plugins

extern "C" {
CLAP_EXPORT const clap_plugin_entry clap_entry =
  zrythm_test_plugins::clap_fixture_entry<
    zrythm_test_plugins::TestSynthClapNotes,
    zrythm_test_plugins::TestSynthClapMidi>;
}
