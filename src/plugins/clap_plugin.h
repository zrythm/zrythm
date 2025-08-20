// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
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

#pragma once

#include "plugins/clap_plugin_param.h"
#include "plugins/plugin.h"

#include <QLibrary>

#include <clap/clap.h>
#include <clap/helpers/event-list.hh>
#include <clap/helpers/host.hh>
#include <clap/helpers/plugin-proxy.hh>
#include <clap/helpers/reducing-param-queue.hh>

namespace zrythm::plugins
{

using ClapHostBase = clap::helpers::Host<
  clap::helpers::MisbehaviourHandler::Terminate,
  clap::helpers::CheckingLevel::Maximal>;

/**
 * @brief CLAP-based plugin host implementation.
 *
 * This class provides hosting capabilities for CLAP plugins.
 */
class ClapPlugin : public Plugin, public ClapHostBase
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

  using ClapPluginProxy = clap::helpers::PluginProxy<
    clap::helpers::MisbehaviourHandler::Terminate,
    clap::helpers::CheckingLevel::Maximal>;

public:
  /**
   * @brief Function that returns whether the caller is an audio DSP thread.
   */
  using AudioThreadChecker = std::function<bool ()>;

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
  ClapPlugin (
    dsp::ProcessorBase::ProcessorBaseDependencies dependencies,
    StateDirectoryParentPathProvider              state_path_provider,
    AudioThreadChecker                            audio_thread_checker,
    QObject *                                     parent = nullptr);

  ~ClapPlugin () override;

  bool load_plugin (const fs::path &path, int plugin_index);
  void unload_current_plugin ();

  // clap_host
  void requestRestart () noexcept override;
  void requestProcess () noexcept override;
  void requestCallback () noexcept override;

  // clap_host_log
  bool implementsLog () const noexcept override { return true; }
  void logLog (clap_log_severity severity, const char * message)
    const noexcept override;

  // clap_host_thread_check
  bool threadCheckIsMainThread () const noexcept override;
  bool threadCheckIsAudioThread () const noexcept override;

  // clap_host_params
  bool implementsParams () const noexcept override { return true; }
  void paramsRescan (clap_param_rescan_flags flags) noexcept override;
  void
  paramsClear (clap_id paramId, clap_param_clear_flags flags) noexcept override;
  void paramsRequestFlush () noexcept override;

  // ============================================================================
  // Plugin Interface Implementation
  // ============================================================================

  void save_state (std::optional<fs::path> abs_state_dir) override;
  void load_state (std::optional<fs::path> abs_state_dir) override;

protected:
  void prepare_for_processing_impl (
    sample_rate_t sample_rate,
    nframes_t     max_block_length) override;

  void process_impl (EngineProcessTimeInfo time_info) noexcept override;

  void release_resources_impl () override;

Q_SIGNALS:
  void paramsChanged ();
  void quickControlsPagesChanged ();
  void quickControlsSelectedPageChanged ();
  void paramAdjusted (clap_id paramId);
  void pluginLoadedChanged (bool pluginLoaded);

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
  void create_ports_from_clap_plugin ();

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
  bool isPluginActive () const;
  bool isPluginProcessing () const;
  bool isPluginSleeping () const;
  void setPluginState (PluginState state);

  /* clap host callbacks */
  void             scanParams ();
  void             scanParam (int32_t index);
  ClapPluginParam &checkValidParamId (
    const std::string_view &function,
    const std::string_view &param_name,
    clap_id                 param_id);
  void        checkValidParamValue (const ClapPluginParam &param, double value);
  double      getParamValue (const clap_param_info &info);
  static bool clapParamsRescanMayValueChange (uint32_t flags)
  {
    return (flags & (CLAP_PARAM_RESCAN_ALL | CLAP_PARAM_RESCAN_VALUES)) != 0u;
  }
  static bool clapParamsRescanMayInfoChange (uint32_t flags)
  {
    return (flags & (CLAP_PARAM_RESCAN_ALL | CLAP_PARAM_RESCAN_INFO)) != 0u;
  }

  void paramFlushOnMainThread ();
  void handlePluginOutputEvents ();
  void generatePluginInputEvents ();

  void setup_audio_ports_for_processing (nframes_t block_size);

  static constexpr auto kStateKey = "state"sv;
  friend void           to_json (nlohmann::json &j, const ClapPlugin &p)
  {
    to_json (j, static_cast<const Plugin &> (p));
  }
  friend void from_json (const nlohmann::json &j, ClapPlugin &p)
  {
    from_json (j, static_cast<Plugin &> (p));
  }

private:
  AudioThreadChecker audio_thread_checker_;

  // ============================================================================
  // CLAP-specific members
  // ============================================================================

  QLibrary library_;

  const clap_plugin_entry *        pluginEntry_ = nullptr;
  const clap_plugin_factory *      pluginFactory_ = nullptr;
  std::unique_ptr<ClapPluginProxy> plugin_;

  /* timers */
  clap_id                                              nextTimerId_ = 0;
  std::unordered_map<clap_id, std::unique_ptr<QTimer>> timers_;

  /* fd events */
  struct Notifiers
  {
    std::unique_ptr<QSocketNotifier> rd;
    std::unique_ptr<QSocketNotifier> wr;
  };
  std::unordered_map<int, std::unique_ptr<Notifiers>> fds_;

  /* process stuff */
  clap_audio_buffer        audioIn_{};
  clap_audio_buffer        audioOut_{};
  juce::AudioSampleBuffer  audio_in_buf_;
  std::vector<float *>     audio_in_channel_ptrs_;
  juce::AudioSampleBuffer  audio_out_buf_;
  std::vector<float *>     audio_out_channel_ptrs_;
  clap::helpers::EventList evIn_;
  clap::helpers::EventList evOut_;
  clap_process             process_{};

  /* param update queues */
  std::unordered_map<clap_id, std::unique_ptr<ClapPluginParam>> params_;

  struct AppToEngineParamQueueValue
  {
    void * cookie;
    double value;
  };

  struct EngineToAppParamQueueValue
  {
    void update (const EngineToAppParamQueueValue &v) noexcept
    {
      if (v.has_value)
        {
          has_value = true;
          value = v.value;
        }

      if (v.has_gesture)
        {
          has_gesture = true;
          is_begin = v.is_begin;
        }
    }

    bool   has_value = false;
    bool   has_gesture = false;
    bool   is_begin = false;
    double value = 0;
  };

  clap::helpers::ReducingParamQueue<clap_id, AppToEngineParamQueueValue>
    appToEngineValueQueue_;
  clap::helpers::ReducingParamQueue<clap_id, AppToEngineParamQueueValue>
    appToEngineModQueue_;
  clap::helpers::ReducingParamQueue<clap_id, EngineToAppParamQueueValue>
    engineToAppValueQueue_;

  PluginState state_{ Inactive };
  bool        stateIsDirty_ = false;

  bool scheduleRestart_ = false;
  bool scheduleDeactivate_ = false;

  bool scheduleProcess_ = true;

  bool scheduleParamFlush_ = false;

  std::unordered_map<clap_id, bool> isAdjustingParameter_;

  const char * guiApi_ = nullptr;
  bool         isGuiCreated_ = false;
  bool         isGuiVisible_ = false;
  bool         isGuiFloating_ = false;

  bool scheduleMainThreadCallback_ = false;

  // work-around the fact that stopProcessing() requires being called by an
  // audio thread for whatever reason
  std::atomic_bool force_audio_thread_check_{ false };
};

} // namespace zrythm::plugins
