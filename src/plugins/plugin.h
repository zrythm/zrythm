// SPDX-FileCopyrightText: © 2018-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
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
#include "plugins/plugin_slot.h"

namespace zrythm::plugins
{

/**
 * @brief DSP processing plugin.
 *
 * Can be external or internal.
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

  Z_DISABLE_COPY_MOVE (Plugin)
public:
  /**
   * @brief Returns the parent path where the plugin should save its state
   * directory in (or load it).
   */
  using StateDirectoryParentPathProvider = std::function<fs::path ()>;

  enum class InstantiationStatus : std::uint8_t
  {
    Pending,    ///< Instantiation underway
    Successful, ///< Instantiation successful
    Failed      ///< Instantiation failed
  };
  Q_ENUM (InstantiationStatus)

  ~Plugin () override = default;

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
   * Implementing plugins should attach to this and create all the plugin
   * ports and parameters.
   */
  Q_SIGNAL void configurationChanged (PluginConfiguration * configuration);

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
   * @brief Sets the plugin setting to use.
   *
   * This must be called exactly once right after construction to set the
   * PluginConfiguration for this plugin.
   *
   * The plugin will have no ports or parameters until this is called.
   */
  void set_configuration (const PluginConfiguration &setting);

  // ============================================================================
  // IProcessable Interface
  // ============================================================================

  void custom_prepare_for_processing (
    sample_rate_t sample_rate,
    nframes_t     max_block_length) final;

  [[gnu::hot]] void
  custom_process_block (EngineProcessTimeInfo time_nfo) noexcept final;

  void custom_release_resources () final;

  // ============================================================================

  fs::path get_state_directory () const
  {
    return state_dir_parent_path_provider_ ()
           / fs::path (
             type_safe::get (get_uuid ())
               .toString (QUuid::WithoutBraces)
               .toStdString ());
  }

  /**
   * Returns whether the plugin is enabled (not bypassed).
   */
  bool currently_enabled () const
  {
    const auto * bypass = bypassParameter ();
    return !bypass->range ().is_toggled (bypass->currentValue ());
  }

  // ============================================================================
  // Implementation Interface
  // ============================================================================

public:
  /**
   * Saves the state inside the standard state directory.
   *
   * @param abs_state_dir If passed, the state will be savedinside this
   * directory instead of the plugin's state directory. Used when saving
   * presets.
   *
   * @throw ZrythmException If the state could not be saved.
   */
  virtual void save_state (std::optional<fs::path> abs_state_dir) = 0;

  /**
   * Load the state from the default directory or from @p abs_state_dir if given.
   *
   * @throw ZrythmException If the state could not be saved.
   */
  virtual void load_state (std::optional<fs::path> abs_state_dir) = 0;

private:
  virtual void prepare_for_processing_impl (
    sample_rate_t sample_rate,
    nframes_t     max_block_length) { };

  virtual void process_impl (EngineProcessTimeInfo time_info) noexcept = 0;

  virtual void release_resources_impl () { }

  /**
   * @brief Processes the plugin by passing through the input to its output.
   *
   * This is called when the plugin is bypassed.
   *
   * A default implementation is provided in case the derived class doesn't
   * override this.
   */
  [[gnu::hot]] virtual void
  process_passthrough_impl (EngineProcessTimeInfo time_nfo) noexcept;

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
  Plugin (
    ProcessorBaseDependencies        dependencies,
    StateDirectoryParentPathProvider state_path_provider,
    QObject *                        parent);

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
  static constexpr auto kSettingKey = "setting"sv;
  static constexpr auto kProgramIndexKey = "programIndex"sv;
  static constexpr auto kVisibleKey = "visible"sv;
  friend void           to_json (nlohmann::json &j, const Plugin &p)
  {
    to_json (j, static_cast<const UuidIdentifiableObject &> (p));
    to_json (j, static_cast<const dsp::ProcessorBase &> (p));
    j[kSettingKey] = p.configuration_;
    j[kProgramIndexKey] = p.program_index_;
    j[kVisibleKey] = p.visible_;
  }
  friend void from_json (const nlohmann::json &j, Plugin &p);

protected:
  /** Set to true if instantiation failed and the plugin will be treated as
   * disabled. */
  bool instantiation_failed_ = false;

  /**
   * @brief Plugins should create a state directory with their UUID as the
   * directory name under the path provided by this.
   */
  StateDirectoryParentPathProvider state_dir_parent_path_provider_;

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

  /* Caches - avoid shared_ptr due to performance cost */
  std::vector<dsp::AudioPort *> audio_in_ports_;
  std::vector<dsp::AudioPort *> audio_out_ports_;
  std::vector<dsp::CVPort *>    cv_in_ports_;
  dsp::MidiPort *               midi_in_port_{};
  dsp::MidiPort *               midi_out_port_{};

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
