// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "plugins/plugin.h"

#include <juce_wrapper.h>

namespace zrythm::plugins
{

/**
 * @brief JUCE-based plugin host implementation.
 *
 * This class provides hosting capabilities for VST3, AU, and other
 * JUCE-supported plugin formats using the JUCE 8 framework.
 */
class JucePlugin : public Plugin
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  using CreatePluginInstanceAsyncFunc = std::function<void (
    const juce::PluginDescription &,
    double,
    int,
    juce::AudioPluginFormat::AudioPluginFormat::PluginCreationCallback)>;

  /**
   * @brief Constructor for JucePlugin.
   *
   * @param dependencies Processor dependencies
   * @param state_path_provider Function to provide state directory path
   * @param format_manager JUCE format manager for plugin creation
   * @param sample_rate_provider Function to provide current sample rate
   * @param buffer_size_provider Function to provide current buffer size
   * @param parent Parent QObject
   */
  JucePlugin (
    dsp::ProcessorBase::ProcessorBaseDependencies dependencies,
    StateDirectoryParentPathProvider              state_path_provider,
    CreatePluginInstanceAsyncFunc          create_plugin_instance_async_func,
    std::function<units::sample_rate_t ()> sample_rate_provider,
    std::function<nframes_t ()>            buffer_size_provider,
    PluginHostWindowFactory                top_level_window_provider,
    QObject *                              parent = nullptr);

  ~JucePlugin () override;

  // ============================================================================
  // Plugin Interface Implementation
  // ============================================================================

  void save_state (std::optional<fs::path> abs_state_dir) override;
  void load_state (std::optional<fs::path> abs_state_dir) override;

  nframes_t get_single_playback_latency () const override;

protected:
  void prepare_for_processing_impl (
    units::sample_rate_t sample_rate,
    nframes_t            max_block_length) override;

  void process_impl (EngineProcessTimeInfo time_info) noexcept override;

private Q_SLOTS:
  /**
   * @brief Handle configuration changes.
   */
  void on_configuration_changed ();

  /**
   * @brief Handle visibility changes.
   */
  void on_ui_visibility_changed ();

private:
  /**
   * @brief Initialize the JUCE plugin instance asynchronously.
   *
   * @throw ZrythmException if plugin initialization fails
   */
  void initialize_juce_plugin_async ();

  /**
   * @brief Create audio and MIDI ports based on the JUCE plugin's configuration.
   */
  void create_ports_from_juce_plugin ();

  /**
   * @brief Create parameters from the JUCE plugin's parameter list.
   */
  void create_parameters_from_juce_plugin ();

  /**
   * @brief Update parameter values from JUCE to Zrythm.
   */
  void update_parameter_values ();

  /**
   * @brief Update parameter values from Zrythm to JUCE.
   */
  void sync_parameters_to_juce ();

  /**
   * @brief Show the plugin's editor window.
   */
  void show_editor ();

  /**
   * @brief Hide the plugin's editor window.
   */
  void hide_editor ();

  static constexpr auto kStateKey = "state"sv;
  friend void           to_json (nlohmann::json &j, const JucePlugin &p)
  {
    to_json (j, static_cast<const Plugin &> (p));
    if (p.juce_plugin_)
      {
        juce::MemoryBlock state_data;
        p.juce_plugin_->getStateInformation (state_data);
        auto encoded = state_data.toBase64Encoding ();
        j[kStateKey] = encoded.toStdString ();
      }
  }
  friend void from_json (const nlohmann::json &j, JucePlugin &p)
  {
    // Note that state must be deserialized first, because the Plugin
    // deserialization may cause an instantiation
    std::string state_str;
    j[kStateKey].get_to (state_str);
    p.state_to_apply_.emplace ();
    p.state_to_apply_->fromBase64Encoding (state_str);

    // Now that we have the state, continue deserializing base Plugin...
    from_json (j, static_cast<Plugin &> (p));
  }

private:
  // ============================================================================
  // JUCE-specific members
  // ============================================================================

  CreatePluginInstanceAsyncFunc create_plugin_instance_async_func_;

  std::unique_ptr<juce::AudioPluginInstance>  juce_plugin_;
  std::unique_ptr<juce::AudioProcessorEditor> editor_;
  std::unique_ptr<plugins::IPluginHostWindow> top_level_window_;

  std::function<units::sample_rate_t ()> sample_rate_provider_;
  std::function<nframes_t ()>            buffer_size_provider_;

  juce::AudioBuffer<float> juce_audio_buffer_;
  juce::MidiBuffer         juce_midi_buffer_;

  class JuceParamListener : public juce::AudioProcessorParameter::Listener
  {
  public:
    JuceParamListener (dsp::ProcessorParameter &zrythm_param)
        : zrythm_param_ (zrythm_param)
    {
    }

    void parameterValueChanged (int parameterIndex, float newValue)
      [[clang::blocking]] override
    {
      // FIXME: this may get called from the audio thread, according to the
      // docs
      // The logic here is not realtime-safe, and currently no test hits this
      z_debug (
        "{}: Parameter on JUCE side changed to {}",
        zrythm_param_.get_node_name (), newValue);
      zrythm_param_.setBaseValue (newValue);
    }
    void parameterGestureChanged (int parameterIndex, bool gestureIsStarting)
      [[clang::blocking]] override
    {
      // FIXME: this may get called from the audio thread, according to the
      // docs
      // The logic here is not realtime-safe, and currently no test hits this
    }

  private:
    dsp::ProcessorParameter &zrythm_param_;
  };

  // Parameter mapping between JUCE and Zrythm
  struct ParameterMapping
  {
    juce::AudioProcessorParameter *    juce_param;
    dsp::ProcessorParameter *          zrythm_param;
    int                                juce_param_index;
    std::unique_ptr<JuceParamListener> juce_param_listener;
  };

  std::vector<ParameterMapping> parameter_mappings_;

  // Audio/MIDI buffer management
  std::vector<float *> input_channels_;
  std::vector<float *> output_channels_;

  bool juce_initialized_ = false;
  bool editor_visible_ = false;
  bool plugin_loading_ = false;

  // Optional state to apply on instantiation (this is mainly used as a
  // temporary space to store the restored state temporarily until the plugin is
  // instantiated).
  std::optional<juce::MemoryBlock> state_to_apply_;

  PluginHostWindowFactory top_level_window_provider_;
};

} // namespace zrythm::plugins
