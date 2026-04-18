// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "plugins/plugin.h"
#include "utils/mpmc_queue.h"

#include <QTimer>

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
   * @brief Constructor for ClapPlugin.
   *
   * @param dependencies Processor dependencies
   * @param audio_thread_checker Function to check if caller is audio thread
   * @param host_window_factory Factory for creating plugin host windows
   * @param parent Parent QObject
   */
  ClapPlugin (
    dsp::ProcessorBase::ProcessorBaseDependencies dependencies,
    AudioThreadChecker                            audio_thread_checker,
    PluginHostWindowFactory                       host_window_factory,
    QObject *                                     parent = nullptr);
  Q_DISABLE_COPY_MOVE (ClapPlugin)
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

  units::sample_u32_t get_single_playback_latency () const override;

protected:
  void prepare_for_processing_impl (
    units::sample_rate_t sample_rate,
    units::sample_u32_t  max_block_length) override;

  void
  process_impl (dsp::graph::EngineProcessTimeInfo time_info) noexcept override;

  void release_resources_impl () override;

Q_SIGNALS:
  void paramsChanged ();
  void paramAdjusted (clap_id paramId);
  void pluginLoadedChanged (bool pluginLoaded);

private Q_SLOTS:
  /**
   * @brief Handle configuration changes.
   */
  void on_configuration_changed (
    PluginConfiguration * config,
    bool                  generateNewPluginPortsAndParams);

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
  bool load_plugin (
    const std::filesystem::path &path,
    int64_t                      plugin_unique_id,
    bool                         generate_new_ports = true);
  void unload_current_plugin ();

  void create_ports_from_clap_plugin ();
  void create_parameters_from_clap_plugin ();
  void rebuild_clap_param_map ();
  void sync_params_from_clap ();

  /**
   * @brief Flushes queued CLAP param changes to Zrythm params on the main
   * thread.
   */
  void flush_clap_params_to_zrythm ();

  void scanParams ();

  void show_editor ();
  void hide_editor ();

  static constexpr auto kStateKey = "state"sv;
  friend void           to_json (nlohmann::json &j, const ClapPlugin &p);
  friend void           from_json (const nlohmann::json &j, ClapPlugin &p);

  /**
   * @brief Saves CLAP plugin state to a QByteArray via the CLAP stream API.
   */
  QByteArray save_state_to_byte_array () const;

  std::string save_state_impl () const override;
  void        load_state_impl (const std::string &base64_state) override;

  /**
   * @brief Applies state from a QByteArray to the CLAP plugin.
   */
  void apply_state_from_byte_array (const QByteArray &data);

private:
  class ClapPluginImpl;
  std::unique_ptr<ClapPluginImpl> pimpl_;

  /** Pending state to apply after plugin initialization (from JSON). */
  std::optional<QByteArray> state_to_apply_;

  /**
   * @brief Mapping from CLAP param ID to Zrythm ProcessorParameter pointer.
   * Populated by rebuild_clap_param_map().
   *
   * Only written by rebuild_clap_param_map() on the main thread before
   * activation. Read on the audio thread during process_impl()
   * (generatePluginInputEvents, sync_params_from_clap) and on the main thread
   * via timer (flush_clap_params_to_zrythm). Safe because audio processing
   * starts after prepare_for_processing_impl(), which is called after the map
   * is built.
   */
  std::unordered_map<clap_id, dsp::ProcessorParameter *> clap_to_zrythm_param_;

  /**
   * @brief Reverse mapping from Zrythm ProcessorParameter pointer to CLAP
   * param ID.
   *
   * Same thread-safety contract as clap_to_zrythm_param_.
   */
  std::unordered_map<dsp::ProcessorParameter *, clap_id> zrythm_to_clap_param_;

  /**
   * @brief Pending CLAP param changes to apply on the main thread.
   * Each entry is {clap_id, normalized_value}.
   */
  struct ClapParamChange
  {
    clap_id id;
    float   normalized_value;
  };
  MPMCQueue<ClapParamChange> clap_to_zrythm_queue_;

  /**
   * @brief Timer that flushes CLAP param changes to Zrythm on the main thread.
   */
  utils::QObjectUniquePtr<QTimer> clap_param_flush_timer_;
};

} // namespace zrythm::plugins
