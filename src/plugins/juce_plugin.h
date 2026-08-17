// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "plugins/plugin.h"

#include <juce_audio_processors_headless/juce_audio_processors_headless.h>

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
   * @param create_plugin_instance_async_func Function to create plugin instance
   * @param sample_rate_provider Function to provide current sample rate
   * @param buffer_size_provider Function to provide current buffer size
   * @param parent Parent QObject
   */
  JucePlugin (
    utils::IObjectRegistry                &registry,
    CreatePluginInstanceAsyncFunc          create_plugin_instance_async_func,
    std::function<units::sample_rate_t ()> sample_rate_provider,
    std::function<units::sample_u32_t ()>  buffer_size_provider,
    QObject *                              parent = nullptr);

  ~JucePlugin () override;

  // ============================================================================
  // Plugin Interface Implementation
  // ============================================================================

  units::sample_u32_t get_single_playback_latency () const override;

  bool hasNativeUi () const override;

protected:
  void prepare_plugin_for_processing (
    units::sample_rate_t sample_rate,
    units::sample_u32_t  max_block_length) override;

  void process_impl (
    dsp::graph::ProcessBlockInfo time_info,
    const dsp::ITransport &,
    const dsp::TempoMap &) noexcept override;

  std::string save_state_impl () const override;
  bool        load_state_impl (const std::string &base64_state) override;

private Q_SLOTS:
  /**
   * @brief Handle configuration changes.
   */
  void on_configuration_changed (
    PluginConfiguration * configuration,
    bool                  generateNewPluginPortsAndParams);

private:
  /**
   * @brief Initialize the JUCE plugin instance asynchronously.
   *
   * @throw ZrythmException if plugin initialization fails
   */
  void initialize_juce_plugin_async (bool generateNewPluginPortsAndParams);

  /**
   * @brief Create audio and MIDI ports based on the JUCE plugin's configuration.
   */
  void create_ports_from_juce_plugin ();

  /**
   * @brief Create parameters from the JUCE plugin's parameter list.
   */
  void create_parameters_from_juce_plugin ();

  /**
   * @brief Send changed parameter values from Zrythm to JUCE.
   *
   * Uses change_tracker() to only send params that changed since the last
   * cycle, with feedback prevention via param_sync_ last_from_plugin guards.
   * Called on the audio thread during process_impl().
   */
  void sync_changed_params_to_juce () noexcept [[clang::nonblocking]];

  /**
   * @brief Read back JUCE parameter values into Zrythm's base values.
   *
   * Called after a state load so Zrythm's parameters reflect the loaded
   * state. Main thread only.
   */
  void sync_param_values_from_juce ();

  static constexpr auto kStateKey = "state"sv;
  friend void           to_json (nlohmann::json &j, const JucePlugin &p);
  friend void           from_json (const nlohmann::json &j, JucePlugin &p);

private:
  // ============================================================================
  // JUCE-specific members
  // ============================================================================

  CreatePluginInstanceAsyncFunc create_plugin_instance_async_func_;

  std::unique_ptr<juce::AudioPluginInstance> juce_plugin_;

  std::function<units::sample_rate_t ()> sample_rate_provider_;
  std::function<units::sample_u32_t ()>  buffer_size_provider_;

  juce::AudioBuffer<float> juce_audio_buffer_;
  juce::MidiBuffer         juce_midi_buffer_;

  class JuceParamListener : public juce::AudioProcessorParameter::Listener
  {
  public:
    JuceParamListener (JucePlugin &parent) : parent_ (parent) { }

    void parameterValueChanged (int parameterIndex, float newValue)
      [[clang::blocking]] override;
    void parameterGestureChanged (int parameterIndex, bool gestureIsStarting)
      [[clang::blocking]] override;

  private:
    JucePlugin &parent_;
  };

  // Parameter mapping between JUCE and Zrythm
  struct ParameterMapping
  {
    juce::AudioProcessorParameter *    juce_param;
    dsp::ProcessorParameter *          zrythm_param;
    int                                juce_param_index;
    size_t                             zrythm_param_index;
    std::unique_ptr<JuceParamListener> juce_param_listener;
  };

  std::vector<ParameterMapping> parameter_mappings_;

  /**
   * @brief Processor-level listener for change notifications.
   *
   * Parameter value changes are handled per-parameter via JuceParamListener;
   * this listener handles processor-wide change flags (currently latency).
   */
  class JuceProcessorListener : public juce::AudioProcessorListener
  {
  public:
    JuceProcessorListener (JucePlugin &parent) : parent_ (parent) { }

    void audioProcessorParameterChanged (
      juce::AudioProcessor * processor,
      int                    parameterIndex,
      float                  newValue) [[clang::blocking]] override;
    void audioProcessorChanged (
      juce::AudioProcessor * processor,
      const ChangeDetails   &details) [[clang::blocking]] override;

  private:
    JucePlugin &parent_;
  };

  std::unique_ptr<JuceProcessorListener> processor_listener_;

  /**
   * @brief Maps JUCE param index -> position in parameter_mappings_.
   * Built in create_parameters_from_juce_plugin() for O(1) lookup.
   */
  std::unordered_map<int, size_t> juce_param_index_to_mapping_;

  /**
   * @brief Maps Zrythm param index (position in get_parameters()) -> position
   * in parameter_mappings_. Used by sync_changed_params_to_juce() to find
   * the JUCE param for each changed Zrythm param.
   */
  std::unordered_map<size_t, size_t> zrythm_param_index_to_mapping_;

  /**
   * @brief Reports a dropped plugin output event (audio-thread safe).
   *
   * The event violated the output contract (out-of-range param index or
   * event position); the drop is counted and logged on the main thread at
   * a bounded (power-of-two) rate.
   */
  void note_invalid_output_event_drop (std::string_view violation) noexcept;

  /** Drop counter for note_invalid_output_event_drop(). */
  std::atomic<uint32_t> invalid_output_event_drops_{ 0 };

  // Audio/MIDI buffer management
  std::vector<float *> input_channels_;
  std::vector<float *> output_channels_;

  bool juce_initialized_ = false;
  bool plugin_loading_ = false;

  // Optional state to apply on instantiation (this is mainly used as a
  // temporary space to store the restored state temporarily until the plugin is
  // instantiated).
  std::optional<juce::MemoryBlock> state_to_apply_;
};

} // namespace zrythm::plugins
