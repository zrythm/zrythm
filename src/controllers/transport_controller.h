// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/snap_grid.h"
#include "dsp/transport.h"

#include <QtQmlIntegration>

namespace zrythm::controllers
{

/**
 * @brief Controller for transport navigation.
 *
 * Provides QML-friendly methods for moving the playhead.
 */
class TransportController : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  explicit TransportController (
    dsp::Transport &transport,
    dsp::SnapGrid  &snap_grid,
    QObject *       parent = nullptr)
      : QObject (parent), transport_ (transport), snap_grid_ (snap_grid)
  {
  }

  /**
   * @brief Moves the playhead backward to the previous snap point.
   */
  Q_INVOKABLE void moveBackward () [[clang::blocking]];

  /**
   * @brief Moves the playhead forward to the next snap point.
   */
  Q_INVOKABLE void moveForward () [[clang::blocking]];

private:
  /** Millisec to allow moving further backward when very close to the
   * calculated backward position. */
  static constexpr auto REPEATED_BACKWARD_MS = au::milli (units::seconds) (240);

  dsp::Transport &transport_;
  dsp::SnapGrid  &snap_grid_;
};

} // namespace zrythm::controllers
