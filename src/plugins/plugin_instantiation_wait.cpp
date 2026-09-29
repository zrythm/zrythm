// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>

#include "plugins/plugin.h"
#include "plugins/plugin_instantiation_wait.h"
#include "utils/logger.h"

#include <QCoreApplication>
#include <QThread>

namespace zrythm::plugins
{

namespace
{

// Processes queued events (excluding user input) while @p has_pending
// reports work left, up to @p timeout; a timeout is logged as a warning
template <typename PendingCheck>
void
process_events_while_pending (
  PendingCheck              has_pending,
  std::chrono::milliseconds timeout)
{
  const auto deadline = std::chrono::steady_clock::now () + timeout;

  while (has_pending ())
    {
      if (std::chrono::steady_clock::now () >= deadline)
        {
          z_warning (
            "Timed out waiting for plugin instantiations after {} "
            "seconds",
            std::chrono::duration_cast<std::chrono::seconds> (timeout).count ());
          return;
        }
      QCoreApplication::processEvents (QEventLoop::ExcludeUserInputEvents);
      QThread::msleep (1);
    }
}

} // namespace

void
wait_for_plugin_instantiations (
  const utils::IObjectRegistry &registry,
  std::span<const QUuid>        plugin_ids,
  std::chrono::milliseconds     timeout)
{
  process_events_while_pending (
    [&registry, plugin_ids] () {
      return std::ranges::any_of (plugin_ids, [&registry] (const QUuid &id) {
        const auto * plugin =
          qobject_cast<Plugin *> (registry.find_by_raw_uuid (id));
        return plugin != nullptr
               && plugin->instantiationStatus ()
                    == Plugin::InstantiationStatus::Pending;
      });
    },
    timeout);
}

void
wait_for_plugin_instantiations (
  const utils::IObjectRegistry &registry,
  std::chrono::milliseconds     timeout)
{
  process_events_while_pending (
    [&registry] () {
      bool has_pending = false;
      registry.for_each_matching<Plugin> ([&] (const auto &plugin) {
        if (
          plugin.instantiationStatus () == Plugin::InstantiationStatus::Pending)
          has_pending = true;
      });
      return has_pending;
    },
    timeout);
}

} // namespace zrythm::plugins
