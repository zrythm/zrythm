// SPDX-FileCopyrightText: © 2018-2022, 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <atomic>
#include <functional>
#include <memory>
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
class Plugin : public utils::UuidIdentifiableObject<Plugin>, public dsp::ProcessorBase
{
  Q_OBJECT
  Q_PROPERTY (
    int programIndex READ programIndex WRITE setProgramIndex NOTIFY
      programIndexChanged)
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

  ~Plugin () override;

  // ============================================================================
  // QML Interface
  // ============================================================================

  /**
   * @brief Returns the current program index, or -1 if no program exists.
   */
  int  programIndex () const { return program_index_.value_or (-1); }
  void setProgramIndex (int index)
  {
    if (program_index_.value_or (-1) == index)
      return;

    if (index >= 0)
      {
        program_index_.emplace (index);
      }
    else
      {
        program_index_.reset ();
      }

    Q_EMIT programIndexChanged (index);
  }

  /**
   * @brief Implementations should attach to this and set the program.
   */
  Q_SIGNAL void programIndexChanged (int index);

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
      [guard = self_guard_, action = std::forward<F> (action)] () mutable {
        if (guard == nullptr)
          return;
        std::invoke (action);
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
   * via the shared dispatcher. The request is dropped if no latency-recalc
   * callback is installed, or if the plugin no longer exists when the
   * request runs.
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

  virtual void
  process_impl (dsp::graph::ProcessBlockInfo time_info) noexcept = 0;

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

private:
  static constexpr auto kConfigurationKey = "configuration"sv;
  static constexpr auto kProgramIndexKey = "programIndex"sv;
  static constexpr auto kProtocolKey = "protocol"sv;
  static constexpr auto kVisibleKey = "visible"sv;
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
   * @brief Currently selected program index.
   */
  std::optional<int> program_index_;

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
};

class CarlaNativePlugin;
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

DEFINE_UUID_HASH_SPECIALIZATION (zrythm::plugins::Plugin::Uuid)
