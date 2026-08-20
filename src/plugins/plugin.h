// SPDX-FileCopyrightText: © 2018-2022, 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <variant>
#include <vector>

#include "dsp/parameter.h"
#include "dsp/port_all.h"
#include "dsp/processor_base.h"
#include "plugins/plugin_configuration.h"
#include "plugins/plugin_descriptor.h"
#include "plugins/plugin_host_window.h"
#include "utils/main_thread_dispatcher.h"
#include "utils/qt.h"
#include "utils/registry_utils.h"
#include "utils/variant_helpers.h"

#include <QPointer>

namespace juce
{
class AudioProcessLoadMeasurer;
}

namespace zrythm::plugins
{

/**
 * @brief Handlers for plugin requests towards the host.
 *
 * Owned by the project and shared with all plugins via dependency
 * injection; an empty std::function means the request is dropped.
 *
 * Handlers are invoked on the main thread (posted via the shared main
 * thread dispatcher when called from other threads).
 */
struct PluginHostMainThreadCallbacks
{
  /** Requests a recalculation of the processing graph (latency changes). */
  std::function<void ()> latency_recalc_;

  /**
   * @brief Requests a hard rebuild of the processing graph (port topology
   * changes).
   *
   * Unlike latency_recalc_, this re-prepares all nodes; required after a
   * bus reconciliation created, detached or reconfigured ports.
   */
  std::function<void ()> graph_recalc_;

  /**
   * @brief Runs the given function with audio processing paused, resuming
   * afterwards (e.g. for plugin-initiated restarts).
   *
   * Called on the main thread; the given function is executed
   * synchronously before this handler returns.
   */
  std::function<void (std::function<void ()>)> with_paused_processing_;
};

/**
 * @brief DSP processing plugin.
 *
 * Can be external or internal.
 *
 * ## Plugin State Persistence
 *
 * All plugin types serialize their state as base64-encoded data within the
 * project JSON file via their `to_json`/`from_json` functions. There are no
 * separate state files on disk — JSON serialization is the sole persistence
 * mechanism.
 *
 * Each plugin subclass includes a "state" key in its `to_json()` output
 * containing the base64-encoded plugin state. During `from_json()`, the state
 * is deserialized first and stored in a temporary member (e.g.,
 * `state_to_apply_`), then applied after the plugin instance is fully
 * initialized. No filesystem paths or state directories are involved.
 */
class Plugin : public dsp::ProcessorBase
{
  Q_OBJECT
  Q_PROPERTY (
    int presetIndex READ presetIndex WRITE setPresetIndex NOTIFY
      presetIndexChanged)
  Q_PROPERTY (bool presetDirty READ presetDirty NOTIFY presetDirtyChanged)
  Q_PROPERTY (
    zrythm::plugins::PluginConfiguration * configuration READ configuration
      NOTIFY configurationChanged)
  Q_PROPERTY (
    zrythm::dsp::ProcessorParameter * bypassParameter READ bypassParameter
      CONSTANT)
  Q_PROPERTY (
    zrythm::dsp::ProcessorParameter * gainParameter READ gainParameter CONSTANT)
  Q_PROPERTY (
    bool uiVisible READ uiVisible WRITE setUiVisible NOTIFY uiVisibleChanged)
  Q_PROPERTY (bool hasNativeUi READ hasNativeUi NOTIFY hasNativeUiChanged)
  Q_PROPERTY (
    bool bypassed READ bypassed WRITE setBypassed NOTIFY bypassedChanged)
  Q_PROPERTY (bool abActive READ abActive NOTIFY abActiveChanged)
  Q_PROPERTY (
    InstantiationStatus instantiationStatus READ instantiationStatus NOTIFY
      instantiationStatusChanged)
  Q_PROPERTY (
    int latencySamples READ latencySamples NOTIFY latencySamplesChanged)
  QML_ELEMENT
  QML_UNCREATABLE ("")

  Q_DISABLE_COPY_MOVE (Plugin)
public:
  enum class InstantiationStatus : std::uint8_t
  {
    Pending,    ///< Instantiation underway
    Successful, ///< Instantiation successful
    Failed      ///< Instantiation failed
  };
  Q_ENUM (InstantiationStatus)

  /**
   * @brief Durable preset identifier, defined by the backend.
   *
   * Indexed formats (VST3 programs) use the program index. Key/URI-based
   * formats (CLAP, LV2) use an opaque backend-defined string.
   */
  using PresetId = std::variant<int, QString>;

  /**
   * @brief A single selectable preset entry.
   */
  struct PresetEntry
  {
    /** Display name. */
    QString name;

    /** Optional grouping name (e.g. a VST3 program list name), or empty
     * when the backend provides no grouping. */
    QString group;

    /** Durable identifier used for selection and application. */
    PresetId id;

    bool operator== (const PresetEntry &) const = default;
  };

  ~Plugin () override;

  // ============================================================================
  // QML Interface
  // ============================================================================

  /**
   * @brief Index of the selected preset within @ref presetEntries, or -1 if
   * no preset is selected.
   *
   * Selection is stored as a durable @ref PresetId and resolved against the
   * current entries on each read, so this returns -1 if the stored preset no
   * longer exists (e.g. a discovered preset was removed between sessions).
   */
  int presetIndex () const;

  /**
   * @brief Selects a preset by index within @ref presetEntries and applies
   * it to the underlying plugin via @ref apply_preset_impl.
   *
   * Passing -1 clears the selection (host-side display state only; plugin
   * formats have no "unselect", so implementations are not notified).
   * Re-selecting the current preset re-applies it, reverting the plugin's
   * state to the preset. Out-of-range indices are refused with a warning.
   */
  void setPresetIndex (int index);

  /**
   * @brief Emitted when the selected preset changed (in either direction).
   */
  Q_SIGNAL void presetIndexChanged (int index);

  /**
   * @brief Returns the list of selectable presets.
   *
   * The default implementation returns an empty list (no preset support).
   * Implementations return a view over cached entries and call @ref
   * notify_presets_rebuilt when the list content changes.
   */
  virtual std::span<const PresetEntry> presetEntries () const { return {}; }

  /**
   * @brief Emitted by implementations when the preset list content changed.
   */
  Q_SIGNAL void presetsChanged ();

  /**
   * @brief Whether the plugin's current parameter state has diverged from
   * the selected preset.
   *
   * This is a host-side heuristic (plugin formats do not report it): the
   * base class tracks host-side user edits (see
   * ProcessorParameter::baseValueEditedByUser), implementations report
   * edits made from the plugin's own UI (e.g. on gesture start), and it is
   * cleared whenever a preset is selected.
   */
  bool presetDirty () const { return preset_dirty_; }

  /**
   * @brief Emitted when @ref presetDirty changed.
   */
  Q_SIGNAL void presetDirtyChanged (bool dirty);

  PluginConfiguration * configuration () const { return configuration_.get (); }
  /**
   * @brief Emitted when the configuration is set on the plugin.
   *
   * Implementing plugins should attach to this and, if
   * @p generateNewPluginPortsAndParams is true, query the underlying plugin
   * for its port/parameter layout and create corresponding Zrythm objects.
   * If false, ports and parameters already exist (e.g., after JSON
   * deserialization) and only the underlying plugin instance needs to be
   * reinitialized.
   *
   * @param configuration The plugin configuration.
   * @param generateNewPluginPortsAndParams Whether the handler should create
   *   new Zrythm ports and parameters from the underlying plugin.
   */
  Q_SIGNAL void configurationChanged (
    PluginConfiguration * configuration,
    bool                  generateNewPluginPortsAndParams);

  dsp::ProcessorParameter * bypassParameter () const;
  dsp::ProcessorParameter * gainParameter () const;

  /**
   * @brief Whether the plugin is currently bypassed, based on the bypass
   * parameter.
   */
  bool bypassed () const;
  void setBypassed (bool bypassed);

  /**
   * @brief Emitted when the plugin's bypass state changed (in either
   * direction).
   */
  Q_SIGNAL void bypassedChanged (bool bypassed);

  /**
   * @brief Whether the A/B comparison state is currently on slot B.
   */
  bool abActive () const { return ab_b_active_; }

  /**
   * @brief Emitted when the active A/B slot changed.
   */
  Q_SIGNAL void abActiveChanged (bool b_active);

  /**
   * @brief Saves the current state into the active A/B slot and switches
   * to the other slot (initialized as a copy of the current state on first
   * use).
   */
  Q_INVOKABLE void switchAbState ();

  /**
   * @brief Returns the plugin's recent processing load as a percentage of
   * the block budget.
   *
   * Exponential moving average over recent blocks (measured against each
   * block's actual length, including passthrough while bypassed but
   * excluding parameter processing), clamped to 0-100, so it is not
   * directly comparable to the engine-wide DSP meter. No change
   * notification is emitted; the value must be polled.
   */
  Q_INVOKABLE double dspLoadPercentage () const;

  /**
   * @brief Returns the plugin's own playback latency in samples (not
   * including route latency).
   */
  int latencySamples () const;

  /**
   * @brief Emitted on the main thread when the plugin's reported latency
   * changed.
   */
  Q_SIGNAL void latencySamplesChanged (int latency_samples);

  bool uiVisible () const { return visible_; }
  void setUiVisible (bool visible)
  {
    if (visible == visible_)
      return;

    visible_ = visible;
    Q_EMIT uiVisibleChanged (visible);
  }

  /**
   * @brief Implementations should listen to this and show/hide the plugin UI
   * accordingly.
   */
  Q_SIGNAL void uiVisibleChanged (bool visible);

protected:
  /**
   * @brief Shows/hides the plugin UI to match @ref uiVisible.
   *
   * Connected to @ref uiVisibleChanged by the base class, and additionally
   * invoked via a queued connection after the configuration is set if the UI
   * was already marked visible (e.g., during project deserialization), so
   * that callers finish setting up before native windows are created.
   */
  virtual void on_ui_visibility_changed () { }

public:
  /**
   * @brief Whether the plugin provides its own native editor UI.
   *
   * When false, a generic parameter UI is shown instead when @ref uiVisible
   * is set.
   */
  virtual bool hasNativeUi () const { return false; }

  /**
   * @brief Whether the native editor UI is both provided and presentable.
   *
   * Wraps @ref hasNativeUi with presentation failures reported via @ref
   * set_native_ui_unavailable (e.g., no usable windowing connection). When
   * false while @ref uiVisible is set, the generic parameter UI is shown.
   */
  bool hasPresentableNativeUi () const
  {
    return hasNativeUi () && !native_ui_unavailable_;
  }

  /**
   * @brief Emitted by implementations when @ref hasNativeUi may have changed
   * (e.g., after asynchronous instantiation completes).
   */
  Q_SIGNAL void hasNativeUiChanged ();

  InstantiationStatus instantiationStatus () const
  {
    return instantiation_status_;
  }
  Q_SIGNAL void instantiationStatusChanged (InstantiationStatus status);

  /**
   * @brief To be emitted by implementations when instantiation finished.
   *
   * This is used since some plugins do instantiation asynchronously. The
   * project should re-calculate the DSP graph when this gets fired.
   *
   * @param error Non-empty on instantiation failure.
   */
  Q_SIGNAL void instantiationFinished (bool successful, const QString &error);

  // ============================================================================

  PluginDescriptor &get_descriptor () const
  {
    return *configuration ()->descr_;
  }
  utils::Utf8String get_name () const
  {
    return configuration ()->descr_->name_;
  }
  Protocol::ProtocolType get_protocol () const
  {
    return configuration ()->descr_->protocol_;
  }

  /**
   * @brief Sets the plugin configuration to use.
   *
   * This must be called exactly once right after construction to set the
   * PluginConfiguration for this plugin.
   *
   * When called during fresh construction, the plugin will have no ports or
   * parameters and the signal will instruct handlers to create them. When
   * called during deserialization (after ports/params are already restored
   * from JSON), the signal will instruct handlers to skip creation and only
   * reinitialize the underlying plugin instance.
   */
  void set_configuration (const PluginConfiguration &setting);

  // ============================================================================
  // IProcessable Interface
  // ============================================================================

  void custom_prepare_for_processing (
    const dsp::graph::GraphNode * node,
    units::sample_rate_t          sample_rate,
    units::sample_u32_t           max_block_length) final;

  [[gnu::hot]] void custom_process_block (
    dsp::graph::ProcessBlockInfo time_nfo,
    const dsp::ITransport       &transport,
    const dsp::TempoMap         &tempo_map) noexcept final;

  void custom_release_resources () final;

  // ============================================================================

  /**
   * Returns whether the plugin is enabled (not bypassed).
   */
  bool currently_enabled () const
  {
    const auto * bypass = bypassParameter ();
    return !bypass->range ().isToggled (bypass->currentValue ());
  }

  bool currently_enabled_rt () const noexcept [[clang::nonblocking]]
  {
    const auto * bypass = bypass_param_rt_;
    return !bypass->range ().isToggled (bypass->currentValue ());
  }

  // ============================================================================
  // Implementation Interface
  // ============================================================================

public:
  /**
   * @brief Serializes the plugin's internal state to a base64-encoded string.
   *
   * @return Base64-encoded state, or empty string if no state is available.
   */
  std::string save_state () const;

  /**
   * @brief Loads a previously saved state into the plugin.
   *
   * If the plugin is instantiated, the state is applied immediately on the
   * main thread and Zrythm's parameter values are synced to the loaded
   * state. Otherwise, the state is stashed and applied during
   * instantiation.
   *
   * @return False if the plugin rejected the state (a deferred application
   * counts as success).
   */
  bool load_state (const std::string &base64_state);

  /**
   * @brief Flushes plugin-reported parameter values to Zrythm params.
   *
   * Called on the main thread by a shared project-wide timer (~20ms).
   * No-op unless values are pending: exchanges pending_value atomics and
   * calls setBaseValue() for any non-sentinel values.
   */
  void flush_plugin_values ();

protected:
  /**
   * @brief Stores a plugin-reported value to be applied to the Zrythm
   * param on the next main thread flush.
   *
   * Realtime-safe. @p index is an index into the live params (same as the
   * ParamSync entries).
   */
  void set_param_pending_from_plugin (size_t index, float value) noexcept
    [[clang::nonblocking]];

public:
  // ============================================================================
  // Main Thread Services
  // ============================================================================

  /**
   * @brief Sets the project-owned services used to fulfill host-bound
   * requests on the main thread.
   *
   * @param dispatcher Shared closure dispatcher. PluginFactory sets this
   * at construction; tests constructing plugins directly must call this
   * before any host-bound request can be posted.
   * @param callbacks Handlers for specific requests; empty std::functions
   * mean the corresponding request is dropped.
   */
  void set_main_thread_services (
    utils::MainThreadClosureDispatcher &dispatcher,
    PluginHostMainThreadCallbacks       callbacks)
  {
    main_thread_dispatcher_ = &dispatcher;
    main_thread_callbacks_ = std::move (callbacks);
  }

  /**
   * @brief Posts an action to run on the main thread as long as this
   * plugin still exists when the action runs.
   *
   * The action is silently dropped if the plugin no longer exists when the
   * dispatcher pump runs, so @p action may safely capture `this` or plugin
   * members.
   *
   * Realtime-safe: no allocations or locks (the weak self-reference was
   * primed at construction). When called on the main thread, the action
   * runs synchronously, after any already-queued actions.
   *
   * @return False if the action was dropped because the dispatcher queue
   * was full.
   */
  template <typename F>
    requires std::is_nothrow_move_constructible_v<std::decay_t<F>>
             && std::is_nothrow_copy_constructible_v<std::decay_t<F>>
  bool post_main_thread_action (F &&action) noexcept [[clang::nonblocking]]
  {
    assert (main_thread_dispatcher_ != nullptr);
    return main_thread_dispatcher_->post (
      guard_main_thread_action (std::forward<F> (action)));
  }

  /**
   * @brief Posts an action to run on the main thread once the posting call
   * stack has unwound.
   *
   * Same guarantees as @ref post_main_thread_action, except the action is
   * always run by a later dispatcher pump, never inline. Required for
   * actions posted from a callback the plugin invoked on us, which may only
   * touch the plugin after its own call has returned.
   *
   * @return False if the action was dropped because the dispatcher queue
   * was full.
   */
  template <typename F>
    requires std::is_nothrow_move_constructible_v<std::decay_t<F>>
             && std::is_nothrow_copy_constructible_v<std::decay_t<F>>
  bool
  post_main_thread_action_deferred (F &&action) noexcept [[clang::nonblocking]]
  {
    assert (main_thread_dispatcher_ != nullptr);
    return main_thread_dispatcher_->post_deferred (
      guard_main_thread_action (std::forward<F> (action)));
  }

private:
  /**
   * @brief Wraps @p action so that it becomes a no-op if this plugin is
   * destroyed before it runs.
   */
  template <typename F>
  auto guard_main_thread_action (F &&action) noexcept [[clang::nonblocking]]
  {
    auto guarded_action =
      [guard = self_guard_, inner_action = std::forward<F> (action)] () mutable {
        if (guard == nullptr)
          return;
        std::invoke (inner_action);
      };
    static_assert (
      std::is_constructible_v<
        utils::MainThreadCallback, decltype (guarded_action)>,
      "guarded action must fit the dispatcher's inline storage");
    return guarded_action;
  }

public:
  /**
   * @brief Notifies the host that the plugin's playback latency changed.
   *
   * Safe to call from any thread: the request is posted to the main thread
   * via the shared dispatcher, which invokes the latency-recalc callback
   * (if installed) and emits latencySamplesChanged(). The request is
   * dropped if the plugin no longer exists when the request runs.
   */
  void notify_latency_changed () noexcept [[clang::nonblocking]];

protected:
  /**
   * @brief Sets whether presenting the native editor is currently
   * unavailable, forcing the generic UI while @ref uiVisible is set.
   *
   * Implementations clear this when a native presentation is attempted and
   * set it when presentation fails for environment reasons (as opposed to
   * the plugin lacking an editor). Emits @ref hasNativeUiChanged on change.
   */
  void set_native_ui_unavailable (bool unavailable)
  {
    if (native_ui_unavailable_ == unavailable)
      return;
    native_ui_unavailable_ = unavailable;
    Q_EMIT hasNativeUiChanged ();
  }

private:
  virtual void prepare_plugin_for_processing (
    units::sample_rate_t sample_rate,
    units::sample_u32_t  max_block_length) { };

  /**
   * @brief Applies a preset selection to the underlying plugin.
   *
   * Receives the selected entry's durable identifier. The default
   * implementation does nothing (no preset support).
   */
  virtual void apply_preset_impl (const PresetId &) { }

  virtual void process_impl (
    dsp::graph::ProcessBlockInfo time_info,
    const dsp::ITransport       &transport,
    const dsp::TempoMap         &tempo_map) noexcept = 0;

  virtual void release_resources_impl () { }

  /**
   * @brief Processes the plugin by passing through the input to its output.
   *
   * This is called when the plugin is bypassed.
   *
   * A default implementation is provided in case the derived class doesn't
   * override this.
   */
  [[gnu::hot]] virtual void process_passthrough_impl (
    dsp::graph::ProcessBlockInfo time_nfo,
    const dsp::ITransport       &transport,
    const dsp::TempoMap         &tempo_map) noexcept;

  virtual std::string save_state_impl () const = 0;
  virtual bool        load_state_impl (const std::string &base64_state) = 0;

  // ============================================================================

  /**
   * @brief Initializes various parameter caches used during processing.
   *
   * @note This is only needed after construction. Logic that needs to be called
   * on sample rate/buffer size changes must go in prepare_for_processing().
   */
  void init_param_caches ();

  /**
   * @brief Connects bypassedChanged to the current bypass parameter's
   * baseValueChanged, replacing any previous connection.
   */
  void arm_bypassed_relay ();

protected:
  /**
   * Creates/initializes a plugin and its internal plugin (LV2, etc.) using
   * the given setting.
   *
   * @throw ZrythmException If the plugin could not be created.
   */
  Plugin (utils::IObjectRegistry &registry, QObject * parent);

  /**
   * @brief To be called by implementations to generate the default bypass
   * parameter if the plugin does not provide its own.
   */
  dsp::ProcessorParameterUuidReference generate_default_bypass_param () const;

  /**
   * @brief To be called by implementations to generate the default gain
   * parameter if the plugin does not provide its own.
   */
  dsp::ProcessorParameterUuidReference generate_default_gain_param () const;

  /**
   * @brief Updates the selected preset without re-applying it.
   *
   * For selections made by the underlying plugin itself (e.g. from its own
   * preset browser). If the selection actually changed, clears @ref
   * presetDirty and emits @ref presetIndexChanged; if the reported preset
   * is already selected, this is a no-op (the dirty flag is kept:
   * plugin-side parameter edits can arrive alongside notifications that
   * re-report the unchanged current preset).
   */
  void update_selected_preset_from_backend (PresetId id);

  /**
   * @brief Emits @ref presetsChanged and @ref presetIndexChanged after the
   * preset entry list changed.
   *
   * The preset index is resolved against the entries on each read, so a
   * content change can move the selection to a different index or make it
   * unresolvable; both signals are emitted together so observers
   * re-evaluate both.
   */
  void notify_presets_rebuilt ();

  /**
   * @brief Sets @ref presetDirty and emits @ref presetDirtyChanged.
   *
   * No-op (forced to false) when no preset is selected.
   */
  void set_preset_dirty (bool dirty);

private:
  /**
   * @brief Connects @p param so that deliberate user edits mark the
   * selected preset dirty.
   *
   * The Zrythm-provided bypass/gain params are skipped: they are host
   * utilities, not part of the plugin's preset state.
   */
  void arm_preset_dirty_tracking_for (dsp::ProcessorParameter &param);

  static constexpr auto kConfigurationKey = "configuration"sv;
  static constexpr auto kPresetKey = "preset"sv;
  static constexpr auto kPresetDirtyKey = "presetDirty"sv;
  static constexpr auto kProtocolKey = "protocol"sv;
  static constexpr auto kVisibleKey = "visible"sv;

  /** Unique IDs of the Zrythm-provided bypass/gain parameters. */
  static constexpr auto kBypassParamUniqueId = u8"/zrythm-bypass"sv;
  static constexpr auto kGainParamUniqueId = u8"/zrythm-gain"sv;
  friend void           to_json (nlohmann::json &j, const Plugin &p);
  friend void           from_json (const nlohmann::json &j, Plugin &p);

protected:
  /** Set to true if instantiation failed and the plugin will be treated as
   * disabled. */
  bool instantiation_failed_ = false;

  // ============================================================================
  // DSP Caches
  // ============================================================================

  /**
   * @brief Bypass toggle parameter,
   *
   * If the plugin itself doesn't provide a bypass parameter, one will be
   * created for it.
   *
   * Set via @ref set_bypass_id, which also connects the bypassedChanged
   * forwarding.
   */
  std::optional<dsp::ProcessorParameter::Uuid> bypass_id_;

  /**
   * @brief Sets the bypass parameter and connects bypassedChanged to it.
   *
   * Implementations must call this whenever the bypass parameter becomes
   * known (construction, deserialization), so that bypassedChanged fires
   * on parameter writes even before the first processing preparation.
   */
  void set_bypass_id (dsp::ProcessorParameter::Uuid id);

  /**
   * @brief Zrythm-provided plugin gain parameter.
   */
  std::optional<dsp::ProcessorParameter::Uuid> gain_id_;

  /** A/B comparison state slots (base64 states) and the active flag. */
  std::string ab_state_a_;
  std::string ab_state_b_;
  bool        ab_b_active_ = false;

  /** Connection re-arming @ref bypassedChanged on the current bypass
   * parameter. */
  QMetaObject::Connection bypassed_relay_connection_;

  /** Preset-dirty tracking connections per parameter (keyed by parameter
   * UUID), so re-arming disconnects the previous connection instead of
   * stacking them. Entries are never pruned: parameters are never removed
   * once added, so the map cannot grow unboundedly. If parameter removal
   * is ever introduced, prune the corresponding entry there. */
  std::map<dsp::ProcessorParameter::Uuid, QMetaObject::Connection>
    preset_dirty_connections_;

  /* Realtime caches */
  std::vector<dsp::AudioPort *> audio_in_ports_;
  std::vector<dsp::AudioPort *> audio_out_ports_;
  std::vector<dsp::CVPort *>    cv_in_ports_;

  /** All MIDI ports, in creation order (VST3 event bus / CLAP note port
   * index). */
  std::vector<dsp::MidiPort *> midi_in_ports_;
  std::vector<dsp::MidiPort *> midi_out_ports_;

  dsp::ProcessorParameter * bypass_param_rt_{};

  // ============================================================================
  // Parameter Synchronization
  // ============================================================================

  /**
   * @brief Per-parameter state for bidirectional plugin sync.
   */
  struct ParamSync
  {
    struct Entry
    {
      /**
       * One-shot feedback guard: set when the plugin reports a value.
       * On the next audio cycle, if the computed value matches, the send
       * is skipped and the guard is consumed (reset to -1.f).
       * Written on the plugin's reporting thread (any thread), consumed on
       * the audio thread.
       */
      std::atomic<float> last_from_plugin{ -1.f };

      /**
       * Cross-thread bridge: audio thread stores normalized value with
       * release ordering; main thread exchanges with -1.f using acq_rel.
       * -1.f means no pending value.
       */
      std::atomic<float> pending_value{ -1.f };

      Entry () = default;
      Entry (const Entry &) = delete;
      Entry &operator= (const Entry &) = delete;
      Entry (Entry &&other) noexcept
          : last_from_plugin (
              other.last_from_plugin.load (std::memory_order_relaxed)),
            pending_value (other.pending_value.load (std::memory_order_relaxed))
      {
      }
      Entry &operator= (Entry &&other) noexcept
      {
        if (this != &other)
          {
            last_from_plugin.store (
              other.last_from_plugin.load (std::memory_order_relaxed),
              std::memory_order_relaxed);
            pending_value.store (
              other.pending_value.load (std::memory_order_relaxed),
              std::memory_order_relaxed);
          }
        return *this;
      }
    };

    /** Parallel to ProcessorBase's live_params_. */
    std::vector<Entry> entries;

    /** Whether any entry has a pending value (fast path for flushes). */
    std::atomic<bool> dirty{ false };

    void prepare (size_t count) { entries.resize (count); }
  };

  ParamSync param_sync_;

  // ============================================================================
  // Main Thread Services
  // ============================================================================

  /**
   * @brief Project-owned shared dispatcher for host-bound requests on the
   * main thread, or nullptr when running without a project
   * (headless/tests).
   */
  utils::MainThreadClosureDispatcher * main_thread_dispatcher_{};

  /**
   * @brief Handlers for host-bound requests on the main thread (empty
   * std::function = the request is dropped).
   */
  PluginHostMainThreadCallbacks main_thread_callbacks_{};

  // ============================================================================

private:
  /** Setting this plugin was instantiated with. */
  std::unique_ptr<PluginConfiguration> configuration_;

  /**
   * @brief Currently selected preset, if any.
   *
   * Stored as a durable @ref PresetId and resolved against @ref
   * presetEntries on read.
   */
  std::optional<PresetId> selected_preset_id_;

  /**
   * @brief Whether the current parameter state has diverged from the
   * selected preset (see @ref presetDirty).
   */
  bool preset_dirty_ = false;

  InstantiationStatus instantiation_status_{ InstantiationStatus::Pending };

  /** Whether plugin UI is opened or not. */
  bool visible_ = false;

  /** Whether presenting the native editor is unavailable (see
   * @ref set_native_ui_unavailable). */
  bool native_ui_unavailable_ = false;

  /**
   * @brief Flag to error out if set_configuration() is called more than once.
   */
  bool set_configuration_called_{};

  /**
   * @brief Weak self-reference used to drop pending main thread actions
   * after the plugin is destroyed.
   *
   * Created at construction so that the QObject weak-reference control
   * block is allocated up-front on the main thread — copying it in
   * realtime code is then just an atomic increment (no allocations).
   */
  QPointer<const Plugin> self_guard_;

  /**
   * @brief Measures this plugin's share of the audio block budget.
   *
   * Kept behind a pointer to avoid pulling JUCE headers into this header.
   */
  std::unique_ptr<juce::AudioProcessLoadMeasurer> load_measurer_;
};

class JucePlugin;
class ClapPlugin;
class Vst3Plugin;
class FaustPlugin;

using PluginVariant =
  std::variant<JucePlugin, ClapPlugin, Vst3Plugin, FaustPlugin>;
using PluginPtrVariant = utils::to_pointer_variant<PluginVariant>;

using PluginUuidReference = utils::TypedUuidReference<Plugin>;

using PluginHostWindowFactory =
  std::function<std::unique_ptr<plugins::PluginHostWindow> (Plugin &)>;

} // namespace zrythm::plugins
