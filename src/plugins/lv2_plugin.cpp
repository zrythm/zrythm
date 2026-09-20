// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <filesystem>
#include <fstream>
#include <limits>
#include <mutex>
#include <optional>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include <fmt/std.h>

#include "dsp/audio_bus_configuration.h"
#include "dsp/midi_event.h"
#include "plugins/gl_context_utils.h"
#include "plugins/host_window_units.h"
#include "plugins/lv2_discovery.h"
#include "plugins/lv2_plugin.h"
#include "plugins/lv2_ui_event_ring.h"
#include "plugins/lv2_urid_map.h"
#include "plugins/lv2_worker_queue.h"
#include "plugins/lv2_world.h"
#include "plugins/plugin_format_utils.h"
#include "plugins/plugin_host_window.h"
#include "plugins/plugin_library.h"
#include "plugins/plugin_run_loop.h"
#include "plugins/plugin_transport_context.h"
#include "utils/base64.h"
#include "utils/exceptions.h"
#include "utils/io_utils.h"
#include "utils/logger.h"
#include "utils/math_utils.h"
#include "utils/qt.h"
#include "utils/registry_utils.h"
#include "utils/serialization.h"
#include "utils/views.h"
#include "utils/zip_utils.h"

#include <QTimer>
#include <QUrl>

#include <fmt/format.h>
#include <juce_core/juce_core.h>
#include <lilv/lilv.h>
#include <lv2/atom/atom.h>
#include <lv2/atom/forge.h>
#include <lv2/atom/util.h>
#include <lv2/buf-size/buf-size.h>
#include <lv2/core/lv2.h>
#include <lv2/data-access/data-access.h>
#include <lv2/instance-access/instance-access.h>
#include <lv2/midi/midi.h>
#include <lv2/options/options.h>
#include <lv2/parameters/parameters.h>
#include <lv2/patch/patch.h>
#include <lv2/port-groups/port-groups.h>
#include <lv2/port-props/port-props.h>
#include <lv2/presets/presets.h>
#include <lv2/resize-port/resize-port.h>
#include <lv2/state/state.h>
#include <lv2/time/time.h>
#include <lv2/ui/ui.h>
#include <lv2/units/units.h>
#include <lv2/urid/urid.h>
#include <lv2/worker/worker.h>

#if defined(__has_feature) && __has_feature(realtime_sanitizer)
#  include <sanitizer/rtsan_interface.h>
#endif

namespace zrythm::plugins
{

namespace
{

/** Capacity of the atom buffers handed to the plugin, in bytes. */
constexpr uint32_t kAtomBufferSize = 16384;

/**
 * Upper bound for atom input buffer capacities derived from
 * rsz:minimumSize: a hostile declaration must not size the buffer
 * arbitrarily.
 */
constexpr uint32_t kMaxAtomBufferSize = 1u << 20;

/**
 * Upper bounds for extracted state archives (entry count and total
 * uncompressed size): a corrupt or hostile state blob must not exhaust
 * host resources at extraction.
 */
constexpr size_t kMaxStateArchiveEntries = 10000;
constexpr size_t kMaxStateArchiveTotalSize = size_t{ 1 } << 30;

/**
 * Sample rate and maximum block length of the instance created at
 * configuration time, before the engine's values are known. Processing
 * preparation re-instantiates the plugin when they differ from the
 * engine's.
 */
constexpr auto kProvisionalSampleRate = units::sample_rate (48000);
constexpr auto kProvisionalMaxBlockLength = units::samples (512u);

/** A whole note is four quarter notes. */
constexpr double kQuartersPerWholeNote = 4.0;

/**
 * Capacity of the UI<->plugin atom and control relay rings, in bytes.
 * Events beyond the capacity are dropped and counted. AbstractFifo
 * reserves one slot internally, so the usable capacity is one byte
 * less: a record sized at exactly this constant may still be dropped.
 */
constexpr size_t kUiEventRingCapacity = size_t{ 1 } << 16;

/** Returns the last path segment of a URI (after '#' or '/'). */
std::string_view
uri_local_name (std::string_view uri)
{
  const auto hash_pos = uri.find_last_of ('#');
  if (hash_pos != std::string_view::npos)
    return uri.substr (hash_pos + 1);
  const auto slash_pos = uri.find_last_of ('/');
  return slash_pos != std::string_view::npos ? uri.substr (slash_pos + 1) : uri;
}

utils::Utf8String
uri_local_name_string (std::string_view uri)
{
  return utils::Utf8String::from_utf8_encoded_string (
    std::string (uri_local_name (uri)));
}

uint32_t
urid_map_callback (void * handle, const char * uri)
{
  return static_cast<Lv2UridMap *> (handle)->map (uri);
}

const char *
urid_unmap_callback (void * handle, uint32_t urid)
{
  return static_cast<Lv2UridMap *> (handle)->unmap (urid);
}

dsp::ParameterRange::Unit
unit_from_lv2_uri (std::string_view uri)
{
  const auto name = uri_local_name (uri);
  if (name == "hz")
    return dsp::ParameterRange::Unit::Hz;
  if (name == "mhz")
    return dsp::ParameterRange::Unit::MHz;
  if (name == "db")
    return dsp::ParameterRange::Unit::Db;
  if (name == "degree")
    return dsp::ParameterRange::Unit::Degrees;
  if (name == "s")
    return dsp::ParameterRange::Unit::Seconds;
  if (name == "ms")
    return dsp::ParameterRange::Unit::Ms;
  return dsp::ParameterRange::Unit::None;
}

} // namespace

class Lv2Plugin::Lv2PluginImpl
{
public:
  Lv2PluginImpl (Lv2Plugin &owner, PluginHostWindowFactory host_window_factory)
      : owner_ (owner), host_window_factory_ (std::move (host_window_factory))
  {
  }

  struct PortInfo
  {
    enum class Type : std::uint8_t
    {
      Audio,
      Control,
      CV,
      Atom,
      /** Directed port with an unrecognized type, e.g. the legacy
       * lv2:event type; treated like an unroutable atom port. */
      Unknown,
    };

    uint32_t          index{};
    utils::Utf8String symbol;
    utils::Utf8String name;
    dsp::PortFlow     flow{};
    Type              type{};

    /* control port metadata */
    float def{};
    float min{};
    float max{};
    bool  is_toggled{};
    bool  is_integer{};
    bool  is_logarithmic{};
    bool  is_enumeration{};
    /** Input control port with a port-props trigger property: the host
     * must reset it to its port default after each run(). */
    bool is_trigger{};
    /** Input control port with lv2:designation lv2:freeWheeling: the
     * host writes 1 while rendering offline. */
    bool is_freewheel{};
    /** Scale point labels, sorted by their values. */
    std::vector<utils::Utf8String> scale_point_labels;
    std::vector<float>             scale_point_values;
    dsp::ParameterRange::Unit      unit{ dsp::ParameterRange::Unit::None };
    /** Control input with lv2:designation lv2:latency. */
    bool is_latency{};

    /** Group URI ("" when the port belongs to no group). */
    utils::Utf8String group_uri;

    /** atom port capabilities */
    bool supports_midi{};
    bool supports_time_position{};
    bool has_sequence_buffer_type{};
    /** Atom input port designated lv2:control (the conventional carrier
     * of MIDI events and patch messages). */
    bool is_control_designated{};
    /** rsz:minimumSize declared for the port (0 when undeclared). */
    uint32_t rsz_minimum_size{};

    /**
     * Position of the port's buffer within its flow's control buffer list
     * (control ports only).
     */
    size_t control_buffer_index{};

    /** Audio ports: the bus the port's group forms within its flow, the
     * port's channel within it (group ports in index order map to channels
     * in order), and the bus's external id (the group's first port index).
     */
    size_t audio_bus_index{};
    size_t audio_bus_external_id{};
    size_t audio_channel{};
    /** CV ports: position among the flow's CV ports. */
    size_t cv_index_in_flow{};

    /* Per-port slots in the scratch buffers assigned to ports this host
     * cannot route to an engine port. */
    size_t audio_scratch_slot{};
    size_t cv_scratch_slot{};
    /** Whether the port owns a buffer in the atom scratch buffers
     * (routed atom ports do not). */
    bool has_atom_scratch = false;
    /** Byte offset of the port's buffer within the atom scratch
     * buffers (ports that own one). */
    size_t atom_scratch_byte_offset{};
  };

  /** Resolves the lilv plugin for @p bundle matching @p uri_hash. */
  bool resolve_plugin (const std::filesystem::path &bundle, int64_t uri_hash);

  /** Reads the port metadata of plugin_ into ports_. Returns false when
   * the plugin data is malformed (e.g. a port without a direction). */
  bool read_port_metadata ();

  /**
   * Groups the audio ports of one flow into buses, returned as ports_
   * indices: one bus per port group (first-occurrence order, the group's
   * ports in ports_ order) and one bus per ungrouped port. Both the
   * per-port bus assignment in read_port_metadata() and the
   * AudioBusConfig list derive from this grouping, so the two can never
   * diverge.
   */
  [[nodiscard]] std::vector<std::vector<size_t>>
  audio_bus_groups (dsp::PortFlow flow) const;

  /** Builds the bus configuration of @p flow from the port metadata. */
  std::vector<dsp::AudioBusConfig>
  build_audio_bus_configs (dsp::PortFlow flow) const;

  /** Instantiates the plugin at @p sample_rate with a maximum block length
   * of @p max_block_length (both are bound at instantiation). */
  bool instantiate (
    units::sample_rate_t sample_rate,
    units::sample_u32_t  max_block_length);

  /** Tears the instance down (deactivate + cleanup). */
  void free_instance ();

  /** Connects the control port buffers to the instance. */
  void connect_control_ports ();

  /** Pairs the audio/CV port metadata with the engine ports. */
  void pair_ports_with_engine_ports ();

  /** Connects a port with plugin code exempted from RTSan:
   * connect_port is plugin code, which non-hardRTCapable plugins may
   * block in. */
  void
  connect_port_rt (uint32_t index, void * data) noexcept [[clang::nonblocking]]
  {
#if defined(__has_feature) && __has_feature(realtime_sanitizer)
    __rtsan::ScopedDisabler disabler;
#endif
    lilv_instance_connect_port (instance_, index, data);
  }

  /** Connects the audio and CV port pointers for this chunk; ports with
   * no routed engine port are connected to their scratch buffer slot. */
  void connect_sample_ports (units::sample_u32_t local_offset) noexcept
    [[clang::nonblocking]];

  /** Forges the atom input buffers for this chunk. */
  void forge_atom_inputs (
    dsp::graph::ProcessBlockInfo time_info,
    const dsp::ITransport       &transport,
    const dsp::TempoMap         &tempo_map) noexcept [[clang::nonblocking]];

  /** Parses the atom output buffer into the MIDI output port; events
   * outside the chunk are dropped. */
  void parse_atom_outputs (
    units::sample_u32_t local_offset,
    units::sample_u32_t nframes) noexcept [[clang::nonblocking]];

  /**
   * Writes an atom:Chunk header with the buffer capacity into every
   * output atom buffer, as required by the LV2 atom port contract before
   * each run.
   */
  void reset_atom_output () noexcept [[clang::nonblocking]];

  /** Writes changed parameter values into the control input buffers. */
  void apply_changed_param_values () noexcept [[clang::nonblocking]];

  /**
   * @brief Creates parameters for the plugin's patch:writable
   * parameters and wires them into patch_params_.
   *
   * Parameters with a non-numeric range (e.g. atom:Path files) and
   * parameters without a declared default are skipped with a log.
   *
   * @param param_index_by_id Unique id -> parameter list index.
   * @param generate_new Whether new parameters may be created (false
   * while deserializing, where existing parameters are matched).
   */
  void discover_patch_parameters (
    const std::unordered_map<std::string_view, size_t> &param_index_by_id,
    bool                                                generate_new);

  /**
   * @brief Forges the queued patch sets into the atom sequence the
   * forge currently points at.
   *
   * Sets that do not fit the buffer are dropped and counted.
   */
  void forge_pending_patch_sets () noexcept [[clang::nonblocking]];

  /**
   * @brief Reads patch:Set messages from the atom outputs and syncs
   * the values of matching patch parameters.
   *
   * The first Set a parameter reports after a state restore is
   * applied as value synchronization (setBaseValue); this is also how
   * state-restored parameter values (delivered through the plugin's
   * own state interface) reach the parameter model. Any other Set is
   * a live edit from the plugin side and is applied as a user edit.
   * Outside a pending synchronization, a value equal to the last
   * forged set is dropped (the plugin echoing the host's own
   * message).
   */
  void parse_patch_messages () noexcept [[clang::nonblocking]];

  /** Reads the control output ports (latency). */
  void read_control_outputs () noexcept [[clang::nonblocking]];

  /**
   * Zeroes the routed output buffers of the chunk window [local_offset,
   * local_offset + nframes) of a block that cannot run on the instance
   * and records the drop: the next processing preparation logs it.
   */
  void degrade_to_silence (
    units::sample_u32_t local_offset,
    units::sample_u32_t nframes) noexcept [[clang::nonblocking]];

  /**
   * Serializes the current instance state to a base64 zip archive
   * string (a state.ttl plus any state-created files), or nullopt when
   * no instance exists or serialization fails.
   */
  [[nodiscard]] std::optional<std::string>
  save_state_to_blob () [[clang::blocking]];

  /** Restores the instance from a base64 zip archive state string. */
  bool
  restore_state_from_blob (const std::string &base64_state) [[clang::blocking]];

  /** Restores the instance from the state of a preset in the world. */
  bool
  restore_preset_from_world (const std::string &preset_uri) [[clang::blocking]];

  /**
   * @brief Applies the default state of the plugin (the
   * state:loadDefaultState feature) read from the world's data.
   *
   * A no-op for plugins that do not declare the feature. Plugins that
   * declare it expect the state described in their data to be loaded
   * into a freshly created instance.
   */
  void restore_default_state_if_declared () [[clang::blocking]];

  /**
   * @brief Restores a state with makePath rooted at the scratch
   * directory.
   */
  void
  restore_state_with_scratch_paths (const LilvState &state) [[clang::blocking]];

  /**
   * @brief Builds the per-restore feature array: makePath points at
   * the restore's root and the worker schedule runs work inline.
   */
  std::vector<const LV2_Feature *>
  features_for_restore (const LV2_Feature * make_path_replacement);

  /**
   * @brief Runs lilv_state_restore with a worker schedule feature
   * that executes work inline.
   *
   * The plugin's set_state() may schedule work through the feature
   * array passed to the restore; the inline schedule completes it
   * before the restore returns. Work scheduled through the
   * instantiate-provided feature during a restore is queued for the
   * worker thread; work_mutex_ serializes its work() against the
   * inline work, and draining the worker first completes requests
   * queued before the restore.
   *
   * Preconditions: callers pause processing, and the restore must not
   * overlap an offline render of the same plugin (the render thread
   * runs work inline during a render).
   *
   * @param state State to restore.
   * @param features Features to pass to the restore.
   */
  void restore_state_with_inline_worker (
    const LilvState                     &state,
    std::span<const LV2_Feature * const> features) [[clang::blocking]];

  /**
   * Finds the UI of plugin_ whose widget type this build embeds and
   * caches it in ui_info_. Emits hasNativeUiChanged when the presence
   * of a usable UI changed.
   */
  void resolve_ui ();

  /** Tears the live UI down (idle pump, UI instance, library, host
   * window, in that order). */
  void destroy_ui ();

  /** LV2 UI write callback: UI -> plugin control/atom input. */
  static void ui_write (
    LV2UI_Controller controller,
    uint32_t         port_index,
    uint32_t         buffer_size,
    uint32_t         port_protocol,
    const void *     buffer);

  /** LV2 UI port map callback: port symbol -> port index. */
  static uint32_t
  ui_port_index (LV2UI_Feature_Handle handle, const char * symbol);

  /** LV2 UI resize feature callback: the UI requests a view size. */
  static int ui_resize (LV2UI_Feature_Handle handle, int width, int height);

  /**
   * Runs one UI pump cycle on the main thread: drains plugin -> UI
   * events into port_event, forwards changed control input values and
   * calls the UI idle interface.
   */
  void ui_idle_tick ();

  /**
   * Drains UI -> plugin events from the ring on the audio thread:
   * control floats are applied to their buffers directly, atom events
   * are appended to their port's sequence during this chunk's forging.
   *
   * @return True while a UI session is dispatched; the audio thread
   * must not touch the drained events when false.
   */
  bool drain_ui_atom_events () noexcept [[clang::nonblocking]];

  /**
   * Pushes the events of the atom output sequences to the plugin -> UI
   * ring on the audio thread.
   */
  void dispatch_atom_outputs_to_ui () noexcept [[clang::nonblocking]];

  /** Writes one record into the plugin -> UI ring, dropping and
   * counting on overflow. */
  void push_ui_event (
    uint32_t                   port_index,
    uint32_t                   protocol,
    std::span<const std::byte> body) noexcept [[clang::nonblocking]];

  /**
   * @brief Queues a worker request (the LV2_Worker_Schedule callback
   * passed to the plugin).
   */
  static LV2_Worker_Status schedule_work_callback (
    LV2_Worker_Schedule_Handle handle,
    uint32_t                   size,
    const void *               data) noexcept;

  /**
   * @brief Runs a worker request inline (the LV2_Worker_Schedule
   * callback passed to the plugin's set_state() in the restore
   * feature array).
   */
  static LV2_Worker_Status restore_schedule_work_callback (
    LV2_Worker_Schedule_Handle handle,
    uint32_t                   size,
    const void *               data) noexcept;

  /**
   * @brief Queues a worker response produced by work() (the respond
   * callback passed to the plugin's work()).
   */
  static LV2_Worker_Status respond_callback (
    LV2_Worker_Respond_Handle handle,
    uint32_t                  size,
    const void *              data) noexcept;

  /** Pops requests and runs the plugin's work() until the thread is
   * stopped. */
  void worker_thread_func (std::stop_token stop_token);

  /**
   * @brief Warns about worker queue drops recorded since the last
   * report.
   *
   * Called from the worker thread (at most once per wakeup) and once
   * after the worker thread joins, never from the audio thread.
   */
  void warn_dropped_records ();

  /**
   * @brief Blocks until the worker thread is idle with an empty
   * request queue.
   *
   * Spins with yield instead of waiting on a condition variable to
   * stay usable from noexcept contexts; the wait is bounded by the
   * duration of an in-flight work() call. No-op when no worker thread
   * is running.
   */
  void wait_for_worker_idle () noexcept;

  /**
   * @brief Runs one work() call inline on the calling thread and
   * delivers its responses immediately (offline rendering).
   */
  LV2_Worker_Status
  run_work_inline (uint32_t size, const std::byte * data) noexcept;

  /**
   * @brief Delivers queued worker responses to the plugin's
   * work_response() (audio thread, after run()).
   *
   * Delivery happens after this cycle's run() and before end_run(),
   * like the reference hosts; atoms a plugin writes during
   * work_response() are then read by the same cycle's output parsing.
   */
  void deliver_worker_responses () noexcept [[clang::nonblocking]];

  /**
   * @brief Calls the plugin's end_run() after run() when the worker
   * interface provides one (audio thread).
   */
  void finish_worker_cycle () noexcept [[clang::nonblocking]];

  /**
   * @brief Resets every port-props trigger control input buffer to its
   * port default (audio thread, after run()).
   */
  void reset_trigger_ports () noexcept [[clang::nonblocking]];

  /**
   * @brief Queues a UI worker request (the LV2_Worker_Schedule
   * callback passed to the UI).
   */
  static LV2_Worker_Status ui_schedule_work_callback (
    LV2_Worker_Schedule_Handle handle,
    uint32_t                   size,
    const void *               data) noexcept;

  /**
   * @brief Collects a response produced by the UI's work() (the
   * respond callback passed to the UI's work()).
   */
  static LV2_Worker_Status ui_respond_callback (
    LV2_Worker_Respond_Handle handle,
    uint32_t                  size,
    const void *              data) noexcept;

  /**
   * @brief Runs queued UI worker jobs and delivers their responses on
   * the UI thread (called from ui_idle_tick()).
   */
  void pump_ui_worker ();

  /** Atom events collected by drain_ui_atom_events() for this chunk. */
  struct PendingUiAtom
  {
    uint32_t                               port_index;
    uint32_t                               size;
    std::array<std::byte, kAtomBufferSize> bytes;

    /** Copies @p src into the body. The body array stays
     * default-initialized until the copy: zeroing 16 KiB per record
     * would be wasted real-time work. */
    PendingUiAtom (uint32_t port_index_, std::span<const std::byte> src)
        : port_index (port_index_), size (static_cast<uint32_t> (src.size ()))
    {
      std::ranges::copy (src, bytes.begin ());
    }
  };
  std::vector<PendingUiAtom> pending_ui_atoms_;
  /** Scratch for draining one record body on the audio thread. */
  std::array<std::byte, kAtomBufferSize> ui_drain_scratch_{};

  /** UI of the loaded plugin matching this build's window system. */
  struct UiInfo
  {
    std::string           uri;
    std::filesystem::path binary_path;
    /** Bundle directory with a trailing separator, as the UI's
     * instantiate() expects it. */
    std::string bundle_path;
    bool        fixed_size{};
    bool        no_user_resize{};
    /** The UI declares ui:showInterface: it opens its own toplevel
     * window instead of being embedded. */
    bool show_interface{};
  };
  std::optional<UiInfo> ui_info_;

  /** A control port relayed to the UI on each idle tick: the LV2 port
   * index paired with the index of its control buffer. */
  struct RelayedControl
  {
    uint32_t lv2_port_index;
    size_t   buffer_index;
  };

  /** Live UI session between show_editor() and destroy_ui(). */
  struct UiSession
  {
    std::unique_ptr<PluginHostWindow>                    editor_window;
    utils::QObjectUniquePtr<PluginViewResizeCoordinator> resize_coordinator;
    PluginRunLoop                                        run_loop_;
    PluginLibrary                                        lib;
    const LV2UI_Descriptor *                             descriptor{};
    LV2UI_Handle                                         handle{};
    bool                                                 created{};
    bool                                                 visible{};
    bool                                                 initial_size_applied{};
    const LV2UI_Idle_Interface *                         idle_iface{};
    const LV2UI_Resize *                                 plugin_resize_iface{};
    /** The UI shows itself through ui:showInterface instead of being
     * embedded in the host window (which then stays hidden). */
    bool                         float_window{};
    const LV2UI_Show_Interface * show_iface{};
    PluginRunLoop::Token         idle_token_{};
    /** Feature storage; the UI keeps these pointers after
     * instantiation, so they must stay stable until cleanup. */
    std::vector<LV2_Feature>          features;
    std::vector<const LV2_Feature *>  feature_ptrs;
    LV2UI_Resize                      resize_feature_{};
    LV2UI_Port_Map                    port_map_feature_{};
    LV2_Extension_Data_Feature        ext_data_feature_{};
    std::array<LV2_Options_Option, 3> options_{};
    std::string                       window_title_;
    float                             scale_opt_{ 1.f };
    /** Native window handle passed as the ui:parent feature data (an X11
     * Window, HWND or NSView* value; the LV2 UI spec requires the value
     * itself, the same type as the LV2UI_Widget, not a pointer to it). */
    quintptr parent_window_{};
    /** Control inputs/outputs relayed to the UI on each idle tick. */
    std::vector<RelayedControl> relayed_control_ins_;
    std::vector<RelayedControl> relayed_control_outs_;
    /** Values last sent to the UI, inputs first then outputs (NaN
     * before the initial full send). */
    std::vector<float> last_control_values_;
    /** Scratch of one drained record body (audio-thread and
     * main-thread sides each use their own). */
    std::array<std::byte, kAtomBufferSize> event_scratch_{};
    /** UI worker: the schedule feature passed to the UI, the interface
     * it serves, and the request queue pumped by the idle tick. This
     * host runs and responds to UI worker jobs on the UI thread, the
     * only thread a UI may assume its code runs on. The queued byte
     * count bounds the deque like the plugin-side queues. */
    LV2_Worker_Schedule                ui_worker_schedule_feature_{};
    const LV2_Worker_Interface *       ui_worker_interface_ = nullptr;
    std::mutex                         ui_worker_mutex_;
    std::deque<std::vector<std::byte>> ui_worker_requests_;
    size_t                             ui_worker_queued_bytes_ = 0;
  };
  std::optional<UiSession> ui_;

  /**
   * Whether atom outputs are relayed to the UI ring: read on the audio
   * thread, set while a UI instance exists.
   */
  std::atomic<bool> ui_dispatch_{ false };

  /** Byte rings relaying events between the UI (main thread) and the
   * plugin (audio thread) in both directions. */
  juce::AbstractFifo ui_to_plugin_fifo_{
    static_cast<int> (kUiEventRingCapacity)
  };
  std::array<std::byte, kUiEventRingCapacity> ui_to_plugin_buf_;
  juce::AbstractFifo                          plugin_to_ui_fifo_{
    static_cast<int> (kUiEventRingCapacity)
  };
  std::array<std::byte, kUiEventRingCapacity> plugin_to_ui_buf_;

  /** Serializes UI -> plugin record writers (a UI may write from
   * several of its own threads) against the session-open reset. The
   * write path never runs on the audio thread. */
  std::mutex ui_write_mutex_;

  /** Events dropped by drain_ui_atom_events() (port not routable or
   * ring full) since the last processing preparation. */
  std::atomic<uint64_t> ui_to_plugin_dropped_{ 0 };
  /** Events dropped because the plugin -> UI ring was full since the
   * last processing preparation. */
  std::atomic<uint64_t> plugin_to_ui_dropped_{ 0 };

  /** LV2 state:makePath callback: returns a path under the current
   * makePath root, creating leading directories. */
  static char *
  state_make_path (LV2_State_Make_Path_Handle handle, const char * path);

  /** LV2 state:freePath callback. */
  static void state_free_path (LV2_State_Free_Path_Handle handle, char * path);

  /**
   * Creates the per-plugin session directories on first instantiation.
   * They outlive every instance of this plugin object so files created
   * by the plugin survive re-instantiation.
   */
  void ensure_session_dirs ();

  /** LV2 state restore callback: applies a control port value. */
  static void state_set_port_value (
    const char * port_symbol,
    void *       user_data,
    const void * value,
    uint32_t     size,
    uint32_t     type);

  /**
   * LV2 state save callback: returns the current value of a control input
   * port, read from the connected control buffer.
   */
  static const void * state_get_port_value (
    const char * port_symbol,
    void *       user_data,
    uint32_t *   size,
    uint32_t *   type);

  /** The ports_ index of the first port matching @p pred, or -1. */
  template <typename Pred> int32_t find_port (Pred pred) const
  {
    const auto it = std::ranges::find_if (ports_, pred);
    return it == ports_.end () ? -1 : static_cast<int32_t> (it - ports_.begin ());
  }

  /**
   * Capacity of the atom sequence buffer of @p port: its
   * rsz:minimumSize when larger than the default capacity (already
   * clamped when read).
   */
  static uint32_t atom_port_capacity (const PortInfo &port)
  {
    return std::max (kAtomBufferSize, port.rsz_minimum_size);
  }

  /** Capacity of the atom sequence buffer of the port at @p ports_idx
   * (the default capacity when there is no such port). */
  uint32_t atom_port_capacity (int32_t ports_idx) const
  {
    if (ports_idx < 0)
      return kAtomBufferSize;
    return atom_port_capacity (ports_[static_cast<size_t> (ports_idx)]);
  }

  /**
   * Invokes @p fn for every event of the output sequence at @p buf.
   *
   * The sequence and its events are plugin-controlled: sizes are
   * validated before any body is read, and only frame-timed sequences
   * are accepted (a beat-timed sequence's unit URID would be misread as
   * frames). Events whose declared end passes the sequence end stop the
   * walk.
   *
   * @return False when @p buf holds no well-formed sequence.
   */
  template <typename Fn>
  bool
  for_each_atom_output_event (const uint8_t * buf, size_t buf_size, Fn &&fn) noexcept
  {
    const auto * seq = reinterpret_cast<const LV2_Atom_Sequence *> (buf);
    if (buf_size < sizeof (LV2_Atom))
      return false;
    if (seq->atom.type != host_urids_.atom_Sequence)
      return false;
    if (
      seq->atom.size < sizeof (LV2_Atom_Sequence_Body)
      || seq->atom.size > buf_size - sizeof (LV2_Atom) || seq->body.unit != 0)
      return false;
    const auto * seq_end = buf + sizeof (LV2_Atom) + seq->atom.size;
    LV2_ATOM_SEQUENCE_FOREACH (seq, ev)
    {
      if (
        reinterpret_cast<const uint8_t *> (ev) + sizeof (LV2_Atom_Event)
          + ev->body.size
        > seq_end)
        break;
      std::forward<Fn> (fn) (ev);
    }
    return true;
  }

  struct CtrlInParam;

  /**
   * Returns the control buffer value of @p ctrl's port for the parameter
   * value @p value_0_to_1. Enumeration parameters are index-valued while
   * their ports expect the selected scale point's value, so the two
   * spaces are mapped through the scale points.
   */
  float
  control_value_for_param (const CtrlInParam &ctrl, float value_0_to_1) const
  {
    const auto range_value = ctrl.param->range ().convertFrom0To1 (value_0_to_1);
    if (
      ctrl.port == nullptr || !ctrl.port->is_enumeration
      || ctrl.port->scale_point_values.empty ())
      return range_value;
    const auto index = static_cast<size_t> (std::lround (
      std::clamp (
        range_value, 0.f, ctrl.port->scale_point_values.size () - 1.f)));
    return ctrl.port->scale_point_values[index];
  }

  /**
   * Returns the normalized parameter value of @p ctrl's parameter for
   * the control buffer value @p control_value (the inverse of
   * control_value_for_param(): enumeration ports map to the nearest
   * scale point's index).
   */
  float
  param_value_0_to_1_for_control (const CtrlInParam &ctrl, float control_value)
    const
  {
    const auto &range = ctrl.param->range ();
    if (
      ctrl.port == nullptr || !ctrl.port->is_enumeration
      || ctrl.port->scale_point_values.empty ())
      return range.convertTo0To1 (control_value);
    const auto it = std::ranges::min_element (
      ctrl.port->scale_point_values, {},
      [control_value] (float v) { return std::abs (v - control_value); });
    return range.convertTo0To1 (
      static_cast<float> (it - ctrl.port->scale_point_values.begin ()));
  }

  /** The metadata of the control input with @p symbol, or nullptr when no
   * such port exists. */
  const PortInfo * find_control_input (std::string_view symbol) const
  {
    const auto it = std::ranges::find_if (ports_, [&symbol] (const PortInfo &p) {
      return p.symbol.view () == symbol;
    });
    return it == ports_.end () ? nullptr : &*it;
  }

  Lv2Plugin &owner_;

  PluginHostWindowFactory host_window_factory_;

  /** Resolved lilv plugin, borrowed from the shared world. */
  const LilvPlugin * plugin_ = nullptr;

  LilvInstance * instance_ = nullptr;

  /** True while the instance is activated (run() is only legal then). */
  bool instance_active_ = false;

  std::vector<PortInfo> ports_;

  /* Feature storage; plugins keep these pointers after instantiation, so
   * they must stay stable and valid until the instance is freed. */
  LV2_URID_Map   urid_map_feature_{};
  LV2_URID_Unmap urid_unmap_feature_{};

  /* Session directories for plugin state: files created by the plugin
   * through state:makePath live under scratch/, and extracted state
   * archives are unpacked into state/ before a restore. */
  std::unique_ptr<QTemporaryDir> session_dir_;
  std::filesystem::path          session_scratch_dir_;
  std::filesystem::path          session_state_dir_;
  /** Root a makePath call resolves paths against. The
   * instantiate-time feature is bound to the scratch directory for
   * the plugin's lifetime; save and restore calls bind their own
   * feature with a call-specific root. */
  struct MakePathContext
  {
    Lv2PluginImpl *               impl;
    const std::filesystem::path * root;
  };
  MakePathContext     make_path_context_{};
  LV2_State_Make_Path make_path_feature_{};
  LV2_State_Free_Path free_path_feature_{};
  /** Inline worker schedule handed to the plugin's set_state() in the
   * restore feature array. */
  LV2_Worker_Schedule restore_worker_schedule_feature_{};
  /** Feature wrapper for restore_worker_schedule_feature_, kept as a
   * direct member so features_for_restore() can hand it out. */
  LV2_Feature restore_worker_schedule_feature_wrapper_{};
  /**
   * The work:schedule feature handed to the plugin at instantiation.
   * Direct member: it is pushed into the feature array before
   * lilv_instance_instantiate(), before the worker state exists (the
   * plugin's interface data is only queryable afterwards). Work
   * scheduled during instantiate() is refused with
   * LV2_WORKER_ERR_UNKNOWN: the worker state is created once
   * instantiation succeeds.
   */
  LV2_Worker_Schedule schedule_feature_{};
  /** Copy of the world's host URIDs, read on the audio thread without
   * touching the world. */
  Lv2HostUrids                      host_urids_{};
  std::array<LV2_Options_Option, 6> options_{};
  std::vector<LV2_Feature>          features_;
  std::vector<const LV2_Feature *>  feature_ptrs_;
  float                             sample_rate_opt_{};
  int32_t                           min_block_opt_{};
  int32_t                           max_block_opt_{};
  int32_t                           nominal_block_opt_{};
  int32_t                           sequence_size_opt_{};

  /**
   * Worker extension state, created when the plugin exposes
   * LV2_Worker_Interface and torn down with the instance. The
   * schedule_work() callback runs on the main thread during load and
   * restore and on the audio thread while processing; the host
   * sequence keeps those phases from overlapping, so each queue has
   * one producer at a time.
   */
  struct WorkerState
  {
    static constexpr size_t      kQueueCapacity = 256uz * 1024;
    const LV2_Worker_Interface * interface_ = nullptr;
    /** Serializes work() calls across the worker thread and inline
     * execution (state restores, offline rendering). Recursive: work()
     * may schedule more work that runs inline within the same call. */
    std::recursive_mutex    work_mutex_;
    Lv2WorkerQueue          requests_{ kQueueCapacity };
    Lv2WorkerQueue          responses_{ kQueueCapacity };
    std::jthread            thread_;
    std::mutex              sleep_mutex_;
    std::condition_variable sleep_cv_;
    /** Request pop scratch, owned by the worker thread. */
    std::array<std::byte, kMaxWorkerRecordSize> request_scratch_{};
    /** Response pop scratch, owned by the audio thread. */
    std::array<std::byte, kMaxWorkerRecordSize> response_scratch_{};
    /**
     * True while the processor renders offline: work scheduled from
     * the processing context runs inline instead of being queued for
     * the worker thread. Toggled while processing is stopped; a
     * producer racing the toggle is out of contract.
     */
    std::atomic<bool> offline_{ false };
    /**
     * True while the worker thread sits in its idle wait with an
     * empty request queue and no in-flight work().
     */
    std::atomic<bool> idle_{ false };
    /** Drop counts already reported by warn_dropped_records(). */
    std::atomic<uint32_t> warned_request_drops_{ 0 };
    std::atomic<uint32_t> warned_response_drops_{ 0 };
  };
  /** Null while the instance exposes no LV2_Worker_Interface. */
  std::unique_ptr<WorkerState> worker_;

  /** Control port buffers, indexed by PortInfo::control_buffer_index. */
  std::vector<float> control_in_bufs_;
  std::vector<float> control_out_bufs_;

  /** Parameter routing, rebuilt on the main thread at load. */
  struct CtrlInParam
  {
    dsp::ProcessorParameter * param{};
    /** The port the parameter drives (null for unrouted buffers). */
    const PortInfo * port{};
  };
  /** Parallel to control_in_bufs_. */
  std::vector<CtrlInParam> ctrl_in_params_;
  /** Control input buffer indices of port-props trigger ports and the
   * port defaults they are reset to after each run(). */
  std::vector<std::pair<size_t, float>> trigger_control_ins_;
  /** Control input buffer indices of freeWheeling-designated ports.
   * The host writes 1 while rendering offline; no parameter is
   * created for them. */
  std::vector<size_t> freewheel_control_ins_;
  /** Parameter list index -> control input buffer index, or -1. */
  std::vector<int32_t> param_to_ctrl_in_;
  /**
   * A patch:writable parameter (an lv2:Parameter the plugin reads from
   * its control atom port instead of a control port).
   */
  struct PatchParam
  {
    dsp::ProcessorParameter * param{};
    /** URID of the parameter URI. */
    uint32_t uri_id{};
    /** Atom type of patch:value (atom:Float, atom:Int or atom:Bool). */
    uint32_t value_type{};
    /** Normalization of the value forged in the last patch:Set
     * (integer and boolean values quantized); a Set echoed back at
     * this value is dropped. */
    float last_sent_0_to_1{};
    /**
     * Set by a state restore (with processing paused): the first
     * patch:Set the parameter reports afterwards is applied as value
     * synchronization, not a user edit.
     */
    bool expect_sync_notify{};
  };
  /** patch:writable parameters. */
  std::vector<PatchParam> patch_params_;
  /** Parameter list index -> index into patch_params_, or -1. */
  std::vector<int32_t> param_to_patch_;
  /**
   * Patch sets queued by apply_changed_param_values() for the next
   * forge_atom_inputs(). Fixed-capacity: sets beyond the capacity are
   * dropped and counted.
   */
  struct PendingPatchSet
  {
    uint32_t patch_idx{};
    float    value_0_to_1{};
  };
  static constexpr size_t kPendingPatchSetCapacity = 32;
  std::array<PendingPatchSet, kPendingPatchSetCapacity> pending_patch_sets_{};
  size_t pending_patch_set_count_ = 0;
  /** Number of patch sets dropped since the last report. */
  std::atomic<uint32_t> patch_sets_dropped_{ 0 };
  /** control_out_bufs_ index of the latency port, or -1. */
  int32_t latency_buf_index_ = -1;

  /** Atom buffers. */
  std::vector<uint8_t> atom_in_buf_;
  std::vector<uint8_t> time_in_buf_;
  std::vector<uint8_t> atom_out_buf_;
  /** Per-port atom buffers for atom ports this host cannot route (and
   * unknown-type ports), concatenated in ports_ order: each port's
   * buffer starts at its PortInfo::atom_scratch_byte_offset and spans
   * its atom_port_capacity(). */
  std::vector<uint8_t> atom_scratch_buf_;
  LV2_Atom_Forge       forge_{};

  int32_t midi_in_port_idx_ = -1;
  int32_t midi_out_port_idx_ = -1;
  int32_t time_in_port_idx_ = -1;
  /** Atom input the patch sets are forged into: the ports_ index of
   * the control-designated port, else the MIDI-routed port, else -1
   * (the first atom input at forge time). */
  int32_t patch_in_port_idx_ = -1;

  /** Engine ports paired with the audio buses, indexed by bus (nullptr
   * while unpaired); CV ports pair positionally in ports_ order. */
  std::vector<dsp::AudioPort *> audio_in_ports_by_bus_;
  std::vector<dsp::AudioPort *> audio_out_ports_by_bus_;
  std::vector<dsp::CVPort *>    cv_in_ports_by_port_;
  std::vector<dsp::CVPort *>    cv_out_ports_by_port_;

  /* Zeroed per-port fallback buffers for audio/CV ports with no routed
   * engine port. Sized (port count * scratch_block_stride_) by
   * resize_scratch_buffers(), which also records the stride so buffer
   * size and the per-block pointer arithmetic can never diverge. */
  std::vector<float>  audio_scratch_buf_;
  std::vector<float>  cv_scratch_buf_;
  units::sample_u32_t scratch_block_stride_{ units::samples (0u) };

  /** (Re-)creates the zeroed audio/CV scratch buffers for @p
   * max_block_length and records the stride they are sized with. */
  void resize_scratch_buffers (units::sample_u32_t max_block_length);

  /** Group URIs designated pg:mainInput/pg:mainOutput by the plugin (""
   * when undeclared). */
  utils::Utf8String main_in_group_uri_;
  utils::Utf8String main_out_group_uri_;

  /** Whether the unroutable-port warning was already logged. */
  bool unroutable_ports_warned_ = false;

  /** Whether a block was dropped by degrade_to_silence() since the last
   * processing preparation (which logs and clears it). */
  std::atomic<bool> degradation_reported_{ false };

  /** Number of input events dropped by forge_atom_inputs() since the
   * last processing preparation (which logs and clears it). */
  std::atomic<uint64_t> atom_events_dropped_{ 0 };

  /** Number of plugin output MIDI events dropped because the engine
   * MIDI buffer was full since the last processing preparation (which
   * logs and clears it). */
  std::atomic<uint64_t> midi_output_events_dropped_{ 0 };

  units::sample_rate_t last_sample_rate_{ units::sample_rate (0) };
  units::sample_u32_t  last_max_block_length_{ units::samples (0u) };

  /** Reported playback latency; written by the audio thread and read on
   * the main thread. The value is self-contained, so relaxed access is
   * sufficient: atomicity is all that is required, and the latency-changed
   * notification provides the cross-thread ordering. */
  std::atomic<units::sample_u32_t> latency_{ units::samples (0u) };
};

// ============================================================================
// Lifecycle
// ============================================================================

Lv2Plugin::Lv2Plugin (
  utils::IObjectRegistry                &registry,
  std::shared_ptr<Lv2World>              world,
  std::function<units::sample_rate_t ()> sample_rate_provider,
  std::function<units::sample_u32_t ()>  buffer_size_provider,
  PluginHostWindowFactory                host_window_factory,
  QObject *                              parent)
    : Plugin (registry, parent),
      pimpl_ (
        std::make_unique<Lv2PluginImpl> (*this, std::move (host_window_factory))),
      world_ (std::move (world)),
      sample_rate_provider_ (std::move (sample_rate_provider)),
      buffer_size_provider_ (std::move (buffer_size_provider))
{
  if (world_ == nullptr)
    {
      throw std::logic_error ("Lv2Plugin requires a Lv2World");
    }

  connect (
    this, &Plugin::configurationChanged, this,
    &Lv2Plugin::on_configuration_changed);

  auto bypass_ref = generate_default_bypass_param ();
  add_parameter (bypass_ref);
  set_bypass_id (bypass_ref.id ());
  auto gain_ref = generate_default_gain_param ();
  add_parameter (gain_ref);
  gain_id_ = gain_ref.id ();
}

Lv2Plugin::~Lv2Plugin ()
{
  // Signals from a dying object have no meaningful receivers: blocking
  // them keeps unload_current_plugin() from emitting changes
  // mid-destruction
  blockSignals (true);
  // Unload before member destruction: pimpl_ is destroyed after the
  // world it borrows the plugin data from, and freeing the instance
  // touches that data
  if (pimpl_ != nullptr && pimpl_->plugin_ != nullptr)
    unload_current_plugin ();
}

void
Lv2Plugin::on_configuration_changed (
  PluginConfiguration *,
  bool generateNewPluginPortsAndParams)
{
  // The bundle path is expected as the path alternative of the descriptor
  // identity; anything else is untrusted input and fails the load cleanly
  const auto * path = std::get_if<std::filesystem::path> (
    &configuration ()->descriptor ()->path_or_id_);
  if (path == nullptr)
    {
      z_warning ("LV2: descriptor of '{}' carries no bundle path", get_name ());
      Q_EMIT instantiationFinished (false, tr ("LV2 descriptor has no path"));
      return;
    }

  const auto load = [this, path, generateNewPluginPortsAndParams] () {
    const auto failure = [this, path] (const QString &error) {
      Q_EMIT instantiationFinished (
        false,
        error.isEmpty ()
          ? tr ("Failed to load LV2 plugin from %1")
              .arg (utils::Utf8String::from_path (*path).to_qstring ())
          : error);
    };

    if (
      !load_plugin (
        *path, configuration ()->descriptor ()->unique_id_,
        generateNewPluginPortsAndParams))
      {
        failure (QString{});
        return;
      }

    // The engine's current values give the best first guess for the
    // instance; processing preparation re-creates the instance when they
    // differ from the engine's values at that point
    const auto provisional_sample_rate =
      sample_rate_provider_ && sample_rate_provider_ () > units::sample_rate (0)
        ? sample_rate_provider_ ()
        : kProvisionalSampleRate;
    const auto provisional_block_length =
      buffer_size_provider_ && buffer_size_provider_ () > units::samples (0u)
        ? buffer_size_provider_ ()
        : kProvisionalMaxBlockLength;
    if (!pimpl_->instantiate (provisional_sample_rate, provisional_block_length))
      {
        pimpl_->last_sample_rate_ = provisional_sample_rate;
        pimpl_->last_max_block_length_ = provisional_block_length;
        failure (tr ("Instantiation failed"));
        return;
      }
    pimpl_->last_sample_rate_ = provisional_sample_rate;
    pimpl_->last_max_block_length_ = provisional_block_length;

    // The instance exists before any processing: the pending state can be
    // applied without pausing anything
    apply_pending_state ();

    Q_EMIT instantiationFinished (true, QString{});
  };

  // Re-configuring frees the live instance, which must not overlap audio
  // processing of the same instance
  if (pimpl_->instance_ != nullptr)
    {
      if (main_thread_callbacks_.with_paused_processing_)
        {
          main_thread_callbacks_.with_paused_processing_ (load);
        }
      else
        {
          z_warning (
            "LV2: cannot re-configure '{}' while processing; the host "
            "cannot pause processing",
            get_name ());
          Q_EMIT instantiationFinished (
            false, tr ("Cannot re-configure LV2 plugin during processing"));
        }
      return;
    }

  load ();
}

bool
Lv2Plugin::Lv2PluginImpl::resolve_plugin (
  const std::filesystem::path &bundle,
  int64_t                      uri_hash)
{
  for (const auto &info : get_plugins_in_bundle (*owner_.world_, bundle))
    {
      if (get_hash_for_range (std::string (info.uri_.view ())) != uri_hash)
        continue;

      const auto * plugin =
        owner_.world_->find_plugin (bundle, info.uri_.view ());
      if (plugin == nullptr)
        {
          z_warning (
            "LV2: plugin '{}' disappeared from bundle '{}' during "
            "resolution",
            info.uri_, bundle);
          return false;
        }
      plugin_ = plugin;
      return true;
    }
  z_warning (
    "LV2: no plugin with URI hash {} found in bundle {}", uri_hash, bundle);
  return false;
}

bool
Lv2Plugin::Lv2PluginImpl::read_port_metadata ()
{
  ports_.clear ();

  auto *             world = owner_.world_->raw ();
  const LilvNodeUPtr input_port{ lilv_new_uri (world, LV2_CORE__InputPort) };
  const LilvNodeUPtr output_port{ lilv_new_uri (world, LV2_CORE__OutputPort) };
  const LilvNodeUPtr audio_port{ lilv_new_uri (world, LV2_CORE__AudioPort) };
  const LilvNodeUPtr control_port{ lilv_new_uri (world, LV2_CORE__ControlPort) };
  const LilvNodeUPtr cv_port{ lilv_new_uri (world, LV2_CORE__CVPort) };
  const LilvNodeUPtr atom_port{ lilv_new_uri (world, LV2_ATOM__AtomPort) };
  const LilvNodeUPtr midi_event{ lilv_new_uri (world, LV2_MIDI__MidiEvent) };
  const LilvNodeUPtr time_position{ lilv_new_uri (world, LV2_TIME__Position) };
  const LilvNodeUPtr buffer_type{ lilv_new_uri (world, LV2_ATOM__bufferType) };
  const LilvNodeUPtr atom_sequence{ lilv_new_uri (world, LV2_ATOM__Sequence) };
  const LilvNodeUPtr designation{ lilv_new_uri (world, LV2_CORE__designation) };
  const LilvNodeUPtr control_uri{ lilv_new_uri (world, LV2_CORE__control) };
  const LilvNodeUPtr latency_uri{ lilv_new_uri (world, LV2_CORE__latency) };
  const LilvNodeUPtr freewheeling_uri{
    lilv_new_uri (world, LV2_CORE__freeWheeling)
  };
  const LilvNodeUPtr group{ lilv_new_uri (world, LV2_PORT_GROUPS__group) };
  const LilvNodeUPtr main_input{
    lilv_new_uri (world, LV2_PORT_GROUPS__mainInput)
  };
  const LilvNodeUPtr main_output{
    lilv_new_uri (world, LV2_PORT_GROUPS__mainOutput)
  };
  const LilvNodeUPtr toggled{ lilv_new_uri (world, LV2_CORE__toggled) };
  const LilvNodeUPtr integer{ lilv_new_uri (world, LV2_CORE__integer) };
  const LilvNodeUPtr enumeration{ lilv_new_uri (world, LV2_CORE__enumeration) };
  const LilvNodeUPtr trigger{ lilv_new_uri (world, LV2_PORT_PROPS__trigger) };
  const LilvNodeUPtr scale_point{ lilv_new_uri (world, LV2_CORE__scalePoint) };
  const LilvNodeUPtr rdf_value{
    lilv_new_uri (world, "http://www.w3.org/1999/02/22-rdf-syntax-ns#value")
  };
  const LilvNodeUPtr rdfs_label{
    lilv_new_uri (world, "http://www.w3.org/2000/01/rdf-schema#label")
  };
  const LilvNodeUPtr logarithmic{
    lilv_new_uri (world, LV2_PORT_PROPS__logarithmic)
  };
  const LilvNodeUPtr default_p{ lilv_new_uri (world, LV2_CORE__default) };
  const LilvNodeUPtr minimum_p{ lilv_new_uri (world, LV2_CORE__minimum) };
  const LilvNodeUPtr maximum_p{ lilv_new_uri (world, LV2_CORE__maximum) };
  const LilvNodeUPtr unit{ lilv_new_uri (world, LV2_UNITS__unit) };
  const LilvNodeUPtr minimum_size{
    lilv_new_uri (world, LV2_RESIZE_PORT__minimumSize)
  };

  const auto plugin_group_uri = [this] (const LilvNode * predicate) {
    const LilvNodesUPtr nodes{ lilv_plugin_get_value (plugin_, predicate) };
    if (nodes == nullptr || lilv_nodes_size (nodes.get ()) == 0)
      return utils::Utf8String{};
    return node_to_utf8 (lilv_nodes_get_first (nodes.get ()));
  };
  main_in_group_uri_ = plugin_group_uri (main_input.get ());
  main_out_group_uri_ = plugin_group_uri (main_output.get ());

  const auto num_ports = lilv_plugin_get_num_ports (plugin_);

  for (const auto port_index : std::views::iota (0u, num_ports))
    {
      const auto * port = lilv_plugin_get_port_by_index (plugin_, port_index);

      auto &info = ports_.emplace_back ();
      info.index = port_index;
      info.symbol = node_to_utf8 (lilv_port_get_symbol (plugin_, port));
      if (
        const LilvNodeUPtr name{ lilv_port_get_name (plugin_, port) };
        name != nullptr)
        {
          info.name = node_to_utf8 (name.get ());
        }

      const bool is_input = lilv_port_is_a (plugin_, port, input_port.get ());
      const bool is_output = lilv_port_is_a (plugin_, port, output_port.get ());
      if (!is_input && !is_output)
        {
          // LV2 requires every port to have a direction
          z_warning (
            "LV2: port {} of '{}' has no direction; refusing to load",
            info.symbol, owner_.get_name ());
          return false;
        }
      info.flow = is_input ? dsp::PortFlow::Input : dsp::PortFlow::Output;

      if (lilv_port_is_a (plugin_, port, audio_port.get ()))
        info.type = PortInfo::Type::Audio;
      else if (lilv_port_is_a (plugin_, port, control_port.get ()))
        info.type = PortInfo::Type::Control;
      else if (lilv_port_is_a (plugin_, port, cv_port.get ()))
        info.type = PortInfo::Type::CV;
      else if (lilv_port_is_a (plugin_, port, atom_port.get ()))
        info.type = PortInfo::Type::Atom;
      else
        {
          // Directed ports with an unrecognized type, e.g. the legacy
          // lv2:event type, are connected to an atom scratch buffer
          z_warning (
            "LV2: port {} of '{}' is no audio, control, CV or atom port; "
            "it is connected to a scratch buffer",
            info.symbol, owner_.get_name ());
          info.type = PortInfo::Type::Unknown;
        }

      if (info.type == PortInfo::Type::Control)
        {
          // Ranges, port properties and scale points only feed the
          // parameters built from input control ports
          if (info.flow == dsp::PortFlow::Input)
            {
              const auto get_port_float =
                [&] (const LilvNodeUPtr &predicate) -> std::optional<float> {
                const LilvNodesUPtr nodes{
                  lilv_port_get_value (plugin_, port, predicate.get ())
                };
                if (nodes == nullptr || lilv_nodes_size (nodes.get ()) == 0)
                  return std::nullopt;
                const auto * node = lilv_nodes_get_first (nodes.get ());
                // Non-numeric literals convert to NaN, which cannot be
                // detected reliably in release builds: accept values by
                // node type instead
                if (
                  !lilv_node_is_float (node) && !lilv_node_is_int (node)
                  && !lilv_node_is_bool (node))
                  return std::nullopt;
                return lilv_node_as_float (node);
              };
              const auto defs = get_port_float (default_p);
              const auto mins = get_port_float (minimum_p);
              const auto maxs = get_port_float (maximum_p);
              const auto range_usable = [&] () {
                if (
                  !defs.has_value () || !mins.has_value () || !maxs.has_value ())
                  return false;
                // min >= max breaks the linear range conversion; such
                // ports fall back to a usable dummy
                return *mins < *maxs;
              }();
              if (!range_usable)
                {
                  // LV2 requires a finite default/min/max with min < max on
                  // input control ports; anything else falls back to a
                  // usable dummy so the port can still be connected
                  z_warning (
                    "LV2: control input port {} of '{}' has no usable "
                    "default/min/max; assuming 0..1",
                    info.symbol, owner_.get_name ());
                  info.def = 0.f;
                  info.min = 0.f;
                  info.max = 1.f;
                }
              else
                {
                  info.def = std::clamp (*defs, *mins, *maxs);
                  info.min = *mins;
                  info.max = *maxs;
                }

              info.is_toggled =
                lilv_port_has_property (plugin_, port, toggled.get ());
              info.is_integer =
                lilv_port_has_property (plugin_, port, integer.get ());
              info.is_enumeration =
                lilv_port_has_property (plugin_, port, enumeration.get ());
              info.is_trigger =
                info.flow == dsp::PortFlow::Input
                && lilv_port_has_property (plugin_, port, trigger.get ());
              info.is_logarithmic =
                lilv_port_has_property (plugin_, port, logarithmic.get ());
              if (info.is_logarithmic && info.min <= 0.f)
                {
                  // A logarithmic parameter needs a positive minimum to
                  // span its normalized range
                  z_warning (
                    "LV2: control port {} of '{}' is logarithmic with a "
                    "non-positive minimum; treating it as linear",
                    info.symbol, owner_.get_name ());
                  info.is_logarithmic = false;
                }

              if (
                const LilvNodesUPtr unit_nodes{
                  lilv_port_get_value (plugin_, port, unit.get ()) };
                unit_nodes != nullptr && lilv_nodes_size (unit_nodes.get ()) > 0)
                {
                  info.unit = unit_from_lv2_uri (
                    node_to_utf8 (lilv_nodes_get_first (unit_nodes.get ()))
                      .view ());
                }

              // Scale points are read through direct world queries:
              // lilv_port_get_scale_points() requires a unique rdfs:label
              // per point and aborts when a point carries several
              // language-tagged labels, while these queries tolerate any
              // number of value or label statements. Labels cannot be
              // told apart by language through lilv's API, so the first
              // one wins. Non-numeric point values are skipped like
              // non-numeric ranges above
              std::vector<std::pair<utils::Utf8String, float>> points;
              {
                const LilvNodesUPtr scale_point_nodes{
                  lilv_port_get_value (plugin_, port, scale_point.get ())
                };
                if (scale_point_nodes != nullptr)
                  {
                    LILV_FOREACH (nodes, sp_iter, scale_point_nodes.get ())
                      {
                        const auto * point_node =
                          lilv_nodes_get (scale_point_nodes.get (), sp_iter);
                        const LilvNodesUPtr value_nodes{ lilv_world_find_nodes (
                          world, point_node, rdf_value.get (), nullptr) };
                        const LilvNodesUPtr label_nodes{ lilv_world_find_nodes (
                          world, point_node, rdfs_label.get (), nullptr) };
                        if (value_nodes == nullptr || label_nodes == nullptr)
                          continue;
                        const auto * value_node =
                          lilv_nodes_get_first (value_nodes.get ());
                        if (
                          !lilv_node_is_float (value_node)
                          && !lilv_node_is_int (value_node)
                          && !lilv_node_is_bool (value_node))
                          continue;
                        points.emplace_back (
                          node_to_utf8 (
                            lilv_nodes_get_first (label_nodes.get ())),
                          lilv_node_as_float (value_node));
                      }
                  }
              }
              std::ranges::sort (
                points, {}, &std::pair<utils::Utf8String, float>::second);
              for (auto &[label, value] : points)
                {
                  info.scale_point_labels.push_back (std::move (label));
                  info.scale_point_values.push_back (value);
                }
            }

          if (
            const LilvNodesUPtr designation_nodes{
              lilv_port_get_value (plugin_, port, designation.get ()) };
            designation_nodes != nullptr
            && lilv_nodes_size (designation_nodes.get ()) > 0)
            {
              const auto * designation_node =
                lilv_nodes_get_first (designation_nodes.get ());
              // The designations are full lv2core URIs; a local-name
              // match would misread any '#latency' URI
              if (lilv_node_equals (designation_node, latency_uri.get ()))
                info.is_latency = true;
              if (lilv_node_equals (designation_node, freewheeling_uri.get ()))
                info.is_freewheel = true;
            }
        }

      if (info.type == PortInfo::Type::Atom)
        {
          info.supports_midi =
            lilv_port_supports_event (plugin_, port, midi_event.get ());
          info.supports_time_position =
            lilv_port_supports_event (plugin_, port, time_position.get ());
          if (
            const LilvNodesUPtr designation_nodes{
              lilv_port_get_value (plugin_, port, designation.get ()) };
            designation_nodes != nullptr
            && lilv_nodes_size (designation_nodes.get ()) > 0)
            {
              const auto * designation_node =
                lilv_nodes_get_first (designation_nodes.get ());
              if (lilv_node_equals (designation_node, control_uri.get ()))
                {
                  info.is_control_designated = true;
                }
            }
          if (
            const LilvNodesUPtr buffer_type_nodes{
              lilv_port_get_value (plugin_, port, buffer_type.get ()) };
            buffer_type_nodes != nullptr
            && lilv_nodes_size (buffer_type_nodes.get ()) > 0)
            {
              info.has_sequence_buffer_type = lilv_node_equals (
                lilv_nodes_get_first (buffer_type_nodes.get ()),
                atom_sequence.get ());
            }

          // rsz:minimumSize requests sequence space for the port's
          // buffer; requests are clamped so a hostile declaration
          // cannot size the buffer arbitrarily
          if (
            const LilvNodesUPtr min_size_nodes{
              lilv_port_get_value (plugin_, port, minimum_size.get ()) };
            min_size_nodes != nullptr
            && lilv_nodes_size (min_size_nodes.get ()) > 0)
            {
              const auto * min_size_node =
                lilv_nodes_get_first (min_size_nodes.get ());
              if (lilv_node_is_int (min_size_node))
                {
                  const auto declared = lilv_node_as_int (min_size_node);
                  if (declared > static_cast<int32_t> (kMaxAtomBufferSize))
                    {
                      z_warning (
                        "LV2: atom port {} of '{}' requests {} bytes of "
                        "sequence space; clamping the buffer to {} bytes",
                        info.symbol, owner_.get_name (), declared,
                        kMaxAtomBufferSize);
                      info.rsz_minimum_size = kMaxAtomBufferSize;
                    }
                  else if (declared > static_cast<int32_t> (kAtomBufferSize))
                    {
                      info.rsz_minimum_size = static_cast<uint32_t> (declared);
                    }
                }
            }
        }

      if (
        const LilvNodesUPtr group_nodes{
          lilv_port_get_value (plugin_, port, group.get ()) };
        group_nodes != nullptr && lilv_nodes_size (group_nodes.get ()) > 0)
        {
          info.group_uri =
            node_to_utf8 (lilv_nodes_get_first (group_nodes.get ()));
        }
    }

  midi_in_port_idx_ = find_port ([] (const PortInfo &p) {
    return p.type == PortInfo::Type::Atom && p.flow == dsp::PortFlow::Input
           && p.has_sequence_buffer_type && p.supports_midi;
  });
  midi_out_port_idx_ = find_port ([] (const PortInfo &p) {
    return p.type == PortInfo::Type::Atom && p.flow == dsp::PortFlow::Output
           && p.has_sequence_buffer_type && p.supports_midi;
  });
  time_in_port_idx_ = find_port ([] (const PortInfo &p) {
    return p.type == PortInfo::Type::Atom && p.flow == dsp::PortFlow::Input
           && p.has_sequence_buffer_type && p.supports_time_position;
  });

  // The atom input the patch sets ride: the control-designated port
  // per convention, else the MIDI-routed one, else the first atom
  // input (a scratch buffer at forge time)
  patch_in_port_idx_ = find_port ([] (const PortInfo &p) {
    return p.type == PortInfo::Type::Atom && p.flow == dsp::PortFlow::Input
           && p.has_sequence_buffer_type && p.is_control_designated;
  });
  if (patch_in_port_idx_ < 0)
    {
      patch_in_port_idx_ = midi_in_port_idx_;
    }

  for (const auto flow : { dsp::PortFlow::Input, dsp::PortFlow::Output })
    {
      const auto count_midi =
        std::ranges::count_if (ports_, [flow] (const PortInfo &p) {
          return p.type == PortInfo::Type::Atom && p.flow == flow
                 && p.has_sequence_buffer_type && p.supports_midi;
        });
      if (count_midi > 1)
        {
          z_warning (
            "LV2: '{}' has {} MIDI atom {} ports; only the first is routed",
            owner_.get_name (), count_midi,
            flow == dsp::PortFlow::Input ? "input" : "output");
        }
    }
  if (
    std::ranges::any_of (ports_, [] (const PortInfo &p) {
      return p.type == PortInfo::Type::Atom
             && (!p.has_sequence_buffer_type || (!p.supports_midi && !p.supports_time_position));
    }))
    {
      z_warning (
        "LV2: '{}' has atom ports this host cannot route (no sequence "
        "buffer type or unsupported event types); they are connected to "
        "scratch buffers",
        owner_.get_name ());
    }

  // Assign the control buffer indices and the latency port
  const auto num_ctrl_ins = std::ranges::count_if (ports_, [] (const PortInfo &p) {
    return p.type == PortInfo::Type::Control && p.flow == dsp::PortFlow::Input;
  });
  const auto num_ctrl_outs =
    std::ranges::count_if (ports_, [] (const PortInfo &p) {
      return p.type == PortInfo::Type::Control && p.flow == dsp::PortFlow::Output;
    });
  control_in_bufs_.assign (num_ctrl_ins, 0.f);
  control_out_bufs_.assign (num_ctrl_outs, 0.f);
  size_t ctrl_in_pos = 0;
  size_t ctrl_out_pos = 0;
  latency_buf_index_ = -1;
  for (auto &port : ports_)
    {
      if (port.type != PortInfo::Type::Control)
        continue;
      if (port.flow == dsp::PortFlow::Input)
        {
          port.control_buffer_index = ctrl_in_pos++;
        }
      else
        {
          port.control_buffer_index = ctrl_out_pos;
          if (port.is_latency)
            latency_buf_index_ = static_cast<int32_t> (ctrl_out_pos);
          ++ctrl_out_pos;
        }
    }

  // Assign the bus and channel positions of the audio and CV ports: the
  // audio buses come from the shared grouping, with the bus's external id
  // being its first port's LV2 index; CV ports are numbered positionally
  // within each flow
  size_t next_cv_per_flow[2] = { 0, 0 };
  for (const auto flow : { dsp::PortFlow::Input, dsp::PortFlow::Output })
    {
      const auto flow_idx = flow == dsp::PortFlow::Input ? 0uz : 1uz;
      for (auto &port : ports_)
        {
          if (port.type != PortInfo::Type::CV || port.flow != flow)
            continue;
          port.cv_index_in_flow = next_cv_per_flow[flow_idx]++;
        }

      const auto buses = audio_bus_groups (flow);
      for (const auto &[bus_index, bus] : utils::views::enumerate (buses))
        {
          const auto external_id = ports_[bus.front ()].index;
          for (const auto &[channel, port_index] : utils::views::enumerate (bus))
            {
              auto &port_info = ports_[port_index];
              port_info.audio_bus_index = bus_index;
              port_info.audio_bus_external_id = external_id;
              port_info.audio_channel = channel;
            }
        }
    }

  // Assign the per-port scratch buffers: one audio/CV slot per port of
  // that type, and one atom buffer per unroutable atom or unknown-type
  // port, sized per port from its rsz:minimumSize
  size_t next_audio_slot = 0;
  size_t next_cv_slot = 0;
  size_t atom_scratch_bytes = 0;
  for (auto &port : ports_)
    {
      switch (port.type)
        {
        case PortInfo::Type::Audio:
          port.audio_scratch_slot = next_audio_slot++;
          break;
        case PortInfo::Type::CV:
          port.cv_scratch_slot = next_cv_slot++;
          break;
        case PortInfo::Type::Unknown:
          port.has_atom_scratch = true;
          port.atom_scratch_byte_offset = atom_scratch_bytes;
          atom_scratch_bytes += atom_port_capacity (port);
          break;
        case PortInfo::Type::Atom:
          {
            const auto ports_idx = static_cast<int32_t> (&port - ports_.data ());
            if (
              ports_idx != midi_in_port_idx_ && ports_idx != midi_out_port_idx_
              && ports_idx != time_in_port_idx_)
              {
                port.has_atom_scratch = true;
                port.atom_scratch_byte_offset = atom_scratch_bytes;
                atom_scratch_bytes += atom_port_capacity (port);
              }
            break;
          }
        default:
          break;
        }
    }

  return true;
}

bool
Lv2Plugin::load_plugin (
  const std::filesystem::path &bundle_path,
  int64_t                      uri_hash,
  bool                         generate_new)
{
  assert (QThread::currentThread () == thread ());

  unload_current_plugin ();

  if (!pimpl_->resolve_plugin (bundle_path, uri_hash))
    {
      return false;
    }

  z_info ("LV2: loading plugin from bundle {}", bundle_path);

  if (!pimpl_->read_port_metadata ())
    {
      // A refused plugin must not leave partially read metadata behind:
      // reset to the unloaded state
      unload_current_plugin ();
      return false;
    }
  create_ports_and_parameters (generate_new);
  rebuild_preset_list ();
  pimpl_->resolve_ui ();

  return true;
}

void
Lv2Plugin::unload_current_plugin ()
{
  // free_instance() tears the UI down before the instance whose handle
  // it captured (instance access)
  pimpl_->free_instance ();
  if (pimpl_->ui_info_.has_value ())
    {
      pimpl_->ui_info_.reset ();
      Q_EMIT hasNativeUiChanged ();
    }
  // Session files are archived on every save, so they must not
  // outlive the plugin they belong to: a plugin loaded into this
  // object later starts with an empty session
  pimpl_->session_dir_.reset ();
  pimpl_->session_scratch_dir_.clear ();
  pimpl_->session_state_dir_.clear ();
  pimpl_->plugin_ = nullptr;
  pimpl_->ports_.clear ();
  pimpl_->control_in_bufs_.clear ();
  pimpl_->control_out_bufs_.clear ();
  pimpl_->ctrl_in_params_.clear ();
  pimpl_->param_to_ctrl_in_.clear ();
  pimpl_->patch_params_.clear ();
  pimpl_->param_to_patch_.clear ();
  pimpl_->pending_patch_set_count_ = 0;
  pimpl_->trigger_control_ins_.clear ();
  pimpl_->freewheel_control_ins_.clear ();
  pimpl_->latency_buf_index_ = -1;
  pimpl_->audio_in_ports_by_bus_.clear ();
  pimpl_->audio_out_ports_by_bus_.clear ();
  pimpl_->cv_in_ports_by_port_.clear ();
  pimpl_->cv_out_ports_by_port_.clear ();
  pimpl_->audio_scratch_buf_.clear ();
  pimpl_->cv_scratch_buf_.clear ();
  pimpl_->atom_scratch_buf_.clear ();
  pimpl_->unroutable_ports_warned_ = false;
  pimpl_->midi_in_port_idx_ = -1;
  pimpl_->midi_out_port_idx_ = -1;
  pimpl_->patch_in_port_idx_ = -1;
  clear_preset_list ();
  pimpl_->time_in_port_idx_ = -1;
  pimpl_->latency_.store (units::samples (0u), std::memory_order_relaxed);
}

void
Lv2Plugin::Lv2PluginImpl::free_instance ()
{
  if (instance_ == nullptr)
    return;

  // The UI holds the instance handle (instance access) and must die
  // before the instance is freed
  destroy_ui ();
  // The worker calls into the instance and must stop before it dies;
  // pending requests and responses are dropped with it
  if (worker_ != nullptr)
    {
      worker_->thread_.request_stop ();
      {
        std::lock_guard lock (worker_->sleep_mutex_);
        worker_->sleep_cv_.notify_all ();
      }
      worker_->thread_.join ();
      // Report drops that landed after the worker's last poll
      warn_dropped_records ();
      worker_.reset ();
    }
  if (instance_active_)
    {
      lilv_instance_deactivate (instance_);
      instance_active_ = false;
    }
  lilv_instance_free (instance_);
  instance_ = nullptr;
  latency_.store (units::samples (0u), std::memory_order_relaxed);
}

// ============================================================================
// Ports and parameters
// ============================================================================

std::vector<std::vector<size_t>>
Lv2Plugin::Lv2PluginImpl::audio_bus_groups (dsp::PortFlow flow) const
{
  std::vector<std::vector<size_t>>             buses;
  std::unordered_map<std::string_view, size_t> bus_index_by_group;
  for (const auto &[i, port] : utils::views::enumerate (ports_))
    {
      if (port.type != PortInfo::Type::Audio || port.flow != flow)
        continue;

      if (port.group_uri.empty ())
        {
          buses.emplace_back ().push_back (i);
          continue;
        }
      const auto [it, inserted] =
        bus_index_by_group.emplace (port.group_uri.view (), buses.size ());
      if (inserted)
        buses.emplace_back ();
      buses[it->second].push_back (i);
    }
  return buses;
}

std::vector<dsp::AudioBusConfig>
Lv2Plugin::Lv2PluginImpl::build_audio_bus_configs (dsp::PortFlow flow) const
{
  const auto  buses = audio_bus_groups (flow);
  const auto &main_group_uri =
    flow == dsp::PortFlow::Input ? main_in_group_uri_ : main_out_group_uri_;

  std::vector<dsp::AudioBusConfig> configs;
  configs.reserve (buses.size ());
  for (const auto &[i, bus] : utils::views::enumerate (buses))
    {
      const auto arrangement = [this, size = bus.size ()] {
        if (size == 1)
          return dsp::SpeakerArrangement::mono ();
        if (size == 2)
          return dsp::SpeakerArrangement::stereo ();
        // Speaker arrangements hold an 8-bit channel count
        if (size > std::numeric_limits<uint8_t>::max ())
          {
            z_warning (
              "LV2: '{}' has an audio group with {} channels; clamping "
              "the arrangement to 255",
              owner_.get_name (), size);
            return dsp::SpeakerArrangement::discrete_channels (
              std::numeric_limits<uint8_t>::max ());
          }
        return dsp::SpeakerArrangement::discrete_channels (
          static_cast<uint8_t> (size));
      }();
      const auto &first_port = ports_[bus.front ()];
      const auto  name =
        first_port.group_uri.empty ()
          ? first_port.name
          : uri_local_name_string (first_port.group_uri.view ());
      // A group explicitly designated pg:mainInput/pg:mainOutput is the
      // main bus; without a designation the first bus is main and the
      // rest sidechains
      const auto is_main =
        !main_group_uri.empty ()
          ? first_port.group_uri.view () == main_group_uri.view ()
          : i == 0;
      configs.push_back (
        dsp::AudioBusConfig{
          .name = name,
          .arrangement = arrangement,
          .purpose =
            is_main ? dsp::AudioPort::Purpose::Main
                    : dsp::AudioPort::Purpose::Sidechain,
          .active = true,
          .external_id = first_port.index });
    }
  return configs;
}

void
Lv2Plugin::create_ports_and_parameters (bool generate_new)
{
  assert (pimpl_->instance_ == nullptr);

  // On deserialization the MIDI and CV ports are restored from JSON before
  // the configuration change arrives, so they are only created for fresh
  // loads; the routing tables below are rebuilt either way
  if (generate_new)
    {
      if (pimpl_->midi_in_port_idx_ >= 0)
        {
          auto port_ref = utils::create_object<dsp::MidiPort> (
            registry (),
            utils::Utf8String::from_utf8_encoded_string ("MIDI Input 1"),
            dsp::PortFlow::Input);
          add_input_port (port_ref);
        }
      if (pimpl_->midi_out_port_idx_ >= 0)
        {
          auto port_ref = utils::create_object<dsp::MidiPort> (
            registry (),
            utils::Utf8String::from_utf8_encoded_string ("MIDI Output 1"),
            dsp::PortFlow::Output);
          add_output_port (port_ref);
        }
    }

  for (const auto flow : { dsp::PortFlow::Input, dsp::PortFlow::Output })
    {
      const auto result = dsp::reconcile_audio_bus_configuration (
        registry (), *this, flow, pimpl_->build_audio_bus_configs (flow));
      if (!generate_new)
        {
          // The ports were restored from a save: a changed topology means
          // the plugin's buses no longer match the saved ones
          if (result.graph_changed)
            {
              z_warning (
                "LV2: the bus topology of '{}' changed against its "
                "restored ports; the graph must be recalculated",
                get_name ());
            }
          else if (result.metadata_changed)
            {
              z_debug (
                "LV2: bus metadata of '{}' changed against its restored "
                "ports",
                get_name ());
            }
        }
    }

  if (generate_new)
    {
      for (const auto &port : pimpl_->ports_)
        {
          if (port.type != Lv2PluginImpl::PortInfo::Type::CV)
            continue;

          auto port_ref = utils::create_object<dsp::CVPort> (
            registry (), port.name, port.flow);
          if (port.flow == dsp::PortFlow::Input)
            add_input_port (port_ref);
          else
            add_output_port (port_ref);
        }
    }

  // Parameters from control input ports, matched by unique id (the port
  // symbol)
  const auto                                  &params = get_parameters ();
  std::unordered_map<std::string_view, size_t> param_index_by_id;
  for (const auto &[i, param_ref] : utils::views::enumerate (params))
    {
      param_index_by_id.emplace (
        type_safe::get (param_ref.get ()->get_unique_id ()).view (), i);
    }

  pimpl_->ctrl_in_params_.assign (pimpl_->control_in_bufs_.size (), {});
  pimpl_->param_to_ctrl_in_.assign (params.size (), -1);
  pimpl_->patch_params_.clear ();
  pimpl_->param_to_patch_.assign (params.size (), -1);
  pimpl_->trigger_control_ins_.clear ();
  pimpl_->freewheel_control_ins_.clear ();
  for (const auto &port : pimpl_->ports_)
    {
      if (port.is_trigger)
        {
          pimpl_->trigger_control_ins_.emplace_back (
            port.control_buffer_index, port.def);
        }
      if (port.is_freewheel)
        {
          pimpl_->freewheel_control_ins_.push_back (port.control_buffer_index);
        }
    }

  for (const auto &port : pimpl_->ports_)
    {
      if (
        port.type != Lv2PluginImpl::PortInfo::Type::Control
        || port.flow != dsp::PortFlow::Input)
        continue;

      const auto buf_index = port.control_buffer_index;
      const auto unique_id = dsp::ProcessorParameter::UniqueId (port.symbol);
      const auto unique_id_view = type_safe::get (unique_id).view ();

      dsp::ProcessorParameter * param = nullptr;
      // freeWheeling-designated ports are host-driven: they never
      // adopt or create a parameter
      const auto param_it = param_index_by_id.find (unique_id_view);
      if (!port.is_freewheel && param_it != param_index_by_id.end ())
        {
          param = params[param_it->second].get ();
        }
      else if (generate_new && !port.is_freewheel)
        {
          const auto range = [&] {
            if (port.is_toggled)
              return dsp::ParameterRange::make_toggle (port.def > 0.5f);
            if (port.is_enumeration && !port.scale_point_labels.empty ())
              {
                const auto default_it =
                  std::ranges::find (port.scale_point_values, port.def);
                auto default_index =
                  default_it != port.scale_point_values.end ()
                    ? static_cast<size_t> (
                        default_it - port.scale_point_values.begin ())
                    : 0uz;
                if (default_it == port.scale_point_values.end ())
                  {
                    // The declared default matches no scale point: snap to
                    // the nearest one so the parameter starts on a listed
                    // value
                    const auto nearest_it = std::ranges::min_element (
                      port.scale_point_values, {}, [def = port.def] (float v) {
                        return std::abs (v - def);
                      });
                    default_index = static_cast<size_t> (
                      nearest_it - port.scale_point_values.begin ());
                    z_warning (
                      "LV2: enumeration port {} of '{}' declares default {} "
                      "matching no scale point; snapping to {}",
                      port.symbol, get_name (), port.def,
                      port.scale_point_labels[default_index].view ());
                  }
                return dsp::ParameterRange::make_enumeration (
                  port.scale_point_labels, default_index);
              }
            if (port.is_enumeration)
              {
                // Enumeration semantics need scale points; without them the
                // parameter falls back to a linear range
                z_warning (
                  "LV2: enumeration port {} of '{}' declares no scale "
                  "points; treating it as linear",
                  port.symbol, get_name ());
              }
            const auto type =
              port.is_integer ? dsp::ParameterRange::Type::Integer
              : port.is_logarithmic
                ? dsp::ParameterRange::Type::Logarithmic
                : dsp::ParameterRange::Type::Linear;
            dsp::ParameterRange r (type, port.min, port.max, 0.f, port.def);
            r.unit_ = port.unit;
            return r;
          }();

          auto param_ref = utils::create_object<dsp::ProcessorParameter> (
            registry (), registry (), unique_id, range, port.name);
          add_parameter (param_ref);
          param = param_ref.get ();
          // The new parameter extends the parameter-to-control-buffer map
          pimpl_->param_to_ctrl_in_.push_back (-1);
          pimpl_->param_to_patch_.push_back (-1);
          param->set_automatable (true);
          if (!port.group_uri.empty ())
            {
              param->set_group_path (
                { uri_local_name_string (port.group_uri.view ()) });
            }
          param_index_by_id.emplace (
            type_safe::get (param->get_unique_id ()).view (),
            params.size () - 1);
        }

      auto &ctrl_param = pimpl_->ctrl_in_params_[buf_index];
      ctrl_param.port = &port;
      if (param != nullptr)
        {
          const auto index_it = param_index_by_id.find (
            type_safe::get (param->get_unique_id ()).view ());
          assert (index_it != param_index_by_id.end ());
          ctrl_param.param = param;
          pimpl_->param_to_ctrl_in_[index_it->second] =
            static_cast<int32_t> (buf_index);

          // Seed the control buffer with the current value: the parameter
          // default for fresh loads, the restored base value after
          // deserialization
          pimpl_->control_in_bufs_[buf_index] =
            pimpl_->control_value_for_param (ctrl_param, param->baseValue ());
        }
      else
        {
          // freeWheeling-designated ports always start at 0 (live
          // processing): the host drives their value
          pimpl_->control_in_bufs_[buf_index] =
            port.is_freewheel ? 0.f : port.def;
        }
    }

  pimpl_->discover_patch_parameters (param_index_by_id, generate_new);
}

void
Lv2Plugin::Lv2PluginImpl::discover_patch_parameters (
  const std::unordered_map<std::string_view, size_t> &param_index_by_id,
  bool                                                generate_new)
{
  auto * world = owner_.world_->raw ();

  const LilvNodeUPtr writable{ lilv_new_uri (world, LV2_PATCH__writable) };
  const LilvNodeUPtr range{ lilv_new_uri (world, LILV_NS_RDFS "range") };
  const LilvNodeUPtr default_p{ lilv_new_uri (world, LV2_CORE__default) };
  const LilvNodeUPtr minimum{ lilv_new_uri (world, LV2_CORE__minimum) };
  const LilvNodeUPtr maximum{ lilv_new_uri (world, LV2_CORE__maximum) };
  const LilvNodeUPtr label{ lilv_new_uri (world, LILV_NS_RDFS "label") };
  const LilvNodeUPtr unit{ lilv_new_uri (world, LV2_UNITS__unit) };

  // The parameter URI is the parameter's unique id, so lilv's value
  // nodes are URIs of lv2:Parameter subjects in the world's data
  const LilvNodesUPtr writables{
    lilv_plugin_get_value (plugin_, writable.get ())
  };
  if (writables == nullptr)
    return;

  const auto first_uri =
    [world] (const LilvNode * subject, const LilvNode * predicate) {
      utils::Utf8String   uri;
      const LilvNodesUPtr nodes{
        lilv_world_find_nodes (world, subject, predicate, nullptr)
      };
      if (nodes != nullptr)
        {
          const auto * node = lilv_nodes_get_first (nodes.get ());
          if (node != nullptr && lilv_node_is_uri (node))
            {
              uri = utils::Utf8String::from_utf8_encoded_string (
                lilv_node_as_uri (node));
            }
        }
      return uri;
    };
  const auto first_float =
    [world] (const LilvNode * subject, const LilvNode * predicate) {
      const LilvNodesUPtr nodes{
        lilv_world_find_nodes (world, subject, predicate, nullptr)
      };
      if (nodes != nullptr)
        {
          const auto * node = lilv_nodes_get_first (nodes.get ());
          if (
            node != nullptr
            && (lilv_node_is_float (node) || lilv_node_is_int (node)))
            return std::optional<float>{ lilv_node_as_float (node) };
          if (node != nullptr && lilv_node_is_bool (node))
            return std::optional<float>{ lilv_node_as_bool (node) ? 1.f : 0.f };
        }
      return std::optional<float>{ std::nullopt };
    };

  // rdfs:label values are string literals
  const auto first_string =
    [world] (const LilvNode * subject, const LilvNode * predicate) {
      utils::Utf8String   str;
      const LilvNodesUPtr nodes{
        lilv_world_find_nodes (world, subject, predicate, nullptr)
      };
      if (nodes != nullptr)
        {
          str = node_to_utf8 (lilv_nodes_get_first (nodes.get ()));
        }
      return str;
    };

  LILV_FOREACH (nodes, it, writables.get ())
    {
      const auto * param_node = lilv_nodes_get (writables.get (), it);
      if (param_node == nullptr || !lilv_node_is_uri (param_node))
        continue;
      const auto param_uri = utils::Utf8String::from_utf8_encoded_string (
        lilv_node_as_uri (param_node));

      // Only numeric parameters are host-controllable: files and other
      // patch subjects stay between the plugin and its state
      const auto range_uri = first_uri (param_node, range.get ());
      bool       is_toggle = false;
      dsp::ParameterRange::Type type = dsp::ParameterRange::Type::Linear;
      uint32_t                  value_type = host_urids_.atom_Float;
      if (range_uri.view () == LV2_ATOM__Bool)
        {
          is_toggle = true;
          value_type = host_urids_.atom_Bool;
        }
      else if (range_uri.view () == LV2_ATOM__Int)
        {
          type = dsp::ParameterRange::Type::Integer;
          value_type = host_urids_.atom_Int;
        }
      else if (range_uri.view () == LV2_ATOM__Double)
        {
          value_type = host_urids_.atom_Double;
        }
      else if (range_uri.view () != LV2_ATOM__Float)
        {
          z_debug (
            "LV2: patch parameter '{}' of '{}' has the non-numeric range "
            "'{}'; skipping it",
            param_uri.view (), owner_.get_name (), range_uri.view ());
          continue;
        }

      const auto def = first_float (param_node, default_p.get ());
      const auto min = first_float (param_node, minimum.get ());
      const auto max = first_float (param_node, maximum.get ());
      if (!def.has_value () || !min.has_value () || !max.has_value ())
        {
          z_warning (
            "LV2: patch parameter '{}' of '{}' does not declare a numeric "
            "default, minimum and maximum; skipping it",
            param_uri.view (), owner_.get_name ());
          continue;
        }
      // min >= max breaks the normalized-range conversion
      if (*min >= *max)
        {
          z_warning (
            "LV2: patch parameter '{}' of '{}' declares an unusable "
            "default/min/max; skipping it",
            param_uri.view (), owner_.get_name ());
          continue;
        }

      dsp::ProcessorParameter * param = nullptr;
      const auto found_it = param_index_by_id.find (param_uri.view ());
      if (found_it != param_index_by_id.end ())
        {
          param = owner_.get_parameters ()[found_it->second].get ();
          param_to_patch_[found_it->second] =
            static_cast<int32_t> (patch_params_.size ());
        }
      else if (generate_new)
        {
          auto name = first_string (param_node, label.get ());
          if (name.view ().empty ())
            name = uri_local_name_string (param_uri.view ());
          dsp::ParameterRange parameter_range =
            is_toggle
              ? dsp::ParameterRange::make_toggle (*def > 0.5f)
              : dsp::ParameterRange (type, *min, *max, 0.f, *def);
          const auto unit_uri = first_uri (param_node, unit.get ());
          if (!unit_uri.view ().empty ())
            {
              parameter_range.unit_ = unit_from_lv2_uri (unit_uri.view ());
            }
          auto param_ref = utils::create_object<dsp::ProcessorParameter> (
            owner_.registry (), owner_.registry (),
            dsp::ProcessorParameter::UniqueId (param_uri), parameter_range,
            name);
          owner_.add_parameter (param_ref);
          param = param_ref.get ();
          param_to_ctrl_in_.push_back (-1);
          param_to_patch_.push_back (
            static_cast<int32_t> (patch_params_.size ()));
          param->set_automatable (true);
        }
      else
        {
          z_warning (
            "LV2: no saved parameter matches the patch parameter '{}' of "
            "'{}'",
            param_uri.view (), owner_.get_name ());
          continue;
        }

      patch_params_.push_back (
        {
          param,
          owner_.world_->urid_map ().map (param_uri.c_str ()),
          value_type,
          param->baseValue (),
        });
    }
}

// ============================================================================
// Instantiation and processing preparation
// ============================================================================

bool
Lv2Plugin::Lv2PluginImpl::instantiate (
  units::sample_rate_t sample_rate,
  units::sample_u32_t  max_block_length)
{
  free_instance ();
  if (plugin_ == nullptr)
    return false;

  // Sequence buffer capacities from the ports' rsz:minimumSize
  const auto atom_in_capacity = atom_port_capacity (midi_in_port_idx_);
  const auto time_in_capacity = atom_port_capacity (time_in_port_idx_);
  const auto atom_out_capacity = atom_port_capacity (midi_out_port_idx_);

  auto * world_urid_map = &owner_.world_->urid_map ();
  urid_map_feature_ = LV2_URID_Map{
    .handle = world_urid_map,
    .map = urid_map_callback,
  };
  urid_unmap_feature_ = LV2_URID_Unmap{
    .handle = world_urid_map,
    .unmap = urid_unmap_callback,
  };

  host_urids_ = owner_.world_->host_urids ();
  const auto &urids = host_urids_;
  const auto  block_length =
    static_cast<int32_t> (max_block_length.in (units::samples));
  // The parameters extension models parameters as floats; the sample rate
  // option is announced as atom:Float like in reference hosts, while the
  // buffer-size options are integers. Blocks are split at loop points, so
  // the smallest chunk run() can be called with is a single frame, while
  // the nominal and maximum chunk is the full block
  sample_rate_opt_ = static_cast<float> (sample_rate.in (units::sample_rate));
  min_block_opt_ = 1;
  max_block_opt_ = block_length;
  nominal_block_opt_ = block_length;
  sequence_size_opt_ = static_cast<int32_t> (std::max (
    { kAtomBufferSize, atom_in_capacity, time_in_capacity, atom_out_capacity }));
  const auto init_option = [] (LV2_Options_Option &option) {
    option.context = LV2_OPTIONS_INSTANCE;
    option.subject = 0;
  };
  init_option (options_[0]);
  options_[0].key = urids.param_sampleRate;
  options_[0].size = sizeof (float);
  options_[0].type = urids.atom_Float;
  options_[0].value = &sample_rate_opt_;
  const auto int_option =
    [&] (uint32_t key, int32_t * value, LV2_Options_Option &option) {
      init_option (option);
      option.key = key;
      option.size = sizeof (int32_t);
      option.type = urids.atom_Int;
      option.value = value;
    };
  int_option (urids.bufsz_minBlockLength, &min_block_opt_, options_[1]);
  int_option (urids.bufsz_maxBlockLength, &max_block_opt_, options_[2]);
  int_option (urids.bufsz_nominalBlockLength, &nominal_block_opt_, options_[3]);
  int_option (urids.bufsz_sequenceSize, &sequence_size_opt_, options_[4]);
  options_[5] = LV2_Options_Option{};

  features_.clear ();
  feature_ptrs_.clear ();
  try
    {
      ensure_session_dirs ();
    }
  catch (const ZrythmException &e)
    {
      z_warning (
        "LV2: failed to create the session directories of '{}': {}",
        owner_.get_name (), e.what ());
      return false;
    }
  const auto push_feature = [this] (const char * uri, void * data) {
    features_.push_back (LV2_Feature{ uri, data });
  };
  push_feature (LV2_URID__map, &urid_map_feature_);
  push_feature (LV2_URID__unmap, &urid_unmap_feature_);
  push_feature (LV2_OPTIONS__options, options_.data ());
  push_feature (LV2_BUF_SIZE__boundedBlockLength, nullptr);
  push_feature (LV2_CORE__hardRTCapable, nullptr);
  push_feature (LV2_STATE__makePath, &make_path_feature_);
  push_feature (LV2_STATE__freePath, &free_path_feature_);
  push_feature (LV2_STATE__loadDefaultState, nullptr);
  schedule_feature_ = LV2_Worker_Schedule{
    .handle = this, .schedule_work = &schedule_work_callback
  };
  push_feature (LV2_WORKER__schedule, &schedule_feature_);
  restore_worker_schedule_feature_ = LV2_Worker_Schedule{
    .handle = this, .schedule_work = &restore_schedule_work_callback
  };
  restore_worker_schedule_feature_wrapper_ =
    LV2_Feature{ LV2_WORKER__schedule, &restore_worker_schedule_feature_ };
  for (const auto &feature : features_)
    {
      feature_ptrs_.push_back (&feature);
    }
  feature_ptrs_.push_back (nullptr);

  instance_ = lilv_plugin_instantiate (
    plugin_, sample_rate.in (units::sample_rate), feature_ptrs_.data ());
  if (instance_ == nullptr)
    {
      const auto is_feature_supported = [] (const char * uri) {
        return std::string_view{ uri } == LV2_URID__map
               || std::string_view{ uri } == LV2_URID__unmap
               || std::string_view{ uri } == LV2_OPTIONS__options
               || std::string_view{ uri } == LV2_BUF_SIZE__boundedBlockLength
               || std::string_view{ uri } == LV2_CORE__hardRTCapable
               || std::string_view{ uri } == LV2_STATE__makePath
               || std::string_view{ uri } == LV2_STATE__freePath
               || std::string_view{ uri } == LV2_STATE__loadDefaultState
               || std::string_view{ uri } == LV2_WORKER__schedule;
      };
      std::string         unsupported_features;
      const LilvNodesUPtr required_features{
        lilv_plugin_get_required_features (plugin_)
      };
      if (required_features != nullptr)
        {
          LILV_FOREACH (nodes, iter, required_features.get ())
            {
              const char * uri = lilv_node_as_uri (
                lilv_nodes_get (required_features.get (), iter));
              if (uri != nullptr && !is_feature_supported (uri))
                {
                  unsupported_features += fmt::format (" {}", uri);
                }
            }
        }
      if (!unsupported_features.empty ())
        {
          z_warning (
            "LV2: failed to instantiate '{}': it requires features this "
            "host does not provide:{}",
            owner_.get_name (), unsupported_features);
        }
      else
        {
          z_warning ("LV2: failed to instantiate '{}'", owner_.get_name ());
        }
      return false;
    }

  atom_in_buf_.assign (atom_in_capacity, 0);
  time_in_buf_.assign (time_in_capacity, 0);
  atom_out_buf_.assign (atom_out_capacity, 0);

  pending_ui_atoms_.clear ();

  // The forge caches the URIDs it stamps into atoms at init time
  lv2_atom_forge_init (&forge_, &urid_map_feature_);

  resize_scratch_buffers (max_block_length);
  const auto block_stride =
    static_cast<std::ptrdiff_t> (scratch_block_stride_.in (units::samples));

  // Every port is connected before activation: routed atom ports to
  // their dedicated buffers, unroutable atom ports (and unknown-type
  // ports) to private scratch buffers holding an empty sequence, and
  // audio/CV ports to their zeroed scratch slots (routed engine buffers
  // replace them per chunk)
  size_t atom_scratch_bytes = 0;
  for (const auto &port : ports_)
    {
      if (port.has_atom_scratch)
        {
          atom_scratch_bytes += atom_port_capacity (port);
        }
    }
  atom_scratch_buf_.assign (atom_scratch_bytes, 0);
  for (auto &port : ports_)
    {
      switch (port.type)
        {
        case PortInfo::Type::Audio:
          lilv_instance_connect_port (
            instance_, port.index,
            audio_scratch_buf_.data ()
              + static_cast<std::ptrdiff_t> (port.audio_scratch_slot)
                  * block_stride);
          break;
        case PortInfo::Type::CV:
          lilv_instance_connect_port (
            instance_, port.index,
            cv_scratch_buf_.data ()
              + static_cast<std::ptrdiff_t> (port.cv_scratch_slot) * block_stride);
          break;
        default:
          if (!port.has_atom_scratch)
            break;
          auto * scratch =
            atom_scratch_buf_.data () + port.atom_scratch_byte_offset;
          lv2_atom_forge_set_buffer (
            &forge_, scratch, atom_port_capacity (port));
          LV2_Atom_Forge_Frame seq_frame;
          lv2_atom_forge_sequence_head (&forge_, &seq_frame, 0);
          lv2_atom_forge_pop (&forge_, &seq_frame);
          connect_port_rt (port.index, scratch);
          break;
        }
    }

  connect_control_ports ();
  if (midi_out_port_idx_ >= 0)
    {
      lilv_instance_connect_port (
        instance_, ports_[midi_out_port_idx_].index, atom_out_buf_.data ());
    }

  // The worker runs from instantiation (restore and activate may
  // schedule work) until the instance is freed
  const auto * instance_descriptor = lilv_instance_get_descriptor (instance_);
  if (instance_descriptor->extension_data != nullptr)
    {
      const auto * worker_interface = static_cast<const LV2_Worker_Interface *> (
        instance_descriptor->extension_data (LV2_WORKER__interface));
      if (worker_interface != nullptr)
        {
          worker_ = std::make_unique<WorkerState> ();
          worker_->interface_ = worker_interface;
          worker_->thread_ = std::jthread ([this] (std::stop_token stop_token) {
            worker_thread_func (stop_token);
          });
        }
    }

  restore_default_state_if_declared ();

  lilv_instance_activate (instance_);
  instance_active_ = true;

  return true;
}

void
Lv2Plugin::Lv2PluginImpl::resize_scratch_buffers (
  units::sample_u32_t max_block_length)
{
  const auto block_length = max_block_length.in<size_t> (units::samples);
  const auto count_ports = [this] (PortInfo::Type type) {
    return static_cast<size_t> (std::ranges::count_if (
      ports_, [type] (const auto &p) { return p.type == type; }));
  };
  audio_scratch_buf_.assign (
    count_ports (PortInfo::Type::Audio) * block_length, 0.f);
  cv_scratch_buf_.assign (count_ports (PortInfo::Type::CV) * block_length, 0.f);
  scratch_block_stride_ = max_block_length;
}

void
Lv2Plugin::Lv2PluginImpl::connect_control_ports ()
{
  for (const auto &port : ports_)
    {
      if (port.type != PortInfo::Type::Control)
        continue;
      auto &buffers =
        port.flow == dsp::PortFlow::Input ? control_in_bufs_ : control_out_bufs_;
      z_return_if_fail (port.control_buffer_index < buffers.size ());
      auto &buffer = buffers[port.control_buffer_index];
      lilv_instance_connect_port (instance_, port.index, &buffer);
    }
}
void
Lv2Plugin::apply_pending_state ()
{
  if (state_to_apply_.has_value () && pimpl_->instance_ != nullptr)
    {
      if (pimpl_->restore_state_from_blob (state_to_apply_->toStdString ()))
        {
          state_to_apply_.reset ();
        }
      else
        {
          // Keep the pending state: it is retried on the next processing
          // preparation
          z_warning (
            "LV2: failed to apply the state of '{}'; it will be retried "
            "on the next processing preparation",
            get_name ());
        }
    }
}

void
Lv2Plugin::prepare_plugin_for_processing (
  units::sample_rate_t sample_rate,
  units::sample_u32_t  max_block_length)
{
  assert (QThread::currentThread () == thread ());

  if (pimpl_->plugin_ == nullptr)
    return;

  if (pimpl_->degradation_reported_.exchange (false, std::memory_order_relaxed))
    {
      z_warning (
        "LV2: '{}' received blocks larger than the maximum block length "
        "its instance was created for; they were dropped",
        get_name ());
    }

  if (
    const auto dropped_events =
      pimpl_->atom_events_dropped_.exchange (0, std::memory_order_relaxed);
    dropped_events > 0)
    {
      z_warning (
        "LV2: '{}' dropped {} input MIDI events that did not fit the "
        "sequence buffer",
        get_name (), dropped_events);
    }

  if (
    const auto dropped_output_events =
      pimpl_->midi_output_events_dropped_.exchange (0, std::memory_order_relaxed);
    dropped_output_events > 0)
    {
      z_warning (
        "LV2: '{}' dropped {} plugin output MIDI events because the "
        "engine MIDI buffer was full",
        get_name (), dropped_output_events);
    }

  if (
    const auto dropped_ui_events =
      pimpl_->ui_to_plugin_dropped_.exchange (0, std::memory_order_relaxed);
    dropped_ui_events > 0)
    {
      z_warning (
        "LV2: '{}' dropped {} UI events (unsupported target port or full "
        "relay ring)",
        get_name (), dropped_ui_events);
    }

  if (
    const auto dropped_ui_output_events =
      pimpl_->plugin_to_ui_dropped_.exchange (0, std::memory_order_relaxed);
    dropped_ui_output_events > 0)
    {
      z_warning (
        "LV2: '{}' dropped {} plugin to UI events because the relay ring "
        "was full",
        get_name (), dropped_ui_output_events);
    }

  if (
    const auto dropped_patch_sets =
      pimpl_->patch_sets_dropped_.exchange (0, std::memory_order_relaxed);
    dropped_patch_sets > 0)
    {
      z_warning (
        "LV2: '{}' dropped {} patch parameter sets (queue or sequence "
        "full, or no carrier port)",
        get_name (), dropped_patch_sets);
    }

  // LV2 binds the sample rate and the buffer-size options at
  // instantiation: a change requires a new instance, carrying the current
  // state over. The scratch buffers are resized inside instantiate(),
  // together with the stride recorded for them.
  const bool instance_stale =
    pimpl_->instance_ != nullptr
    && (pimpl_->last_sample_rate_ != sample_rate || pimpl_->last_max_block_length_ != max_block_length);

  std::optional<std::string> carried_state;
  const auto prepare = [this, sample_rate, max_block_length, &carried_state] () {
    if (pimpl_->instance_ != nullptr)
      {
        carried_state = pimpl_->save_state_to_blob ();
        pimpl_->free_instance ();
        z_debug (
          "LV2: re-instantiating '{}' at {} Hz with a maximum block "
          "length of {}",
          get_name (), sample_rate.in (units::sample_rate),
          max_block_length.in (units::samples));
      }

    if (pimpl_->instance_ == nullptr)
      {
        if (!pimpl_->instantiate (sample_rate, max_block_length))
          {
            pimpl_->last_sample_rate_ = sample_rate;
            pimpl_->last_max_block_length_ = max_block_length;
            // Keep the state of the freed instance as pending so it is
            // not lost; it is applied when instantiation succeeds
            if (carried_state.has_value ())
              {
                state_to_apply_ = QByteArray::fromStdString (*carried_state);
              }
            // The plugin was resolved and reported loaded earlier:
            // surface the late instantiation failure to listeners
            Q_EMIT instantiationFinished (false, tr ("Instantiation failed"));
            return;
          }

        const auto state_to_restore = [&] () -> std::optional<std::string> {
          if (carried_state.has_value ())
            return carried_state;
          if (state_to_apply_.has_value ())
            return state_to_apply_->toStdString ();
          return std::nullopt;
        }();
        if (state_to_restore.has_value ())
          {
            if (pimpl_->restore_state_from_blob (*state_to_restore))
              {
                state_to_apply_.reset ();
                carried_state.reset ();
              }
            else
              {
                // Keep the state as pending: it is retried on the next
                // processing preparation
                state_to_apply_ = QByteArray::fromStdString (*state_to_restore);
                z_warning (
                  "LV2: failed to restore the state of '{}'; it will be "
                  "retried on the next processing preparation",
                  get_name ());
              }
          }
      }

    pimpl_->pair_ports_with_engine_ports ();
    pimpl_->last_sample_rate_ = sample_rate;
    pimpl_->last_max_block_length_ = max_block_length;

    // A UI that died with the freed instance (instance access) is
    // re-opened for the new one
    if (uiVisible ())
      {
        show_editor ();
      }
  };

  // Freeing and re-creating a live instance must not overlap audio
  // processing of the same instance
  if (instance_stale)
    {
      if (main_thread_callbacks_.with_paused_processing_)
        {
          main_thread_callbacks_.with_paused_processing_ (prepare);
        }
      else
        {
          // Keep the current instance; the next processing preparation
          // retries the re-instantiation
          z_warning (
            "LV2: cannot re-instantiate '{}' while processing; the host "
            "cannot pause processing",
            get_name ());
        }
      return;
    }

  if (pimpl_->instance_ == nullptr)
    {
      prepare ();
      return;
    }

  // An instance released by a graph rechain at unchanged settings is
  // only reactivated
  if (!pimpl_->instance_active_)
    {
      const auto reactivate = [this] () {
        z_debug ("LV2: reactivating '{}'", get_name ());
        lilv_instance_activate (pimpl_->instance_);
        pimpl_->instance_active_ = true;
      };
      if (main_thread_callbacks_.with_paused_processing_)
        {
          main_thread_callbacks_.with_paused_processing_ (reactivate);
        }
      else
        {
          z_warning (
            "LV2: cannot reactivate '{}' while processing; the host "
            "cannot pause processing",
            get_name ());
        }
    }

  // Live instance at an unchanged rate and block length: retry any
  // pending state. Restoring writes the control buffers of a running
  // instance, so it is paused
  if (state_to_apply_.has_value ())
    {
      if (main_thread_callbacks_.with_paused_processing_)
        {
          main_thread_callbacks_.with_paused_processing_ ([this] () {
            apply_pending_state ();
          });
        }
      else
        {
          z_warning (
            "LV2: cannot retry applying the state of '{}' while "
            "processing; the host cannot pause processing",
            get_name ());
        }
    }

  pimpl_->pair_ports_with_engine_ports ();
  pimpl_->last_sample_rate_ = sample_rate;
  pimpl_->last_max_block_length_ = max_block_length;
}

void
Lv2Plugin::release_resources_impl ()
{
  // Like the other backends, releasing resources deactivates the
  // instance instead of freeing it: hard graph rechains are frequent,
  // and the instance — and any open UI, which holds the instance handle
  // through instance access — must survive them. Control values live in
  // host-owned buffers and persist; per the LV2 activate() contract the
  // plugin resets run-history-dependent internal state (e.g. delay
  // tails) on reactivation.
  if (pimpl_->instance_ == nullptr || !pimpl_->instance_active_)
    return;

  pimpl_->instance_active_ = false;
  lilv_instance_deactivate (pimpl_->instance_);
}

void
Lv2Plugin::Lv2PluginImpl::pair_ports_with_engine_ports ()
{
  // Audio buses are matched with engine ports by external id (the bus's
  // first port index); each port of ports_ carries its bus's id. Buses
  // with no paired engine port are routed to scratch buffers
  const auto pair_audio = [this] (dsp::PortFlow flow, auto &by_bus) {
    by_bus.assign (audio_bus_groups (flow).size (), nullptr);
    const auto attached =
      owner_.get_attached_audio_ports (flow) | std::ranges::to<std::vector> ();
    for (const auto &port_info : ports_)
      {
        if (port_info.type != PortInfo::Type::Audio || port_info.flow != flow)
          continue;
        const auto it = std::ranges::find_if (
          attached, [&port_info] (const dsp::AudioPort * engine_port) {
            return engine_port->external_port_id ()
                   == port_info.audio_bus_external_id;
          });
        by_bus[port_info.audio_bus_index] =
          it != attached.end () ? *it : nullptr;
      }
    if (std::ranges::contains (by_bus, nullptr) && !unroutable_ports_warned_)
      {
        unroutable_ports_warned_ = true;
        z_warning (
          "LV2: '{}' has audio buses with no paired engine port; they are "
          "routed to silent scratch buffers",
          owner_.get_name ());
      }
  };
  pair_audio (dsp::PortFlow::Input, audio_in_ports_by_bus_);
  pair_audio (dsp::PortFlow::Output, audio_out_ports_by_bus_);

  // CV ports have no stable id: they pair positionally with the attached CV
  // ports in creation order
  const auto pair_cv = [this] (dsp::PortFlow flow, auto &by_port) {
    const auto cv_ports = [this, flow] {
      const auto attached =
        flow == dsp::PortFlow::Input
          ? owner_.get_attached_input_ports () | std::ranges::to<std::vector> ()
          : owner_.get_attached_output_ports () | std::ranges::to<std::vector> ();
      return attached | std::views::transform (&dsp::PortUuidReference::get)
             | utils::views::qobject_cast_and_filter<dsp::CVPort>
             | std::ranges::to<std::vector> ();
    }();
    by_port.clear ();
    size_t pos = 0;
    for (const auto &port_info : ports_)
      {
        if (port_info.type != PortInfo::Type::CV || port_info.flow != flow)
          continue;
        by_port.push_back (pos < cv_ports.size () ? cv_ports[pos] : nullptr);
        ++pos;
      }
    if (std::ranges::contains (by_port, nullptr) && !unroutable_ports_warned_)
      {
        unroutable_ports_warned_ = true;
        z_warning (
          "LV2: '{}' has CV ports with no paired engine port; they are "
          "routed to scratch buffers",
          owner_.get_name ());
      }
  };
  pair_cv (dsp::PortFlow::Input, cv_in_ports_by_port_);
  pair_cv (dsp::PortFlow::Output, cv_out_ports_by_port_);
}

// ============================================================================
// Processing
// ============================================================================

void
Lv2Plugin::Lv2PluginImpl::connect_sample_ports (
  units::sample_u32_t local_offset) noexcept
{
  const auto offset = local_offset.in<std::ptrdiff_t> (units::samples);
  const auto block_stride =
    static_cast<std::ptrdiff_t> (scratch_block_stride_.in (units::samples));

  for (const auto &port : ports_)
    {
      switch (port.type)
        {
        case PortInfo::Type::Audio:
          {
            const auto &by_port =
              port.flow == dsp::PortFlow::Input
                ? audio_in_ports_by_bus_
                : audio_out_ports_by_bus_;
            auto * engine_port =
              port.audio_bus_index < by_port.size ()
                ? by_port[port.audio_bus_index]
                : nullptr;
            float * data = nullptr;
            if (
              engine_port != nullptr && engine_port->buffers () != nullptr
              && port.audio_channel < static_cast<size_t> (
                   engine_port->buffers ()->getNumChannels ()))
              {
                data = engine_port->buffers ()->getWritePointer (
                  static_cast<int> (port.audio_channel));
              }
            else
              {
                // Ports without a routed engine buffer are connected to
                // their zeroed scratch slot
                data =
                  audio_scratch_buf_.data ()
                  + static_cast<std::ptrdiff_t> (port.audio_scratch_slot)
                      * block_stride;
              }
            connect_port_rt (port.index, data + offset);
            break;
          }
        case PortInfo::Type::CV:
          {
            const auto &by_port =
              port.flow == dsp::PortFlow::Input
                ? cv_in_ports_by_port_
                : cv_out_ports_by_port_;
            auto * engine_port =
              port.cv_index_in_flow < by_port.size ()
                ? by_port[port.cv_index_in_flow]
                : nullptr;
            float * data =
              engine_port != nullptr && !engine_port->buf_.empty ()
                ? engine_port->buf_.data ()
                : cv_scratch_buf_.data ()
                    + static_cast<std::ptrdiff_t> (port.cv_scratch_slot)
                        * block_stride;
            connect_port_rt (port.index, data + offset);
            break;
          }
        default:
          break;
        }
    }
}

void
Lv2Plugin::process_impl (
  dsp::graph::ProcessBlockInfo time_info,
  const dsp::ITransport       &transport,
  const dsp::TempoMap         &tempo_map) noexcept
{
  if (pimpl_->instance_ == nullptr || !pimpl_->instance_active_)
    return;

  const auto local_offset = time_info.buffer_offset_;
  const auto nframes = time_info.nframes_;

  // The instance and its scratch buffers were created for the recorded
  // maximum block length: running it with a longer block writes past
  // those buffers, so the block is dropped
  if (local_offset + nframes > pimpl_->scratch_block_stride_)
    {
      pimpl_->degrade_to_silence (local_offset, nframes);
      return;
    }

  pimpl_->apply_changed_param_values ();
  pimpl_->connect_sample_ports (local_offset);
  pimpl_->forge_atom_inputs (time_info, transport, tempo_map);
  pimpl_->reset_atom_output ();

  {
#if defined(__has_feature) && __has_feature(realtime_sanitizer)
    // Plugin code is not ours; RTSan violations inside it are not actionable
    __rtsan::ScopedDisabler d;
#endif
    lilv_instance_run (pimpl_->instance_, nframes.in<uint32_t> (units::samples));
  }

  // Responses are delivered after run() and before end_run(), like
  // the reference hosts; atoms written during work_response() are
  // then picked up by this cycle's output parsing below
  pimpl_->deliver_worker_responses ();
  pimpl_->finish_worker_cycle ();
  pimpl_->reset_trigger_ports ();
  pimpl_->parse_atom_outputs (local_offset, nframes);
  pimpl_->parse_patch_messages ();
  pimpl_->read_control_outputs ();
  pimpl_->dispatch_atom_outputs_to_ui ();
}

void
Lv2Plugin::Lv2PluginImpl::forge_atom_inputs (
  dsp::graph::ProcessBlockInfo time_info,
  const dsp::ITransport       &transport,
  const dsp::TempoMap         &tempo_map) noexcept
{
  const auto &urids = host_urids_;
  const auto  local_offset = time_info.buffer_offset_;
  const auto  nframes = time_info.nframes_;

  // The drained events exist only while a session is dispatched; the
  // flag is read once here (acquire, pairing with the session open and
  // teardown stores) and gates every use below
  const bool ui_session_live = drain_ui_atom_events ();

  // Atom events from the UI land at the end of the chunk: they carry
  // no frame of their own, and the last chunk frame keeps them inside
  // the sequence's time range
  const auto ui_atom_chunk_time = [&] () -> int64_t {
    const auto frames = nframes.in<uint32_t> (units::samples);
    return frames > 0 ? frames - 1 : 0;
  }();
  // Appends the pending UI atoms targeting @p port_index to the
  // sequence currently open in the forge, dropping events that do not
  // fit whole
  const auto append_ui_atoms = [&] (uint32_t port_index) {
    uint32_t dropped_events = 0;
    for (const auto &atom : pending_ui_atoms_)
      {
        if (atom.port_index != port_index)
          continue;
        const auto * event_atom =
          reinterpret_cast<const LV2_Atom *> (atom.bytes.data ());
        const auto needed =
          sizeof (LV2_Atom_Event) + lv2_atom_pad_size (event_atom->size);
        if (forge_.offset + needed > forge_.size)
          {
            ++dropped_events;
            continue;
          }
        lv2_atom_forge_frame_time (&forge_, ui_atom_chunk_time);
        lv2_atom_forge_atom (&forge_, event_atom->size, event_atom->type);
        lv2_atom_forge_write (&forge_, event_atom + 1, event_atom->size);
      }
    if (dropped_events > 0)
      {
        atom_events_dropped_.fetch_add (
          dropped_events, std::memory_order_relaxed);
      }
  };

  // Forges the position object as a frame-timed event of the sequence
  // currently open in the forge, at frame 0 so it sorts before the
  // chunk's MIDI events. Its size is fixed and far below the buffer
  // capacity, so its forge writes cannot fail
  const auto forge_position_event = [&] {
    const auto transport_context = build_plugin_transport_context (
      transport, tempo_map, units::sample_t{ time_info.transport_position_ });
    lv2_atom_forge_frame_time (&forge_, 0);
    LV2_Atom_Forge_Frame pos_frame;
    lv2_atom_forge_object (&forge_, &pos_frame, 0, urids.time_Position);
    lv2_atom_forge_key (&forge_, urids.time_speed);
    lv2_atom_forge_float (&forge_, transport_context.playing_ ? 1.f : 0.f);
    lv2_atom_forge_key (&forge_, urids.time_frame);
    lv2_atom_forge_long (
      &forge_,
      static_cast<int64_t> (time_info.transport_position_.in (units::samples)));
    lv2_atom_forge_key (&forge_, urids.time_framesPerSecond);
    lv2_atom_forge_float (
      &forge_, static_cast<float> (last_sample_rate_.in (units::sample_rate)));
    lv2_atom_forge_key (&forge_, urids.time_bar);
    lv2_atom_forge_long (&forge_, transport_context.bar_number_);
    lv2_atom_forge_key (&forge_, urids.time_barBeat);
    // Typed as Float, which is what shipped hosts forge, though the time
    // spec types the property xsd:double
    const auto quarters_into_bar =
      (transport_context.position_ - transport_context.bar_start_)
        .in (units::quarter_notes);
    lv2_atom_forge_float (
      &forge_,
      static_cast<float> (
        quarters_into_bar * transport_context.time_sig_denominator_
        / kQuartersPerWholeNote));
    lv2_atom_forge_key (&forge_, urids.time_beatsPerBar);
    lv2_atom_forge_float (
      &forge_, static_cast<float> (transport_context.time_sig_numerator_));
    lv2_atom_forge_key (&forge_, urids.time_beatUnit);
    lv2_atom_forge_int (&forge_, transport_context.time_sig_denominator_);
    lv2_atom_forge_key (&forge_, urids.time_beatsPerMinute);
    lv2_atom_forge_float (
      &forge_, static_cast<float> (transport_context.tempo_.in (units::bpm)));
    lv2_atom_forge_pop (&forge_, &pos_frame);
  };

  // The sequence the patch sets are forged into: the carrier resolved
  // from the port metadata (a dedicated buffer block below or a
  // carrier port in the scratch loop)
  bool patch_sets_forged = false;

  if (time_in_port_idx_ >= 0 && time_in_port_idx_ != midi_in_port_idx_)
    {
      // A time:Position port of its own receives its own sequence
      lv2_atom_forge_set_buffer (
        &forge_, time_in_buf_.data (),
        static_cast<uint32_t> (time_in_buf_.size ()));
      LV2_Atom_Forge_Frame seq_frame;
      lv2_atom_forge_sequence_head (&forge_, &seq_frame, 0);
      forge_position_event ();
      if (patch_in_port_idx_ == time_in_port_idx_)
        {
          forge_pending_patch_sets ();
          patch_sets_forged = true;
        }
      lv2_atom_forge_pop (&forge_, &seq_frame);
      connect_port_rt (ports_[time_in_port_idx_].index, time_in_buf_.data ());
    }

  if (midi_in_port_idx_ >= 0)
    {
      // The buffer is forged and connected even with no routed MIDI port:
      // the plugin reads the connected sequence every run(). A port
      // supporting both MIDI and time:Position receives the position
      // object and the MIDI events in the same sequence
      lv2_atom_forge_set_buffer (
        &forge_, atom_in_buf_.data (),
        static_cast<uint32_t> (atom_in_buf_.size ()));
      LV2_Atom_Forge_Frame seq_frame;
      lv2_atom_forge_sequence_head (&forge_, &seq_frame, 0);
      if (time_in_port_idx_ == midi_in_port_idx_)
        forge_position_event ();
      if (patch_in_port_idx_ == midi_in_port_idx_)
        {
          forge_pending_patch_sets ();
          patch_sets_forged = true;
        }
      const auto in_chunk = [local_offset, nframes] (const auto &ev) {
        return ev.time () >= local_offset && ev.time () < local_offset + nframes;
      };
      // Events that do not fit the buffer are skipped whole: a partially
      // forged event would claim body bytes that were never written, and
      // plugins would read past the buffer parsing them
      uint32_t dropped_events = 0;
      if (!owner_.midi_in_ports_.empty ())
        {
          for (
            const auto &ev :
            owner_.midi_in_ports_.front ()->buffer_
              | std::views::filter (in_chunk))
            {
              const auto chunk_time =
                (ev.time () - local_offset).in<uint32_t> (units::samples);
              const auto ev_data = ev.data ();
              const auto needed =
                sizeof (LV2_Atom_Event)
                + lv2_atom_pad_size (static_cast<uint32_t> (ev_data.size ()));
              if (forge_.offset + needed > forge_.size)
                {
                  ++dropped_events;
                  continue;
                }
              lv2_atom_forge_frame_time (&forge_, chunk_time);
              lv2_atom_forge_atom (
                &forge_, ev_data.size (), urids.midi_MidiEvent);
              lv2_atom_forge_write (&forge_, ev_data.data (), ev_data.size ());
            }
        }
      if (dropped_events > 0)
        {
          atom_events_dropped_.fetch_add (
            dropped_events, std::memory_order_relaxed);
        }
      if (ui_session_live)
        {
          append_ui_atoms (ports_[midi_in_port_idx_].index);
        }
      lv2_atom_forge_pop (&forge_, &seq_frame);
      connect_port_rt (ports_[midi_in_port_idx_].index, atom_in_buf_.data ());
    }

  // Scratch atom inputs (atom inputs this host cannot route and
  // unknown-type ports) receive the UI's atoms in their own sequence.
  // The empty sequence is re-forged every chunk: connect_port is
  // sticky, so a sequence left over from an earlier chunk would be
  // delivered again on every later run()
  for (const auto &port : ports_)
    {
      if (
        !port.has_atom_scratch || port.flow == dsp::PortFlow::Output
        || (port.type != PortInfo::Type::Atom && port.type != PortInfo::Type::Unknown))
        continue;
      auto * scratch = atom_scratch_buf_.data () + port.atom_scratch_byte_offset;
      lv2_atom_forge_set_buffer (&forge_, scratch, atom_port_capacity (port));
      LV2_Atom_Forge_Frame seq_frame;
      lv2_atom_forge_sequence_head (&forge_, &seq_frame, 0);
      const auto ports_idx = static_cast<int32_t> (&port - ports_.data ());
      if (
        !patch_sets_forged
        && (patch_in_port_idx_ < 0 || ports_idx == patch_in_port_idx_))
        {
          forge_pending_patch_sets ();
          patch_sets_forged = true;
        }
      if (ui_session_live)
        {
          append_ui_atoms (port.index);
        }
      lv2_atom_forge_pop (&forge_, &seq_frame);
      connect_port_rt (port.index, scratch);
    }
  if (!patch_sets_forged && pending_patch_set_count_ > 0)
    {
      // No atom input can carry the messages: the sets can never be
      // delivered
      patch_sets_dropped_.fetch_add (
        static_cast<uint32_t> (pending_patch_set_count_),
        std::memory_order_relaxed);
      pending_patch_set_count_ = 0;
    }
}

void
Lv2Plugin::Lv2PluginImpl::reset_atom_output () noexcept
{
  // LV2 contract for output atom ports (including legacy event ports
  // routed to scratch): the host announces the buffer capacity by
  // writing an atom:Chunk header before each run, and the plugin
  // replaces it with the sequence it produces
  for (const auto &port : ports_)
    {
      if (
        (port.type != PortInfo::Type::Atom
         && port.type != PortInfo::Type::Unknown)
        || port.flow != dsp::PortFlow::Output)
        continue;
      const auto ports_idx = static_cast<int32_t> (&port - ports_.data ());
      const auto routed = ports_idx == midi_out_port_idx_;
      auto *     buf =
        routed
          ? atom_out_buf_.data ()
          : atom_scratch_buf_.data () + port.atom_scratch_byte_offset;
      auto * atom = reinterpret_cast<LV2_Atom *> (buf);
      atom->type = host_urids_.atom_Chunk;
      atom->size =
        routed
          ? static_cast<uint32_t> (atom_out_buf_.size ())
          : atom_port_capacity (port);
    }
}

void
Lv2Plugin::Lv2PluginImpl::parse_atom_outputs (
  units::sample_u32_t local_offset,
  units::sample_u32_t nframes) noexcept
{
  if (midi_out_port_idx_ < 0 || owner_.midi_out_ports_.empty ())
    return;

  const auto &urids = host_urids_;

  auto *   midi_out_port = owner_.midi_out_ports_.front ();
  uint32_t dropped_events = 0;
  for_each_atom_output_event (
    atom_out_buf_.data (), atom_out_buf_.size (),
    [&] (const LV2_Atom_Event * ev) {
      if (ev->body.type != urids.midi_MidiEvent)
        return;
      // Events must land inside this chunk: negative times and times past
      // the chunk end carry stale timestamps
      if (
        ev->time.frames < 0
        || ev->time.frames
             >= static_cast<int64_t> (nframes.in<uint32_t> (units::samples)))
        return;
      const auto time =
        units::samples (static_cast<uint32_t> (ev->time.frames)) + local_offset;
      const auto * data =
        reinterpret_cast<const midi_byte_t *> (LV2_ATOM_BODY_CONST (&ev->body));
      if (!midi_out_port->buffer_.push_back (
            time, std::span<const midi_byte_t> (data, ev->body.size)))
        {
          ++dropped_events;
        }
    });
  if (dropped_events > 0)
    {
      midi_output_events_dropped_.fetch_add (
        dropped_events, std::memory_order_relaxed);
    }
}

void
Lv2Plugin::Lv2PluginImpl::apply_changed_param_values () noexcept
{
  for (const auto &change : owner_.change_tracker ().changes ())
    {
      if (change.index >= param_to_patch_.size ())
        continue;
      const auto patch_idx = param_to_patch_[change.index];
      if (patch_idx >= 0)
        {
          // patch:writable parameters reach the plugin as patch:Set
          // messages in the control sequence
          if (pending_patch_set_count_ < kPendingPatchSetCapacity)
            {
              pending_patch_sets_[pending_patch_set_count_++] = {
                static_cast<uint32_t> (patch_idx), change.modulated_value
              };
            }
          else
            {
              patch_sets_dropped_.fetch_add (1, std::memory_order_relaxed);
            }
          continue;
        }
      if (change.index >= param_to_ctrl_in_.size ())
        continue;
      const auto ctrl_in = param_to_ctrl_in_[change.index];
      if (ctrl_in < 0)
        continue;
      const auto &ctrl_param = ctrl_in_params_[static_cast<size_t> (ctrl_in)];
      control_in_bufs_[static_cast<size_t> (ctrl_in)] =
        control_value_for_param (ctrl_param, change.modulated_value);
    }
}

void
Lv2Plugin::Lv2PluginImpl::forge_pending_patch_sets () noexcept
{
  if (pending_patch_set_count_ == 0)
    return;

  const auto &urids = host_urids_;
  // Bytes a set takes in the sequence: the event's frame stamp, an
  // object head, and two properties (a URID and a 4-byte value; every
  // supported value kind is 4 bytes), each padded to 64-bit boundaries
  const auto property_bytes =
    2 * sizeof (uint32_t)
    + lv2_atom_pad_size (sizeof (LV2_Atom) + sizeof (float));
  const auto needed =
    sizeof (int64_t)
    + lv2_atom_pad_size (sizeof (LV2_Atom_Object) + 2 * property_bytes);

  uint32_t dropped = 0;
  for (size_t i = 0; i < pending_patch_set_count_; ++i)
    {
      const auto &set = pending_patch_sets_[i];
      auto       &patch_param = patch_params_[set.patch_idx];
      const auto  range_value =
        patch_param.param->range ().convertFrom0To1 (set.value_0_to_1);

      // Sets that do not fit are skipped whole: a partially forged set
      // would claim body bytes that were never written
      if (forge_.offset + needed > forge_.size)
        {
          ++dropped;
          continue;
        }

      LV2_Atom_Forge_Frame object_frame;
      lv2_atom_forge_frame_time (&forge_, 0);
      lv2_atom_forge_object (&forge_, &object_frame, 0, urids.patch_Set);
      lv2_atom_forge_key (&forge_, urids.patch_property);
      lv2_atom_forge_urid (&forge_, patch_param.uri_id);
      lv2_atom_forge_key (&forge_, urids.patch_value);
      // Integer and boolean values are quantized before forging
      auto forged_value = range_value;
      if (patch_param.value_type == urids.atom_Int)
        {
          forged_value = static_cast<float> (std::lround (range_value));
          lv2_atom_forge_int (&forge_, static_cast<int32_t> (forged_value));
        }
      else if (patch_param.value_type == urids.atom_Bool)
        {
          forged_value = range_value > 0.5f ? 1.f : 0.f;
          lv2_atom_forge_bool (&forge_, forged_value > 0.5f);
        }
      else if (patch_param.value_type == urids.atom_Double)
        {
          lv2_atom_forge_double (&forge_, static_cast<double> (range_value));
        }
      else
        {
          lv2_atom_forge_float (&forge_, range_value);
        }
      lv2_atom_forge_pop (&forge_, &object_frame);
      patch_param.last_sent_0_to_1 =
        patch_param.param->range ().convertTo0To1 (forged_value);
    }
  pending_patch_set_count_ = 0;
  if (dropped > 0)
    {
      patch_sets_dropped_.fetch_add (dropped, std::memory_order_relaxed);
    }
}

void
Lv2Plugin::Lv2PluginImpl::parse_patch_messages () noexcept
{
  if (patch_params_.empty ())
    return;

  const auto &urids = host_urids_;
  const auto  handle_set = [&] (const LV2_Atom_Object * object) noexcept {
    uint32_t         property = 0;
    const LV2_Atom * value = nullptr;
    LV2_ATOM_OBJECT_FOREACH (object, prop)
    {
      if (
        prop->key == urids.patch_property && prop->value.type == urids.atom_URID
        && prop->value.size == sizeof (uint32_t))
        {
          property = *reinterpret_cast<const uint32_t *> (
            LV2_ATOM_BODY_CONST (&prop->value));
        }
      else if (prop->key == urids.patch_value)
        {
          value = &prop->value;
        }
    }
    if (property == 0 || value == nullptr)
      return;

    const auto patch_it =
      std::ranges::find_if (patch_params_, [property] (const PatchParam &p) {
        return p.uri_id == property;
      });
    if (patch_it == patch_params_.end ())
      return;

    float range_value = 0.f;
    if (value->type == urids.atom_Float && value->size == sizeof (float))
      {
        range_value =
          *reinterpret_cast<const float *> (LV2_ATOM_BODY_CONST (value));
      }
    else if (value->type == urids.atom_Int && value->size == sizeof (int32_t))
      {
        range_value = static_cast<float> (
          *reinterpret_cast<const int32_t *> (LV2_ATOM_BODY_CONST (value)));
      }
    else if (value->type == urids.atom_Bool && value->size == sizeof (int32_t))
      {
        range_value =
          *reinterpret_cast<const int32_t *> (LV2_ATOM_BODY_CONST (value)) != 0
            ? 1.f
            : 0.f;
      }
    else if (value->type == urids.atom_Double && value->size == sizeof (double))
      {
        range_value = static_cast<float> (
          *reinterpret_cast<const double *> (LV2_ATOM_BODY_CONST (value)));
      }
    else
      {
        return;
      }

    const auto normalized =
      patch_it->param->range ().convertTo0To1 (range_value);

    // The first Set a parameter reports after a state restore
    // synchronizes the value; later ones are live edits from the
    // plugin side. A pending sync is applied even when the value
    // equals the last forged set
    const auto sync = patch_it->expect_sync_notify;
    patch_it->expect_sync_notify = false;

    // A value equal to the last forged set is the plugin echoing the
    // host's own message back
    if (!sync && std::abs (normalized - patch_it->last_sent_0_to_1) < 1e-6f)
      return;

    auto * param = patch_it->param;
    owner_.post_main_thread_action_deferred ([param, normalized, sync] {
      if (sync)
        {
          param->setBaseValue (normalized);
        }
      else
        {
          param->setBaseValueByUser (normalized);
        }
    });
  };

  const auto walk_sequence =
    [this, &handle_set] (const uint8_t * buf, size_t buf_size) noexcept {
      for_each_atom_output_event (buf, buf_size, [&] (const LV2_Atom_Event * ev) {
        if (
          ev->body.type != host_urids_.atom_Object
          || ev->body.size < sizeof (LV2_Atom_Object_Body))
          return;
        handle_set (reinterpret_cast<const LV2_Atom_Object *> (&ev->body));
      });
    };

  if (midi_out_port_idx_ >= 0)
    {
      walk_sequence (atom_out_buf_.data (), atom_out_buf_.size ());
    }
  for (const auto &port : ports_)
    {
      if (
        !port.has_atom_scratch || port.flow != dsp::PortFlow::Output
        || (port.type != PortInfo::Type::Atom && port.type != PortInfo::Type::Unknown))
        continue;
      walk_sequence (
        atom_scratch_buf_.data () + port.atom_scratch_byte_offset,
        atom_port_capacity (port));
    }
}

namespace
{
// Collects responses while work() runs inline for offline rendering or
// a state restore on this thread; null when work() is not running
// inline. Appending allocates, which is acceptable because inline work
// only ever runs on the offline-render or main thread, never the live
// audio thread
thread_local std::vector<std::vector<std::byte>> * t_inline_responses = nullptr;
}

/* Runs a work() call inline on the calling thread: used through the
 * restore feature array, so set_state()'s scheduled work completes
 * before the restore returns. */
LV2_Worker_Status
Lv2Plugin::Lv2PluginImpl::restore_schedule_work_callback (
  LV2_Worker_Schedule_Handle handle,
  uint32_t                   size,
  const void *               data) noexcept
{
  auto &self = *static_cast<Lv2PluginImpl *> (handle);
  if (self.worker_ == nullptr)
    return LV2_WORKER_ERR_UNKNOWN;
  return self.run_work_inline (size, static_cast<const std::byte *> (data));
}

/* Queues a request and wakes the worker. The queue push is a bounded
 * copy; the wake is futex-based and does not block. */
LV2_Worker_Status
Lv2Plugin::Lv2PluginImpl::schedule_work_callback (
  LV2_Worker_Schedule_Handle handle,
  uint32_t                   size,
  const void *               data) noexcept
{
  auto &self = *static_cast<Lv2PluginImpl *> (handle);
  if (self.worker_ == nullptr)
    return LV2_WORKER_ERR_UNKNOWN;
  const auto * bytes = static_cast<const std::byte *> (data);
  if (self.worker_->offline_)
    return self.run_work_inline (size, bytes);
  if (!self.worker_->requests_.push ({ bytes, size }))
    return LV2_WORKER_ERR_NO_SPACE;
  self.worker_->sleep_cv_.notify_one ();
  return LV2_WORKER_SUCCESS;
}

LV2_Worker_Status
Lv2Plugin::Lv2PluginImpl::respond_callback (
  LV2_Worker_Respond_Handle handle,
  uint32_t                  size,
  const void *              data) noexcept
{
  auto        &self = *static_cast<Lv2PluginImpl *> (handle);
  const auto * bytes = static_cast<const std::byte *> (data);
  if (t_inline_responses != nullptr)
    {
      // Collected here and delivered once work() returns, so
      // work_response() never reenters work()
      try
        {
          if (bytes == nullptr)
            {
              t_inline_responses->emplace_back ();
            }
          else
            {
              t_inline_responses->emplace_back (bytes, bytes + size);
            }
          return LV2_WORKER_SUCCESS;
        }
      catch (...)
        {
          return LV2_WORKER_ERR_NO_SPACE;
        }
    }
  return self.worker_->responses_.push ({ bytes, size })
           ? LV2_WORKER_SUCCESS
           : LV2_WORKER_ERR_NO_SPACE;
}

LV2_Worker_Status
Lv2Plugin::Lv2PluginImpl::run_work_inline (
  uint32_t          size,
  const std::byte * data) noexcept
{
#if defined(__has_feature) && __has_feature(realtime_sanitizer)
  // Plugin code is not ours; RTSan violations inside it are not actionable
  __rtsan::ScopedDisabler d;
#endif
  std::vector<std::vector<std::byte>> responses;
  auto *                              previous_responses = t_inline_responses;
  t_inline_responses = &responses;
  const auto status = [&] {
    std::lock_guard work_lock (worker_->work_mutex_);
    return worker_->interface_->work (
      lilv_instance_get_handle (instance_), respond_callback, this, size, data);
  }();
  t_inline_responses = previous_responses;
  for (const auto &response : responses)
    {
      worker_->interface_->work_response (
        lilv_instance_get_handle (instance_),
        static_cast<uint32_t> (response.size ()), response.data ());
    }
  return status;
}

void
Lv2Plugin::Lv2PluginImpl::worker_thread_func (std::stop_token stop_token)
{
  std::unique_lock lock (worker_->sleep_mutex_);
  while (!stop_token.stop_requested ())
    {
      warn_dropped_records ();
      uint32_t size = 0;
      if (worker_->requests_.pop (worker_->request_scratch_, size))
        {
          // work() runs without sleep_mutex_ held (it may block or
          // take arbitrarily long); work_mutex_ serializes it against
          // inline execution
          lock.unlock ();
          {
            std::lock_guard work_lock (worker_->work_mutex_);
            worker_->interface_->work (
              lilv_instance_get_handle (instance_), respond_callback, this,
              size, worker_->request_scratch_.data ());
          }
          lock.lock ();
          continue;
        }
      worker_->idle_.store (true, std::memory_order_release);
      // The timeout bounds the wake when a notify races the wait
      worker_->sleep_cv_.wait_for (lock, std::chrono::milliseconds (10), [&] {
        return stop_token.stop_requested () || !worker_->requests_.empty ();
      });
      worker_->idle_.store (false, std::memory_order_release);
    }
  // Report drops that landed after the last poll inside the loop
  warn_dropped_records ();
}

void
Lv2Plugin::Lv2PluginImpl::warn_dropped_records ()
{
  const auto request_drops = worker_->requests_.dropped_records ();
  const auto response_drops = worker_->responses_.dropped_records ();
  const auto reported_requests =
    worker_->warned_request_drops_.load (std::memory_order_relaxed);
  const auto reported_responses =
    worker_->warned_response_drops_.load (std::memory_order_relaxed);
  if (request_drops <= reported_requests && response_drops <= reported_responses)
    {
      return;
    }
  z_warning (
    "LV2 worker of '{}': dropped {} request(s) and {} response(s) because the queues were full",
    owner_.get_name (), request_drops - reported_requests,
    response_drops - reported_responses);
  worker_->warned_request_drops_.store (
    request_drops, std::memory_order_relaxed);
  worker_->warned_response_drops_.store (
    response_drops, std::memory_order_relaxed);
}

void
Lv2Plugin::Lv2PluginImpl::wait_for_worker_idle () noexcept
{
  if (worker_ == nullptr)
    return;
  while (
    !worker_->idle_.load (std::memory_order_acquire)
    || !worker_->requests_.empty ())
    {
      std::this_thread::yield ();
    }
}

void
Lv2Plugin::Lv2PluginImpl::deliver_worker_responses () noexcept
{
  if (worker_ == nullptr)
    return;
#if defined(__has_feature) && __has_feature(realtime_sanitizer)
  // Plugin code is not ours; RTSan violations inside it are not actionable
  __rtsan::ScopedDisabler d;
#endif
  uint32_t size = 0;
  while (worker_->responses_.pop (worker_->response_scratch_, size))
    {
      worker_->interface_->work_response (
        lilv_instance_get_handle (instance_), size,
        worker_->response_scratch_.data ());
    }
}

void
Lv2Plugin::Lv2PluginImpl::finish_worker_cycle () noexcept
{
  // end_run is optional worker interface data; when present, the spec
  // requires calling it after every run cycle, with or without
  // scheduled work
  if (worker_ != nullptr && worker_->interface_->end_run != nullptr)
    {
#if defined(__has_feature) && __has_feature(realtime_sanitizer)
      // Plugin code is not ours; RTSan violations inside it are not actionable
      __rtsan::ScopedDisabler d;
#endif
      worker_->interface_->end_run (lilv_instance_get_handle (instance_));
    }
}

void
Lv2Plugin::Lv2PluginImpl::reset_trigger_ports () noexcept
{
  for (const auto &[buffer_index, default_value] : trigger_control_ins_)
    {
      control_in_bufs_[buffer_index] = default_value;
    }
}

LV2_Worker_Status
Lv2Plugin::Lv2PluginImpl::ui_schedule_work_callback (
  LV2_Worker_Schedule_Handle handle,
  uint32_t                   size,
  const void *               data) noexcept
{
  auto *       session = static_cast<UiSession *> (handle);
  const auto * bytes = static_cast<const std::byte *> (data);
  // The payload size comes from plugin UI code: bound it like the
  // plugin-side queue and refuse allocation failures instead of
  // terminating the host
  if (size > kMaxWorkerRecordSize)
    return LV2_WORKER_ERR_NO_SPACE;
  try
    {
      std::lock_guard lock (session->ui_worker_mutex_);
      // Total queued bytes are bounded like the plugin-side queue
      // capacity: a UI scheduling faster than the idle pump drains
      // must not grow the deque without limit
      if (session->ui_worker_queued_bytes_ + size > WorkerState::kQueueCapacity)
        return LV2_WORKER_ERR_NO_SPACE;
      if (bytes == nullptr)
        {
          session->ui_worker_requests_.emplace_back ();
        }
      else
        {
          session->ui_worker_requests_.emplace_back (bytes, bytes + size);
        }
      session->ui_worker_queued_bytes_ += size;
      return LV2_WORKER_SUCCESS;
    }
  catch (...)
    {
      return LV2_WORKER_ERR_NO_SPACE;
    }
}

LV2_Worker_Status
Lv2Plugin::Lv2PluginImpl::ui_respond_callback (
  LV2_Worker_Respond_Handle handle,
  uint32_t                  size,
  const void *              data) noexcept
{
  auto * responses = static_cast<std::vector<std::vector<std::byte>> *> (handle);
  const auto * bytes = static_cast<const std::byte *> (data);
  // The payload size comes from plugin UI code: bound it like the
  // plugin-side queue and refuse allocation failures instead of
  // terminating the host
  if (size > kMaxWorkerRecordSize)
    return LV2_WORKER_ERR_NO_SPACE;
  try
    {
      if (bytes == nullptr)
        {
          responses->emplace_back ();
        }
      else
        {
          responses->emplace_back (bytes, bytes + size);
        }
      return LV2_WORKER_SUCCESS;
    }
  catch (...)
    {
      return LV2_WORKER_ERR_NO_SPACE;
    }
}

void
Lv2Plugin::Lv2PluginImpl::pump_ui_worker ()
{
  auto      &ui = *ui_;
  const auto session_alive = [this, session = &ui] () {
    return ui_.has_value () && &*ui_ == session && ui_->created;
  };

  std::deque<std::vector<std::byte>> requests;
  const LV2_Worker_Interface *       interface = nullptr;
  {
    std::lock_guard lock (ui.ui_worker_mutex_);
    interface = ui.ui_worker_interface_;
    if (interface == nullptr)
      return;
    requests.swap (ui.ui_worker_requests_);
    ui.ui_worker_queued_bytes_ = 0;
  }
  // work() and work_response() run outside the lock: they may
  // schedule more UI work through the same mutex. The handle is
  // copied because work() may re-enter the host and close the UI,
  // tearing the session down
  const auto handle = ui.handle;
  for (const auto &request : requests)
    {
      std::vector<std::vector<std::byte>> responses;
      interface->work (
        handle, ui_respond_callback, &responses,
        static_cast<uint32_t> (request.size ()), request.data ());
      if (!session_alive ())
        return;
      for (const auto &response : responses)
        {
          interface->work_response (
            handle, static_cast<uint32_t> (response.size ()), response.data ());
          if (!session_alive ())
            return;
        }
    }
}

void
Lv2Plugin::Lv2PluginImpl::read_control_outputs () noexcept
{
  if (latency_buf_index_ < 0)
    return;

  // The finite check rejects non-finite reports with defined behavior
  // in default builds; fast-math builds fold it away, where the integer
  // bounds below still reject lround's mapping of non-finite input. A
  // latency report is a non-negative sample count that fits 32 bits
  const auto reported_value =
    control_out_bufs_[static_cast<size_t> (latency_buf_index_)];
  if (!std::isfinite (reported_value))
    return;
  const auto reported = static_cast<int64_t> (std::lround (reported_value));
  if (reported < 0 || reported > std::numeric_limits<int32_t>::max ())
    return;
  const auto new_latency = units::samples (static_cast<uint32_t> (reported));
  if (new_latency != latency_.load (std::memory_order_relaxed))
    {
      latency_.store (new_latency, std::memory_order_relaxed);
      owner_.notify_latency_changed ();
    }
}

void
Lv2Plugin::Lv2PluginImpl::degrade_to_silence (
  units::sample_u32_t local_offset,
  units::sample_u32_t nframes) noexcept
{
  // Only the dropped chunk's window is zeroed: output earlier chunks of
  // the same cycle wrote must survive
  const auto offset = local_offset.in<size_t> (units::samples);
  const auto length = nframes.in<size_t> (units::samples);
  for (auto * port : audio_out_ports_by_bus_)
    {
      if (port == nullptr || port->buffers () == nullptr)
        continue;
      const auto num_samples =
        static_cast<size_t> (port->buffers ()->getNumSamples ());
      if (offset >= num_samples)
        continue;
      port->buffers ()->clear (
        static_cast<int> (offset),
        static_cast<int> (std::min (length, num_samples - offset)));
    }
  for (auto * port : cv_out_ports_by_port_)
    {
      if (port == nullptr || offset >= port->buf_.size ())
        continue;
      std::fill_n (
        port->buf_.begin () + static_cast<std::ptrdiff_t> (offset),
        std::min (length, port->buf_.size () - offset), 0.f);
    }
  degradation_reported_.store (true, std::memory_order_relaxed);
}

units::sample_u32_t
Lv2Plugin::get_single_playback_latency () const
{
  return pimpl_->latency_.load (std::memory_order_relaxed);
}

// ============================================================================
// State
// ============================================================================

namespace
{

// Creates the subdirectory structure of @p from under @p to, so that
// files copied by relative path from one tree to the other always
// have a destination
void
mirror_directory_tree (
  const std::filesystem::path &from,
  const std::filesystem::path &to)
{
  std::error_code ec;
  if (!std::filesystem::exists (from, ec))
    return;

  auto it = std::filesystem::recursive_directory_iterator (from, ec);
  if (ec != std::error_code{})
    {
      throw ZrythmException (
        fmt::format (
          "failed to walk the directory '{}': {}", from, ec.message ()));
    }
  for (
    ; it != std::filesystem::recursive_directory_iterator (); it.increment (ec))
    {
      if (ec != std::error_code{})
        {
          throw ZrythmException (
            fmt::format (
              "failed to walk the directory '{}': {}", from, ec.message ()));
        }
      const bool is_directory = it->is_directory (ec);
      if (ec != std::error_code{})
        {
          throw ZrythmException (
            fmt::format (
              "failed to inspect the directory entry '{}': {}", it->path (),
              ec.message ()));
        }
      if (!is_directory)
        continue;

      const auto relative_path =
        std::filesystem::relative (it->path (), from, ec);
      if (ec != std::error_code{})
        {
          throw ZrythmException (
            fmt::format (
              "failed to relativize the directory '{}': {}", it->path (),
              ec.message ()));
        }
      std::filesystem::create_directories (to / relative_path, ec);
      if (ec != std::error_code{})
        {
          throw ZrythmException (
            fmt::format (
              "failed to create the directory '{}': {}", to / relative_path,
              ec.message ()));
        }
    }
}

// Returns a copy of @p features with the makePath entry replaced by
// @p replacement, for passing a call-specific root to lilv while the
// instantiate-time feature the plugin holds stays untouched
std::vector<const LV2_Feature *>
features_with_make_path (
  const std::vector<const LV2_Feature *> &features,
  const LV2_Feature *                     replacement)
{
  auto result = features;
  std::ranges::replace_if (
    result,
    [] (const LV2_Feature * feature) {
      return feature != nullptr
             && std::string_view (feature->URI) == LV2_STATE__makePath;
    },
    replacement);
  return result;
}

// Returns the label of @p subject (the first object of the label
// predicate), or an empty string when it has none
QString
first_node_label (
  LilvWorld *      world,
  const LilvNode * subject,
  const LilvNode * label_predicate)
{
  const LilvNodesUPtr labels{
    lilv_world_find_nodes (world, subject, label_predicate, nullptr)
  };
  if (labels == nullptr || lilv_nodes_size (labels.get ()) == 0)
    return {};
  return utils::Utf8String::from_utf8_encoded_string (
           lilv_node_as_string (lilv_nodes_get_first (labels.get ())))
    .to_qstring ();
}

} // namespace

// Builds the per-restore feature array: makePath points at the
// restore's root, and the worker schedule runs work inline so
// set_state()'s scheduled work completes before the restore returns
std::vector<const LV2_Feature *>
Lv2Plugin::Lv2PluginImpl::features_for_restore (
  const LV2_Feature * make_path_replacement)
{
  auto result = features_with_make_path (feature_ptrs_, make_path_replacement);
  std::ranges::replace_if (
    result,
    [] (const LV2_Feature * feature) {
      return feature != nullptr
             && std::string_view (feature->URI) == LV2_WORKER__schedule;
    },
    &restore_worker_schedule_feature_wrapper_);
  return result;
}

void
Lv2Plugin::Lv2PluginImpl::ensure_session_dirs ()
{
  if (session_dir_ != nullptr)
    return;

  // The members are only assigned after every step succeeded: a
  // partially initialized session could not be rolled back or
  // retried, since the temporary directory owner would already be
  // attached
  auto session_dir = utils::io::make_tmp_dir ();
  // lilv compares plugin-supplied paths (which it canonicalizes)
  // against these directories, so the root must use its canonical
  // spelling: a symlinked temp directory would otherwise make lilv
  // treat every state file as external
  std::error_code ec;
  const auto      root = std::filesystem::canonical (
    utils::Utf8String::from_qstring (session_dir->path ()).to_path (), ec);
  if (ec != std::error_code{})
    {
      throw ZrythmException (
        fmt::format (
          "failed to canonicalize the session directory: {}", ec.message ()));
    }

  session_scratch_dir_ = root / "scratch";
  session_state_dir_ = root / "state";

  make_path_context_ = MakePathContext{ this, &session_scratch_dir_ };
  make_path_feature_ = LV2_State_Make_Path{
    .handle = &make_path_context_, .path = state_make_path
  };
  free_path_feature_ = LV2_State_Free_Path{
    .handle = nullptr,
    .free_path = state_free_path,
  };
  session_dir_ = std::move (session_dir);
}

char *
Lv2Plugin::Lv2PluginImpl::state_make_path (
  LV2_State_Make_Path_Handle handle,
  const char *               path)
{
  const auto * context = static_cast<const MakePathContext *> (handle);

  // The path is plugin-supplied: anything absolute or containing dot
  // components would escape the root the feature is bound to
  const std::filesystem::path relative_path =
    utils::Utf8String::from_utf8_encoded_string (path).to_path ();
  const auto escapes_root =
    relative_path.empty () || relative_path.is_absolute ()
    || relative_path.has_root_name ()
    || std::ranges::any_of (relative_path, [] (const auto &component) {
         return component == "." || component == ".." || component.empty ();
       });
  if (escapes_root)
    {
      z_warning (
        "LV2: '{}' requested a state path outside its session directory "
        "('{}'); refusing",
        context->impl->owner_.get_name (), path);
      return nullptr;
    }

  const auto      full_path = *context->root / relative_path;
  std::error_code ec;
  std::filesystem::create_directories (full_path.parent_path (), ec);
  if (ec)
    {
      z_warning (
        "LV2: failed to create the state directory of '{}' '{}': {}",
        context->impl->owner_.get_name (), full_path.parent_path (),
        ec.message ());
      return nullptr;
    }
  return strdup (utils::Utf8String::from_path (full_path).c_str ());
}

void
Lv2Plugin::Lv2PluginImpl::state_free_path (
  LV2_State_Free_Path_Handle handle,
  char *                     path)
{
  (void) handle;
  free (path);
}

std::optional<std::string>
Lv2Plugin::Lv2PluginImpl::save_state_to_blob ()
{
  if (instance_ == nullptr || plugin_ == nullptr)
    return std::nullopt;

  try
    {
      // Files created during save are written into a staging directory
      // that is archived and removed when this scope ends. The path is
      // canonicalized for the same reason as the session directories
      std::error_code             canonical_ec;
      const auto                  staging_dir = utils::io::make_tmp_dir ();
      const std::filesystem::path staging_path = std::filesystem::canonical (
        utils::Utf8String::from_qstring (staging_dir->path ()).to_path (),
        canonical_ec);
      if (canonical_ec != std::error_code{})
        {
          throw ZrythmException (
            fmt::format (
              "failed to canonicalize the staging directory: {}",
              canonical_ec.message ()));
        }

      // Files the plugin created at runtime are referenced by paths
      // relative to the scratch directory; lilv copies them into
      // staging (copy_dir) but creates only the staging root itself,
      // so the scratch directory tree is mirrored into staging first
      // to give nested files a destination
      mirror_directory_tree (session_scratch_dir_, staging_path);

      // The plugin's save() runs inside this call and receives a
      // makePath feature rooted at the staging directory (the host
      // feature creates leading directories, unlike the save-scoped
      // one lilv would add itself, which also drops all state
      // properties if the plugin's save() fails on a nested path).
      // lilv passes host features through, so only the makePath entry
      // of the per-call array below is swapped
      MakePathContext     staging_context{ this, &staging_path };
      LV2_State_Make_Path staging_make_path{
        .handle = &staging_context, .path = state_make_path
      };
      LV2_Feature staging_make_path_feature{
        LV2_STATE__makePath, &staging_make_path
      };
      const auto save_features =
        features_with_make_path (feature_ptrs_, &staging_make_path_feature);
      const LilvStateUPtr state{ lilv_state_new_from_instance (
        plugin_, instance_, &urid_map_feature_,
        utils::Utf8String::from_path (session_scratch_dir_).c_str (),
        utils::Utf8String::from_path (staging_path).c_str (), nullptr,
        utils::Utf8String::from_path (staging_path).c_str (),
        &Lv2PluginImpl::state_get_port_value, this,
        LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE, save_features.data ()) };
      if (state == nullptr)
        {
          throw ZrythmException ("failed to snapshot the instance state");
        }

      // The state TTL is keyed by the plugin URI, so it must be passed
      // as the state subject (lilv_plugin_get_uri returns a borrowed
      // node). External files keep their absolute paths: no link
      // directory is given, so lilv never creates symlinks.
      const LilvNode * plugin_uri = lilv_plugin_get_uri (plugin_);
      if (
        lilv_state_save (
          owner_.world_->raw (), &urid_map_feature_, &urid_unmap_feature_,
          state.get (), lilv_node_as_uri (plugin_uri),
          utils::Utf8String::from_path (staging_path).c_str (), "state.ttl")
        != 0)
        {
          throw ZrythmException ("failed to write the state TTL");
        }

      // Archive everything lilv wrote: the TTL plus state-created files.
      // The same caps as on restore are enforced here so an oversized
      // state fails at save time instead of producing a state that can
      // never be restored; sizes are checked before each file is read
      // so a plugin cannot force unbounded buffering
      std::vector<utils::zip_utils::Entry> entries;
      size_t                               total_size = 0;
      std::error_code                      ec;
      auto it = std::filesystem::recursive_directory_iterator (staging_path, ec);
      if (ec != std::error_code{})
        {
          throw ZrythmException (
            fmt::format (
              "failed to walk the staging directory: {}", ec.message ()));
        }
      for (
        ; it != std::filesystem::recursive_directory_iterator ();
        it.increment (ec))
        {
          if (ec != std::error_code{})
            {
              throw ZrythmException (
                fmt::format (
                  "failed to walk the staging directory: {}", ec.message ()));
            }
          const bool regular = it->is_regular_file (ec);
          if (ec != std::error_code{})
            {
              throw ZrythmException (
                fmt::format (
                  "failed to inspect the staging entry '{}': {}", it->path (),
                  ec.message ()));
            }
          if (!regular)
            continue;

          const auto file_size = it->file_size (ec);
          if (ec != std::error_code{})
            {
              throw ZrythmException (
                fmt::format (
                  "failed to size the staging entry '{}': {}", it->path (),
                  ec.message ()));
            }
          if (entries.size () == kMaxStateArchiveEntries)
            {
              throw ZrythmException (
                fmt::format (
                  "the state holds more than {} files", kMaxStateArchiveEntries));
            }
          if (
            total_size + static_cast<size_t> (file_size)
            > kMaxStateArchiveTotalSize)
            {
              throw ZrythmException (
                fmt::format (
                  "the state files hold more than {} bytes",
                  kMaxStateArchiveTotalSize));
            }

          std::ifstream stream{ it->path (), std::ios::binary };
          if (!stream)
            {
              throw ZrythmException (
                fmt::format ("failed to read the state file '{}'", it->path ()));
            }
          // The read is bounded by the sized cap; a file that grew
          // between the size query and the read fails instead of
          // buffering the growth
          auto bytes = std::vector<char> (static_cast<size_t> (file_size));
          if (file_size > 0)
            {
              stream.read (
                bytes.data (), static_cast<std::streamsize> (file_size));
            }
          if (stream.gcount () != static_cast<std::streamsize> (file_size))
            {
              throw ZrythmException (
                fmt::format (
                  "failed to read the state file '{}': short read", it->path ()));
            }
          if (stream.peek () != std::char_traits<char>::eof ())
            {
              throw ZrythmException (
                fmt::format (
                  "the state file '{}' grew while being read", it->path ()));
            }
          const auto relative_path =
            std::filesystem::relative (it->path (), staging_path, ec);
          if (ec != std::error_code{})
            {
              throw ZrythmException (
                fmt::format (
                  "failed to relativize the state file path '{}'", it->path ()));
            }
          entries.push_back (
            utils::zip_utils::Entry{
              utils::Utf8String::from_path (relative_path).str (),
              QByteArray{
                         bytes.data (), static_cast<qsizetype> (bytes.size ()) }
          });
          total_size += static_cast<size_t> (file_size);
        }

      return utils::to_std_string (
        utils::base64::encode (utils::zip_utils::create (entries)));
    }
  catch (const ZrythmException &e)
    {
      z_warning (
        "LV2: failed to serialize the state of '{}': {}", owner_.get_name (),
        e.what ());
      return std::nullopt;
    }
}

bool
Lv2Plugin::Lv2PluginImpl::restore_state_from_blob (
  const std::string &base64_state)
{
  if (instance_ == nullptr || plugin_ == nullptr)
    return false;

  std::vector<utils::zip_utils::Entry> entries;
  try
    {
      entries = utils::zip_utils::extract (
        utils::base64::decode (QByteArray::fromStdString (base64_state)),
        kMaxStateArchiveEntries, kMaxStateArchiveTotalSize);
    }
  catch (const ZrythmException &e)
    {
      z_warning (
        "LV2: failed to parse the state of '{}': {}", owner_.get_name (),
        e.what ());
      return false;
    }

  // The state directory is only rewritten once the archive is known
  // to hold a state TTL: a failed restore then leaves the previous
  // contents fully intact
  if (!std::ranges::any_of (entries, [] (const utils::zip_utils::Entry &entry) {
        return entry.path_ == "state.ttl";
      }))
    {
      z_warning (
        "LV2: the state of '{}' holds no state.ttl", owner_.get_name ());
      return false;
    }

  // Rewrite the session state directory with the archive contents. The
  // state TTL refers to the extracted files by paths relative to its
  // directory, which lilv resolves against the state->dir it derives
  // from the TTL location.
  std::error_code ec;
  std::filesystem::remove_all (session_state_dir_, ec);
  if (ec != std::error_code{})
    {
      z_warning (
        "LV2: failed to clear the state directory of '{}': {}",
        owner_.get_name (), ec.message ());
      return false;
    }
  std::filesystem::create_directories (session_state_dir_, ec);
  if (ec != std::error_code{})
    {
      z_warning (
        "LV2: failed to reset the state directory of '{}': {}",
        owner_.get_name (), ec.message ());
      return false;
    }
  for (const auto &entry : entries)
    {
      const auto path =
        session_state_dir_
        / utils::Utf8String::from_utf8_encoded_string (entry.path_).to_path ();
      std::filesystem::create_directories (path.parent_path (), ec);
      if (ec != std::error_code{})
        {
          z_warning (
            "LV2: failed to create the parent directory of the state file "
            "'{}' of '{}': {}",
            path, owner_.get_name (), ec.message ());
          return false;
        }
      std::ofstream stream{ path, std::ios::binary | std::ios::trunc };
      if (!stream)
        {
          z_warning (
            "LV2: failed to write the state file '{}' of '{}'", path,
            owner_.get_name ());
          return false;
        }
      stream.write (entry.data_.constData (), entry.data_.size ());
      if (!stream)
        {
          z_warning (
            "LV2: failed to write the state file '{}' of '{}'", path,
            owner_.get_name ());
          return false;
        }
    }

  const LilvStateUPtr state{ lilv_state_new_from_file (
    owner_.world_->raw (), &urid_map_feature_, nullptr,
    utils::Utf8String::from_path (session_state_dir_ / "state.ttl").c_str ()) };
  if (state == nullptr)
    {
      z_warning ("LV2: failed to parse the state of '{}'", owner_.get_name ());
      return false;
    }

  restore_state_with_scratch_paths (*state);
  return true;
}

void
Lv2Plugin::Lv2PluginImpl::restore_state_with_inline_worker (
  const LilvState                     &state,
  std::span<const LV2_Feature * const> features) [[clang::blocking]]
{
  // Complete requests queued before the caller paused processing
  // first; work() scheduled during set_state() through the cached
  // instantiate feature is serialized against the inline work by
  // work_mutex_
  wait_for_worker_idle ();
  lilv_state_restore (
    &state, instance_, state_set_port_value, this, 0, features.data ());
}

bool
Lv2Plugin::Lv2PluginImpl::restore_preset_from_world (
  const std::string &preset_uri)
{
  const LilvNodeUPtr preset_node{
    lilv_new_uri (owner_.world_->raw (), preset_uri.c_str ())
  };
  if (preset_node == nullptr)
    {
      z_warning (
        "LV2: invalid preset URI '{}' for '{}'", preset_uri, owner_.get_name ());
      return false;
    }

  // The preset state is read from the world's data: files its TTL
  // references keep their locations in the preset's bundle, resolved
  // by the mapPath feature lilv adds itself
  const LilvStateUPtr state{ lilv_state_new_from_world (
    owner_.world_->raw (), &urid_map_feature_, preset_node.get ()) };
  if (state == nullptr)
    {
      z_warning (
        "LV2: failed to load the state of preset '{}' of '{}'", preset_uri,
        owner_.get_name ());
      return false;
    }

  restore_state_with_scratch_paths (*state);
  return true;
}

void
Lv2Plugin::Lv2PluginImpl::restore_default_state_if_declared ()
{
  const LilvNodeUPtr load_default_state{
    lilv_new_uri (owner_.world_->raw (), LV2_STATE__loadDefaultState)
  };
  if (
    load_default_state == nullptr
    || !lilv_plugin_has_feature (plugin_, load_default_state.get ()))
    {
      return;
    }

  // The default state is described by the plugin's own data: files it
  // references keep their bundle locations, resolved by the mapPath
  // feature lilv adds itself
  const LilvStateUPtr state{ lilv_state_new_from_world (
    owner_.world_->raw (), &urid_map_feature_, lilv_plugin_get_uri (plugin_)) };
  if (state == nullptr)
    {
      z_warning (
        "LV2: failed to load the default state of '{}'", owner_.get_name ());
      return;
    }

  restore_state_with_scratch_paths (*state);
}

void
Lv2Plugin::Lv2PluginImpl::restore_state_with_scratch_paths (
  const LilvState &state)
{
  // The plugin's restore() may create files through makePath: they
  // are rooted at the scratch directory so they behave like runtime
  // files and are carried into the next saved state (the state
  // directory is wiped by the next restore, and paths in it would be
  // stored as absolute paths outside the archive)
  MakePathContext     restore_context{ this, &session_scratch_dir_ };
  LV2_State_Make_Path restore_make_path{
    .handle = &restore_context, .path = state_make_path
  };
  LV2_Feature restore_make_path_feature{
    LV2_STATE__makePath, &restore_make_path
  };
  const auto restore_features =
    features_for_restore (&restore_make_path_feature);
  restore_state_with_inline_worker (state, restore_features);
  for (auto &patch_param : patch_params_)
    {
      patch_param.expect_sync_notify = true;
    }
}

std::string
Lv2Plugin::save_state_impl () const
{
  const auto blob = pimpl_->save_state_to_blob ();
  if (!blob.has_value ())
    {
      // Not instantiated: keep any pending state instead of dropping it
      if (state_to_apply_.has_value ())
        return state_to_apply_->toStdString ();
      return {};
    }
  return *blob;
}

void
Lv2Plugin::Lv2PluginImpl::state_set_port_value (
  const char * port_symbol,
  void *       user_data,
  const void * value,
  uint32_t     size,
  uint32_t     type)
{
  auto * impl = static_cast<Lv2PluginImpl *> (user_data);

  // The data comes from saved project files or external presets and
  // may hold any literal type: convert the numeric and boolean ones
  // and reject everything else. An atom:Bool holds a 32-bit integer
  // body regardless of the C++ bool size
  const auto &urids = impl->host_urids_;
  const auto  new_value = [&] () -> std::optional<float> {
    if (type == urids.atom_Bool && size == sizeof (int32_t))
      return *static_cast<const int32_t *> (value) != 0 ? 1.f : 0.f;
    if (type == urids.atom_Float && size == sizeof (float))
      return *static_cast<const float *> (value);
    if (type == urids.atom_Double && size == sizeof (double))
      return static_cast<float> (*static_cast<const double *> (value));
    if (type == urids.atom_Int && size == sizeof (int32_t))
      return static_cast<float> (*static_cast<const int32_t *> (value));
    if (type == urids.atom_Long && size == sizeof (int64_t))
      return static_cast<float> (*static_cast<const int64_t *> (value));
    return std::nullopt;
  }();
  if (!new_value.has_value ())
    {
      z_warning (
        "LV2: state value of port '{}' of '{}' has an unsupported type or "
        "size; skipping it",
        port_symbol, impl->owner_.get_name ());
      return;
    }

  const auto * port_info = impl->find_control_input (port_symbol);
  if (port_info == nullptr)
    return;
  // freeWheeling-designated ports are host-driven: state saved by
  // other hosts may record them, but the value does not apply here
  if (port_info->is_freewheel)
    return;

  const auto buf_index = port_info->control_buffer_index;
  impl->control_in_bufs_[buf_index] = *new_value;
  const auto &ctrl_param = impl->ctrl_in_params_[buf_index];
  if (ctrl_param.param != nullptr)
    {
      ctrl_param.param->setBaseValue (
        impl->param_value_0_to_1_for_control (ctrl_param, *new_value));
    }
}

const void *
Lv2Plugin::Lv2PluginImpl::state_get_port_value (
  const char * port_symbol,
  void *       user_data,
  uint32_t *   size,
  uint32_t *   type)
{
  auto * impl = static_cast<Lv2PluginImpl *> (user_data);

  // The LV2 state specification allows save() to run concurrently with
  // run(); the control buffers are plain shared memory for this read
  const auto * port_info = impl->find_control_input (port_symbol);
  if (port_info == nullptr)
    return nullptr;
  // freeWheeling-designated ports are host-driven: their value is not
  // project state and is never saved
  if (port_info->is_freewheel)
    return nullptr;

  *size = sizeof (float);
  *type = impl->host_urids_.atom_Float;
  return &impl->control_in_bufs_[port_info->control_buffer_index];
}

bool
Lv2Plugin::load_state_impl (const std::string &base64_state)
{
  if (base64_state.empty ())
    return false;

  if (pimpl_->instance_ == nullptr)
    {
      // Not instantiated yet: applied during processing preparation
      state_to_apply_ = QByteArray::fromStdString (base64_state);
      return true;
    }

  // LV2 state restore runs on the plugin instance and must not overlap
  // audio processing of the same instance
  if (main_thread_callbacks_.with_paused_processing_)
    {
      auto restored = false;
      main_thread_callbacks_.with_paused_processing_ ([&] () {
        restored = pimpl_->restore_state_from_blob (base64_state);
      });
      return restored;
    }

  z_warning (
    "LV2: cannot apply state to '{}' while processing; the host cannot "
    "pause processing",
    get_name ());
  return false;
}

std::span<const Plugin::PresetEntry>
Lv2Plugin::presetEntries () const
{
  return preset_entries_;
}

void
Lv2Plugin::apply_preset_impl (const PresetId &id)
{
  assert (QThread::currentThread () == thread ());

  // LV2 presets are identified by their URI; index ids belong to
  // other formats
  const auto * preset_uri = std::get_if<QString> (&id);
  if (preset_uri == nullptr)
    {
      z_warning (
        "LV2 plugin '{}': refusing to apply a non-URI preset id", get_name ());
      return;
    }

  if (pimpl_->instance_ == nullptr)
    {
      z_warning (
        "LV2 plugin '{}': cannot apply preset '{}' while the plugin is not "
        "instantiated",
        get_name (), utils::Utf8String::from_qstring (*preset_uri));
      return;
    }

  // The restore runs on the plugin instance and must not overlap audio
  // processing of the same instance
  if (main_thread_callbacks_.with_paused_processing_)
    {
      const auto uri = utils::Utf8String::from_qstring (*preset_uri);
      main_thread_callbacks_.with_paused_processing_ ([this, &uri] () {
        pimpl_->restore_preset_from_world (uri.str ());
      });
      return;
    }

  z_warning (
    "LV2: cannot apply a preset to '{}' while processing; the host cannot "
    "pause processing",
    get_name ());
}

void
Lv2Plugin::rebuild_preset_list ()
{
  std::vector<PresetEntry> entries;

  auto *             world = world_->raw ();
  const LilvNodeUPtr preset_type{ lilv_new_uri (world, LV2_PRESETS__Preset) };
  const LilvNodeUPtr label_predicate{
    lilv_new_uri (world, LILV_NS_RDFS "label")
  };
  const LilvNodeUPtr  bank_predicate{ lilv_new_uri (world, LV2_PRESETS__bank) };
  const LilvNodesUPtr presets{
    lilv_plugin_get_related (pimpl_->plugin_, preset_type.get ())
  };
  if (presets != nullptr)
    {
      LILV_FOREACH (nodes, iter, presets.get ())
        {
          const LilvNode * preset = lilv_nodes_get (presets.get (), iter);
          if (!lilv_node_is_uri (preset))
            {
              z_warning (
                "LV2: '{}' has a preset that is not a URI; skipping it",
                get_name ());
              continue;
            }

          // Loads the data the preset's rdfs:seeAlso points to (its
          // label and port values)
          lilv_world_load_resource (world, preset);

          const LilvNodesUPtr labels{ lilv_world_find_nodes (
            world, preset, label_predicate.get (), nullptr) };
          if (labels == nullptr || lilv_nodes_size (labels.get ()) == 0)
            {
              z_warning (
                "LV2: '{}' has a preset without an rdfs:label; skipping it",
                get_name ());
              continue;
            }

          PresetEntry entry;
          entry.name =
            utils::Utf8String::from_utf8_encoded_string (
              lilv_node_as_string (lilv_nodes_get_first (labels.get ())))
              .to_qstring ();
          entry.id =
            utils::Utf8String::from_utf8_encoded_string (
              lilv_node_as_uri (preset))
              .to_qstring ();

          const LilvNodesUPtr banks{ lilv_world_find_nodes (
            world, preset, bank_predicate.get (), nullptr) };
          if (banks != nullptr && lilv_nodes_size (banks.get ()) > 0)
            {
              entry.group = first_node_label (
                world, lilv_nodes_get_first (banks.get ()),
                label_predicate.get ());
            }

          entries.push_back (std::move (entry));
        }
    }

  // The pset spec defines no order: sort by name, then URI, so equal
  // labels cannot change the list order between rebuilds
  std::ranges::sort (entries, [] (const PresetEntry &a, const PresetEntry &b) {
    if (a.name != b.name)
      return a.name < b.name;
    return std::get<QString> (a.id) < std::get<QString> (b.id);
  });

  if (entries != preset_entries_)
    {
      preset_entries_ = std::move (entries);
      notify_presets_rebuilt ();
    }
}

void
Lv2Plugin::clear_preset_list ()
{
  if (preset_entries_.empty ())
    return;

  preset_entries_.clear ();
  notify_presets_rebuilt ();
}

// ============================================================================
// UI
// ============================================================================

bool
Lv2Plugin::hasNativeUi () const
{
  return pimpl_->ui_info_.has_value ();
}

void
Lv2Plugin::set_offline_mode (bool offline) noexcept
{
  if (pimpl_->worker_ != nullptr)
    {
      if (offline)
        {
          // Drain before the switch: with the flag already set, work
          // scheduled by an in-flight work() would run inline on the
          // worker thread instead of the render thread
          pimpl_->wait_for_worker_idle ();
          pimpl_->worker_->offline_.store (true, std::memory_order_release);
        }
      else
        {
          pimpl_->worker_->offline_.store (false, std::memory_order_release);
        }
    }
  // FreeWheeling-designated ports are host-driven and written here
  // only; processing is stopped across the toggle
  for (const auto buffer_index : pimpl_->freewheel_control_ins_)
    {
      pimpl_->control_in_bufs_[buffer_index] = offline ? 1.f : 0.f;
    }
}

void
Lv2Plugin::on_ui_visibility_changed ()
{
  // The session is kept alive while hidden, so the live-session check
  // is on visibility: a re-show of a hidden UI goes back through
  // show_editor(), which re-shows the existing window
  if (uiVisible () && !(pimpl_->ui_.has_value () && pimpl_->ui_->visible))
    {
      show_editor ();
    }
  else if (!uiVisible () && pimpl_->ui_.has_value () && pimpl_->ui_->visible)
    {
      hide_editor ();
    }
}

void
Lv2Plugin::show_editor (bool force_float_window)
{
  assert (QThread::currentThread () == thread ());

  if (!pimpl_->ui_info_.has_value ())
    return;

  // The UI is kept alive while hidden: just re-show its window
  if (pimpl_->ui_.has_value () && pimpl_->ui_->created)
    {
      auto &ui = *pimpl_->ui_;
      if (ui.float_window)
        {
          const ScopedGlContextRelease gl_release;
          ui.show_iface->show (ui.handle);
        }
      else
        {
          ui.editor_window->setVisible (true);
        }
      ui.visible = true;
      return;
    }

  set_native_ui_unavailable (false);

  if (pimpl_->instance_ == nullptr)
    {
      // The UI features reference the plugin instance (instance access)
      z_warning (
        "LV2: cannot show the UI of '{}' while it is not instantiated",
        get_name ());
      set_native_ui_unavailable (true);
      return;
    }

  auto &ui = pimpl_->ui_.emplace ();
  ui.float_window = force_float_window;

  if (!ui.float_window)
    {
      ui.editor_window = pimpl_->host_window_factory_ (*this);
      // Without a windowing connection no embedding is possible, but a
      // UI that opens its own window can still be shown
      if (ui.editor_window == nullptr && pimpl_->ui_info_->show_interface)
        {
          ui.float_window = true;
        }
    }
  if (!ui.float_window && ui.editor_window == nullptr)
    {
      z_warning (
        "LV2: no host window available for the UI of '{}'; showing the "
        "generic UI",
        get_name ());
      pimpl_->destroy_ui ();
      set_native_ui_unavailable (true);
      return;
    }

  if (!ui.float_window)
    {
      // The UI was discovered for this build's window system; a window
      // of a different system (e.g. a Wayland window where X11 was
      // expected) cannot host it
      if (
        ui.editor_window->windowSystem ()
        != PluginHostWindow::currentWindowSystem ())
        {
          z_warning (
            "LV2: the host window of '{}' uses a window system this build "
            "cannot embed LV2 UIs in; showing the generic UI",
            get_name ());
          pimpl_->destroy_ui ();
          set_native_ui_unavailable (true);
          return;
        }

      // The host window may hide itself before the embedding handshake
      // completes; teardown must be deferred because the emission comes
      // from within the window's own call stack
      connect (
        ui.editor_window.get (), &PluginHostWindow::embeddingFailed, this,
        [this] {
          QTimer::singleShot (std::chrono::milliseconds{ 0 }, this, [this] {
            pimpl_->destroy_ui ();
            // A UI that can open its own window gets a second chance
            // as a floating window
            if (
              pimpl_->ui_info_.has_value () && pimpl_->ui_info_->show_interface)
              {
                show_editor (/*force_float_window=*/true);
              }
            else
              {
                set_native_ui_unavailable (true);
              }
          });
        });
    }

  {
    const auto lib_opened = ui.lib.load (
      utils::Utf8String::from_path (pimpl_->ui_info_->binary_path));
    auto descriptor_fn = reinterpret_cast<LV2UI_DescriptorFunction> (
      ui.lib.resolve ("lv2ui_descriptor"));
    if (!lib_opened || descriptor_fn == nullptr)
      {
        z_warning (
          "LV2: failed to load the UI library of '{}' from '{}'; showing "
          "the generic UI",
          get_name (), pimpl_->ui_info_->binary_path);
        pimpl_->destroy_ui ();
        set_native_ui_unavailable (true);
        return;
      }
    for (auto i = 0u;; ++i)
      {
        const auto * descriptor = descriptor_fn (i);
        if (descriptor == nullptr)
          break;
        if (pimpl_->ui_info_->uri == descriptor->URI)
          {
            ui.descriptor = descriptor;
            break;
          }
      }
    if (ui.descriptor == nullptr)
      {
        z_warning (
          "LV2: the UI library of '{}' holds no UI '{}'; showing the "
          "generic UI",
          get_name (), pimpl_->ui_info_->uri);
        pimpl_->destroy_ui ();
        set_native_ui_unavailable (true);
        return;
      }
  }

  // Features; their storage lives in the session and stays valid until
  // cleanup
  {
    const auto &urids = pimpl_->host_urids_;
    const auto window_title_urid = world_->urid_map ().map (LV2_UI__windowTitle);
    const auto scale_factor_urid = world_->urid_map ().map (LV2_UI__scaleFactor);
    ui.window_title_ = get_name ().str ();
    ui.scale_opt_ =
      ui.editor_window != nullptr ? ui.editor_window->contentScaleFactor () : 1.f;
    ui.options_[0] = LV2_Options_Option{
      .context = LV2_OPTIONS_INSTANCE,
      .subject = 0,
      .key = window_title_urid,
      .size = static_cast<uint32_t> (ui.window_title_.size () + 1),
      .type = urids.atom_String,
      .value = ui.window_title_.c_str ()
    };
    ui.options_[1] = LV2_Options_Option{
      .context = LV2_OPTIONS_INSTANCE,
      .subject = 0,
      .key = scale_factor_urid,
      .size = sizeof (float),
      .type = urids.atom_Float,
      .value = &ui.scale_opt_
    };
    ui.options_[2] = LV2_Options_Option{};

    if (ui.editor_window != nullptr)
      {
        ui.parent_window_ =
          static_cast<quintptr> (ui.editor_window->getEmbedWindowId ());
      }
    ui.resize_feature_ = LV2UI_Resize{
      .handle = pimpl_.get (), .ui_resize = &Lv2PluginImpl::ui_resize
    };
    ui.port_map_feature_ = LV2UI_Port_Map{
      .handle = pimpl_.get (), .port_index = &Lv2PluginImpl::ui_port_index
    };
    ui.ext_data_feature_.data_access =
      lilv_instance_get_descriptor (pimpl_->instance_)->extension_data;

    ui.features.clear ();
    ui.feature_ptrs.clear ();
    const auto push_feature = [&ui] (const char * uri, void * data) {
      ui.features.push_back (LV2_Feature{ uri, data });
    };
    push_feature (LV2_URID__map, &pimpl_->urid_map_feature_);
    push_feature (LV2_URID__unmap, &pimpl_->urid_unmap_feature_);
    push_feature (LV2_OPTIONS__options, ui.options_.data ());
    // A self-shown UI owns its toplevel window and is never parented
    if (!ui.float_window)
      {
        push_feature (
          LV2_UI__parent, reinterpret_cast<void *> (ui.parent_window_));
      }
    push_feature (LV2_UI__resize, &ui.resize_feature_);
    push_feature (LV2_UI__portMap, &ui.port_map_feature_);
    // ui:idleInterface is extension data with no feature payload; the
    // feature is passed to acknowledge that this host drives idle (the
    // idle pump below), which UIs may declare required
    push_feature (LV2_UI__idleInterface, nullptr);
    // UI worker jobs run on the idle pump (the UI thread)
    ui.ui_worker_schedule_feature_ = LV2_Worker_Schedule{
      .handle = &ui, .schedule_work = &Lv2PluginImpl::ui_schedule_work_callback
    };
    push_feature (LV2_WORKER__schedule, &ui.ui_worker_schedule_feature_);
    // The resizability hints are passed as features so UIs requiring
    // them instantiate; the host honors both by pinning the window
    if (pimpl_->ui_info_->fixed_size)
      push_feature (LV2_UI__fixedSize, nullptr);
    if (pimpl_->ui_info_->no_user_resize)
      push_feature (LV2_UI__noUserResize, nullptr);
    push_feature (
      LV2_INSTANCE_ACCESS_URI, lilv_instance_get_handle (pimpl_->instance_));
    push_feature (LV2_DATA_ACCESS_URI, &ui.ext_data_feature_);
    for (const auto &feature : ui.features)
      {
        ui.feature_ptrs.push_back (&feature);
      }
    ui.feature_ptrs.push_back (nullptr);
  }

  // The pending-atom list never allocates on the audio thread: extra
  // events are dropped and counted. The reservation happens when a UI
  // session starts — records can only exist once a UI does, so plugins
  // whose UI is never opened do not pay for it. The audio thread
  // touches the list only while the session dispatch flag is set, and
  // this reservation runs strictly before the flag is stored, so the
  // allocation never races the drain; later reservations are no-ops
  // because clear() never shrinks capacity
  pimpl_->pending_ui_atoms_.reserve (64);

  // Records of a previous session must not leak into this one: a later
  // plugin in this object may have fewer ports. The reset runs before
  // instantiate (a UI may write during instantiate and those records
  // must survive) and under the writer mutex, which orders it against
  // any record still being pushed
  {
    const std::scoped_lock lock (pimpl_->ui_write_mutex_);
    pimpl_->ui_to_plugin_fifo_.reset ();
    pimpl_->plugin_to_ui_fifo_.reset ();
  }

  LV2UI_Widget widget = nullptr;
  {
    const ScopedGlContextRelease gl_release;
    ui.handle = ui.descriptor->instantiate (
      ui.descriptor, lilv_node_as_uri (lilv_plugin_get_uri (pimpl_->plugin_)),
      pimpl_->ui_info_->bundle_path.c_str (), &Lv2PluginImpl::ui_write,
      pimpl_.get (), &widget, ui.feature_ptrs.data ());
  }
  if (ui.handle == nullptr)
    {
      z_warning (
        "LV2: failed to instantiate the UI of '{}'; showing the generic UI",
        get_name ());
      pimpl_->destroy_ui ();
      set_native_ui_unavailable (true);
      return;
    }
  ui.created = true;
  ui.visible = true;

  // For X11UI the widget out-parameter is the X11 Window ID: adopt the
  // view now so the window can host it and adopt its size. The UI's own
  // ui:resize call during instantiate may have set a size already; the
  // adopted geometry is the view's actual size and wins. A nullopt
  // return is not a final failure: the view may be invalid or appear
  // late, and the window's retry scan still runs; the window emits
  // embeddingFailed once the embedding is definitely over, and the
  // fallback queued for it tears this session (including the idle
  // pump) down before it can tick
  if (!ui.float_window && widget != nullptr)
    {
      const auto view_size = ui.editor_window->attachNativeView (
        reinterpret_cast<quintptr> (widget));
      if (view_size.has_value ())
        {
          const auto [w, h] = plugin_view_size_to_host_window_size (
            view_size->width (), view_size->height (),
            ui.editor_window->contentScaleFactor ());
          if (!ui.initial_size_applied)
            {
              ui.editor_window->setSizeAndCenter (w, h);
              ui.initial_size_applied = true;
            }
          else
            {
              ui.editor_window->setSize (w, h);
            }
        }
    }

  if (ui.descriptor->extension_data != nullptr)
    {
      ui.idle_iface = static_cast<const LV2UI_Idle_Interface *> (
        ui.descriptor->extension_data (LV2_UI__idleInterface));
      ui.plugin_resize_iface = static_cast<const LV2UI_Resize *> (
        ui.descriptor->extension_data (LV2_UI__resize));
      ui.ui_worker_interface_ = static_cast<const LV2_Worker_Interface *> (
        ui.descriptor->extension_data (LV2_WORKER__interface));
      if (ui.float_window)
        {
          ui.show_iface = static_cast<const LV2UI_Show_Interface *> (
            ui.descriptor->extension_data (LV2_UI__showInterface));
        }
    }
  if (ui.float_window && (ui.show_iface == nullptr || ui.show_iface->show == nullptr || ui.show_iface->hide == nullptr))
    {
      z_warning (
        "LV2: the UI of '{}' declares ui:showInterface but provides no "
        "show/hide functions; showing the generic UI",
        get_name ());
      pimpl_->destroy_ui ();
      set_native_ui_unavailable (true);
      return;
    }

  // Control values are sent in full on the first pump cycle
  const auto collect_relayed =
    [this] (dsp::PortFlow flow) -> std::vector<Lv2PluginImpl::RelayedControl> {
    std::vector<Lv2PluginImpl::RelayedControl> relayed;
    for (const auto &port : pimpl_->ports_)
      {
        if (
          port.type == Lv2PluginImpl::PortInfo::Type::Control
          && port.flow == flow)
          {
            relayed.push_back ({ port.index, port.control_buffer_index });
          }
      }
    return relayed;
  };
  ui.relayed_control_ins_ = collect_relayed (dsp::PortFlow::Input);
  ui.relayed_control_outs_ = collect_relayed (dsp::PortFlow::Output);
  ui.last_control_values_.assign (
    ui.relayed_control_ins_.size () + ui.relayed_control_outs_.size (),
    std::numeric_limits<float>::quiet_NaN ());
  pimpl_->ui_dispatch_.store (true, std::memory_order_release);
  ui.idle_token_ = ui.run_loop_.register_timer (
    std::chrono::milliseconds{ 16 }, [this] { pimpl_->ui_idle_tick (); });

  if (!ui.float_window)
    {
      // Host-initiated embed area resizes are forwarded to the UI's own
      // resize interface; LV2 defines no way to constrain a size first
      ui.resize_coordinator = utils::make_qobject_unique<
        PluginViewResizeCoordinator> (
        *ui.editor_window,
        PluginViewResizeCoordinator::Hooks{
          .gui_active =
            [this] { return pimpl_->ui_.has_value () && pimpl_->ui_->created; },
          .can_resize =
            [this] {
              return pimpl_->ui_info_.has_value () && !pimpl_->ui_info_->fixed_size
                     && !pimpl_->ui_info_->no_user_resize;
            },
          .adjust_size = [] (int &, int &) { },
          .apply_size =
            [this] (int width, int height) {
              if (
                !pimpl_->ui_.has_value () || !pimpl_->ui_->created
                || pimpl_->ui_->plugin_resize_iface == nullptr)
                return;
              const auto scale =
                pimpl_->ui_->editor_window->contentScaleFactor ();
              const ScopedGlContextRelease gl_release;
              pimpl_->ui_->plugin_resize_iface->ui_resize (
                pimpl_->ui_->handle,
                host_window_logical_to_physical (width, scale),
                host_window_logical_to_physical (height, scale));
            },
        });
      ui.editor_window->setResizable (
        !pimpl_->ui_info_->fixed_size && !pimpl_->ui_info_->no_user_resize);
      ui.editor_window->setVisible (true);
      ui.editor_window->completeNativeEmbedding ();
    }
  else
    {
      const ScopedGlContextRelease gl_release;
      const auto                   shown = ui.show_iface->show (ui.handle);
      if (shown != 0)
        {
          z_warning (
            "LV2: the UI of '{}' refused to show its window; showing the "
            "generic UI",
            get_name ());
          pimpl_->destroy_ui ();
          set_native_ui_unavailable (true);
          return;
        }
    }
}

void
Lv2Plugin::hide_editor ()
{
  if (!pimpl_->ui_.has_value () || !pimpl_->ui_->visible)
    return;

  auto &ui = *pimpl_->ui_;
  if (ui.float_window)
    {
      const ScopedGlContextRelease gl_release;
      ui.show_iface->hide (ui.handle);
    }
  else
    {
      ui.editor_window->setVisible (false);
    }
  ui.visible = false;
}

void
Lv2Plugin::Lv2PluginImpl::resolve_ui ()
{
  std::optional<UiInfo> new_info;

  const char * widget_type_uri = [] -> const char * {
    switch (PluginHostWindow::currentWindowSystem ())
      {
      case WindowSystem::X11:
        return LV2_UI__X11UI;
      case WindowSystem::Win32:
        return LV2_UI__WindowsUI;
      case WindowSystem::Cocoa:
        return LV2_UI__CocoaUI;
      case WindowSystem::Wayland:
        return nullptr;
      }
    return nullptr;
  }();

  if (widget_type_uri != nullptr)
    {
      auto *             world = owner_.world_->raw ();
      const LilvNodeUPtr widget_type{ lilv_new_uri (world, widget_type_uri) };
      const LilvNodeUPtr fixed_size{ lilv_new_uri (world, LV2_UI__fixedSize) };
      const LilvNodeUPtr no_user_resize{
        lilv_new_uri (world, LV2_UI__noUserResize)
      };
      // ui:fixedSize and ui:noUserResize are features: the UI announces
      // them through the standard feature predicates, with the feature URI
      // as the object
      const LilvNodeUPtr optional_feature_pred{
        lilv_new_uri (world, LV2_CORE__optionalFeature)
      };
      const LilvNodeUPtr required_feature_pred{
        lilv_new_uri (world, LV2_CORE__requiredFeature)
      };
      const LilvNodeUPtr extension_data_pred{
        lilv_new_uri (world, LV2_CORE__extensionData)
      };
      const LilvNodeUPtr show_interface{
        lilv_new_uri (world, LV2_UI__showInterface)
      };
      const auto declares_feature =
        [world, &optional_feature_pred, &required_feature_pred] (
          const LilvNode * ui_node, const LilvNode * feature) {
          return lilv_world_ask (
                   world, ui_node, optional_feature_pred.get (), feature)
                 || lilv_world_ask (
                   world, ui_node, required_feature_pred.get (), feature);
        };
      // The feature set below is what UI instantiation passes; a UI
      // requiring anything else must be skipped, not instantiated.
      // ui:parent is included although self-shown UIs do not get it:
      // those open no embed area at all, so a UI requiring it cannot
      // be floated either way
      const std::string_view supported_ui_features[] = {
        LV2_URID__map,       LV2_URID__unmap,       LV2_OPTIONS__options,
        LV2_UI__parent,      LV2_UI__resize,        LV2_UI__portMap,
        LV2_UI__fixedSize,   LV2_UI__noUserResize,  LV2_INSTANCE_ACCESS_URI,
        LV2_DATA_ACCESS_URI, LV2_UI__idleInterface, LV2_WORKER__schedule
      };
      const LilvUIsUPtr uis{ lilv_plugin_get_uis (plugin_) };
      if (uis != nullptr)
        {
          LILV_FOREACH (uis, iter, uis.get ())
            {
              const auto * ui = lilv_uis_get (uis.get (), iter);
              if (!lilv_ui_is_a (ui, widget_type.get ()))
                continue;

              const auto * uri = lilv_node_as_uri (lilv_ui_get_uri (ui));
              const auto * binary_uri =
                lilv_node_as_uri (lilv_ui_get_binary_uri (ui));
              const auto * bundle_uri =
                lilv_node_as_uri (lilv_ui_get_bundle_uri (ui));
              if (
                uri == nullptr || binary_uri == nullptr || bundle_uri == nullptr)
                continue;

              // The URIs are file URIs; non-file locations cannot be
              // hosted
              const auto binary_path =
                utils::Utf8String::from_qstring (
                  QUrl (QString::fromUtf8 (binary_uri)).toLocalFile ())
                  .to_path ();
              const auto bundle_path =
                utils::Utf8String::from_qstring (
                  QUrl (QString::fromUtf8 (bundle_uri)).toLocalFile ())
                  .to_path ();
              if (binary_path.empty () || bundle_path.empty ())
                {
                  z_warning (
                    "LV2: UI '{}' of '{}' does not live in a local bundle; "
                    "skipping it",
                    uri, owner_.get_name ());
                  continue;
                }

              const LilvNodeUPtr ui_node{ lilv_new_uri (world, uri) };
              // UI descriptions usually live in a separate data file
              // linked from the manifest via rdfs:seeAlso, and lilv
              // loads those on demand only: without this call, feature
              // and extension-data statements (ui:noUserResize,
              // ui:showInterface, ...) are missing from the model
              lilv_world_load_resource (world, ui_node.get ());
              bool                ui_features_supported = true;
              const LilvNodesUPtr ui_required_features{ lilv_world_find_nodes (
                world, ui_node.get (), required_feature_pred.get (), nullptr) };
              if (ui_required_features != nullptr)
                {
                  LILV_FOREACH (nodes, riter, ui_required_features.get ())
                    {
                      const auto * req_uri = lilv_node_as_uri (
                        lilv_nodes_get (ui_required_features.get (), riter));
                      if (
                        req_uri != nullptr
                        && std::ranges::none_of (
                          supported_ui_features,
                          [req_uri] (const auto s) { return s == req_uri; }))
                        {
                          ui_features_supported = false;
                          z_warning (
                            "LV2: UI '{}' of '{}' requires unsupported "
                            "feature '{}'; skipping it",
                            uri, owner_.get_name (), req_uri);
                          break;
                        }
                    }
                }
              if (!ui_features_supported)
                continue;
              UiInfo info;
              info.uri = uri;
              info.binary_path = binary_path;
              info.bundle_path =
                utils::Utf8String::from_path (bundle_path / "").str ();
              info.fixed_size =
                declares_feature (ui_node.get (), fixed_size.get ());
              info.no_user_resize =
                declares_feature (ui_node.get (), no_user_resize.get ());
              info.show_interface = lilv_world_ask (
                world, ui_node.get (), extension_data_pred.get (),
                show_interface.get ());
              new_info = std::move (info);
              break;
            }
        }
    }

  const bool presence_changed = new_info.has_value () != ui_info_.has_value ();
  ui_info_ = std::move (new_info);
  if (presence_changed)
    {
      Q_EMIT owner_.hasNativeUiChanged ();
    }
}

void
Lv2Plugin::Lv2PluginImpl::destroy_ui ()
{
  if (!ui_.has_value ())
    return;

  auto &ui = *ui_;
  // The pump stops first: no further ticks may run against the dying
  // session
  ui.run_loop_.unregister_timer (ui.idle_token_);
  ui_dispatch_.store (false, std::memory_order_release);
  {
    std::lock_guard lock (ui.ui_worker_mutex_);
    ui.ui_worker_requests_.clear ();
    ui.ui_worker_queued_bytes_ = 0;
    ui.ui_worker_interface_ = nullptr;
  }
  ui.resize_coordinator.reset ();
  // A shown self-managed window is hidden before the host stops
  // driving the UI, per the show interface contract
  if (
    ui.float_window && ui.visible && ui.show_iface != nullptr
    && ui.show_iface->hide != nullptr)
    {
      const ScopedGlContextRelease gl_release;
      ui.show_iface->hide (ui.handle);
    }
  if (ui.handle != nullptr && ui.descriptor->cleanup != nullptr)
    {
      const ScopedGlContextRelease gl_release;
      ui.descriptor->cleanup (ui.handle);
    }
  ui.handle = nullptr;
  ui.created = false;
  ui.visible = false;
  ui.lib.unload ();
  // The host window is destroyed only after the UI was cleaned up: UIs
  // may touch the embed parent during their own teardown
  ui.editor_window.reset ();
  ui_.reset ();
}

void
Lv2Plugin::Lv2PluginImpl::ui_write (
  LV2UI_Controller controller,
  uint32_t         port_index,
  uint32_t         buffer_size,
  uint32_t         port_protocol,
  const void *     buffer)
{
  auto * impl = static_cast<Lv2PluginImpl *> (controller);

  if (port_index >= impl->ports_.size ())
    {
      z_warning (
        "LV2: the UI of '{}' wrote to unknown port index {}; ignoring it",
        impl->owner_.get_name (), port_index);
      return;
    }
  const auto &port = impl->ports_[port_index];

  const auto push_record =
    [impl] (std::span<const std::byte> a, std::span<const std::byte> b) {
      // The ring admits a single writer: concurrent writes from the
      // UI's own threads are serialized here
      const std::scoped_lock lock (impl->ui_write_mutex_);
      if (
        fifo_write_record (
          impl->ui_to_plugin_fifo_, impl->ui_to_plugin_buf_, a, b))
        return;
      impl->ui_to_plugin_dropped_.fetch_add (1, std::memory_order_relaxed);
      z_warning (
        "LV2: the UI event ring of '{}' is full; dropping an event",
        impl->owner_.get_name ());
    };

  if (port_protocol == 0)
    {
      if (buffer_size != sizeof (float))
        {
          z_warning (
            "LV2: the UI of '{}' wrote {} bytes to control port {} instead "
            "of a float; ignoring it",
            impl->owner_.get_name (), buffer_size, port.symbol);
          return;
        }
      if (
        port.type != PortInfo::Type::Control
        || port.flow != dsp::PortFlow::Input)
        {
          z_warning (
            "LV2: the UI of '{}' wrote a float to port {} which is no "
            "control input; ignoring it",
            impl->owner_.get_name (), port.symbol);
          return;
        }
      if (port.is_freewheel)
        {
          z_warning (
            "LV2: the UI of '{}' wrote to freeWheeling-designated port {}, "
            "which is host-driven; ignoring it",
            impl->owner_.get_name (), port.symbol);
          return;
        }
      const auto  value = *static_cast<const float *> (buffer);
      const auto &ctrl_param = impl->ctrl_in_params_[port.control_buffer_index];
      if (ctrl_param.param != nullptr)
        {
          // Ports with a parameter ride the parameter path: the value
          // reaches the audio thread through the change tracker and the
          // edit is attributed as a user edit
          if (QThread::currentThread () == impl->owner_.thread ())
            {
              ctrl_param.param->setBaseValueByUser (
                impl->param_value_0_to_1_for_control (ctrl_param, value));
              return;
            }
          // UIs may call write() from their own threads, where Qt
          // signal emission is not allowed. The value is delivered
          // twice by design: the ring record applies it to the
          // control buffer on the audio thread, and the deferred
          // parameter update attributes the edit as a user action
          const auto   header = UiEventHeader{ port_index, 0, sizeof (float) };
          const auto * value_bytes =
            reinterpret_cast<const std::byte *> (&value);
          push_record (
            { reinterpret_cast<const std::byte *> (&header), sizeof (header) },
            { value_bytes, sizeof (float) });
          auto *     param = ctrl_param.param;
          const auto normalized =
            impl->param_value_0_to_1_for_control (ctrl_param, value);
          impl->owner_.post_main_thread_action_deferred ([param, normalized] {
            param->setBaseValueByUser (normalized);
          });
          return;
        }
      const auto   header = UiEventHeader{ port_index, 0, sizeof (float) };
      const auto * value_bytes = reinterpret_cast<const std::byte *> (&value);
      push_record (
        { reinterpret_cast<const std::byte *> (&header), sizeof (header) },
        { value_bytes, sizeof (float) });
      return;
    }

  if (port_protocol == impl->host_urids_.atom_eventTransfer)
    {
      if (buffer_size < sizeof (LV2_Atom))
        return;
      const auto * atom = static_cast<const LV2_Atom *> (buffer);
      const auto   total = sizeof (LV2_Atom) + atom->size;
      if (
        total > buffer_size || total > kAtomBufferSize
        || (port.type != PortInfo::Type::Atom && port.type != PortInfo::Type::Unknown))
        {
          z_warning (
            "LV2: the UI of '{}' wrote an atom the port {} cannot take; "
            "ignoring it",
            impl->owner_.get_name (), port.symbol);
          return;
        }
      // Only atom inputs this host routes to the plugin (the MIDI input
      // or a scratch port) receive UI atoms: the routed time port is
      // host-driven and output ports are plugin-driven
      if (
        port.flow != dsp::PortFlow::Input
        || (impl->midi_in_port_idx_ != static_cast<int32_t> (port_index)
            && !port.has_atom_scratch))
        {
          impl->ui_to_plugin_dropped_.fetch_add (1, std::memory_order_relaxed);
          z_warning (
            "LV2: the UI of '{}' wrote an atom to port {} which this host "
            "does not route from UIs; ignoring it",
            impl->owner_.get_name (), port.symbol);
          return;
        }
      const auto header = UiEventHeader{
        port_index, port_protocol, static_cast<uint32_t> (total)
      };
      push_record (
        { reinterpret_cast<const std::byte *> (&header), sizeof (header) },
        { reinterpret_cast<const std::byte *> (atom), total });
      return;
    }

  z_warning (
    "LV2: the UI of '{}' wrote with an unsupported protocol; ignoring it",
    impl->owner_.get_name ());
}

uint32_t
Lv2Plugin::Lv2PluginImpl::ui_port_index (
  LV2UI_Feature_Handle handle,
  const char *         symbol)
{
  const auto * impl = static_cast<const Lv2PluginImpl *> (handle);
  const auto   it =
    std::ranges::find_if (impl->ports_, [symbol] (const PortInfo &p) {
      return p.symbol.view () == symbol;
    });
  return it == impl->ports_.end () ? LV2UI_INVALID_PORT_INDEX : it->index;
}

int
Lv2Plugin::Lv2PluginImpl::
  ui_resize (LV2UI_Feature_Handle handle, int width, int height)
{
  auto * impl = static_cast<Lv2PluginImpl *> (handle);
  if (
    !impl->ui_.has_value () || impl->ui_->editor_window == nullptr
    || impl->ui_->float_window)
    return 0;

  if (width <= 0 || height <= 0)
    {
      z_warning (
        "LV2: the UI of '{}' requested an invalid size {}x{}; refusing it",
        impl->owner_.get_name (), width, height);
      return -1;
    }

  auto &ui = *impl->ui_;
  const auto [w, h] = plugin_view_size_to_host_window_size (
    width, height, ui.editor_window->contentScaleFactor ());
  z_debug (
    "LV2: the UI of '{}' resized its view to {}x{} physical ({}x{} logical)",
    impl->owner_.get_name (), width, height, w, h);
  if (!ui.initial_size_applied)
    {
      ui.editor_window->setSizeAndCenter (w, h);
      ui.initial_size_applied = true;
    }
  else
    {
      ui.editor_window->setSize (w, h);
    }
  return 0;
}

void
Lv2Plugin::Lv2PluginImpl::ui_idle_tick ()
{
  if (!ui_.has_value () || !ui_->created)
    return;

  auto &ui = *ui_;

  // port_event() and idle() re-enter the host synchronously (e.g. a
  // write() during port_event() running parameter change handlers), and
  // a re-entrant call may tear this session down or replace it. The
  // pump re-checks after every such call that the session storage it
  // holds is still a live session: a session synchronously recreated at
  // the same address is safe to keep pumping (its callbacks tolerate an
  // extra round)
  auto * const session = &*ui_;
  const auto   session_alive = [this, session] () {
    return ui_.has_value () && &*ui_ == session && ui_->created;
  };

  // Relay the events the audio thread collected; the record layout
  // matches the atom transfer protocol (atom header + body) or a plain
  // float
  UiEventHeader header{};
  while (fifo_read_bytes (
    plugin_to_ui_fifo_, plugin_to_ui_buf_,
    { reinterpret_cast<std::byte *> (&header), sizeof (header) }))
    {
      if (
        header.size > ui.event_scratch_.size ()
        || !fifo_read_bytes (
          plugin_to_ui_fifo_, plugin_to_ui_buf_,
          { ui.event_scratch_.data (), header.size }))
        break;
      if (ui.descriptor->port_event != nullptr)
        {
          const ScopedGlContextRelease gl_release;
          ui.descriptor->port_event (
            ui.handle, header.port_index, header.size, header.protocol,
            ui.event_scratch_.data ());
        }
      if (!session_alive ())
        return;
    }

  pump_ui_worker ();
  if (!session_alive ())
    return;

  // Forward changed control values, inputs and outputs (the initial
  // NaN shadow sends every value once). The audio thread writes these
  // buffers; they hold plain floats, so torn reads are benign
  // Returns false when a re-entrant call ended this session
  const auto relay_changed =
    [&] (
      const std::vector<RelayedControl> &relayed, std::span<const float> values,
      size_t shadow_base) {
      for (const auto &[i, entry] : utils::views::enumerate (relayed))
        {
          auto  value = values[entry.buffer_index];
          auto &last = ui.last_control_values_[shadow_base + i];
          if (utils::math::floats_equal (last, value))
            continue;
          last = value;
          if (ui.descriptor->port_event != nullptr)
            {
              const ScopedGlContextRelease gl_release;
              ui.descriptor->port_event (
                ui.handle, entry.lv2_port_index, sizeof (float), 0, &value);
            }
          if (!session_alive ())
            return false;
        }
      return true;
    };
  if (!relay_changed (ui.relayed_control_ins_, control_in_bufs_, 0))
    return;
  if (
    !relay_changed (
      ui.relayed_control_outs_, control_out_bufs_,
      ui.relayed_control_ins_.size ()))
    return;

  if (ui.idle_iface == nullptr)
    return;

  auto closed = 0;
  {
    const ScopedGlContextRelease gl_release;
    closed = ui.idle_iface->idle (ui.handle);
  }
  if (!session_alive ())
    return;
  if (closed != 0)
    {
      // The UI closed itself: stop the pump, then tear down outside
      // this callback's stack
      ui.run_loop_.unregister_timer (ui.idle_token_);
      QTimer::singleShot (std::chrono::milliseconds{ 0 }, &owner_, [this] {
        destroy_ui ();
        owner_.setUiVisible (false);
      });
    }
}

bool
Lv2Plugin::Lv2PluginImpl::drain_ui_atom_events () noexcept
{
  // Acquire pairs with the release stores at session open/teardown:
  // the rings and the pending-atom list are reset at session open on
  // the main thread, and a reset racing this drain could leave the
  // fifo indices half-updated here. Records pushed during instantiate
  // stay in the freshly reset ring and are read once the session is
  // dispatched
  if (!ui_dispatch_.load (std::memory_order_acquire))
    return false;

  pending_ui_atoms_.clear ();

  UiEventHeader header{};
  while (fifo_read_bytes (
    ui_to_plugin_fifo_, ui_to_plugin_buf_,
    { reinterpret_cast<std::byte *> (&header), sizeof (header) }))
    {
      // The body is always drained so the record stream stays aligned;
      // records larger than the scratch cannot exist because the
      // writer rejects them, so this failure would mean a corrupted
      // stream and stops the drain
      if (
        header.size > kAtomBufferSize
        || !fifo_read_bytes (
          ui_to_plugin_fifo_, ui_to_plugin_buf_,
          { ui_drain_scratch_.data (), header.size }))
        break;

      // Port indices are only validated against the current ports_ on
      // the drain side (a record may predate a reconfiguration)
      if (header.port_index >= ports_.size ())
        {
          ui_to_plugin_dropped_.fetch_add (1, std::memory_order_relaxed);
          continue;
        }

      if (header.protocol == 0)
        {
          if (header.size != sizeof (float))
            {
              ui_to_plugin_dropped_.fetch_add (1, std::memory_order_relaxed);
              continue;
            }
          float value = 0.f;
          std::memcpy (&value, ui_drain_scratch_.data (), sizeof (value));
          const auto &port = ports_[header.port_index];
          if (
            port.type == PortInfo::Type::Control
            && port.flow == dsp::PortFlow::Input)
            {
              control_in_bufs_[port.control_buffer_index] = value;
            }
          else
            {
              ui_to_plugin_dropped_.fetch_add (1, std::memory_order_relaxed);
            }
          continue;
        }

      // Atom records are collected and appended to their port's
      // sequence during this chunk's forging
      if (pending_ui_atoms_.size () >= pending_ui_atoms_.capacity ())
        {
          ui_to_plugin_dropped_.fetch_add (1, std::memory_order_relaxed);
          continue;
        }
      pending_ui_atoms_.emplace_back (
        header.port_index,
        std::span<const std::byte>{
          ui_drain_scratch_.data (), static_cast<size_t> (header.size) });
    }
  return true;
}

void
Lv2Plugin::Lv2PluginImpl::push_ui_event (
  uint32_t                   port_index,
  uint32_t                   protocol,
  std::span<const std::byte> body) noexcept
{
  if (body.size () > kAtomBufferSize)
    {
      plugin_to_ui_dropped_.fetch_add (1, std::memory_order_relaxed);
      return;
    }
  const auto header =
    UiEventHeader{ port_index, protocol, static_cast<uint32_t> (body.size ()) };
  if (
    !fifo_write_record (
      plugin_to_ui_fifo_, plugin_to_ui_buf_,
      { reinterpret_cast<const std::byte *> (&header), sizeof (header) }, body))
    {
      plugin_to_ui_dropped_.fetch_add (1, std::memory_order_relaxed);
    }
}

void
Lv2Plugin::Lv2PluginImpl::dispatch_atom_outputs_to_ui () noexcept
{
  // Acquire pairs with the release stores at session open/teardown:
  // the ring reset of a freshly opened session must be visible before
  // its first record is pushed. A call that already passed this check
  // during teardown can still push after the next session's reset, at
  // worst desyncing the byte stream — the read-side size validation
  // detects that and stops the drain — and the rings are reset again
  // at the next session open; only paused processing (the sample-rate
  // recreation path) fully excludes the audio thread
  if (!ui_dispatch_.load (std::memory_order_acquire))
    return;

  // The sequence and its events carry the same validation as the MIDI
  // output parser
  const auto dispatch_sequence =
    [this] (uint32_t port_index, const uint8_t * buf, size_t buf_size) noexcept {
      for_each_atom_output_event (buf, buf_size, [&] (const LV2_Atom_Event * ev) {
        push_ui_event (
          port_index, host_urids_.atom_eventTransfer,
          { reinterpret_cast<const std::byte *> (&ev->body),
            sizeof (LV2_Atom) + ev->body.size });
      });
    };

  if (midi_out_port_idx_ >= 0)
    {
      dispatch_sequence (
        ports_[midi_out_port_idx_].index, atom_out_buf_.data (),
        atom_out_buf_.size ());
    }
  for (const auto &port : ports_)
    {
      if (
        !port.has_atom_scratch || port.flow != dsp::PortFlow::Output
        || (port.type != PortInfo::Type::Atom && port.type != PortInfo::Type::Unknown))
        continue;
      dispatch_sequence (
        port.index, atom_scratch_buf_.data () + port.atom_scratch_byte_offset,
        atom_port_capacity (port));
    }
}

void
to_json (nlohmann::json &j, const Lv2Plugin &p)
{
  to_json (j, static_cast<const Plugin &> (p));
  auto state = p.save_state_impl ();
  if (!state.empty ())
    j[Lv2Plugin::kStateKey] = std::move (state);
}

void
from_json (const nlohmann::json &j, Lv2Plugin &p)
{
  // State must be deserialized first, because the Plugin deserialization
  // may cause an instantiation
  if (j.contains (Lv2Plugin::kStateKey))
    p.load_state_impl (j[Lv2Plugin::kStateKey].get<std::string> ());

  from_json (j, static_cast<Plugin &> (p));
}

} // namespace zrythm::plugins
