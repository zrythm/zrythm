// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <memory>
#include <span>
#include <unordered_map>
#include <vector>

#include "dsp/port.h"

#include <QObject>
#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::dsp
{

class PortObserver;
struct PortObservationCache;

/**
 * @brief Manages port observer lifecycle and runs a drain timer.
 *
 * Owned by Project. UI components create ObservationToken instances to
 * request observation of a port. The manager creates/removes PortObserver
 * instances based on ref counts and emits observationChanged() to trigger
 * graph rebuilds.
 *
 * A 60fps drain timer consuming-reads from observer ring buffers into
 * per-requester caches on the UI thread.
 *
 * @par Thread safety
 * All register/unregister calls and drain_all() run on the Qt event loop
 * (main thread). observationChanged() is emitted from the same thread, and
 * the Engine connects it to a graph rebuild that pauses processing first.
 * Therefore, observers() is never read concurrently with writes — the
 * pause-before-rebuild ordering is guaranteed by the signal/slot delivery
 * mechanism. This contract must be preserved: if tokens are ever created or
 * destroyed from a non-main thread, a mutex will be required.
 */
class PortObservationManager : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  using RegistrationId = int;

  PortObservationManager (
    utils::IObjectRegistry &registry,
    QObject *               parent = nullptr);

  ~PortObservationManager () override;

  Q_DISABLE_COPY_MOVE (PortObservationManager)

  // Called by ObservationToken on construction/destruction
  RegistrationId register_request (const Port &port);
  void           unregister_request (RegistrationId id);

  // Cache access for registered requesters
  PortObservationCache &cache (RegistrationId id);

  // Called by graph builder during build
  PortObserver * get_observer (const Port &port) const;

  // Called by graph builder during build
  std::span<PortObserver * const> observers () const;

  // For testing — manually triggers drain
  void drain_all ();

Q_SIGNALS:
  void observationChanged ();

private:
  PortObserver * find_observer_by_uuid (const PortUuid &port_uuid) const;

  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}
