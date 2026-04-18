// SPDX-FileCopyrightText: © 2018-2022, 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <memory>
#include <vector>

#include "dsp/parameter.h"
#include "dsp/port_all.h"
#include "dsp/port_span.h"
#include "dsp/processor_base.h"
#include "plugins/iplugin_host_window.h"
#include "plugins/plugin_configuration.h"
#include "plugins/plugin_descriptor.h"

namespace zrythm::plugins
{

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
class Plugin
    : public QObject,
      public dsp::ProcessorBase,
      public utils::UuidIdentifiableObject<Plugin>
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

  dsp::ProcessorParameter * bypassParameter () const
  {
    return std::get<dsp::ProcessorParameter *> (
      dependencies ().param_registry_.find_by_id_or_throw (bypass_id_.value ()));
  }
  dsp::ProcessorParameter * gainParameter () const
  {
    return std::get<dsp::ProcessorParameter *> (
      dependencies ().param_registry_.find_by_id_or_throw (gain_id_.value ()));
  }

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
    dsp::graph::EngineProcessTimeInfo time_nfo,
    const dsp::ITransport            &transport,
    const dsp::TempoMap              &tempo_map) noexcept final;

  void custom_release_resources () final;

  // ============================================================================

  /**
   * Returns whether the plugin is enabled (not bypassed).
   */
  bool currently_enabled () const
  {
    const auto * bypass = bypassParameter ();
    return !bypass->range ().is_toggled (bypass->currentValue ());
  }

  bool currently_enabled_rt () const noexcept [[clang::nonblocking]]
  {
    const auto * bypass = bypass_param_rt_;
    return !bypass->range ().is_toggled (bypass->currentValue ());
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
   * @brief Queues a previously saved state to be applied to the plugin.
   *
   * The state will be applied during the next processing cycle.
   */
  void load_state (const std::string &base64_state);

private:
  virtual void prepare_for_processing_impl (
    units::sample_rate_t sample_rate,
    units::sample_u32_t  max_block_length) { };

  virtual void
  process_impl (dsp::graph::EngineProcessTimeInfo time_info) noexcept = 0;

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
    dsp::graph::EngineProcessTimeInfo time_nfo,
    const dsp::ITransport            &transport,
    const dsp::TempoMap              &tempo_map) noexcept;

  virtual std::string save_state_impl () const = 0;
  virtual void        load_state_impl (const std::string &base64_state) = 0;

  // ============================================================================

  /**
   * @brief Initializes various parameter caches used during processing.
   *
   * @note This is only needed after construction. Logic that needs to be called
   * on sample rate/buffer size changes must go in prepare_for_processing().
   */
  void init_param_caches ();

protected:
  /**
   * Creates/initializes a plugin and its internal plugin (LV2, etc.) using
   * the given setting.
   *
   * @throw ZrythmException If the plugin could not be created.
   */
  Plugin (ProcessorBaseDependencies dependencies, QObject * parent);

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
   */
  std::optional<dsp::ProcessorParameter::Uuid> bypass_id_;

  /**
   * @brief Zrythm-provided plugin gain parameter.
   */
  std::optional<dsp::ProcessorParameter::Uuid> gain_id_;

  /* Realtime caches */
  std::vector<dsp::AudioPort *> audio_in_ports_;
  std::vector<dsp::AudioPort *> audio_out_ports_;
  std::vector<dsp::CVPort *>    cv_in_ports_;
  dsp::MidiPort *               midi_in_port_{};
  dsp::MidiPort *               midi_out_port_{};
  dsp::ProcessorParameter *     bypass_param_rt_{};

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

  /**
   * @brief Flag to error out if set_configuration() is called more than once.
   */
  bool set_configuration_called_{};
};

class CarlaNativePlugin;
class JucePlugin;
class ClapPlugin;
class InternalPluginBase;

using PluginVariant = std::variant<JucePlugin, ClapPlugin, InternalPluginBase>;
using PluginPtrVariant = to_pointer_variant<PluginVariant>;

// TODO: consider having a ProcessorRegistry instead for all
// ProcessorBase-derived classes
using PluginRegistry = utils::OwningObjectRegistry<PluginPtrVariant, Plugin>;
using PluginUuidReference = utils::UuidReference<PluginRegistry>;

using PluginHostWindowFactory =
  std::function<std::unique_ptr<plugins::IPluginHostWindow> (Plugin &)>;

} // namespace zrythm::plugins

DEFINE_UUID_HASH_SPECIALIZATION (zrythm::plugins::Plugin::Uuid)

void
from_json (const nlohmann::json &j, zrythm::plugins::PluginRegistry &registry);
