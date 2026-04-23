// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QtQmlIntegration>

namespace zrythm::dsp
{
class Transport;
class SnapGrid;
}

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
    QObject *       parent = nullptr);

  /**
   * @brief Moves the playhead backward to the previous snap point.
   */
  Q_INVOKABLE void moveBackward () [[clang::blocking]];

  /**
   * @brief Moves the playhead forward to the next snap point.
   */
  Q_INVOKABLE void moveForward () [[clang::blocking]];

private:
  dsp::Transport &transport_;
  dsp::SnapGrid  &snap_grid_;
};

} // namespace zrythm::controllers
