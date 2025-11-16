// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <chrono>
#include <functional>

#include "utils/qt.h"

#include <QTimer>

namespace zrythm::utils
{

/**
 * @brief Generic debouncer that delays execution of a callback until a
 * specified time period has elapsed without the debounce being called again.
 *
 * This is useful for scenarios where you want to batch multiple rapid events
 * into a single operation, such as:
 * - Search boxes that should only search after user stops typing
 * - Cache updates that should only run after multiple rapid changes
 * - UI updates that should be debounced to avoid excessive redraws
 */
class Debouncer : public QObject
{
  Q_OBJECT

public:
  using Callback = std::function<void ()>;

  /**
   * @brief Construct a debouncer
   * @param delay The delay to wait before executing the callback
   * @param callback The function to call when debounce period expires
   * @param parent QObject parent
   */
  Debouncer (
    std::chrono::milliseconds delay,
    Callback                  callback,
    QObject *                 parent = nullptr);

  /**
   * @brief Call this to schedule the callback execution.
   *
   * If called multiple times before the delay expires, the timer will restart
   * and the callback will only execute once after the final call.
   */
  void operator() ();

  /**
   * @brief Alternative method name for scheduling callback execution
   */
  void debounce ();

  /**
   * @brief Set the delay for future debounce operations
   * @param delay New delay duration
   */
  void set_delay (std::chrono::milliseconds delay);

  /**
   * @brief Get the current delay
   */
  [[nodiscard]] std::chrono::milliseconds get_delay () const;

  /**
   * @brief Check if there's a pending callback execution
   */
  [[nodiscard]] bool is_pending () const;

  /**
   * @brief Cancel any pending callback execution
   */
  void cancel ();

private Q_SLOTS:
  void execute_pending ();

private:
  utils::QObjectUniquePtr<QTimer> timer_;
  std::chrono::milliseconds       delay_;
  Callback                        callback_;
  bool                            pending_{ false };
};

} // namespace zrythm::utils
