// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>

#include <QObject>

namespace zrythm::plugins
{

/**
 * @brief Main-thread run loop primitives bridged into Qt's event loop.
 *
 * Provides file-descriptor watches (via QSocketNotifier) and periodic timers
 * (via QTimer) for plugin formats that require host-provided run loop
 * services.
 *
 * Compiles on all platforms, but usage is platform-dependent:
 * - VST3's Linux::IRunLoop: GNU/Linux only (by design).
 * - CLAP's posix-fd extension: POSIX platforms (GNU/Linux, macOS).
 * - CLAP's timer extension: all platforms.
 *
 * All methods must be called on the thread this object lives on (the main
 * thread in practice).
 */
class PluginRunLoop final : public QObject
{
public:
  /** Opaque handle identifying a registration. */
  using Token = std::uint64_t;

  /**
   * @brief Called when the watched fd is ready.
   *
   * @param read_ready True for read readiness, false for write readiness.
   */
  using FdCallback = std::function<void (bool read_ready)>;

  using TimerCallback = std::function<void ()>;

  explicit PluginRunLoop (QObject * parent = nullptr);
  ~PluginRunLoop () override;

  /**
   * @brief Registers a file descriptor watch.
   *
   * @param fd The file descriptor to watch.
   * @param read Watch for read readiness.
   * @param write Watch for write readiness.
   * @param callback Invoked on readiness.
   * @return Token identifying the registration.
   */
  Token register_fd (int fd, bool read, bool write, FdCallback callback)
    [[clang::blocking]];

  /**
   * @brief Changes the watched read/write state of an fd registration.
   */
  void update_fd (Token token, bool read, bool write) [[clang::blocking]];

  void unregister_fd (Token token) [[clang::blocking]];

  /**
   * @brief Registers a periodically firing timer.
   *
   * @return Token identifying the registration.
   */
  Token
  register_timer (std::chrono::milliseconds interval, TimerCallback callback)
    [[clang::blocking]];

  void unregister_timer (Token token) [[clang::blocking]];

  /**
   * @brief Removes all fd watches and timers.
   *
   * Call before the resources behind the registrations become invalid
   * (e.g. before destroying the plugin that registered them): notifiers
   * would otherwise observe closed fds and timers would fire into dead
   * callees.
   */
  void clear () [[clang::blocking]];

private:
  class Impl;
  std::unique_ptr<Impl> pimpl_;
};

} // namespace zrythm::plugins
