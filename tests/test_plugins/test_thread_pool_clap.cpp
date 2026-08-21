// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <ranges>
#include <string>
#include <thread>

#include "clap_fixture_factory.h"
#include <nlohmann/json.hpp>

namespace zrythm_test_plugins
{

/**
 * @brief Fixture plugin that exercises the host's CLAP thread-pool
 * extension.
 *
 * Each process() call requests "Num Tasks" parallel tasks from the host
 * pool (falling back to a serial loop when the request is rejected or the
 * host has no pool) and records, for host-side verification via stateSave()
 * JSON: how often each task index ran, how many executions happened on the
 * process thread, and the pool request outcomes.
 */
class TestThreadPoolClap final : public ClapFixturePluginBase
{
public:
  static constexpr clap_id  kNumTasksParamId = 0;
  static constexpr uint32_t kMaxTasks = 64;

  explicit TestThreadPoolClap (const clap_host * host)
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
      .id = "org.zrythm.TestThreadPool",
      .name = "Test Thread Pool",
      .vendor = "Zrythm",
      .url = "https://zrythm.org",
      .manual_url = "https://manual.zrythm.org",
      .support_url = "https://gitlab.zrythm.org/zrythm/zrythm/-/issues",
      .version = "1.0.0",
      .description = "Thread pool fixture plugin",
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
    info->id = kNumTasksParamId;
    info->flags = CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_STEPPED;
    info->cookie = nullptr;
    std::snprintf (info->name, sizeof (info->name), "%s", "Num Tasks");
    info->module[0] = '\0';
    info->min_value = 0.0;
    info->max_value = static_cast<double> (kMaxTasks);
    info->default_value = 8.0;
    return true;
  }
  bool paramsValue (clap_id paramId, double * value) noexcept override
  {
    if (paramId != kNumTasksParamId)
      return false;
    *value = num_tasks_param_.load ();
    return true;
  }
  bool paramsValueToText (
    clap_id  paramId,
    double   value,
    char *   display,
    uint32_t size) noexcept override
  {
    if (paramId != kNumTasksParamId)
      return false;
    std::snprintf (display, size, "%d", static_cast<int> (std::lround (value)));
    return true;
  }
  bool paramsTextToValue (
    clap_id      paramId,
    const char * display,
    double *     value) noexcept override
  {
    if (paramId != kNumTasksParamId)
      return false;
    char *       end = nullptr;
    const double v = std::strtod (display, &end);
    if (end == display)
      return false;
    *value = std::clamp (v, 0.0, static_cast<double> (kMaxTasks));
    return true;
  }
  void paramsFlush (
    const clap_input_events * in,
    const clap_output_events * /*out*/) noexcept override
  {
    apply_events (in);
  }

  // thread pool
  bool implementsThreadPool () const noexcept override { return true; }
  void threadPoolExec (uint32_t taskIndex) noexcept override
  {
    if (taskIndex >= kMaxTasks)
      return;
    exec_counts_[taskIndex].fetch_add (1, std::memory_order_relaxed);
    if (
      std::hash<std::thread::id>{}(std::this_thread::get_id ())
      == process_thread_hash_.load (std::memory_order_relaxed))
      {
        execs_on_process_thread_.fetch_add (1, std::memory_order_relaxed);
      }
  }

  // state
  bool implementsState () const noexcept override { return true; }
  bool stateSave (const clap_ostream * stream) noexcept override
  {
    nlohmann::json counts = nlohmann::json::array ();
    for (const auto &count : exec_counts_)
      {
        counts.push_back (count.load (std::memory_order_relaxed));
      }
    const nlohmann::json j{
      { "num_tasks",               num_tasks_param_.load ()                        },
      { "exec_counts",             counts                                          },
      { "execs_on_process_thread",
       execs_on_process_thread_.load (std::memory_order_relaxed)                   },
      { "pool_requests_succeeded",
       pool_requests_succeeded_.load (std::memory_order_relaxed)                   },
      { "pool_requests_rejected",
       pool_requests_rejected_.load (std::memory_order_relaxed)                    },
      { "fallback_runs",           fallback_runs_.load (std::memory_order_relaxed) },
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
    if (j.is_discarded () || !j.contains ("num_tasks"))
      return false;
    num_tasks_param_.store (
      std::clamp (
        j["num_tasks"].get<double> (), 0.0, static_cast<double> (kMaxTasks)));
    return true;
  }

  clap_process_status process (const clap_process * process) noexcept override
  {
    apply_events (process->in_events);

    process_thread_hash_.store (
      std::hash<std::thread::id>{}(std::this_thread::get_id ()),
      std::memory_order_relaxed);

    const auto num_tasks = static_cast<uint32_t> (
      std::lround (num_tasks_param_.load (std::memory_order_relaxed)));

    if (num_tasks > 0)
      {
        bool used_pool = false;
        if (_host.canUseThreadPool ())
          {
            used_pool = _host.threadPoolRequestExec (num_tasks);
            if (used_pool)
              {
                pool_requests_succeeded_.fetch_add (
                  1, std::memory_order_relaxed);
              }
            else
              {
                pool_requests_rejected_.fetch_add (1, std::memory_order_relaxed);
              }
          }
        if (!used_pool)
          {
            fallback_runs_.fetch_add (1, std::memory_order_relaxed);
            for (const auto i : std::views::iota (0u, num_tasks))
              {
                threadPoolExec (i);
              }
          }
      }

    // passthrough
    const auto num_frames = process->frames_count;
    if (process->audio_inputs_count < 1 || process->audio_outputs_count < 1)
      return CLAP_PROCESS_CONTINUE;
    for (uint32_t ch = 0; ch < 2; ++ch)
      {
        const auto * in = process->audio_inputs[0].data32[ch];
        auto *       out = process->audio_outputs[0].data32[ch];
        std::copy_n (in, num_frames, out);
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
            if (ev->param_id == kNumTasksParamId)
              {
                num_tasks_param_.store (
                  std::clamp (ev->value, 0.0, static_cast<double> (kMaxTasks)));
              }
          }
      }
  }

  std::atomic<double> num_tasks_param_{ 8.0 };

  std::array<std::atomic<uint32_t>, kMaxTasks> exec_counts_{};
  std::atomic<uint32_t>                        execs_on_process_thread_{ 0 };
  std::atomic<uint32_t>                        pool_requests_succeeded_{ 0 };
  std::atomic<uint32_t>                        pool_requests_rejected_{ 0 };
  std::atomic<uint32_t>                        fallback_runs_{ 0 };

  /** Hash of the thread id of the last process() call, to detect whether
   * task executions happened on the process thread. */
  std::atomic<uint64_t> process_thread_hash_{ 0 };
};

} // namespace zrythm_test_plugins

extern "C" {
CLAP_EXPORT const clap_plugin_entry clap_entry = zrythm_test_plugins::
  clap_fixture_entry<zrythm_test_plugins::TestThreadPoolClap>;
}
