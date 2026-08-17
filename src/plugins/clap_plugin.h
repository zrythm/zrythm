// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
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
   * @brief Constructor for ClapPlugin.
   *
   * @param dependencies Processor dependencies
   * @param host_window_factory Factory for creating plugin host windows
   * @param parent Parent QObject
   */
  ClapPlugin (
    utils::IObjectRegistry &registry,
    PluginHostWindowFactory host_window_factory,
    QObject *               parent = nullptr);
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
  bool implementsLatency () const noexcept override { return true; }
  void latencyChanged () noexcept override;

  // clap_host_audio_ports
  bool implementsAudioPorts () const noexcept override { return true; }
  bool audioPortsIsRescanFlagSupported (uint32_t flag) noexcept override;
  void audioPortsRescan (uint32_t flags) noexcept override;

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
  // TODO: populate threadPool_ with worker threads and advertise the pool
  // (threadPoolRequestExec currently has no consumers, so advertising it
  // would deadlock the audio thread for numTasks > 1). The spec permits
  // hosts not to offer a thread pool; plugins must then fall back to their
  // own threading.
  bool implementsThreadPool () const noexcept override { return false; }
  bool threadPoolRequestExec (uint32_t numTasks) noexcept override;

  // clap_host_state
  bool implementsState () const noexcept override { return true; }
  void stateMarkDirty () noexcept override;

  // clap_host_preset_load
  bool implementsPresetLoad () const noexcept override { return true; }
  void presetLoadLoaded (
    uint32_t     locationKind,
    const char * location,
    const char * loadKey) noexcept override;
  void presetLoadOnError (
    uint32_t     locationKind,
    const char * location,
    const char * loadKey,
    int32_t      osError,
    const char * msg) noexcept override;

  // ============================================================================
  // Plugin Interface Implementation
  // ============================================================================

  units::sample_u32_t get_single_playback_latency () const override;

  bool hasNativeUi () const override;

  /**
   * @brief Negotiates the saved (restored) port topology into the plugin, or
   * adopts the live layout when negotiation is impossible or refused.
   *
   * Must be called on the main thread with the engine paused while the
   * plugin is deactivated (the reconciliation mutates engine-visible ports
   * and drops buffers, and configuration pushes are only valid on
   * deactivated plugins). When any port changed, the graph must be
   * recalculated and all nodes re-prepared before processing resumes:
   * reconciliation may have dropped port buffers or created unprepared
   * ports.
   */
  void restore_saved_bus_arrangements ();

protected:
  void prepare_plugin_for_processing (
    units::sample_rate_t sample_rate,
    units::sample_u32_t  max_block_length) override;

  void process_impl (
    dsp::graph::ProcessBlockInfo time_info,
    const dsp::ITransport       &transport,
    const dsp::TempoMap         &tempo_map) noexcept override;

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
  void on_ui_visibility_changed () override;

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

  /**
   * @brief Applies the port topology pending from audioPortsRescan() calls.
   *
   * Runs on the main thread after any in-flight restart completes; reconciles
   * the ports with the scanned configuration and requests a graph rebuild
   * when the change is graph-affecting (see dsp::AudioBusReconcileResult).
   */
  void apply_audio_ports_rescan ();

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
  bool        load_state_impl (const std::string &base64_state) override;

  /**
   * @brief Applies state from a QByteArray to the CLAP plugin.
   */
  bool apply_state_from_byte_array (const QByteArray &data);

  /**
   * @brief Flushes parameter updates and syncs Zrythm parameter values to
   * match.
   *
   * Calls paramsFlush() with empty input events (per the CLAP spec, plugins
   * may defer parameter updates until the next paramsFlush() after a state
   * load), then reads the resulting values back into Zrythm's baseValues.
   * Main thread only; the plugin must be instantiated and inactive.
   */
  void flush_and_sync_params_when_inactive ();

  /**
   * @brief Reads all parameter values from the plugin into Zrythm's
   * baseValues.
   *
   * Main thread only; the plugin must be instantiated.
   */
  void sync_param_values_from_plugin ();

private:
  class ClapPluginImpl;
  std::unique_ptr<ClapPluginImpl> pimpl_;

  /** Pending state to apply after plugin initialization (from JSON). */
  std::optional<QByteArray> state_to_apply_;
};

} // namespace zrythm::plugins
