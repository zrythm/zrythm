// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "plugins/plugin.h"

#include <clap/clap.h>
#include <clap/helpers/host.hh>

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
    PluginHostWindowFactory                       host_window_factory,
    QObject *                                     parent = nullptr);
  Z_DISABLE_COPY_MOVE (ClapPlugin)
  ~ClapPlugin () override;

  // clap_host
  void requestRestart () noexcept override;
  void requestProcess () noexcept override;
  void requestCallback () noexcept override;

  // clap_host_gui
  bool implementsGui () const noexcept override { return true; }
  void guiResizeHintsChanged () noexcept override;
  bool guiRequestResize (uint32_t width, uint32_t height) noexcept override;
  bool guiRequestShow () noexcept override;
  bool guiRequestHide () noexcept override;
  void guiClosed (bool wasDestroyed) noexcept override;

  // clap_host_timer_support
  bool implementsTimerSupport () const noexcept override { return true; }
  bool timerSupportRegisterTimer (uint32_t periodMs, clap_id * timerId) noexcept
    override;
  bool timerSupportUnregisterTimer (clap_id timerId) noexcept override;

  // clap_host_log
  bool implementsLog () const noexcept override { return true; }
  void logLog (clap_log_severity severity, const char * message)
    const noexcept override;

  // clap_host_latency
  bool implementsLatency () const noexcept override { return false; }
  void latencyChanged () noexcept override;

  // clap_host_thread_check
  bool threadCheckIsMainThread () const noexcept override;
  bool threadCheckIsAudioThread () const noexcept override;

  // clap_host_params
  bool implementsParams () const noexcept override { return true; }
  void paramsRescan (clap_param_rescan_flags flags) noexcept override;
  void
  paramsClear (clap_id paramId, clap_param_clear_flags flags) noexcept override;
  void paramsRequestFlush () noexcept override;

  // clap_host_posix_fd_support
  bool implementsPosixFdSupport () const noexcept override { return true; }
  bool posixFdSupportRegisterFd (int fd, clap_posix_fd_flags_t flags) noexcept
    override;
  bool
  posixFdSupportModifyFd (int fd, clap_posix_fd_flags_t flags) noexcept override;
  bool posixFdSupportUnregisterFd (int fd) noexcept override;

  // clap_host_thread_pool
  bool implementsThreadPool () const noexcept override { return true; }
  bool threadPoolRequestExec (uint32_t numTasks) noexcept override;

  // clap_host_state
  bool implementsState () const noexcept override { return true; }
  void stateMarkDirty () noexcept override;

  // ============================================================================
  // Plugin Interface Implementation
  // ============================================================================

  nframes_t get_single_playback_latency () const override;

  void save_state (std::optional<fs::path> abs_state_dir) override;
  void load_state (std::optional<fs::path> abs_state_dir) override;

protected:
  void prepare_for_processing_impl (
    units::sample_rate_t sample_rate,
    nframes_t            max_block_length) override;

  void process_impl (EngineProcessTimeInfo time_info) noexcept override;

  void release_resources_impl () override;

Q_SIGNALS:
  void paramsChanged ();
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
  /**
   * @brief Loads the plugin with the given unique ID hash at the given path.
   *
   * This is done with a hash at the moment because CLAPPluginFormat only saves
   * the hash in the juce descriptor (we don't have the full ID).
   *
   * @param path
   * @param plugin_unique_id The hash of the CLAP plugin ID.
   */
  bool load_plugin (const fs::path &path, int64_t plugin_unique_id);
  void unload_current_plugin ();

  void create_ports_from_clap_plugin ();
  void scanParams ();

  void show_editor ();
  void hide_editor ();

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
  class ClapPluginImpl;
  std::unique_ptr<ClapPluginImpl> pimpl_;
};

} // namespace zrythm::plugins
