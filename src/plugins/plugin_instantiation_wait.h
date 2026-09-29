// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <chrono>
#include <span>

#include "utils/iobject_registry.h"

#include <QUuid>

namespace zrythm::plugins
{

/**
 * @brief Processes queued events until none of the given plugins'
 * instantiation is Pending, or the timeout elapses.
 *
 * Plugin instantiation may complete asynchronously (external plugin
 * formats initialize on their own threads/loops); this spins the event
 * loop, excluding user input, so pending instantiations can finish on
 * the main thread. A timeout is logged as a warning and the caller is
 * left to deal with any plugins still pending.
 *
 * @param registry Registry the plugins are registered in.
 * @param plugin_ids IDs of the plugins whose instantiations to wait
 * for. IDs that are not registered are ignored.
 * @param timeout Maximum time to wait.
 */
void
wait_for_plugin_instantiations (
  const utils::IObjectRegistry &registry,
  std::span<const QUuid>        plugin_ids,
  std::chrono::milliseconds     timeout = std::chrono::seconds (30));

/**
 * @brief Processes queued events until no plugin registered in @p
 * registry is Pending, or the timeout elapses.
 *
 * A timeout is logged as a warning and the caller is left to deal with
 * any plugins still pending.
 *
 * @param registry Registry whose plugins are checked.
 * @param timeout Maximum time to wait.
 */
void
wait_for_plugin_instantiations (
  const utils::IObjectRegistry &registry,
  std::chrono::milliseconds     timeout = std::chrono::seconds (30));

} // namespace zrythm::plugins
