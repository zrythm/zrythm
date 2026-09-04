// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * ---
 *
 * SPDX-FileCopyrightText: Copyright (c) 2021 Alexandre BIQUE
 * SPDX-License-Identifier: MIT
 *
 * Copyright (c) 2021 Alexandre BIQUE
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * ---
 *
 */

#include "zrythm-config.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <deque>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <utility>

#include "utils/format_qt.h"
#include <fmt/std.h>

#include "dsp/audio_bus_configuration.h"
#include "dsp/fork_join_executor.h"
#include "dsp/midi_event.h"
#include "plugins/CLAPPluginFormat.h"
#include "plugins/clap_plugin.h"
#include "plugins/clap_speaker_arrangement.h"
#include "plugins/gl_context_utils.h"
#include "plugins/host_window_units.h"
#include "plugins/plugin_format_utils.h"
#include "plugins/plugin_library.h"
#include "plugins/plugin_run_loop.h"
#include "plugins/plugin_transport_context.h"
#include "utils/concurrency.h"
#include "utils/logger.h"
#include "utils/qt.h"
#include "utils/raii_utils.h"
#include "utils/registry_utils.h"
#include "utils/serialization.h"
#include "utils/views.h"

#include <QFile>
#include <QSocketNotifier>
#include <QTimer>

#include <clap/helpers/event-list.hh>
#include <clap/helpers/host.hxx>
#include <clap/helpers/plugin-proxy.hh>
#include <clap/helpers/plugin-proxy.hxx>
#include <farbot/RealtimeObject.hpp>
#if defined(__has_feature) && __has_feature(realtime_sanitizer)
#  include <sanitizer/rtsan_interface.h>
#endif

namespace zrythm::plugins
{
thread_local bool is_main_thread = false;
// Set by process_impl() for both real-time audio and offline rendering.
thread_local bool is_audio_thread = false;

using ClapPluginProxy = clap::helpers::PluginProxy<
  clap::helpers::MisbehaviourHandler::Terminate,
  clap::helpers::CheckingLevel::Maximal>;

class ClapPlugin::ClapPluginImpl
{
  friend class ClapPlugin;

public:
  ClapPluginImpl (ClapPlugin &owner, PluginHostWindowFactory host_window_factory)
      : owner_ (owner), host_window_factory_ (std::move (host_window_factory))
  {
  }

  struct ClapParamAdapter
  {
    clap_id                   id;
    clap_param_info           info;
    dsp::ProcessorParameter * zrythm_param = nullptr;
    size_t                    param_index = 0;
  };

  /** Parameter routing tables (CLAP id <-> Zrythm parameter). */
  struct ParamMaps
  {
    std::unordered_map<clap_id, ClapParamAdapter>          by_id_;
    std::unordered_map<dsp::ProcessorParameter *, clap_id> by_param_;
  };

  enum PluginState
  {
    // The plugin is inactive, only the main thread uses it
    Inactive,

    // Activation failed
    InactiveWithError,

    // The plugin is active and sleeping, the audio engine can call
    // set_processing()
    ActiveAndSleeping,

    // The plugin is processing
    ActiveAndProcessing,

    // The plugin did process but is in error
    ActiveWithError,

    // The plugin is not used anymore by the audio engine and can be
    // deactivated on the main thread
    ActiveAndReadyToDeactivate,
  };
  bool is_plugin_active () const;
  bool is_plugin_processing () const;
  bool is_plugin_sleeping () const;
  void set_plugin_state (PluginState state);

  /* clap host callbacks */

  [[nodiscard]] bool
  check_valid_param_value (const ClapParamAdapter &param, double value);
  std::optional<double> get_param_value (const clap_param_info &info);
  static bool           clap_params_rescan_may_value_change (uint32_t flags)
  {
    return (flags & (CLAP_PARAM_RESCAN_ALL | CLAP_PARAM_RESCAN_VALUES)) != 0u;
  }
  static bool clap_params_rescan_may_info_change (uint32_t flags)
  {
    return (flags & (CLAP_PARAM_RESCAN_ALL | CLAP_PARAM_RESCAN_INFO)) != 0u;
  }

  /**
   * @brief Performs a CLAP paramsFlush when the plugin is inactive.
   *
   * Calls paramsFlush on the plugin, then drains any output values through
   * param_sync_. Main thread only.
   */
  void param_flush_on_main_thread () [[clang::blocking]];

  /**
   * @brief Reports a dropped plugin output event (audio-thread safe).
   *
   * The event violated the output contract (out-of-range port index or
   * note fields); the drop is counted and logged on the main thread at a
   * bounded (power-of-two) rate.
   */
  void note_invalid_output_event_drop (std::string_view violation) noexcept
  {
    const auto drops =
      invalid_output_event_drops_.fetch_add (1, std::memory_order_relaxed) + 1;
    if (drops == 1 || (drops & (drops - 1)) == 0)
      {
        owner_.post_main_thread_action ([this, drops, violation] {
          z_warning (
            "CLAP plugin '{}': dropped {} invalid output event(s) so far "
            "(last: {})",
            owner_.get_name (), drops, violation);
        });
      }
  }

  std::atomic<uint32_t> invalid_output_event_drops_{ 0 };

  /**
   * @brief Processes CLAP output events from the plugin's last process() call.
   *
   * For param value events, stores the normalized value in param_sync_ for
   * cross-thread bridging and sets the feedback guard.
   *
   * @param maps Parameter routing tables from a caller-held ScopedAccess
   * (realtime on the audio thread, nonRealtime on the main thread).
   * @param event_time_offset Offset to add to plugin-reported event times
   * to convert them from chunk-relative to cycle-relative samples (zero
   * outside of block processing).
   * @param block_length Length of the processed block; events the plugin
   * timestamps outside it are dropped. Pass std::nullopt where no block is
   * being processed (main-thread flushes) to skip time validation.
   */
  void handle_plugin_output_events (
    const ParamMaps                   &maps,
    units::sample_u32_t                event_time_offset,
    std::optional<units::sample_u32_t> block_length) noexcept
    [[clang::nonblocking]];

  /**
   * @brief Generates CLAP param value events for changed parameters only.
   *
   * Uses ParameterChangeTracker to iterate only params that changed this
   * cycle, with feedback prevention via ParamSync. Audio thread only.
   *
   * @param maps Parameter routing tables from the caller's realtime
   * ScopedAccess.
   */
  void generate_changed_param_input_events (const ParamMaps &maps) noexcept
    [[clang::nonblocking]];

  /**
   * @brief Generates CLAP param value events for all parameters.
   *
   * Sends current base values for all mapped params. Main thread only
   * (used by paramFlush when plugin is inactive).
   *
   * @param maps Parameter routing tables from the caller's nonRealtime
   * ScopedAccess.
   */
  void
  generate_all_param_input_events (const ParamMaps &maps) [[clang::blocking]];

  /**
   * @brief Generates CLAP MIDI events from the MIDI input port.
   *
   * The port buffer holds events for the whole cycle, so only events inside
   * the current chunk are emitted, re-based to chunk-relative times.
   */
  void generate_midi_input_events (
    units::sample_u32_t local_offset,
    units::sample_u32_t nframes) noexcept [[clang::nonblocking]];

  /**
   * @brief Sizes the scratch buffers and pairs the enumerated buses with the
   * engine ports, for both flows.
   *
   * @return false when a flow's port report is untrustworthy (see
   * enumerate_validated_audio_port_infos()); the plugin must not be
   * activated then.
   */
  [[nodiscard]] bool
  setup_audio_ports_for_processing (units::sample_u32_t block_size);

  /**
   * @brief Resolves the arrangement for the audio port at @p index from the
   * plugin's port type and the surround/ambisonic extensions.
   *
   * Falls back to a discrete arrangement with the port's channel count when
   * the port type is empty or unknown, when the extension lookup fails or
   * reports something unrepresentable, or when the port type implies a
   * different channel count than reported.
   */
  dsp::SpeakerArrangement resolve_audio_port_arrangement (
    dsp::PortFlow                 flow,
    uint32_t                      index,
    const clap_audio_port_info_t &nfo) const;

  /**
   * @brief The surround channel map for the port at @p index, when the
   * port's type is surround and the plugin's surround extension provides a
   * complete map.
   */
  std::optional<std::vector<uint8_t>> get_surround_channel_map (
    dsp::PortFlow flow,
    uint32_t      index,
    uint32_t      channel_count) const;

  /**
   * @brief The current audio bus configuration for the given flow, one entry
   * per enumerated audio port.
   *
   * Arrangements reflect the plugin's port type and the surround/ambisonic
   * extensions; the first enumerated port is the main bus (misplaced or
   * absent CLAP_AUDIO_PORT_IS_MAIN flags are warned about and ignored);
   * entries carry the stable CLAP port ids as external ids.
   *
   * @return std::nullopt when the plugin's report is untrustworthy (see
   * enumerate_validated_audio_port_infos()).
   */
  std::optional<std::vector<dsp::AudioBusConfig>>
  build_audio_bus_configs (dsp::PortFlow flow) const;

  /**
   * @brief Pushes the saved bus configuration into the plugin via the
   * configurable audio ports extension.
   *
   * Builds one configuration request per saved bus, matched to live
   * enumeration indices by stable id (positionally for ports saved before
   * ids were recorded).
   *
   * @return std::nullopt when the plugin implements no configurable audio
   * ports extension; otherwise whether a configuration was applied and the
   * accepted layout must be re-scanned (false also when no saved bus matched
   * a live bus and nothing was submitted).
   */
  std::optional<bool> push_saved_bus_configuration (
    const std::vector<dsp::AudioBusConfig> &saved_inputs,
    const std::vector<dsp::AudioBusConfig> &saved_outputs,
    const std::vector<dsp::AudioBusConfig> &live_inputs,
    const std::vector<dsp::AudioBusConfig> &live_outputs);

  /**
   * @brief Syncs the engine ports to the accepted bus configuration after a
   * restore.
   *
   * The accepted layout only differs from the live one after a successful
   * configuration push, so the live configurations are reused when @p pushed
   * is false.
   *
   * @return What changed; see dsp::AudioBusReconcileResult for the caller's
   * obligations.
   */
  dsp::AudioBusReconcileResult sync_ports_to_live_configuration (
    const std::vector<dsp::AudioBusConfig> &live_inputs,
    const std::vector<dsp::AudioBusConfig> &live_outputs,
    bool                                    pushed);

  /** Maximum audio buses per flow the host supports. */
  static constexpr uint32_t kMaxAudioBusesPerFlow = 128;

  /**
   * @brief The validated audio port info for every port in the given flow,
   * in enumeration order.
   *
   * The report is untrustworthy — and std::nullopt returned — when the
   * plugin declares more buses than @ref kMaxAudioBusesPerFlow, stops
   * reporting below its declared count, reports duplicate stable ids, or
   * reports a bus with fewer than 1 or more channels than SpeakerArrangement
   * models. (Misused main-port flags are warned about and normalized during
   * configuration building instead: they only affect purpose assignment.)
   *
   * An empty vector (not an error) is returned when the plugin lacks the
   * audio ports extension.
   */
  std::optional<std::vector<clap_audio_port_info_t>>
  enumerate_validated_audio_port_infos (dsp::PortFlow flow) const;

  /**
   * @brief The plugin's extension with the given id, falling back to the
   * pre-1.4 compatibility id.
   *
   * @return nullptr when the plugin implements neither id.
   */
  template <typename Ext>
  const Ext * get_extension (const char * id, const char * compat_id) const
  {
    const Ext * ext = nullptr;
    plugin_->getExtension (ext, id);
    if (ext == nullptr && compat_id != nullptr)
      plugin_->getExtension (ext, compat_id);
    return ext;
  }

  void set_plugin_window_visibility (bool isVisible);

  /**
   * @brief Destroys the plugin GUI (if created) and the host window. Main
   * thread.
   */
  void destroy_gui ();

private:
  ClapPlugin &owner_;

  PluginLibrary library_;

  const clap_plugin_entry *        pluginEntry_ = nullptr;
  const clap_plugin_factory *      pluginFactory_ = nullptr;
  std::unique_ptr<ClapPluginProxy> plugin_;

  /* timers & fd events (CLAP timer/posix-fd extensions) */
  clap_id                                           nextTimerId_ = 0;
  PluginRunLoop                                     run_loop_;
  std::unordered_map<int, PluginRunLoop::Token>     fd_tokens_;
  std::unordered_map<clap_id, PluginRunLoop::Token> timer_tokens_;

  /**
   * @brief Fork-join executor for the current process call (non-owning).
   *
   * Set from ProcessBlockInfo at the top of every process_impl() call and
   * only meaningful during that call; used to serve CLAP thread-pool
   * requests (see threadPoolRequestExec).
   */
  dsp::graph::ForkJoinExecutor * fork_join_executor_ = nullptr;

  /* process stuff */
  std::vector<clap_audio_buffer> audio_in_clap_bufs_;
  std::vector<clap_audio_buffer> audio_out_clap_bufs_;

  // each CLAP port can have multiple channels. there is 1 AudioSampleBuffer per
  // CLAP port
  // FIXME: these temporary buffers can be removed - use audio port buffers
  // directly to avoid unnecessary copies
  std::vector<juce::AudioSampleBuffer> audio_in_bufs_;

  std::vector<juce::AudioSampleBuffer> audio_out_bufs_;

  /**
   * Per-port channel pointer arrays backing clap_audio_buffer::data32.
   *
   * Most ports use the identity order (pointers straight from the JUCE
   * buffer); ports whose plugin wire order is not canonical (surround
   * channel maps) get a permuted order so the plugin's channel i maps to the
   * buffer's canonical channel permutation[i], with no sample copies.
   */
  std::vector<std::vector<float *>> audio_in_channel_ptrs_;
  std::vector<std::vector<float *>> audio_out_channel_ptrs_;

  /**
   * Engine ports for each audio bus, in the plugin's bus enumeration order
   * (index-aligned with the scratch buffers).
   *
   * Ports are paired with buses by stable external id; the plugin's
   * enumeration order is independent of the engine's port list order.
   * Holds nullptr for buses no port carries (yet) while a rescan awaits its
   * deferred reconciliation.
   *
   * The raw pointers stay valid across process blocks because the
   * reconciler never destroys ports; the vectors are only read while the
   * plugin is active and are repopulated on every prepare (and cleared on
   * release).
   */
  std::vector<dsp::AudioPort *> audio_in_ports_by_bus_;
  std::vector<dsp::AudioPort *> audio_out_ports_by_bus_;

  clap::helpers::EventList evIn_;
  clap::helpers::EventList evOut_;

  /** Always-empty input event list for scheduled audio-thread flushes. */
  clap::helpers::EventList evFlushIn_;

  /**
   * @brief Pending audio ports rescan state (see ClapPlugin::audioPortsRescan).
   *
   * The port scan is done eagerly in the rescan callback (only valid while
   * the plugin reports the change, e.g. inside its deactivate() during a
   * restart) and the engine-side reconciliation is deferred. The state is
   * cleared when the plugin instance is recreated so a stale scan never
   * reaches a fresh instance.
   */
  std::array<std::vector<dsp::AudioBusConfig>, 2> pending_audio_port_configs_;
  bool pending_audio_port_scan_ = false;

  /** The index of a flow in the per-flow arrays below. */
  static constexpr size_t flow_index (dsp::PortFlow flow)
  {
    return flow == dsp::PortFlow::Input ? 0 : 1;
  }

  std::vector<dsp::AudioBusConfig> &
  pending_audio_port_configs_for (dsp::PortFlow flow)
  {
    return pending_audio_port_configs_[flow_index (flow)];
  }

  /** Drops any pending rescan state (see pending_audio_port_scan_). */
  void clear_pending_audio_port_scan ()
  {
    pending_audio_port_scan_ = false;
    pending_audio_port_configs_[flow_index (dsp::PortFlow::Input)].clear ();
    pending_audio_port_configs_[flow_index (dsp::PortFlow::Output)].clear ();
  }

  /**
   * Misplaced/absent CLAP_AUDIO_PORT_IS_MAIN flags are warned about once per
   * flow per instance: the scans that would re-report them run several times
   * per load and per rescan.
   */
  mutable std::array<bool, 2> misplaced_main_warning_emitted_{};
  mutable std::array<bool, 2> no_main_warning_emitted_{};

  /** Thread-pool misuse (request_exec without providing
   * clap_plugin_thread_pool) is warned about once per instance: a
   * misbehaving plugin would otherwise spam the log every block on the
   * audio thread. Audio thread only. */
  bool thread_pool_misuse_warning_emitted_ = false;

  clap_process process_{};

  /**
   * Transport info passed to the plugin via process_.transport, filled
   * per block on the audio thread in process_impl().
   */
  clap_event_transport_t transport_{};

  /**
   * @brief Supported note dialects of each input note port (bitmask of
   * clap_note_dialect), queried on the main thread when the ports are
   * created while the plugin is inactive. Read on the audio thread during
   * process_impl().
   */
  std::vector<uint32_t> note_in_supported_dialects_;

  /* param update queues */

  /**
   * Parameter routing tables published to the audio thread. The main
   * thread mutates through a nonRealtime ScopedAccess (paramsRescan,
   * unload); the audio thread holds one realtime ScopedAccess per process
   * block and passes the snapshot down to the event helpers.
   *
   * @pre The engine must not be running process() on this plugin while
   * the plugin is destroyed: farbot's destructor spins until any
   * in-flight realtime access is released.
   */
  farbot::RealtimeObject<
    ParamMaps,
    farbot::RealtimeObjectOptions::nonRealtimeMutatable>
    param_maps_{ ParamMaps{} };

  /**
   * Number of routed parameters, cached on the main thread when maps are
   * published. Used for sizing reads so they don't need a (deep-copying)
   * nonRealtime acquire.
   */
  size_t param_count_ = 0;

  // Written on the audio thread (processing state changes), read on the
  // main thread (rescan gating, latency notification)
  std::atomic<PluginState> state_{ Inactive };
  bool                     stateIsDirty_ = false;

  /** Latency change reported while being-activated (latencyGet() is only
   * allowed once activate() returns) - queried at the end of
   * prepare_plugin_for_processing(). */
  bool latency_dirty_ = false;

  std::atomic_bool scheduleDeactivate_{ false };

  std::atomic_bool scheduleProcess_{ true };

  std::atomic_bool scheduleParamFlush_{ false };

  const char * guiApi_ = nullptr;
  bool         isGuiCreated_ = false;
  bool         isGuiVisible_ = false;
  bool         isGuiFloating_ = false;

  // work-around the fact that stopProcessing() requires being called by an
  // audio thread for whatever reason
  std::atomic_bool force_audio_thread_check_{ false };

  PluginHostWindowFactory                              host_window_factory_;
  std::unique_ptr<PluginHostWindow>                    editor_;
  utils::QObjectUniquePtr<PluginViewResizeCoordinator> resize_coordinator_;

  /** Last processing configuration, needed to re-activate the plugin when
   * it requests a restart. */
  units::sample_rate_t last_sample_rate_{ units::sample_rate (0) };
  units::sample_u32_t  last_max_block_length_{ units::samples (0u) };

  units::sample_u32_t latency_{ units::samples (0u) };
};

ClapPlugin::ClapPlugin (
  utils::IObjectRegistry &registry,
  PluginHostWindowFactory host_window_factory,
  QObject *               parent)
    : Plugin (registry, parent),
      ClapHostBase (
        "Zrythm",
        "Alexandros Theodotou",
        "https://www.zrythm.org",
        PACKAGE_VERSION),
      pimpl_ (
        std::make_unique<ClapPluginImpl> (*this, std::move (host_window_factory)))
{
  is_main_thread = true;

  // Connect to configuration changes
  connect (
    this, &Plugin::configurationChanged, this,
    &ClapPlugin::on_configuration_changed);

  auto bypass_ref = generate_default_bypass_param ();
  add_parameter (bypass_ref);
  set_bypass_id (bypass_ref.id ());
  auto gain_ref = generate_default_gain_param ();
  add_parameter (gain_ref);
  gain_id_ = gain_ref.id ();
}

ClapPlugin::~ClapPlugin ()
{
  if (pimpl_ && pimpl_->library_.is_loaded ())
    unload_current_plugin ();
}

void
ClapPlugin::on_configuration_changed (
  PluginConfiguration *,
  bool generateNewPluginPortsAndParams)
{
  z_debug ("configuration changed");
  const auto &path = std::get<std::filesystem::path> (
    configuration ()->descriptor ()->path_or_id_);
  auto success = load_plugin (
    path, configuration ()->descriptor ()->unique_id_,
    generateNewPluginPortsAndParams);
  Q_EMIT instantiationFinished (
    success,
    success
      ? QString{}
      : tr ("Failed to load CLAP plugin from %1")
          .arg (utils::Utf8String::from_path (path).to_qstring ()));
}

bool
ClapPlugin::hasNativeUi () const
{
  return pimpl_->plugin_ != nullptr && pimpl_->plugin_->canUseGui ();
}

void
ClapPlugin::on_ui_visibility_changed ()
{
  if (uiVisible () && !pimpl_->isGuiVisible_)
    {
      show_editor ();
    }
  else if (!uiVisible () && pimpl_->isGuiVisible_)
    {
      hide_editor ();
    }
}

static clap_window
make_clap_window (plugins::WindowSystem window_system, WId window)
{
  clap_window w{};
  switch (window_system)
    {
    case plugins::WindowSystem::X11:
      w.api = CLAP_WINDOW_API_X11;
      w.x11 = window;
      break;
    case plugins::WindowSystem::Wayland:
      w.api = CLAP_WINDOW_API_WAYLAND;
      w.ptr = reinterpret_cast<void *> (window);
      break;
    case plugins::WindowSystem::Cocoa:
      w.api = CLAP_WINDOW_API_COCOA;
      w.cocoa = reinterpret_cast<clap_nsview> (window);
      break;
    case plugins::WindowSystem::Win32:
      w.api = CLAP_WINDOW_API_WIN32;
      w.win32 = reinterpret_cast<clap_hwnd> (window);
      break;
    }

  return w;
}

void
ClapPlugin::show_editor ()
{
  assert (is_main_thread);

  if (pimpl_->plugin_ == nullptr || !pimpl_->plugin_->canUseGui ())
    return;

  // The GUI is kept alive while hidden - just re-show it
  if (pimpl_->isGuiCreated_)
    {
      pimpl_->set_plugin_window_visibility (true);
      return;
    }

  set_native_ui_unavailable (false);
  pimpl_->editor_ = pimpl_->host_window_factory_ (*this);
  if (pimpl_->editor_ == nullptr)
    {
      z_warning (
        "CLAP: no host window available for plugin editor - showing "
        "generic UI");
      set_native_ui_unavailable (true);
      return;
    }

  const auto embed_id = pimpl_->editor_->getEmbedWindowId ();
  auto       w = make_clap_window (pimpl_->editor_->windowSystem (), embed_id);
  pimpl_->guiApi_ = w.api;

  // The host window failed to embed the plugin's window (already hidden
  // by the window itself): tear down and fall back to the generic UI.
  // Deferred because the emission comes from within the window's own
  // call stack
  connect (
    pimpl_->editor_.get (), &plugins::PluginHostWindow::embeddingFailed, this,
    [this] {
      QTimer::singleShot (std::chrono::milliseconds{ 0 }, this, [this] {
        pimpl_->destroy_gui ();
        set_native_ui_unavailable (true);
      });
    });

  pimpl_->isGuiFloating_ = false;
  {
    const ScopedGlContextRelease gl_release;
    if (!pimpl_->plugin_->guiIsApiSupported (pimpl_->guiApi_, false))
      {
        if (!pimpl_->plugin_->guiIsApiSupported (pimpl_->guiApi_, true))
          {
            z_warning ("could not find a suitable gui api");
            pimpl_->destroy_gui ();
            set_native_ui_unavailable (true);
            return;
          }
        pimpl_->isGuiFloating_ = true;
      }
  }

  {
    const ScopedGlContextRelease gl_release;
    if (!pimpl_->plugin_->guiCreate (w.api, pimpl_->isGuiFloating_))
      {
        z_warning ("could not create the plugin gui");
        pimpl_->destroy_gui ();
        set_native_ui_unavailable (true);
        return;
      }
  }

  pimpl_->isGuiCreated_ = true;
  assert (pimpl_->isGuiVisible_ == false);

  if (pimpl_->isGuiFloating_)
    {
      const ScopedGlContextRelease gl_release;
      pimpl_->plugin_->guiSetTransient (&w);
      pimpl_->plugin_->guiSuggestTitle (get_name ().c_str ());
    }
  else
    {
      // Feed the host scale before querying sizes, per the clap_gui
      // creation sequence (not on Cocoa: it uses logical sizes, so
      // set_scale must not be called there)
      const auto initial_scale = pimpl_->editor_->contentScaleFactor ();
      {
        const ScopedGlContextRelease gl_release;
        if (pimpl_->editor_->windowSystem () != WindowSystem::Cocoa)
          pimpl_->plugin_->guiSetScale (initial_scale);
      }

      // Re-feed the scale and re-negotiate the size when the window's
      // screen scale changes
      connect (
        pimpl_->editor_.get (), &PluginHostWindow::contentScaleFactorChanged,
        this, [this] (float factor) {
          if (pimpl_->editor_ == nullptr || !pimpl_->isGuiCreated_)
            return;
          uint32_t width = 0;
          uint32_t height = 0;
          bool     have_size = false;
          {
            const ScopedGlContextRelease gl_release;
            if (pimpl_->editor_->windowSystem () != WindowSystem::Cocoa)
              pimpl_->plugin_->guiSetScale (factor);
            have_size =
              pimpl_->plugin_->guiGetSize (&width, &height) && width > 0
              && height > 0;
          }
          if (have_size)
            {
              const auto [view_w, view_h] = plugin_view_size_to_host_window_size (
                static_cast<int> (width), static_cast<int> (height), factor);
              pimpl_->editor_->setSize (view_w, view_h);
            }
          else
            {
              z_warning (
                "CLAP plugin '{}' reported an invalid GUI size ({}x{}) after "
                "a scale change; keeping the current size",
                get_name (), width, height);
            }
        });

      uint32_t width = 0;
      uint32_t height = 0;
      bool     have_initial_size = false;
      {
        const ScopedGlContextRelease gl_release;
        have_initial_size =
          pimpl_->plugin_->guiGetSize (&width, &height) && width > 0
          && height > 0;
      }

      if (!have_initial_size)
        {
          z_warning ("could not get the size of the plugin gui");
          pimpl_->destroy_gui ();
          set_native_ui_unavailable (true);
          return;
        }

      const auto [view_w, view_h] = plugin_view_size_to_host_window_size (
        static_cast<int> (width), static_cast<int> (height), initial_scale);
      pimpl_->editor_->setSizeAndCenter (view_w, view_h);
      bool can_resize = false;
      {
        const ScopedGlContextRelease gl_release;
        can_resize = pimpl_->plugin_->guiCanResize ();
      }
      pimpl_->editor_->setResizable (can_resize);

      // Forward host-side embed area resizes (e.g., the user resizing the
      // window) to the plugin
      pimpl_->resize_coordinator_ = utils::make_qobject_unique<
        PluginViewResizeCoordinator> (
        *pimpl_->editor_,
        PluginViewResizeCoordinator::Hooks{
          .gui_active = [this] { return pimpl_->isGuiCreated_; },
          .can_resize =
            [this] {
              const ScopedGlContextRelease gl_release;
              return pimpl_->plugin_->guiCanResize ();
            },
          .adjust_size =
            [this] (int &view_width, int &view_height) {
              const ScopedGlContextRelease gl_release;
              auto physical_width = static_cast<uint32_t> (view_width);
              auto physical_height = static_cast<uint32_t> (view_height);
              pimpl_->plugin_->guiAdjustSize (&physical_width, &physical_height);
              view_width = static_cast<int> (physical_width);
              view_height = static_cast<int> (physical_height);
            },
          .apply_size =
            [this] (int view_width, int view_height) {
              const ScopedGlContextRelease gl_release;
              pimpl_->plugin_->guiSetSize (
                static_cast<uint32_t> (view_width),
                static_cast<uint32_t> (view_height));
            },
        });

      bool set_parent_ok = false;
      {
        const ScopedGlContextRelease gl_release;
        set_parent_ok = pimpl_->plugin_->guiSetParent (&w);
      }
      if (!set_parent_ok)
        {
          z_warning ("could not embbed the plugin gui");
          pimpl_->destroy_gui ();
          set_native_ui_unavailable (true);
          return;
        }
    }

  pimpl_->set_plugin_window_visibility (true);

  // Floating GUIs manage their own window; for embedded GUIs, run the
  // platform embedding handshake (XEmbed on X11) so a plugin whose window
  // never appears falls back to the generic UI
  if (!pimpl_->isGuiFloating_)
    pimpl_->editor_->completeNativeEmbedding ();
}

void
ClapPlugin::hide_editor ()
{
  pimpl_->set_plugin_window_visibility (false);
}

void
ClapPlugin::ClapPluginImpl::set_plugin_window_visibility (bool isVisible)
{
  assert (is_main_thread);

  if (!isGuiCreated_)
    return;

  // For floating GUIs the plugin manages its own window: the (hidden) host
  // window only serves as the transient parent
  if (isVisible && !isGuiVisible_)
    {
      if (!isGuiFloating_)
        editor_->setVisible (true);
      {
        const ScopedGlContextRelease gl_release;
        plugin_->guiShow ();
      }
      isGuiVisible_ = true;
    }
  else if (!isVisible && isGuiVisible_)
    {
      {
        const ScopedGlContextRelease gl_release;
        plugin_->guiHide ();
      }
      if (!isGuiFloating_)
        editor_->setVisible (false);
      isGuiVisible_ = false;
    }
}

void
ClapPlugin::guiResizeHintsChanged () noexcept
{
  // TODO
}

void
ClapPlugin::ClapPluginImpl::destroy_gui ()
{
  assert (is_main_thread);

  if (plugin_ != nullptr && isGuiCreated_)
    {
      const ScopedGlContextRelease gl_release;
      plugin_->guiDestroy ();
      isGuiCreated_ = false;
      isGuiVisible_ = false;
    }
  resize_coordinator_.reset ();
  editor_.reset ();
}

bool
ClapPlugin::guiRequestResize (uint32_t width, uint32_t height) noexcept
{
  if (width == 0 || height == 0)
    {
      z_warning (
        "CLAP plugin '{}' requested an invalid GUI resize to {}x{}; refusing",
        get_name (), width, height);
      return false;
    }
  // Deferred: resizing the host window calls gui.set_size() back on the
  // plugin, which may only happen once its own request_resize() returned
  return post_main_thread_action_deferred ([this, width, height] {
    if (pimpl_->editor_ != nullptr)
      {
        const auto [w, h] = plugin_view_size_to_host_window_size (
          static_cast<int> (width), static_cast<int> (height),
          pimpl_->editor_->contentScaleFactor ());
        pimpl_->editor_->setSize (w, h);
      }
  });
}

bool
ClapPlugin::guiRequestShow () noexcept
{
  // Deferred: showing the UI calls gui.show() back on the plugin, which may
  // only happen once its own request_show() returned
  return post_main_thread_action_deferred ([this] { setUiVisible (true); });
}

bool
ClapPlugin::guiRequestHide () noexcept
{
  // Deferred: hiding the UI calls gui.hide() back on the plugin, which may
  // only happen once its own request_hide() returned
  return post_main_thread_action_deferred ([this] { setUiVisible (false); });
}

void
ClapPlugin::guiClosed (bool wasDestroyed) noexcept
{
  // Deferred: guiDestroy() may only be called once the plugin's own
  // gui.closed() call has returned
  post_main_thread_action_deferred ([this, wasDestroyed] {
    // The spec requires the host to call guiDestroy() to acknowledge
    // plugin-side GUI destruction; destroying unconditionally covers the
    // floating-window-closed and connection-lost cases too
    z_debug (
      "CLAP: plugin GUI closed by the plugin (was_destroyed={})", wasDestroyed);
    pimpl_->destroy_gui ();
    setUiVisible (false);
  });
}

bool
ClapPlugin::posixFdSupportRegisterFd (int fd, clap_posix_fd_flags_t flags) noexcept
{
  assert (is_main_thread);

  z_warn_if_fail (pimpl_->plugin_->canUsePosixFdSupport ())

    // Re-registering an already-registered fd replaces the old watch
    const auto it = pimpl_->fd_tokens_.find (fd);
  if (it != pimpl_->fd_tokens_.end ())
    {
      pimpl_->run_loop_.unregister_fd (it->second);
    }

  const auto token = pimpl_->run_loop_.register_fd (
    fd, (flags & CLAP_POSIX_FD_READ) != 0, (flags & CLAP_POSIX_FD_WRITE) != 0,
    [this, fd] (bool read_ready) {
      assert (is_main_thread);
      pimpl_->plugin_->posixFdSupportOnFd (
        fd, read_ready ? CLAP_POSIX_FD_READ : CLAP_POSIX_FD_WRITE);
    });
  pimpl_->fd_tokens_.insert_or_assign (fd, token);
  return true;
}

bool
ClapPlugin::posixFdSupportModifyFd (int fd, clap_posix_fd_flags_t flags) noexcept
{
  assert (is_main_thread);

  z_warn_if_fail (pimpl_->plugin_->canUsePosixFdSupport ());

  const auto it = pimpl_->fd_tokens_.find (fd);
  if (it == pimpl_->fd_tokens_.end ()) [[unlikely]]
    {
      z_warning (
        "CLAP plugin '{}' tried to modify an unregistered fd ({})",
        get_node_name (), fd);
      return false;
    }

  pimpl_->run_loop_.update_fd (
    it->second, (flags & CLAP_POSIX_FD_READ) != 0,
    (flags & CLAP_POSIX_FD_WRITE) != 0);
  return true;
}

bool
ClapPlugin::posixFdSupportUnregisterFd (int fd) noexcept
{
  assert (is_main_thread);

  z_warn_if_fail (pimpl_->plugin_->canUsePosixFdSupport ());

  const auto it = pimpl_->fd_tokens_.find (fd);
  if (it == pimpl_->fd_tokens_.end ()) [[unlikely]]
    {
      z_warning (
        "CLAP plugin '{}' tried to unregister an unregistered fd ({})",
        get_node_name (), fd);
      return false;
    }

  pimpl_->run_loop_.unregister_fd (it->second);
  pimpl_->fd_tokens_.erase (it);
  return true;
}

bool
ClapPlugin::threadPoolRequestExec (uint32_t numTasks) noexcept
{
  // The assert guards direct calls; clap-helpers' proxy already aborts
  // proxied calls made outside the process call
  assert (threadCheckIsAudioThread ());

  if (!pimpl_->plugin_->canUseThreadPool ())
    {
      // Contract violation: the plugin called request_exec without providing
      // clap_plugin_thread_pool
      if (!pimpl_->thread_pool_misuse_warning_emitted_)
        {
          pimpl_->thread_pool_misuse_warning_emitted_ = true;
          z_warning (
            "CLAP plugin '{}' called thread-pool request_exec without "
            "providing the thread-pool extension",
            get_node_name ());
        }
      return false;
    }

  if (numTasks == 0)
    return true;

  auto * executor = pimpl_->fork_join_executor_;
  if (executor == nullptr)
    {
      // No executor in this processing context: only a trivial request can
      // be served inline; larger ones are rejected so the plugin falls back
      // to processing by its own means (as the spec allows us to reject)
      if (numTasks == 1)
        {
          pimpl_->plugin_->threadPoolExec (0);
          return true;
        }
      return false;
    }

  // The blocking fork-join here is sanctioned by the CLAP thread-pool spec
  // (and runs inside the RTSan disabler around plugin_->process()).
  // Single-task jobs run inline on this thread and nested submissions are
  // rejected by exec()
  return executor->exec (
    [] (void * context, uint32_t task_index) noexcept {
      static_cast<ClapPluginProxy *> (context)->threadPoolExec (task_index);
    },
    pimpl_->plugin_.get (), numTasks);
}

bool
ClapPlugin::timerSupportRegisterTimer (
  uint32_t  periodMs,
  clap_id * timerId) noexcept
{
  assert (is_main_thread);

  // Dexed fails this check even though it uses timer so make it a warning...
  z_warn_if_fail (pimpl_->plugin_->canUseTimerSupport ());

  auto id = pimpl_->nextTimerId_++;
  *timerId = id;

  const auto token = pimpl_->run_loop_.register_timer (
    std::chrono::milliseconds{ periodMs }, [this, id] {
      assert (is_main_thread);
      pimpl_->plugin_->timerSupportOnTimer (id);
    });
  pimpl_->timer_tokens_.insert_or_assign (id, token);
  return true;
}

bool
ClapPlugin::timerSupportUnregisterTimer (clap_id timerId) noexcept
{
  assert (is_main_thread);

  z_warn_if_fail (pimpl_->plugin_->canUseTimerSupport ());

  const auto it = pimpl_->timer_tokens_.find (timerId);
  if (it == pimpl_->timer_tokens_.end ()) [[unlikely]]
    {
      z_warning (
        "CLAP plugin '{}' tried to unregister an unregistered timer ({})",
        get_node_name (), timerId);
      return false;
    }

  pimpl_->run_loop_.unregister_timer (it->second);
  pimpl_->timer_tokens_.erase (it);
  return true;
}

void
ClapPlugin::presetLoadLoaded (
  uint32_t     locationKind,
  const char * location,
  const char * loadKey) noexcept
{
  assert (is_main_thread);
  z_info (
    "CLAP preset loaded: location_kind={} location='{}' load_key='{}'",
    locationKind, location, loadKey);
}

void
ClapPlugin::presetLoadOnError (
  uint32_t     locationKind,
  const char * location,
  const char * loadKey,
  int32_t      osError,
  const char * msg) noexcept
{
  assert (is_main_thread);
  z_warning (
    "CLAP preset load error: location_kind={} location='{}' load_key='{}' "
    "os_error={} msg='{}'",
    locationKind, location, loadKey, osError, msg);
}

void
ClapPlugin::stateMarkDirty () noexcept
{
  assert (is_main_thread);

  z_warn_if_fail (pimpl_->plugin_->canUseState ());

  pimpl_->stateIsDirty_ = true;
}

void
ClapPlugin::latencyChanged () noexcept
{
  // [main-thread & being-activated] per the latency extension contract
  z_return_if_fail (is_main_thread);

  if (pimpl_->is_plugin_active ())
    {
      const auto latency = pimpl_->plugin_->latencyGet ();
      z_debug ("{} latency changed to {}", get_name (), latency);
      pimpl_->latency_ = units::samples (latency);
      notify_latency_changed ();
      return;
    }
  // latencyGet() is only allowed once activate() returns - defer the
  // query until activation completes (prepare_plugin_for_processing)
  pimpl_->latency_dirty_ = true;
}

void
ClapPlugin::prepare_plugin_for_processing (
  units::sample_rate_t sample_rate,
  units::sample_u32_t  max_block_length)
{
  assert (is_main_thread);

  if (!pimpl_->plugin_)
    return;

  // Clear resources if already active (this also deactivates the plugin)
  if (pimpl_->is_plugin_active ())
    {
      release_resources_impl ();
    }

  pimpl_->last_sample_rate_ = sample_rate;
  pimpl_->last_max_block_length_ = max_block_length;

  if (!pimpl_->setup_audio_ports_for_processing (max_block_length))
    {
      // The port report is untrustworthy (warned about during enumeration).
      // Activating would hand the plugin a process struct contradicting its
      // own declaration, risking out-of-bounds access inside the plugin
      pimpl_->set_plugin_state (ClapPluginImpl::InactiveWithError);
      return;
    }

  const size_t max_midi_events =
    static_cast<size_t> (get_descriptor ().num_midi_ins_)
    * static_cast<size_t> (max_block_length.in (units::samples)) * 4;
  const size_t total_events = pimpl_->param_count_ + max_midi_events;

  // Pre-allocate the input event list so that
  // generate_changed_param_input_events() and generate_midi_input_events()
  // never
  // need to grow on the audio thread. Both the events vector and the heap
  // must be reserved (see MidiPort::prepare_for_processing for buffer size).
  pimpl_->evIn_.reserveEvents (total_events);
  pimpl_->evIn_.reserveHeap (
    total_events
    * (sizeof (clap_event_param_value) + alignof (clap_event_param_value)));

  if (
    !pimpl_->plugin_->activate (
      sample_rate.in (units::sample_rate), 1,
      max_block_length.in (units::samples)))
    {
      pimpl_->set_plugin_state (ClapPluginImpl::InactiveWithError);
      return;
    }

  pimpl_->scheduleProcess_ = true;
  pimpl_->set_plugin_state (ClapPluginImpl::ActiveAndSleeping);
  if (pimpl_->plugin_->canUseLatency ())
    {
      pimpl_->latency_ = units::samples (pimpl_->plugin_->latencyGet ());
    }
  if (pimpl_->latency_dirty_)
    {
      // The plugin reported a latency change from within activate() - the
      // value was just queried above, only the notification is due
      pimpl_->latency_dirty_ = false;
      notify_latency_changed ();
    }
}

void
ClapPlugin::release_resources_impl ()
{
  assert (is_main_thread);

  if (!pimpl_->is_plugin_active ())
    return;

  if (pimpl_->state_ == ClapPluginImpl::ActiveAndProcessing)
    {
      // Pretend to be the audio thread — stopProcessing() is called here
      // from the main thread during release_resources, but the CLAP plugin
      // expects it from the audio thread.
      AtomicBoolRAII audio_thread_check{ pimpl_->force_audio_thread_check_ };
      pimpl_->plugin_->stopProcessing ();
    }
  pimpl_->set_plugin_state (ClapPluginImpl::ActiveAndReadyToDeactivate);
  pimpl_->scheduleDeactivate_ = false;

  pimpl_->plugin_->deactivate ();
  pimpl_->set_plugin_state (ClapPluginImpl::Inactive);

  // Stale bus pairings are never read while inactive; clear them so a
  // missed prepare cannot pair new buses with old ports
  pimpl_->audio_in_ports_by_bus_.clear ();
  pimpl_->audio_out_ports_by_bus_.clear ();
}

void
ClapPlugin::process_impl (
  dsp::graph::ProcessBlockInfo time_info,
  const dsp::ITransport       &transport,
  const dsp::TempoMap         &tempo_map) noexcept
{
  ScopedBool audio_thread_guard{ is_audio_thread };

  pimpl_->fork_join_executor_ = time_info.fork_join_executor_;
  pimpl_->process_.frames_count = time_info.nframes_.in (units::samples);
  pimpl_->process_.steady_time = -1;

  if (!pimpl_->plugin_)
    return;

  // Can't process a plugin that is not active
  if (!pimpl_->is_plugin_active ())
    return;

  // Do we want to deactivate the plugin?
  if (pimpl_->scheduleDeactivate_.load (std::memory_order_acquire))
    {
      pimpl_->scheduleDeactivate_.store (false, std::memory_order_release);
      if (pimpl_->state_ == ClapPluginImpl::ActiveAndProcessing)
        pimpl_->plugin_->stopProcessing ();
      pimpl_->set_plugin_state (ClapPluginImpl::ActiveAndReadyToDeactivate);
      return;
    }

  // We can't process a plugin which failed to start processing
  if (pimpl_->state_ == ClapPluginImpl::ActiveWithError)
    return;

  {
    const auto transport_context = build_plugin_transport_context (
      transport, tempo_map, time_info.transport_position_);
    // CLAP times are 64-bit fixed point; llround keeps 64 bits on all
    // platforms (long is 32-bit on Windows and overflows past 1 beat/second)
    const auto to_beattime = [] (units::quarter_note_t quarters) {
      return static_cast<clap_beattime> (std::llround (
        quarters.in (units::quarter_notes)
        * static_cast<double> (CLAP_BEATTIME_FACTOR)));
    };
    const auto to_sectime = [] (units::precise_second_t seconds) {
      return static_cast<clap_sectime> (std::llround (
        seconds.in (units::seconds)
        * static_cast<double> (CLAP_SECTIME_FACTOR)));
    };
    // The CLAP transport flags are a C enum; combine them as the uint32_t
    // field type
    const auto flag_if = [] (bool condition, clap_transport_flags flag) {
      return condition ? static_cast<uint32_t> (flag) : 0u;
    };
    auto &transport_info = pimpl_->transport_;
    transport_info = {};
    transport_info.header.size = sizeof (transport_info);
    transport_info.header.type = CLAP_EVENT_TRANSPORT;
    transport_info.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
    transport_info.flags =
      static_cast<uint32_t> (
        CLAP_TRANSPORT_HAS_TEMPO | CLAP_TRANSPORT_HAS_BEATS_TIMELINE
        | CLAP_TRANSPORT_HAS_SECONDS_TIMELINE | CLAP_TRANSPORT_HAS_TIME_SIGNATURE)
      | flag_if (transport_context.playing_, CLAP_TRANSPORT_IS_PLAYING)
      | flag_if (transport_context.recording_, CLAP_TRANSPORT_IS_RECORDING)
      | flag_if (transport_context.loop_enabled_, CLAP_TRANSPORT_IS_LOOP_ACTIVE)
      | flag_if (
        transport_context.within_preroll_, CLAP_TRANSPORT_IS_WITHIN_PRE_ROLL);
    transport_info.song_pos_beats = to_beattime (transport_context.position_);
    transport_info.song_pos_seconds =
      to_sectime (transport_context.position_seconds_);
    transport_info.tempo = transport_context.tempo_.in (units::bpm);
    transport_info.tempo_inc = 0.0;
    transport_info.loop_start_beats =
      to_beattime (transport_context.loop_start_);
    transport_info.loop_end_beats = to_beattime (transport_context.loop_end_);
    transport_info.loop_start_seconds =
      to_sectime (transport_context.loop_start_seconds_);
    transport_info.loop_end_seconds =
      to_sectime (transport_context.loop_end_seconds_);
    transport_info.bar_start = to_beattime (transport_context.bar_start_);
    transport_info.bar_number = transport_context.bar_number_;
    transport_info.tsig_num =
      static_cast<uint16_t> (transport_context.time_sig_numerator_);
    transport_info.tsig_denom =
      static_cast<uint16_t> (transport_context.time_sig_denominator_);
    pimpl_->process_.transport = &transport_info;
  }

  pimpl_->process_.in_events = pimpl_->evIn_.clapInputEvents ();
  pimpl_->process_.out_events = pimpl_->evOut_.clapOutputEvents ();

  pimpl_->process_.audio_inputs = pimpl_->audio_in_clap_bufs_.data ();
  pimpl_->process_.audio_inputs_count =
    static_cast<uint32_t> (pimpl_->audio_in_clap_bufs_.size ());
  pimpl_->process_.audio_outputs = pimpl_->audio_out_clap_bufs_.data ();
  pimpl_->process_.audio_outputs_count =
    static_cast<uint32_t> (pimpl_->audio_out_clap_bufs_.size ());

  pimpl_->evOut_.clear ();

  // One snapshot for the whole block: the helpers below read a consistent
  // view of the routing tables, even if a rescan publishes mid-block
  decltype (pimpl_->param_maps_)::ScopedAccess<farbot::ThreadType::realtime>
    param_maps{ pimpl_->param_maps_ };

  pimpl_->generate_changed_param_input_events (*param_maps);
  pimpl_->generate_midi_input_events (
    time_info.buffer_offset_, time_info.nframes_);

  // Honor scheduled flushes (plugin-requested via request_flush(), or after
  // a runtime state load): paramsFlush is audio-thread-only while the plugin
  // is active. Flush with empty input events so the plugin can resolve and
  // output parameter values without re-receiving this cycle's input events.
  if (pimpl_->scheduleParamFlush_.exchange (false, std::memory_order_acq_rel))
    {
#if defined(__has_feature) && __has_feature(realtime_sanitizer)
      // Not our code, we don't care about RTSan violations here.
      __rtsan::ScopedDisabler d;
#endif
      pimpl_->plugin_->paramsFlush (
        pimpl_->evFlushIn_.clapInputEvents (),
        pimpl_->evOut_.clapOutputEvents ());
      pimpl_->handle_plugin_output_events (
        *param_maps, time_info.buffer_offset_, time_info.nframes_);
      pimpl_->evOut_.clear ();
    }

  if (pimpl_->is_plugin_sleeping ())
    {
      if (
        !pimpl_->scheduleProcess_.load (std::memory_order_acquire)
        && pimpl_->evIn_.empty ())
        // The plugin is sleeping, there is no request to wake it up and there
        // are no events to process
        return;

      pimpl_->scheduleProcess_.store (false, std::memory_order_release);
      if (!pimpl_->plugin_->startProcessing ())
        {
          // the plugin failed to start processing
          pimpl_->set_plugin_state (ClapPluginImpl::ActiveWithError);
          return;
        }

      pimpl_->set_plugin_state (ClapPluginImpl::ActiveAndProcessing);
    }

  int32_t status = CLAP_PROCESS_SLEEP;
  if (pimpl_->is_plugin_processing ())
    {
      const auto local_offset = time_info.buffer_offset_;
      const auto nframes = time_info.nframes_;

      // Copy the chunk's input audio to the scratch buffers the plugin
      // reads from offset 0 (data32 points at the scratch base). Buses are
      // paired with ports by stable id (enumeration order may differ from
      // the port list order after a list rescan). The scratch channel count
      // can exceed the port's while a channel-count rescan awaits its
      // deferred reconciliation, so the copy is clamped to both and the
      // unfed channels are cleared
      for (
        const auto &[in_buf, port] :
        std::views::zip (pimpl_->audio_in_bufs_, pimpl_->audio_in_ports_by_bus_))
        {
          // A bus without a port while a rescan awaits reconciliation: its
          // input is silence
          if (port == nullptr)
            {
              in_buf.clear (0, nframes.in<int> (units::samples));
              continue;
            }
          // Reconciliation drops port buffers; a missed re-prepare (which
          // must never happen: the graph recalculation precedes the resume)
          // surfaces here instead of dereferencing null
          assert (port->buffers () != nullptr);
          const auto scratch_channels = in_buf.getNumChannels ();
          const auto fed_channels =
            std::min (scratch_channels, port->buffers ()->getNumChannels ());
          for (const auto ch : std::views::iota (0, fed_channels))
            {
              in_buf.copyFrom (
                ch, 0, *port->buffers (), ch,
                local_offset.in<int> (units::samples),
                nframes.in<int> (units::samples));
            }
          for (const auto ch : std::views::iota (fed_channels, scratch_channels))
            {
              in_buf.clear (ch, 0, nframes.in<int> (units::samples));
            }
        }

      // Run plugin processing
      {
#if defined(__has_feature) && __has_feature(realtime_sanitizer)
        // Not our code, we don't care about RTSan violations here.
        // TODO: add option to keep this enabled (we might want to test our own
        // CLAP plugins in the future)
        __rtsan::ScopedDisabler d;
#endif
        status = pimpl_->plugin_->process (&pimpl_->process_);
      }

      // Copy the chunk's output audio from the scratch buffers back to the
      // chunk's position in the port buffers (paired by stable id, like the
      // input copy above)
      for (
        const auto &[buf, port] : std::views::zip (
          pimpl_->audio_out_bufs_, pimpl_->audio_out_ports_by_bus_))
        {
          // A bus without a port while a rescan awaits reconciliation has
          // its output dropped; on a processing error the port buffers keep
          // the silence process_block cleared into them before this block
          if (port == nullptr || status == CLAP_PROCESS_ERROR) [[unlikely]]
            continue;
          // Same tripwire as the input copy above
          assert (port->buffers () != nullptr);

          // TODO: handle other states
          // Clamp like the input copy above: the scratch channel count
          // can exceed the port's while a channel-count rescan awaits
          // its deferred reconciliation
          const auto port_channels = port->buffers ()->getNumChannels ();
          const auto channels = std::min (buf.getNumChannels (), port_channels);
          for (const auto ch : std::views::iota (0, channels))
            {
              port->buffers ()->copyFrom (
                ch, local_offset.in<int> (units::samples), buf, ch, 0,
                nframes.in<int> (units::samples));
            }
        }
    }

  pimpl_->handle_plugin_output_events (
    *param_maps, time_info.buffer_offset_, time_info.nframes_);

  pimpl_->evOut_.clear ();
  pimpl_->evIn_.clear ();

  // TODO: send plugin to sleep if possible
}

units::sample_u32_t
ClapPlugin::get_single_playback_latency () const
{
  return pimpl_->latency_;
}

bool
ClapPlugin::load_plugin (
  const std::filesystem::path &path,
  int64_t                      plugin_unique_id,
  bool                         generate_new_ports)
{
  assert (is_main_thread);

  if (pimpl_->library_.is_loaded ())
    unload_current_plugin ();

  if (!pimpl_->library_.load (utils::Utf8String::from_path (path)))
    {
      z_warning (
        "Failed to load plugin '{}': {}", path,
        pimpl_->library_.error_string ());
      return false;
    }

  pimpl_->pluginEntry_ = reinterpret_cast<const struct clap_plugin_entry *> (
    pimpl_->library_.resolve ("clap_entry"));
  if (pimpl_->pluginEntry_ == nullptr)
    {
      z_warning ("Unable to resolve entry point 'clap_entry' in '{}'", path);
      pimpl_->library_.unload ();
      return false;
    }

  if (!pimpl_->pluginEntry_->init (utils::Utf8String::from_path (path).c_str ()))
    {
      z_warning ("clap_entry->init() failed for '{}'", path);
    }

  pimpl_->pluginFactory_ = static_cast<const clap_plugin_factory *> (
    pimpl_->pluginEntry_->get_factory (CLAP_PLUGIN_FACTORY_ID));
  if (pimpl_->pluginFactory_ == nullptr)
    {
      z_warning ("Plugin '{}' has no CLAP plugin factory", path);
      return false;
    }

  const auto * const desc = [&] () -> const clap_plugin_descriptor_t * {
    const auto count =
      pimpl_->pluginFactory_->get_plugin_count (pimpl_->pluginFactory_);
    for (const auto i : std::views::iota (0u, count))
      {
        const auto * cur_desc = pimpl_->pluginFactory_->get_plugin_descriptor (
          pimpl_->pluginFactory_, i);
        if (cur_desc == nullptr || cur_desc->id == nullptr)
          continue;
        if (get_hash_for_range (std::string (cur_desc->id)) == plugin_unique_id)
          {
            return cur_desc;
          }
      }
    return nullptr;
  }();

  if (desc == nullptr)
    {
      z_warning ("no plugin descriptor");
      return false;
    }

  if (!clap_version_is_compatible (desc->clap_version))
    {
      z_warning (
        "Incompatible clap version: Plugin is: {}.{}.{} Host is: {}.{}.{}",
        desc->clap_version.major, desc->clap_version.minor,
        desc->clap_version.revision, CLAP_VERSION.major, CLAP_VERSION.minor,
        CLAP_VERSION.revision);
      return false;
    }

  z_info ("Loading plugin with id: {}", desc->id);

  const auto * const plugin = pimpl_->pluginFactory_->create_plugin (
    pimpl_->pluginFactory_, clapHost (), desc->id);
  if (plugin == nullptr)
    {
      z_warning ("could not create the plugin with id: {}", desc->id);
      return false;
    }

  pimpl_->plugin_ = std::make_unique<ClapPluginProxy> (*plugin, *this);

  if (!pimpl_->plugin_->init ())
    {
      z_warning ("could not init the plugin with id: {}", desc->id);
      return false;
    }

  if (generate_new_ports)
    {
      create_ports_from_clap_plugin ();
    }
  paramsRescan (CLAP_PARAM_RESCAN_ALL);
  // scanQuickControls ();

  // Apply any pending state from JSON deserialization
  if (state_to_apply_.has_value ())
    {
      z_debug (
        "CLAP: applying saved state ({} bytes)", state_to_apply_->size ());

      apply_state_from_byte_array (*state_to_apply_);
      state_to_apply_.reset ();

      // The plugin is inactive at this point, so flushing and syncing on the
      // main thread is allowed
      flush_and_sync_params_when_inactive ();
    }
  else
    {
      z_debug ("CLAP: no saved state to apply");
    }

  if (!generate_new_ports)
    {
      // Ports were restored from the project: negotiate the saved bus
      // topology into the (still inactive) plugin, after its state is
      // applied so the negotiation acts on the final state. Loading always
      // recalculates the graph afterwards, so the result is discarded
      restore_saved_bus_arrangements ();
    }

  Q_EMIT pluginLoadedChanged (true);

  Q_EMIT hasNativeUiChanged ();

  return true;
}

void
ClapPlugin::unload_current_plugin ()
{
  assert (is_main_thread);

  {
    decltype (pimpl_->param_maps_)::ScopedAccess<farbot::ThreadType::nonRealtime>
      param_maps{ pimpl_->param_maps_ };
    *param_maps = ClapPluginImpl::ParamMaps{};
  }
  pimpl_->param_count_ = 0;

  Q_EMIT pluginLoadedChanged (false);
  Q_EMIT hasNativeUiChanged ();

  if (!pimpl_->library_.is_loaded ())
    return;

  pimpl_->destroy_gui ();

  release_resources_impl ();

  if (pimpl_->plugin_)
    {
      pimpl_->plugin_->destroy ();
      pimpl_->plugin_.reset ();
      // Drop watches the plugin did not unregister before destroy()
      // closed its fds, so no notifier observes a dead fd and no timer
      // fires into the destroyed plugin
      pimpl_->run_loop_.clear ();
      pimpl_->fd_tokens_.clear ();
      pimpl_->timer_tokens_.clear ();
    }

  // A rescan scanned from the destroyed instance is stale; its deferred
  // action must not reconcile ports against dead configurations
  pimpl_->clear_pending_audio_port_scan ();

  pimpl_->pluginEntry_->deinit ();
  pimpl_->pluginEntry_ = nullptr;

  pimpl_->library_.unload ();
}

bool
ClapPlugin::ClapPluginImpl::is_plugin_active () const
{
  switch (state_)
    {
    case Inactive:
    case InactiveWithError:
      return false;
    default:
      return true;
    }
}

bool
ClapPlugin::ClapPluginImpl::is_plugin_processing () const
{
  return state_ == ActiveAndProcessing;
}

bool
ClapPlugin::ClapPluginImpl::is_plugin_sleeping () const
{
  return state_ == ActiveAndSleeping;
}

bool
ClapPlugin::threadCheckIsAudioThread () const noexcept
{
  if (pimpl_->force_audio_thread_check_.load ())
    {
      return true;
    }

  return is_audio_thread;
}
bool
ClapPlugin::threadCheckIsMainThread () const noexcept
{
  return is_main_thread;
}

void
ClapPlugin::ClapPluginImpl::generate_changed_param_input_events (
  const ParamMaps &maps) noexcept
{
  // Audio thread: send only changed params with feedback prevention.
  for (const auto &change : owner_.change_tracker ().changes ())
    {
      auto &entry = owner_.param_sync_.entries[change.index];

      // Feedback prevention: skip if value came from the plugin
      if (
        utils::math::floats_equal (
          change.modulated_value,
          entry.last_from_plugin.load (std::memory_order_relaxed)))
        {
          entry.last_from_plugin = -1.f;
          continue;
        }

      // Find CLAP ID for this param
      auto * param = change.param;
      auto   it = maps.by_param_.find (param);
      if (it == maps.by_param_.end ())
        continue;

      const clap_id clap_id_val = it->second;
      auto          adapter_it = maps.by_id_.find (clap_id_val);
      if (adapter_it == maps.by_id_.end ())
        continue;

      const auto  &adapter = adapter_it->second;
      const auto   range = param->range ();
      const double clap_value = range.convertFrom0To1 (change.modulated_value);

      clap_event_param_value ev{};
      ev.header.time = 0;
      ev.header.type = CLAP_EVENT_PARAM_VALUE;
      ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
      ev.header.flags = 0;
      ev.header.size = sizeof (ev);
      ev.param_id = clap_id_val;
      ev.cookie = adapter.info.cookie;
      ev.port_index = 0;
      ev.key = -1;
      ev.channel = -1;
      ev.note_id = -1;
      ev.value = clap_value;
      evIn_.push (&ev.header);
    }
}

void
ClapPlugin::ClapPluginImpl::generate_all_param_input_events (
  const ParamMaps &maps)
{
  // Main thread path (paramFlush): send all base values.
  for (const auto &[zrythm_param, clap_id_val] : maps.by_param_)
    {
      auto it = maps.by_id_.find (clap_id_val);
      if (it == maps.by_id_.end ())
        continue;

      const auto  &adapter = it->second;
      const auto   range = zrythm_param->range ();
      const float  zrythm_value = zrythm_param->baseValue ();
      const double clap_value = range.convertFrom0To1 (zrythm_value);

      clap_event_param_value ev{};
      ev.header.time = 0;
      ev.header.type = CLAP_EVENT_PARAM_VALUE;
      ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
      ev.header.flags = 0;
      ev.header.size = sizeof (ev);
      ev.param_id = clap_id_val;
      ev.cookie = adapter.info.cookie;
      ev.port_index = 0;
      ev.key = -1;
      ev.channel = -1;
      ev.note_id = -1;
      ev.value = clap_value;
      evIn_.push (&ev.header);
    }
}

void
ClapPlugin::ClapPluginImpl::generate_midi_input_events (
  units::sample_u32_t local_offset,
  units::sample_u32_t nframes) noexcept
{
  // Fill MIDI events from the first MIDI input port (multi-note-port input
  // routing is not supported; see the dialect TODO below)
  if (owner_.midi_in_ports_.empty ())
    return;

  // Send raw MIDI events if the first note port supports the MIDI dialect
  // (lossless passthrough of the internal MIDI stream). Otherwise, convert
  // note on/off messages to CLAP note events, which all note ports are
  // required to support.
  // TODO: only the first note port's dialect is checked; plugins with
  // multiple note ports that declare different dialects are not handled.
  const bool send_midi_dialect =
    note_in_supported_dialects_.empty ()
    || (note_in_supported_dialects_[0] & CLAP_NOTE_DIALECT_MIDI) != 0;

  // The port buffer holds events for the whole cycle: only emit events
  // inside this chunk, re-based to chunk-relative times
  const auto in_chunk = [local_offset, nframes] (const auto &ev) {
    return ev.time () >= local_offset && ev.time () < local_offset + nframes;
  };
  auto chunk_events =
    owner_.midi_in_ports_.front ()->buffer_ | std::views::filter (in_chunk);

  if (send_midi_dialect)
    {
      // Raw MIDI passthrough covers CC, pitch bend, aftertouch and SysEx
      // losslessly.
      // TODO: assign unique note_ids so CLAP synths that support per-note
      // expression can track individual notes.
      for (const auto &ev : chunk_events)
        {
          const auto chunk_time =
            (ev.time () - local_offset).in<uint32_t> (units::samples);
          const auto ev_data = ev.data ();
          if (ev_data.size () <= 3)
            {
              clap_event_midi clap_ev{};
              clap_ev.header.time = chunk_time;
              clap_ev.header.type = CLAP_EVENT_MIDI;
              clap_ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
              clap_ev.header.flags = 0;
              clap_ev.header.size = sizeof (clap_ev);
              clap_ev.port_index = 0;
              std::ranges::copy (ev_data, clap_ev.data);
              evIn_.push (&clap_ev.header);
            }
          else
            {
              clap_event_midi_sysex clap_ev{};
              clap_ev.header.time = chunk_time;
              clap_ev.header.type = CLAP_EVENT_MIDI_SYSEX;
              clap_ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
              clap_ev.header.flags = 0;
              clap_ev.header.size = sizeof (clap_ev);
              clap_ev.port_index = 0;
              clap_ev.buffer = ev_data.data ();
              clap_ev.size = static_cast<uint32_t> (ev_data.size ());
              evIn_.push (&clap_ev.header);
            }
        }
      return;
    }

  for (const auto &ev : chunk_events)
    {
      const auto chunk_time =
        (ev.time () - local_offset).in<uint32_t> (units::samples);
      const auto ev_data = ev.data ();
      if (ev_data.size () < 2)
        continue;

      const auto status = ev_data[0] & 0xF0;

      // Polyphonic (3-byte) and channel (2-byte) aftertouch -> note
      // expressions (channel pressure uses key=-1 for channel-wide). CC and
      // pitch bend have no CLAP-dialect representation (the midi-mappings
      // extension is not widespread), so they remain dropped in this path.
      if ((status == 0xA0 && ev_data.size () >= 3) || status == 0xD0)
        {
          clap_event_note_expression clap_ev{};
          clap_ev.header.size = sizeof (clap_ev);
          clap_ev.header.time = chunk_time;
          clap_ev.header.type = CLAP_EVENT_NOTE_EXPRESSION;
          clap_ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
          clap_ev.header.flags = 0;
          clap_ev.expression_id = CLAP_NOTE_EXPRESSION_PRESSURE;
          clap_ev.note_id = -1;
          clap_ev.port_index = 0;
          clap_ev.channel = static_cast<int16_t> (ev_data[0] & 0x0F);
          clap_ev.key =
            status == 0xA0
              ? static_cast<int16_t> (ev_data[1])
              : static_cast<int16_t> (-1);
          clap_ev.value =
            status == 0xA0 ? ev_data[2] / 127.0 : ev_data[1] / 127.0;
          evIn_.push (&clap_ev.header);
          continue;
        }

      if (ev_data.size () < 3)
        continue;

      const bool is_note_on = status == 0x90 && ev_data[2] != 0;
      const bool is_note_off =
        status == 0x80 || (status == 0x90 && ev_data[2] == 0);
      if (!is_note_on && !is_note_off)
        continue;

      clap_event_note clap_ev{};
      clap_ev.header.size = sizeof (clap_ev);
      clap_ev.header.time = chunk_time;
      clap_ev.header.type =
        is_note_on ? CLAP_EVENT_NOTE_ON : CLAP_EVENT_NOTE_OFF;
      clap_ev.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
      clap_ev.header.flags = 0;
      clap_ev.note_id = -1; // no per-note tracking
      clap_ev.port_index = 0;
      clap_ev.channel = static_cast<int16_t> (ev_data[0] & 0x0F);
      clap_ev.key = static_cast<int16_t> (ev_data[1]);
      clap_ev.velocity = ev_data[2] / 127.0;
      evIn_.push (&clap_ev.header);
    }
}

void
ClapPlugin::ClapPluginImpl::handle_plugin_output_events (
  const ParamMaps                   &maps,
  units::sample_u32_t                event_time_offset,
  std::optional<units::sample_u32_t> block_length) noexcept
{
  for (uint32_t i = 0; i < evOut_.size (); ++i)
    {
      auto * h = evOut_.get (i);
      // Event times must lie within the processed block
      if (block_length.has_value () && units::samples (h->time) >= *block_length)
        {
          note_invalid_output_event_drop ("event time out of range"sv);
          continue;
        }
      switch (h->type)
        {
        case CLAP_EVENT_PARAM_VALUE:
          {
            const auto * ev =
              reinterpret_cast<const clap_event_param_value *> (h); // NOLINT
            auto it = maps.by_id_.find (ev->param_id);
            if (it == maps.by_id_.end ())
              break;

            const auto &adapter = it->second;
            auto *      zrythm_param = adapter.zrythm_param;
            if (zrythm_param == nullptr)
              break;

            const size_t param_index = adapter.param_index;
            if (param_index >= owner_.param_sync_.entries.size ())
              {
                // Events can arrive before param_sync_ is prepared (e.g.,
                // while the plugin emits parameter changes during state
                // restore at project load); the full sync after state load
                // is authoritative, so those events are safe to drop. With
                // a prepared param_sync_, an out-of-range index comes from
                // the plugin's own parameter reporting
                if (!owner_.param_sync_.entries.empty ())
                  {
                    note_invalid_output_event_drop (
                      "param index out of range"sv);
                  }
                break;
              }
            const auto  range = zrythm_param->range ();
            const float normalized =
              range.convertTo0To1 (static_cast<float> (ev->value));

            auto &entry = owner_.param_sync_.entries[param_index];
            owner_.set_param_pending_from_plugin (param_index, normalized);
            entry.last_from_plugin = normalized;
            break;
          }
        case CLAP_EVENT_NOTE_ON:
        case CLAP_EVENT_NOTE_OFF:
        case CLAP_EVENT_NOTE_END:
        case CLAP_EVENT_NOTE_CHOKE:
          {
            const auto * ev =
              reinterpret_cast<const clap_event_note *> (h); // NOLINT
            if (
              ev->port_index < 0
              || static_cast<size_t> (ev->port_index)
                   >= owner_.midi_out_ports_.size ())
              {
                note_invalid_output_event_drop (
                  "note port index out of range"sv);
                break;
              }
            // Plugin-supplied fields are forwarded as raw MIDI bytes, so
            // they must be in range (a NaN check is implied by the negated
            // comparison)
            if (
              ev->channel < 0 || ev->channel > 15 || ev->key < 0
              || ev->key > 127 || !(ev->velocity >= 0.0 && ev->velocity <= 1.0))
              {
                note_invalid_output_event_drop ("note field out of range"sv);
                break;
              }

            auto * midi_out_port =
              owner_.midi_out_ports_[static_cast<size_t> (ev->port_index)];
            // Plugin-reported times are chunk-relative; the MIDI port
            // buffer holds cycle-relative times
            const auto time =
              units::samples (static_cast<uint32_t> (h->time))
              + event_time_offset;
            // NOTE_END/NOTE_CHOKE, and NOTE_ON with velocity 0, all
            // terminate the note
            const auto midi_ev =
              (h->type == CLAP_EVENT_NOTE_ON && ev->velocity > 0.0)
                ? dsp::midi_event::make_note_on (
                    static_cast<midi_byte_t> (ev->channel),
                    static_cast<midi_byte_t> (ev->key),
                    static_cast<midi_byte_t> (std::lround (ev->velocity * 127.0)),
                    time)
                : dsp::midi_event::make_note_off (
                    static_cast<midi_byte_t> (ev->channel),
                    static_cast<midi_byte_t> (ev->key),
                    static_cast<midi_byte_t> (std::lround (ev->velocity * 127.0)),
                    time);
            if (
              !midi_out_port->buffer_.push_back (midi_ev.time_, midi_ev.data ()))
              note_invalid_output_event_drop ("MIDI output buffer overflow"sv);
            break;
          }
        case CLAP_EVENT_MIDI:
          {
            const auto * ev =
              reinterpret_cast<const clap_event_midi *> (h); // NOLINT
            if (
              static_cast<size_t> (ev->port_index)
              >= owner_.midi_out_ports_.size ())
              {
                note_invalid_output_event_drop (
                  "MIDI port index out of range"sv);
                break;
              }

            auto * midi_out_port =
              owner_.midi_out_ports_[static_cast<size_t> (ev->port_index)];
            // Plugin-reported times are chunk-relative; the MIDI port
            // buffer holds cycle-relative times
            const auto time =
              units::samples (static_cast<uint32_t> (h->time))
              + event_time_offset;
            if (!midi_out_port->buffer_.push_back (
                  time, std::span<const midi_byte_t> (ev->data, 3)))
              note_invalid_output_event_drop ("MIDI output buffer overflow"sv);
            break;
          }
        default:
          break;
        }
    }
}

void
ClapPlugin::requestRestart () noexcept
{
  // Deferred: the plugin may only be deactivated once its own
  // request_restart() call has returned
  post_main_thread_action_deferred ([this] {
    if (!pimpl_->is_plugin_active ())
      return;

    z_debug ("CLAP: restarting plugin at the plugin's request");
    const auto restart = [this] {
      prepare_plugin_for_processing (
        pimpl_->last_sample_rate_, pimpl_->last_max_block_length_);
    };
    if (main_thread_callbacks_.with_paused_processing_)
      {
        main_thread_callbacks_.with_paused_processing_ (restart);
      }
    else
      {
        // Restarting here would race in-flight audio processing
        z_warning (
          "CLAP: plugin '{}' requested a restart but the host cannot pause "
          "processing; reload the plugin",
          get_name ());
      }
  });
}

void
ClapPlugin::requestProcess () noexcept
{
  pimpl_->scheduleProcess_.store (true, std::memory_order_release);
}

void
ClapPlugin::requestCallback () noexcept
{
  // Deferred: on_main_thread() is called back on a later main thread turn,
  // not from within the plugin's own request_callback() call
  post_main_thread_action_deferred ([this] {
    if (pimpl_->plugin_ != nullptr)
      pimpl_->plugin_->onMainThread ();
  });
}

void
ClapPlugin::logLog (clap_log_severity severity, const char * message)
  const noexcept
{
  switch (severity)
    {
    case CLAP_LOG_DEBUG:
      z_debug ("{}", message);
      break;
    case CLAP_LOG_INFO:
      z_info ("{}", message);
      break;
    case CLAP_LOG_WARNING:
      z_warning ("{}", message);
      break;
    case CLAP_LOG_FATAL:
      z_error ("[fatal CLAP error] {}", message);
      break;
    case CLAP_LOG_HOST_MISBEHAVING:
      z_error ("[CLAP host misbehaving] {}", message);
      break;
    case CLAP_LOG_PLUGIN_MISBEHAVING:
      z_warning ("[CLAP plugin misbehaving] {}", message);
      break;
    case CLAP_LOG_ERROR:
    default:
      z_error ("{}", message);
    }
}

dsp::SpeakerArrangement
ClapPlugin::ClapPluginImpl::resolve_audio_port_arrangement (
  dsp::PortFlow                 flow,
  uint32_t                      index,
  const clap_audio_port_info_t &nfo) const
{
  const bool is_input = flow == dsp::PortFlow::Input;
  const auto discrete_fallback = [&nfo] {
    return dsp::SpeakerArrangement::discrete_channels (
      static_cast<uint8_t> (nfo.channel_count));
  };

  const auto arrangement = [&] {
    if (nfo.port_type == nullptr || nfo.port_type[0] == '\0')
      // An absent port type is "unspecified (arbitrary audio)" by
      // definition, not a failure
      return discrete_fallback ();
    const std::string_view port_type = nfo.port_type;
    if (port_type == CLAP_PORT_MONO)
      return dsp::SpeakerArrangement::mono ();
    if (port_type == CLAP_PORT_STEREO)
      return dsp::SpeakerArrangement::stereo ();

    if (port_type == CLAP_PORT_SURROUND)
      {
        const auto map =
          get_surround_channel_map (flow, index, nfo.channel_count);
        if (!map.has_value ())
          {
            z_warning (
              "CLAP: plugin '{}' reports {} audio port {} as surround but "
              "provided no complete channel map; treating it as discrete",
              owner_.get_name (), is_input ? "input" : "output", index);
            return discrete_fallback ();
          }
        const auto resolved =
          clap_speaker_arrangement::arrangement_from_surround_channel_map (*map);
        if (!resolved.has_value ())
          {
            z_warning (
              "CLAP: plugin '{}' reports {} audio port {} with a channel "
              "map containing unknown or duplicate speaker ids; treating "
              "it as discrete",
              owner_.get_name (), is_input ? "input" : "output", index);
            return discrete_fallback ();
          }
        return *resolved;
      }

    if (port_type == CLAP_PORT_AMBISONIC)
      {
        const auto * ext = get_extension<clap_plugin_ambisonic> (
          CLAP_EXT_AMBISONIC, CLAP_EXT_AMBISONIC_COMPAT);
        clap_ambisonic_config config{};
        if (
          ext == nullptr
          || !ext->get_config (plugin_->clapPlugin (), is_input, index, &config))
          {
            z_warning (
              "CLAP: plugin '{}' reports {} audio port {} as ambisonic but "
              "provided no configuration; treating it as discrete",
              owner_.get_name (), is_input ? "input" : "output", index);
            return discrete_fallback ();
          }
        const auto resolved =
          clap_speaker_arrangement::arrangement_from_ambisonic_config (
            nfo.channel_count, config.ordering, config.normalization);
        if (!resolved.has_value ())
          {
            z_warning (
              "CLAP: plugin '{}' reports {} audio port {} with an ambisonic "
              "configuration this host cannot represent (SN2D/N2D, a "
              "non-square channel count or FuMa above 3rd order); treating "
              "it as discrete",
              owner_.get_name (), is_input ? "input" : "output", index);
            return discrete_fallback ();
          }
        return *resolved;
      }

    z_warning (
      "CLAP: plugin '{}' reports {} audio port {} with the unknown port "
      "type '{}'; treating it as discrete",
      owner_.get_name (), is_input ? "input" : "output", index, port_type);
    return discrete_fallback ();
  }();

  // Mono and stereo port types imply the channel count; when the reported
  // count disagrees, the plugin's actual channel count is what flows on the
  // wire
  if (arrangement.channel_count () != nfo.channel_count)
    {
      z_warning (
        "CLAP: plugin '{}' reports {} audio port {} as {} channel(s) with a "
        "port type implying {} channel(s); treating it as discrete",
        owner_.get_name (), is_input ? "input" : "output", index,
        nfo.channel_count, arrangement.channel_count ());
      return discrete_fallback ();
    }
  return arrangement;
}

std::optional<std::vector<uint8_t>>
ClapPlugin::ClapPluginImpl::get_surround_channel_map (
  dsp::PortFlow flow,
  uint32_t      index,
  uint32_t      channel_count) const
{
  // TODO: implement the clap_host_surround and clap_host_ambisonic
  // extensions: a plugin that changes only its channel map or ambisonic
  // configuration signals it via the host extension's changed(), not via
  // audio_ports.rescan(). Until then such a change leaves the arrangement
  // and the wire-order permutation stale until the next rescan
  const bool   is_input = flow == dsp::PortFlow::Input;
  const auto * ext = get_extension<clap_plugin_surround> (
    CLAP_EXT_SURROUND, CLAP_EXT_SURROUND_COMPAT);
  if (ext == nullptr)
    return std::nullopt;
  std::vector<uint8_t> map (channel_count);
  const auto           stored = ext->get_channel_map (
    plugin_->clapPlugin (), is_input, index, map.data (),
    static_cast<uint32_t> (map.size ()));
  // a partial map leaves channels without speaker semantics: no honest layout
  if (stored != channel_count)
    return std::nullopt;
  return map;
}

std::optional<std::vector<clap_audio_port_info_t>>
ClapPlugin::ClapPluginImpl::enumerate_validated_audio_port_infos (
  dsp::PortFlow flow) const
{
  const bool is_input = flow == dsp::PortFlow::Input;
  if (!plugin_->canUseAudioPorts ())
    return std::vector<clap_audio_port_info_t>{};
  const auto count = plugin_->audioPortsCount (is_input);
  if (count > kMaxAudioBusesPerFlow)
    {
      z_warning (
        "CLAP: plugin '{}' declares {} {} audio buses, above the supported "
        "{}; ignoring the report",
        owner_.get_name (), count, is_input ? "input" : "output",
        kMaxAudioBusesPerFlow);
      return std::nullopt;
    }
  std::vector<clap_audio_port_info_t> infos;
  infos.reserve (count);
  std::vector<uint32_t> seen_ids;
  seen_ids.reserve (count);
  for (const auto i : std::views::iota (0u, count))
    {
      clap_audio_port_info_t nfo{};
      if (!plugin_->audioPortsGet (i, is_input, &nfo))
        {
          // A plugin must report info for every index below its declared
          // count
          z_warning (
            "CLAP: plugin '{}' did not report {} audio port {} below its "
            "declared count of {}",
            owner_.get_name (), is_input ? "input" : "output", i, count);
          return std::nullopt;
        }
      // A bus carries at least one channel; SpeakerArrangement models at
      // most 255
      if (
        nfo.channel_count < 1
        || nfo.channel_count > std::numeric_limits<uint8_t>::max ())
        {
          z_warning (
            "CLAP: plugin '{}' reports {} audio port {} with {} channels; "
            "ignoring the report",
            owner_.get_name (), is_input ? "input" : "output", i,
            nfo.channel_count);
          return std::nullopt;
        }
      // Stable ids must be unique per flow: duplicates would alias two
      // buses onto the same engine port
      if (std::ranges::find (seen_ids, nfo.id) != seen_ids.end ())
        {
          z_warning (
            "CLAP: plugin '{}' reports duplicate {} audio port id {}",
            owner_.get_name (), is_input ? "input" : "output", nfo.id);
          return std::nullopt;
        }
      seen_ids.push_back (nfo.id);
      infos.push_back (nfo);
    }
  return infos;
}

std::optional<std::vector<dsp::AudioBusConfig>>
ClapPlugin::ClapPluginImpl::build_audio_bus_configs (dsp::PortFlow flow) const
{
  const auto infos = enumerate_validated_audio_port_infos (flow);
  if (!infos.has_value ())
    return std::nullopt;

  const bool is_input = flow == dsp::PortFlow::Input;
  // Only the first port may be flagged main (audio-ports.h). Misplaced or
  // duplicate main flags only affect the Main/Sidechain purpose assignment,
  // so they are warned about and normalized rather than rejecting the
  // report; likewise a flow with no main flag gets its first port treated
  // as main so automatic chain wiring can connect it
  if (!infos->empty ())
    {
      const auto is_main_flagged = [] (const auto &nfo) {
        return (nfo.flags & CLAP_AUDIO_PORT_IS_MAIN) != 0;
      };
      auto &misplaced_warned =
        misplaced_main_warning_emitted_[flow_index (flow)];
      auto &no_main_warned = no_main_warning_emitted_[flow_index (flow)];
      if (std::ranges::any_of (*infos | std::views::drop (1), is_main_flagged))
        {
          if (!misplaced_warned)
            {
              misplaced_warned = true;
              z_warning (
                "CLAP: plugin '{}' flags {} audio ports other than the first "
                "as main; ignoring the flags",
                owner_.get_name (), is_input ? "input" : "output");
            }
        }
      else if (std::ranges::none_of (*infos, is_main_flagged))
        {
          if (!no_main_warned)
            {
              no_main_warned = true;
              z_warning (
                "CLAP: plugin '{}' flags no {} audio port as main; treating "
                "the first port as main",
                owner_.get_name (), is_input ? "input" : "output");
            }
        }
    }

  std::vector<dsp::AudioBusConfig> configs;
  configs.reserve (infos->size ());
  for (const auto &[i, nfo] : utils::views::enumerate (*infos))
    {
      configs.push_back (
        dsp::AudioBusConfig{
          .name = utils::Utf8String::from_utf8_encoded_string (nfo.name),
          .arrangement = resolve_audio_port_arrangement (
            flow, static_cast<uint32_t> (i), nfo),
          .purpose =
            i == 0 ? dsp::AudioPort::Purpose::Main
                   : dsp::AudioPort::Purpose::Sidechain,
          .active = true,
          .external_id = nfo.id });
    }
  return configs;
}

void
ClapPlugin::create_ports_from_clap_plugin ()
{
  assert (is_main_thread);
  assert (!pimpl_->is_plugin_active ());

  if (pimpl_->plugin_->canUseNotePorts ())
    {
      const auto midi_in_ports = pimpl_->plugin_->notePortsCount (true);
      const auto midi_out_ports = pimpl_->plugin_->notePortsCount (false);
      pimpl_->note_in_supported_dialects_.clear ();
      for (const auto i : std::views::iota (0u, midi_in_ports))
        {
          clap_note_port_info info{};
          // Always push so the vector stays index-aligned with the note
          // ports. Default to the MIDI dialect when unknown: its passthrough
          // is lossless, while the CLAP-note conversion currently drops
          // non-note messages (see TODOs in generate_midi_input_events()).
          pimpl_->note_in_supported_dialects_.push_back (
            pimpl_->plugin_->notePortsGet (i, true, &info)
              ? info.supported_dialects
              : static_cast<uint32_t> (CLAP_NOTE_DIALECT_MIDI));
          auto port_ref = utils::create_object<dsp::MidiPort> (
            registry (),
            utils::Utf8String::from_utf8_encoded_string (
              fmt::format ("MIDI Input {}", i + 1)),
            dsp::PortFlow::Input);
          add_input_port (port_ref);
        }
      for (const auto i : std::views::iota (0u, midi_out_ports))
        {
          auto port_ref = utils::create_object<dsp::MidiPort> (
            registry (),
            utils::Utf8String::from_utf8_encoded_string (
              fmt::format ("MIDI Output {}", i + 1)),
            dsp::PortFlow::Output);
          add_output_port (port_ref);
        }
    }

  if (pimpl_->plugin_->canUseAudioPorts ())
    {
      // No audio ports exist yet, so reconciliation reduces to its create
      // branch (MIDI ports are not audio ports and are left alone)
      for (const auto flow : { dsp::PortFlow::Input, dsp::PortFlow::Output })
        {
          const auto configs = pimpl_->build_audio_bus_configs (flow);
          if (!configs.has_value ())
            continue;
          static_cast<void> (dsp::reconcile_audio_bus_configuration (
            registry (), *this, flow, *configs));
        }
    }
}

bool
ClapPlugin::audioPortsIsRescanFlagSupported (uint32_t flag) noexcept
{
  switch (flag)
    {
    case CLAP_AUDIO_PORTS_RESCAN_NAMES:
    case CLAP_AUDIO_PORTS_RESCAN_FLAGS:
    case CLAP_AUDIO_PORTS_RESCAN_CHANNEL_COUNT:
    case CLAP_AUDIO_PORTS_RESCAN_PORT_TYPE:
    case CLAP_AUDIO_PORTS_RESCAN_LIST:
      return true;
    // We never process in place, so pairing changes are inert to us
    case CLAP_AUDIO_PORTS_RESCAN_IN_PLACE_PAIR:
      return true;
    default:
      return false;
    }
}

void
ClapPlugin::audioPortsRescan (uint32_t flags) noexcept
{
  z_return_if_fail (is_main_thread);

  if (pimpl_->plugin_ == nullptr)
    return;

  // Requesting a rescan with a flag is_rescan_flag_supported() rejected is
  // illegal (clap/ext/audio-ports.h)
  constexpr uint32_t known_flags =
    CLAP_AUDIO_PORTS_RESCAN_NAMES | CLAP_AUDIO_PORTS_RESCAN_FLAGS
    | CLAP_AUDIO_PORTS_RESCAN_CHANNEL_COUNT | CLAP_AUDIO_PORTS_RESCAN_PORT_TYPE
    | CLAP_AUDIO_PORTS_RESCAN_IN_PLACE_PAIR | CLAP_AUDIO_PORTS_RESCAN_LIST;
  if ((flags & ~known_flags) != 0u)
    {
      z_warning (
        "CLAP: plugin '{}' requested an audio ports rescan with unsupported "
        "flags {:#x}; ignoring it",
        get_name (), flags);
      return;
    }
  if (flags == 0)
    {
      // No aspect flagged: nothing to do
      z_debug (
        "CLAP: plugin '{}' requested an audio ports rescan without flags; "
        "ignoring it",
        get_name ());
      return;
    }
  // A plugin may not request a rescan of an extension it does not implement
  if (!pimpl_->plugin_->canUseAudioPorts ())
    {
      z_warning (
        "CLAP: plugin '{}' requested an audio ports rescan without "
        "implementing the audio ports extension",
        get_name ());
      return;
    }

  // Only RESCAN_NAMES may be requested while the plugin is up and running
  // (clap/ext/audio-ports.h); topology changes require deactivation first.
  // The restart flow legitimately reaches this from within the plugin's
  // deactivate() (state ActiveAndReadyToDeactivate), so refuse only the
  // running states
  const bool running =
    pimpl_->state_ == ClapPluginImpl::ActiveAndSleeping
    || pimpl_->state_ == ClapPluginImpl::ActiveAndProcessing;
  if (running && (flags & ~CLAP_AUDIO_PORTS_RESCAN_NAMES) != 0u)
    {
      z_warning (
        "CLAP: plugin '{}' requested a topology-changing audio ports rescan "
        "while active; ignoring it (the plugin must be restarted first)",
        get_name ());
      return;
    }

  // Scan eagerly: topology-changing rescans are only reported while the
  // plugin is deactivated (e.g. from within its deactivate() during a
  // restart), so the scan must happen inside that window
  pimpl_->pending_audio_port_scan_ = true;
  for (const auto flow : { dsp::PortFlow::Input, dsp::PortFlow::Output })
    {
      auto configs = pimpl_->build_audio_bus_configs (flow);
      if (!configs.has_value ())
        {
          // A failed scan yields no trustworthy configuration; the
          // reconciler detaches ports absent from a configuration, so the
          // request is dropped
          z_warning (
            "CLAP: plugin '{}' failed to report its audio ports; ignoring "
            "the rescan request",
            get_name ());
          pimpl_->clear_pending_audio_port_scan ();
          return;
        }
      pimpl_->pending_audio_port_configs_for (flow) = std::move (*configs);
    }

  // The reconciliation mutates engine objects, which requires the processing
  // pause the restart handler may already hold: defer so it runs after any
  // in-flight restart completes
  if (
    !post_main_thread_action_deferred ([this] { apply_audio_ports_rescan (); }))
    {
      z_warning (
        "CLAP: dropping the audio ports rescan of '{}': the main thread "
        "action queue is full",
        get_name ());
      pimpl_->clear_pending_audio_port_scan ();
    }
}

void
ClapPlugin::apply_audio_ports_rescan ()
{
  assert (is_main_thread);

  if (!std::exchange (pimpl_->pending_audio_port_scan_, false))
    return;
  std::array<std::vector<dsp::AudioBusConfig>, 2> scanned_configs;
  for (const auto flow : { dsp::PortFlow::Input, dsp::PortFlow::Output })
    {
      scanned_configs[ClapPluginImpl::flow_index (flow)] =
        std::exchange (pimpl_->pending_audio_port_configs_for (flow), {});
    }

  if (main_thread_callbacks_.graph_recalc_ == nullptr)
    {
      // Reconciling drops the buffers of ports whose arrangement changed
      // and creates new ports unprepared, and only a graph recalculation
      // reallocates them: without one the current topology keeps running
      z_warning (
        "CLAP: ports of '{}' changed but the host cannot recalculate the "
        "processing graph; keeping the current port topology",
        get_name ());
      return;
    }

  dsp::AudioBusReconcileResult result{
    .graph_changed = false, .metadata_changed = false
  };
  const auto apply = [&] {
    for (const auto flow : { dsp::PortFlow::Input, dsp::PortFlow::Output })
      {
        const auto flow_result = dsp::reconcile_audio_bus_configuration (
          registry (), *this, flow,
          scanned_configs[ClapPluginImpl::flow_index (flow)]);
        result.graph_changed |= flow_result.graph_changed;
        result.metadata_changed |= flow_result.metadata_changed;
      }
    if (result.graph_changed)
      {
        // The recalculation's node preparation reallocates the dropped and
        // new port buffers, and must happen before processing resumes.
        // Metadata-only changes (labels, id adoption) need neither the
        // recalculation nor the plugin restart it would cause
        main_thread_callbacks_.graph_recalc_ ();
      }
  };
  if (main_thread_callbacks_.with_paused_processing_)
    {
      main_thread_callbacks_.with_paused_processing_ (apply);
    }
  else
    {
      z_warning (
        "CLAP: plugin '{}' changed its audio ports but the host cannot pause "
        "processing to apply the change; reload the plugin",
        get_name ());
      return;
    }

  if (result.graph_changed || result.metadata_changed)
    {
      z_debug (
        "CLAP: reconciled ports of '{}' after audio ports rescan", get_name ());
    }
}

void
ClapPlugin::restore_saved_bus_arrangements ()
{
  assert (is_main_thread);
  assert (!pimpl_->is_plugin_active ());

  if (pimpl_->plugin_ == nullptr || !pimpl_->plugin_->canUseAudioPorts ())
    return;

  // A rescan requested before this negotiation (e.g. while the plugin's
  // state loaded) or during it (from inside apply_configuration, which is
  // legal while deactivated) is superseded by it: the negotiation
  // enumerates the live topology directly and syncs the ports to the
  // accepted layout, so the rescan's deferred action must not reconcile
  // them back
  pimpl_->clear_pending_audio_port_scan ();
  [this] {
    // Negotiation is done per flow over the *audio* ports; their position in
    // the bus-ordered lists is the enumeration index. Detached ports are left
    // out: they hold buses the plugin removed, which are not pushed again (the
    // final reconciliation re-attaches them if the bus returns)
    const auto saved_configs = [this] (dsp::PortFlow flow) {
      return get_attached_audio_ports (flow)
             | std::views::transform ([] (const dsp::AudioPort * port) {
                 return dsp::AudioBusConfig{
                   .name = port->get_label (),
                   .arrangement = port->arrangement (),
                   .purpose = port->purpose (),
                   .external_id = port->external_port_id ()
                 };
               })
             | std::ranges::to<std::vector> ();
    };
    const auto saved_inputs = saved_configs (dsp::PortFlow::Input);
    const auto saved_outputs = saved_configs (dsp::PortFlow::Output);

    const auto live_inputs =
      pimpl_->build_audio_bus_configs (dsp::PortFlow::Input);
    const auto live_outputs =
      pimpl_->build_audio_bus_configs (dsp::PortFlow::Output);
    if (!live_inputs.has_value () || !live_outputs.has_value ())
      {
        // Without a trustworthy scan the restored ports carry no stable ids
        // and cannot be paired with the plugin's buses: the plugin is fed
        // silence and its output is dropped until it reports a valid
        // configuration
        z_warning (
          "CLAP: plugin '{}' failed to report its audio ports; its buses "
          "cannot be paired with the restored ports",
          get_name ());
        return;
      }

    // Skip the negotiation when the live layout already matches the restored
    // topology; the sync below still runs to sync labels, purposes and
    // detached states. Ports created from a bus enumeration always carry its
    // stable id, so the comparison is strict on ids. It is also
    // order-independent: the saved list is in port-list order while the live
    // list is in enumeration order, and a reordering list rescan diverges the
    // two permanently (stable ids are unique per flow)
    const auto sorted_topology =
      [] (const std::vector<dsp::AudioBusConfig> &configs) {
        auto pairs =
          configs
          | std::views::transform ([] (const dsp::AudioBusConfig &config) {
              return std::pair{ config.external_id, config.arrangement };
            })
          | std::ranges::to<std::vector> ();
        std::ranges::stable_sort (pairs, [] (const auto &a, const auto &b) {
          return a.first < b.first;
        });
        return pairs;
      };
    const auto matches_live =
      [&] (
        const std::vector<dsp::AudioBusConfig> &saved,
        const std::vector<dsp::AudioBusConfig> &live) {
        return std::ranges::equal (
          sorted_topology (saved), sorted_topology (live));
      };
    const bool topology_matches =
      matches_live (saved_inputs, *live_inputs)
      && matches_live (saved_outputs, *live_outputs);

    // Ports saved before ids were recorded carry no id, so the comparison
    // above always fails for them. When their arrangements already match the
    // live layout positionally there is nothing to negotiate; the sync below
    // adopts the live ids
    const auto has_no_ids = [] (const std::vector<dsp::AudioBusConfig> &saved) {
      return std::ranges::none_of (saved, [] (const dsp::AudioBusConfig &config) {
        return config.external_id.has_value ();
      });
    };
    const auto arrangements_match_live =
      [&] (
        const std::vector<dsp::AudioBusConfig> &saved,
        const std::vector<dsp::AudioBusConfig> &live) {
        return std::ranges::equal (
          saved, live, {}, &dsp::AudioBusConfig::arrangement,
          &dsp::AudioBusConfig::arrangement);
      };
    const bool legacy_layout_matches =
      has_no_ids (saved_inputs) && has_no_ids (saved_outputs)
      && arrangements_match_live (saved_inputs, *live_inputs)
      && arrangements_match_live (saved_outputs, *live_outputs);

    bool pushed = false;
    if (!topology_matches && !legacy_layout_matches)
      {
        const auto push_result = pimpl_->push_saved_bus_configuration (
          saved_inputs, saved_outputs, *live_inputs, *live_outputs);
        if (!push_result.has_value ())
          {
            z_warning (
              "CLAP: plugin '{}' does not implement configurable audio ports; "
              "adopting its current layout",
              get_name ());
          }
        // A refused or empty push is warned about / logged inside
        pushed = push_result.value_or (false);
      }

    static_cast<void> (pimpl_->sync_ports_to_live_configuration (
      *live_inputs, *live_outputs, pushed));
  }();
  pimpl_->clear_pending_audio_port_scan ();
}

std::optional<bool>
ClapPlugin::ClapPluginImpl::push_saved_bus_configuration (
  const std::vector<dsp::AudioBusConfig> &saved_inputs,
  const std::vector<dsp::AudioBusConfig> &saved_outputs,
  const std::vector<dsp::AudioBusConfig> &live_inputs,
  const std::vector<dsp::AudioBusConfig> &live_outputs)
{
  const auto * configurable = get_extension<clap_plugin_configurable_audio_ports> (
    CLAP_EXT_CONFIGURABLE_AUDIO_PORTS, CLAP_EXT_CONFIGURABLE_AUDIO_PORTS_COMPAT);
  if (configurable == nullptr)
    return std::nullopt;

  // Requests point into these deques, whose elements never relocate
  std::deque<std::vector<uint8_t>>                   channel_maps;
  std::deque<clap_ambisonic_config>                  ambisonic_configs;
  std::vector<clap_audio_port_configuration_request> requests;
  const auto                                         build_requests =
    [&] (
      const std::vector<dsp::AudioBusConfig> &saved,
      const std::vector<dsp::AudioBusConfig> &live, dsp::PortFlow flow) {
      // Id-less saved buses match positionally; detached ports filtered out
      // of the saved list shift positions when one sat before a surviving
      // bus
      const bool any_idless =
        std::ranges::any_of (saved, [] (const dsp::AudioBusConfig &config) {
          return !config.external_id.has_value ();
        });
      const bool any_detached = std::ranges::any_of (
        owner_.get_all_audio_ports (flow),
        [] (const auto * port) { return port->detached (); });
      if (any_idless && any_detached)
        {
          z_warning (
            "CLAP: plugin '{}': saved {} buses without stable ids are "
            "matched positionally, but a removed bus shifts the positions; "
            "bus assignments may be wrong",
            owner_.get_name (),
            flow == dsp::PortFlow::Input ? "input" : "output");
        }
      std::vector<bool> claimed (live.size (), false);
      for (const auto &[saved_index, config] : utils::views::enumerate (saved))
        {
          if (config.arrangement.channel_count () == 0)
            {
              // Only reachable via a hand-edited project; a channel-less
              // bus cannot be expressed in a configuration request
              z_warning (
                "CLAP: saved bus '{}' of plugin '{}' has no channels; "
                "skipping it",
                config.name, owner_.get_name ());
              continue;
            }

          // A saved bus with a stable id only ever targets the live bus
          // carrying that id; only ports saved before ids were recorded
          // match by position. Positional matching assumes the saved list
          // order (detached ports filtered out) still matches the live
          // enumeration order
          std::optional<size_t> index;
          if (config.external_id.has_value ())
            {
              const auto live_index = std::ranges::find_if (
                live, [&] (const dsp::AudioBusConfig &live_config) {
                  return live_config.external_id == config.external_id;
                });
              if (live_index != live.end ())
                {
                  index = static_cast<size_t> (
                    std::distance (live.begin (), live_index));
                }
            }
          else
            {
              index = static_cast<size_t> (saved_index);
            }
          if (!index.has_value () || *index >= live.size () || claimed[*index])
            {
              // No live bus to push this saved bus onto, or another request
              // already targets it
              z_debug (
                "CLAP: saved bus '{}' of plugin '{}' matches no unclaimed "
                "live {} bus; not pushing it",
                config.name, owner_.get_name (),
                flow == dsp::PortFlow::Input ? "input" : "output");
              continue;
            }

          const auto * port_type = clap_speaker_arrangement::
            port_type_from_arrangement (config.arrangement);
          const void * details = nullptr;
          if (
            config.arrangement.kind () == dsp::SpeakerArrangement::Kind::Speakers
            && !config.arrangement.is_mono ()
            && !config.arrangement.is_stereo ())
            {
              auto map = clap_speaker_arrangement::
                surround_channel_map_from_arrangement (config.arrangement);
              if (!map.has_value ())
                {
                  z_warning (
                    "CLAP: saved bus '{}' of plugin '{}' has no CLAP channel "
                    "map encoding; skipping it",
                    config.name, owner_.get_name ());
                  continue;
                }
              channel_maps.push_back (std::move (*map));
              details = channel_maps.back ().data ();
            }
          else if (
            config.arrangement.kind ()
            == dsp::SpeakerArrangement::Kind::Ambisonics)
            {
              const auto ambisonic = clap_speaker_arrangement::
                ambisonic_config_from_arrangement (config.arrangement);
              if (!ambisonic.has_value ())
                {
                  z_warning (
                    "CLAP: saved bus '{}' of plugin '{}' has no CLAP "
                    "ambisonic encoding; skipping it",
                    config.name, owner_.get_name ());
                  continue;
                }
              ambisonic_configs.push_back (
                clap_ambisonic_config{
                  .ordering = ambisonic->first,
                  .normalization = ambisonic->second });
              details = &ambisonic_configs.back ();
            }

          claimed[*index] = true;
          requests.push_back (
            clap_audio_port_configuration_request{
              .is_input = flow == dsp::PortFlow::Input,
              .port_index = static_cast<uint32_t> (*index),
              .channel_count = config.arrangement.channel_count (),
              .port_type = port_type,
              .port_details = details });
        }
    };
  build_requests (saved_inputs, live_inputs, dsp::PortFlow::Input);
  build_requests (saved_outputs, live_outputs, dsp::PortFlow::Output);

  if (requests.empty ())
    {
      // No saved bus could be matched to a live bus, so there is nothing to
      // negotiate; the live layout is adopted as-is
      z_info (
        "CLAP: no saved bus of plugin '{}' matches a live bus; adopting the "
        "live layout",
        owner_.get_name ());
      return false;
    }

  if (
    !configurable->can_apply_configuration (
      plugin_->clapPlugin (), requests.data (),
      static_cast<uint32_t> (requests.size ()))
    || !configurable->apply_configuration (
      plugin_->clapPlugin (), requests.data (),
      static_cast<uint32_t> (requests.size ())))
    {
      z_warning (
        "CLAP: plugin '{}' refused to restore the saved bus configuration; "
        "adopting its current layout",
        owner_.get_name ());
      return false;
    }
  return true;
}

dsp::AudioBusReconcileResult
ClapPlugin::ClapPluginImpl::sync_ports_to_live_configuration (
  const std::vector<dsp::AudioBusConfig> &live_inputs,
  const std::vector<dsp::AudioBusConfig> &live_outputs,
  bool                                    pushed)
{
  // The accepted layout only differs from the live one after a successful
  // configuration push, so the live configurations are reused when nothing
  // was pushed
  dsp::AudioBusReconcileResult result{
    .graph_changed = false, .metadata_changed = false
  };
  for (const auto flow : { dsp::PortFlow::Input, dsp::PortFlow::Output })
    {
      const auto &live =
        flow == dsp::PortFlow::Input ? live_inputs : live_outputs;
      if (!pushed)
        {
          const auto flow_result = dsp::reconcile_audio_bus_configuration (
            owner_.registry (), owner_, flow, live);
          result.graph_changed |= flow_result.graph_changed;
          result.metadata_changed |= flow_result.metadata_changed;
          continue;
        }
      const auto accepted = build_audio_bus_configs (flow);
      if (!accepted.has_value ())
        {
          // A failed scan yields no trustworthy configuration; the
          // reconciler detaches ports absent from a configuration, so the
          // flow is skipped
          z_warning (
            "CLAP: plugin '{}' failed to report its audio ports after "
            "configuration; keeping the current port topology",
            owner_.get_name ());
          continue;
        }
      const auto flow_result = dsp::reconcile_audio_bus_configuration (
        owner_.registry (), owner_, flow, *accepted);
      result.graph_changed |= flow_result.graph_changed;
      result.metadata_changed |= flow_result.metadata_changed;
    }
  if (result.graph_changed || result.metadata_changed)
    {
      z_debug (
        "CLAP: reconciled ports of '{}' with the accepted bus configuration",
        owner_.get_name ());
    }
  return result;
}

bool
ClapPlugin::ClapPluginImpl::setup_audio_ports_for_processing (
  units::sample_u32_t block_size)
{
  const auto setup_for_direction = [&] (dsp::PortFlow flow) {
    auto &audio_bufs =
      flow == dsp::PortFlow::Input ? audio_in_bufs_ : audio_out_bufs_;
    auto &audio_clap_bufs =
      flow == dsp::PortFlow::Input ? audio_in_clap_bufs_ : audio_out_clap_bufs_;
    auto &channel_ptrs =
      flow == dsp::PortFlow::Input
        ? audio_in_channel_ptrs_
        : audio_out_channel_ptrs_;
    auto &ports_by_bus =
      flow == dsp::PortFlow::Input
        ? audio_in_ports_by_bus_
        : audio_out_ports_by_bus_;
    const auto infos = enumerate_validated_audio_port_infos (flow);
    if (!infos.has_value ())
      {
        // The report is untrustworthy (warned about during enumeration);
        // the flow gets no buses and the plugin must not be activated
        audio_bufs.clear ();
        audio_clap_bufs.clear ();
        channel_ptrs.clear ();
        ports_by_bus.clear ();
        return false;
      }
    const auto attached_ports = owner_.get_attached_audio_ports (flow);
    audio_bufs.resize (infos->size ());
    audio_clap_bufs.resize (infos->size ());
    channel_ptrs.resize (infos->size ());
    ports_by_bus.clear ();
    ports_by_bus.reserve (infos->size ());
    for (const auto &[i, juce_buf] : utils::views::enumerate (audio_bufs))
      {
        const auto &nfo = infos->at (i);

        // Pair the bus with the engine port carrying its stable id; null
        // while a rescan awaits its deferred reconciliation
        const auto port_it =
          std::ranges::find_if (attached_ports, [&] (const auto * port) {
            return port->external_port_id () == nfo.id;
          });
        ports_by_bus.push_back (
          port_it != attached_ports.end () ? *port_it : nullptr);

        juce_buf.setSize (
          static_cast<int> (nfo.channel_count),
          block_size.in<int> (units::samples));

        // Permute the channel pointers handed to the plugin when its wire
        // order is not canonical (surround channel maps only: ambisonic
        // ports carry their ordering inside the arrangement). The scratch
        // buffer itself stays canonical so the port buffer copies are
        // untouched.
        const auto permutation = [&] () -> std::optional<std::vector<uint8_t>> {
          if (
            nfo.port_type == nullptr
            || std::string_view (nfo.port_type) != CLAP_PORT_SURROUND)
            return std::nullopt;
          const auto map = get_surround_channel_map (
            flow, static_cast<uint32_t> (i), nfo.channel_count);
          if (!map.has_value ())
            return std::nullopt;
          return clap_speaker_arrangement::surround_channel_permutation (*map);
        }();

        auto &ptrs = channel_ptrs.at (i);
        ptrs.resize (nfo.channel_count);
        for (const auto ch : std::views::iota (0u, nfo.channel_count))
          {
            const auto canonical_ch =
              permutation.has_value () ? (*permutation)[ch] : ch;
            ptrs[ch] =
              juce_buf.getWritePointer (static_cast<int> (canonical_ch));
          }

        auto &clap_buf = audio_clap_bufs.at (i);
        clap_buf.channel_count = nfo.channel_count;
        clap_buf.data32 = ptrs.data ();
        clap_buf.data64 = nullptr;
        clap_buf.constant_mask = 0;
        clap_buf.latency = 0;
      }
    return true;
  };

  // Evaluate both flows unconditionally: a failed flow's buffers must be
  // cleared even when the other flow failed first
  const bool inputs_ok = setup_for_direction (dsp::PortFlow::Input);
  const bool outputs_ok = setup_for_direction (dsp::PortFlow::Output);
  return inputs_ok && outputs_ok;
}

bool
ClapPlugin::ClapPluginImpl::check_valid_param_value (
  const ClapParamAdapter &adapter,
  double                  value)
{
  assert (is_main_thread);

  if (adapter.info.min_value > value || value > adapter.info.max_value)
    {
      z_warning (
        "Invalid value for param id: {}, name: '{}'; value: {}", adapter.id,
        adapter.info.name, value);
      return false;
    }

  return true;
}

void
ClapPlugin::paramsRescan (uint32_t flags) noexcept
{
  assert (is_main_thread);

  if (!pimpl_->plugin_->canUseParams ())
    return;

  // CLAP_PARAM_RESCAN_ALL is forbidden while the plugin is active
  // (clap/ext/params.h): it may add/remove parameters, which would grow
  // get_parameters()/param_sync_ mid-processing. Refuse noisily instead of
  // racing the audio thread (some plugins still try)
  if (pimpl_->is_plugin_active () && (flags & CLAP_PARAM_RESCAN_ALL) != 0u)
    {
      z_warning ("CLAP: plugin requested RESCAN_ALL while active; ignoring");
      return;
    }

  // Scan phase: call into the plugin WITHOUT holding the farbot access.
  // The plugin may reentrantly invoke host callbacks (e.g.
  // paramsRequestFlush) that acquire a nonRealtime access themselves, and
  // farbot's nonRealtime lock is not recursive
  const auto count = pimpl_->plugin_->paramsCount ();
  std::vector<std::pair<clap_param_info, double>> scanned;
  scanned.reserve (count);
  for (const auto i : std::views::iota (0u, count))
    {
      clap_param_info info{};
      z_return_if_fail (pimpl_->plugin_->paramsGetInfo (i, &info));
      auto value = pimpl_->get_param_value (info);
      if (!value.has_value ())
        continue;
      scanned.emplace_back (info, *value);
    }

  // Mutation phase: the audio thread keeps reading the previously published
  // snapshot until the updated maps are published on scope exit
  decltype (pimpl_->param_maps_)::ScopedAccess<farbot::ThreadType::nonRealtime>
    param_maps{ pimpl_->param_maps_ };

  // 2. Build a lookup from ProcessorParameter* to its index in
  //    get_parameters(). This avoids repeated O(n) linear scans.
  std::unordered_map<dsp::ProcessorParameter *, size_t> param_index_map;
  for (
    const auto &[idx, param_ref] : utils::views::enumerate (get_parameters ()))
    {
      param_index_map[param_ref.get ()] = static_cast<size_t> (idx);
    }

  std::unordered_set<clap_id> param_ids (count * 2);

  for (const auto &[info, value] : scanned)
    {
      assert (info.id != CLAP_INVALID_ID);

      // check that the parameter is not declared twice
      assert (!param_ids.contains (info.id));
      param_ids.insert (info.id);

      auto it = param_maps->by_id_.find (info.id);

      if (it == param_maps->by_id_.end ())
        {
          assert ((flags & CLAP_PARAM_RESCAN_ALL) != 0u);

          ClapPluginImpl::ClapParamAdapter adapter{
            .id = info.id, .info = info, .zrythm_param = nullptr, .param_index = 0
          };
          const bool value_valid =
            pimpl_->check_valid_param_value (adapter, value);

          // Look up or create a ProcessorParameter
          const auto unique_id = dsp::ProcessorParameter::UniqueId (
            utils::Utf8String::from_utf8_encoded_string (
              std::to_string (info.id)));

          dsp::ProcessorParameter * zrythm_param = nullptr;
          for (const auto &param_ref : get_parameters ())
            {
              auto * p = param_ref.get ();
              if (p->get_unique_id () == unique_id)
                {
                  zrythm_param = p;
                  break;
                }
            }

          if (zrythm_param == nullptr)
            {
              dsp::ParameterRange range{
                dsp::ParameterRange::Type::Linear,
                static_cast<float> (info.min_value),
                static_cast<float> (info.max_value), 0.f,
                static_cast<float> (info.default_value)
              };

              if (info.flags & CLAP_PARAM_IS_STEPPED)
                {
                  if (info.flags & CLAP_PARAM_IS_BYPASS)
                    {
                      range = dsp::ParameterRange::make_toggle (
                        info.default_value > 0.5);
                    }
                  else
                    {
                      range = dsp::ParameterRange{
                        dsp::ParameterRange::Type::Integer,
                        static_cast<float> (info.min_value),
                        static_cast<float> (info.max_value), 0.f,
                        static_cast<float> (info.default_value)
                      };
                    }
                }

              auto zrythm_param_ref = utils::create_object<
                dsp::ProcessorParameter> (
                registry (), registry (), unique_id, range,
                utils::Utf8String::from_utf8_encoded_string (info.name));
              add_parameter (zrythm_param_ref);
              zrythm_param = zrythm_param_ref.get ();
              zrythm_param->set_automatable (
                (info.flags & CLAP_PARAM_IS_AUTOMATABLE) != 0);

              // Update index map for the newly added parameter
              param_index_map[zrythm_param] = get_parameters ().size () - 1;

              if (value_valid)
                {
                  const auto normalized_live =
                    zrythm_param->range ().convertTo0To1 (
                      static_cast<float> (value));
                  zrythm_param->setBaseValue (normalized_live);
                }
            }

          adapter.zrythm_param = zrythm_param;
          auto index_it = param_index_map.find (zrythm_param);
          assert (index_it != param_index_map.end ());
          adapter.param_index = index_it->second;

          param_maps->by_id_.insert_or_assign (info.id, adapter);
          param_maps->by_param_[zrythm_param] = info.id;
        }
      else
        {
          auto &adapter = it->second;

          // Check if param info changed
          const auto &old_info = adapter.info;
          bool        info_changed = !(
            old_info.cookie == info.cookie
            && old_info.default_value == info.default_value
            && old_info.max_value == info.max_value
            && old_info.min_value == info.min_value
            && old_info.flags == info.flags && old_info.id == info.id
            && std::strncmp (old_info.name, info.name, sizeof (info.name)) == 0
            && std::strncmp (old_info.module, info.module, sizeof (info.module))
                 == 0);

          if (info_changed)
            {
              z_warn_if_fail (
                pimpl_->clap_params_rescan_may_info_change (flags));
              constexpr uint32_t critical_flags =
                CLAP_PARAM_IS_AUTOMATABLE | CLAP_PARAM_IS_AUTOMATABLE_PER_NOTE_ID
                | CLAP_PARAM_IS_AUTOMATABLE_PER_KEY
                | CLAP_PARAM_IS_AUTOMATABLE_PER_CHANNEL
                | CLAP_PARAM_IS_AUTOMATABLE_PER_PORT | CLAP_PARAM_IS_MODULATABLE
                | CLAP_PARAM_IS_MODULATABLE_PER_NOTE_ID
                | CLAP_PARAM_IS_MODULATABLE_PER_KEY
                | CLAP_PARAM_IS_MODULATABLE_PER_CHANNEL
                | CLAP_PARAM_IS_MODULATABLE_PER_PORT | CLAP_PARAM_IS_READONLY
                | CLAP_PARAM_REQUIRES_PROCESS;
              z_warn_if_fail (
                ((flags & CLAP_PARAM_RESCAN_ALL) != 0u)
                || ((old_info.flags & critical_flags)
                      == (info.flags & critical_flags)
                    && old_info.min_value == info.min_value
                    && old_info.max_value == info.max_value));
              adapter.info = info;
            }

          // Check if value changed and sync to ProcessorParameter
          if (adapter.zrythm_param != nullptr)
            {
              if (pimpl_->check_valid_param_value (adapter, value))
                {
                  const auto range = adapter.zrythm_param->range ();
                  const auto current_normalized =
                    adapter.zrythm_param->baseValue ();
                  const auto new_normalized =
                    range.convertTo0To1 (static_cast<float> (value));
                  if (!utils::math::floats_near (
                        current_normalized, new_normalized, 0.0001f))
                    {
                      assert (
                        pimpl_->clap_params_rescan_may_value_change (flags));
                      adapter.zrythm_param->setBaseValue (new_normalized);
                    }
                }
            }
        }
    }

  // remove parameters which are gone
  for (auto it = param_maps->by_id_.begin (); it != param_maps->by_id_.end ();)
    {
      if (param_ids.contains (it->first))
        ++it;
      else
        {
          assert ((flags & CLAP_PARAM_RESCAN_ALL) != 0u);
          if (it->second.zrythm_param != nullptr)
            {
              param_maps->by_param_.erase (it->second.zrythm_param);
            }
          it = param_maps->by_id_.erase (it);
        }
    }

  // Defensive: rebuild param_index for all remaining adapters in case
  // get_parameters() ordering ever changes.
  for (auto &[clap_id_val, adapter] : param_maps->by_id_)
    {
      if (adapter.zrythm_param == nullptr)
        continue;
      auto index_it = param_index_map.find (adapter.zrythm_param);
      if (index_it != param_index_map.end ())
        adapter.param_index = index_it->second;
    }

  // Cache the routed-param count for sizing reads, then publish on scope
  // exit
  pimpl_->param_count_ = param_maps->by_param_.size ();

  if ((flags & CLAP_PARAM_RESCAN_ALL) != 0u)
    paramsChanged ();
}

void
ClapPlugin::paramsClear (clap_id paramId, clap_param_clear_flags flags) noexcept
{
  assert (is_main_thread);
}

void
ClapPlugin::ClapPluginImpl::param_flush_on_main_thread ()
{
  assert (is_main_thread);

  assert (!is_plugin_active ());

  scheduleParamFlush_.store (false, std::memory_order_release);

  evIn_.clear ();
  evOut_.clear ();

  {
    decltype (param_maps_)::ScopedAccess<farbot::ThreadType::nonRealtime>
      param_maps{ param_maps_ };
    generate_all_param_input_events (*param_maps);
  }

  // The farbot access is released before calling into the plugin: the
  // plugin may reentrantly invoke host callbacks that acquire a nonRealtime
  // access themselves, and farbot's nonRealtime lock is not recursive
  if (plugin_->canUseParams ())
    plugin_->paramsFlush (evIn_.clapInputEvents (), evOut_.clapOutputEvents ());

  z_trace (
    "CLAP: param_flush_on_main_thread got {} output events", evOut_.size ());

  {
    decltype (param_maps_)::ScopedAccess<farbot::ThreadType::nonRealtime>
      param_maps{ param_maps_ };
    handle_plugin_output_events (*param_maps, units::samples (0u), std::nullopt);
  }

  evOut_.clear ();
  owner_.flush_plugin_values ();
}

void
ClapPlugin::paramsRequestFlush () noexcept
{
  if (!pimpl_->is_plugin_active () && threadCheckIsMainThread ())
    {
      pimpl_->param_flush_on_main_thread ();
      return;
    }

  pimpl_->scheduleParamFlush_.store (true, std::memory_order_release);
}

std::optional<double>
ClapPlugin::ClapPluginImpl::get_param_value (const clap_param_info &info)
{
  assert (is_main_thread);

  if (!plugin_->canUseParams ())
    return 0.0;

  double value{};
  if (plugin_->paramsGetValue (info.id, &value))
    return value;

  // A plugin reporting a parameter via paramsCount/paramsGetInfo but then
  // refusing paramsGetValue is a contract violation; refuse it noisily
  // rather than throwing across the noexcept host-callback boundary.
  z_warning (
    "Failed to get the param value, id: {}, name: {}, module: {}", info.id,
    info.name, info.module);
  return std::nullopt;
}

void
ClapPlugin::ClapPluginImpl::set_plugin_state (PluginState state)
{
  switch (state)
    {
    case Inactive:
      Q_ASSERT (state_ == ActiveAndReadyToDeactivate);
      break;

    case InactiveWithError:
      // A failed activation or an untrustworthy port report can repeat on
      // the next prepare (nothing resets the state while the plugin never
      // became active), so re-entering the error state is valid
      Q_ASSERT (state_ == Inactive || state_ == InactiveWithError);
      break;

    case ActiveAndSleeping:
      Q_ASSERT (state_ == Inactive || state_ == ActiveAndProcessing);
      break;

    case ActiveAndProcessing:
      Q_ASSERT (state_ == ActiveAndSleeping);
      break;

    case ActiveWithError:
      Q_ASSERT (state_ == ActiveAndProcessing);
      break;

    case ActiveAndReadyToDeactivate:
      Q_ASSERT (
        state_ == ActiveAndProcessing || state_ == ActiveAndSleeping
        || state_ == ActiveWithError);
      break;

    default:
      std::terminate ();
    }

  state_ = state;
}

QByteArray
ClapPlugin::save_state_to_byte_array () const
{
  if (!pimpl_ || !pimpl_->plugin_ || !pimpl_->plugin_->canUseState ())
    return {};

  QByteArray     state_data;
  clap_ostream_t ostream{};
  ostream.ctx = &state_data;
  ostream.write =
    +[] (const clap_ostream * stream, const void * buffer, uint64_t size)
    -> int64_t {
    auto * data = static_cast<QByteArray *> (stream->ctx);
    data->append (
      static_cast<const char *> (buffer), static_cast<qsizetype> (size));
    return static_cast<int64_t> (size);
  };
  pimpl_->plugin_->stateSave (&ostream);
  return state_data;
}

std::string
ClapPlugin::save_state_impl () const
{
  auto data = save_state_to_byte_array ();
  return zrythm::utils::to_std_string (data.toBase64 ());
}

bool
ClapPlugin::load_state_impl (const std::string &base64_state)
{
  auto data = QByteArray::fromBase64 (QByteArray::fromStdString (base64_state));

  if (!pimpl_ || !pimpl_->plugin_ || !pimpl_->plugin_->canUseState ())
    {
      // Not instantiated yet - apply during instantiation
      state_to_apply_ = std::move (data);
      return true;
    }

  const bool applied = apply_state_from_byte_array (data);

  if (pimpl_->is_plugin_active ())
    {
      // paramsFlush is audio-thread-only while the plugin is active.
      // Schedule it so the plugin can resolve deferred parameter updates;
      // the resolved values flow back via the flush's output events.
      pimpl_->scheduleParamFlush_.store (true, std::memory_order_release);

      // Read back values now for plugins that apply state synchronously
      sync_param_values_from_plugin ();
    }
  else
    {
      flush_and_sync_params_when_inactive ();
    }
  return applied;
}

bool
ClapPlugin::apply_state_from_byte_array (const QByteArray &data)
{
  if (!pimpl_ || !pimpl_->plugin_ || !pimpl_->plugin_->canUseState ())
    return false;

  QByteArray     mutable_data = data;
  clap_istream_t istream{};
  istream.ctx = &mutable_data;
  istream.read =
    +[] (const clap_istream * stream, void * buffer, uint64_t size) -> int64_t {
    auto * d = static_cast<QByteArray *> (stream->ctx);
    if (d->isEmpty ())
      return 0;
    size = std::min<uint64_t> (size, d->size ());
    std::memcpy (buffer, d->constData (), size);
    d->remove (0, static_cast<qsizetype> (size));
    return static_cast<int64_t> (size);
  };
  if (!pimpl_->plugin_->stateLoad (&istream))
    {
      z_warning ("CLAP: failed to restore plugin state");
      return false;
    }
  return true;
}

void
ClapPlugin::flush_and_sync_params_when_inactive ()
{
  assert (is_main_thread);
  assert (!pimpl_->is_plugin_active ());

  if (!pimpl_->plugin_->canUseParams ())
    return;

  // Per the CLAP spec, plugins may defer parameter updates until the
  // next paramsFlush() after state load. Flush to ensure parameter
  // values are current before reading them back.
  pimpl_->evIn_.clear ();
  pimpl_->evOut_.clear ();
  pimpl_->plugin_->paramsFlush (
    pimpl_->evIn_.clapInputEvents (), pimpl_->evOut_.clapOutputEvents ());
  // Discard output events — the flush is only to trigger internal
  // plugin state resolution, not to process output parameter values.
  pimpl_->evOut_.clear ();

  sync_param_values_from_plugin ();
}

void
ClapPlugin::sync_param_values_from_plugin ()
{
  assert (is_main_thread);

  if (!pimpl_->plugin_->canUseParams ())
    return;

  // Per the CLAP spec, the plugin is responsible for persisting its
  // parameter values via its state. Read back the parameter values (which
  // reflect the loaded state) and update Zrythm's baseValues to match.
  //
  // Snapshot the routed params first, then call into the plugin without
  // holding the farbot access: the plugin may reentrantly invoke host
  // callbacks that acquire a nonRealtime access themselves, and farbot's
  // nonRealtime lock is not recursive
  std::vector<std::pair<clap_id, dsp::ProcessorParameter *>> routed_params;
  {
    decltype (pimpl_->param_maps_)::ScopedAccess<farbot::ThreadType::nonRealtime>
      param_maps{ pimpl_->param_maps_ };
    routed_params.reserve (param_maps->by_id_.size ());
    for (const auto &[clap_id_val, adapter] : param_maps->by_id_)
      {
        if (adapter.zrythm_param != nullptr)
          routed_params.emplace_back (clap_id_val, adapter.zrythm_param);
      }
  }

  int updated = 0;
  for (const auto &[clap_id_val, zrythm_param] : routed_params)
    {
      double value = 0;
      if (!pimpl_->plugin_->paramsGetValue (clap_id_val, &value))
        continue;

      const auto old_base = zrythm_param->baseValue ();
      const auto range = zrythm_param->range ();
      const auto normalized = range.convertTo0To1 (static_cast<float> (value));
      if (!utils::math::floats_near (old_base, normalized, 0.001f))
        {
          zrythm_param->setBaseValue (normalized);
          ++updated;
          z_trace (
            "CLAP: get_value updated '{}' "
            "old={:.4f} new={:.4f}",
            zrythm_param->label (), old_base, normalized);
        }
    }
  z_debug ("CLAP: get_value updated {} params", updated);
}

void
to_json (nlohmann::json &j, const ClapPlugin &p)
{
  to_json (j, static_cast<const Plugin &> (p));
  auto state = p.save_state_impl ();
  if (!state.empty ())
    j[ClapPlugin::kStateKey] = std::move (state);
}

void
from_json (const nlohmann::json &j, ClapPlugin &p)
{
  // State must be deserialized first, because the Plugin deserialization
  // may cause an instantiation
  if (j.contains (ClapPlugin::kStateKey))
    p.load_state_impl (j[ClapPlugin::kStateKey].get<std::string> ());

  from_json (j, static_cast<Plugin &> (p));
}

} // namespace zrythm::plugins
