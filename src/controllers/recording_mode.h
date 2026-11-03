// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <cstdint>

#include <QObject>
#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::controllers::recording
{
Q_NAMESPACE
QML_ELEMENT

enum class RecordingMode
{
  ///< Unimplemented
  OverwriteEvents,

  ///< Unimplemented
  MergeEvents,

  ///< New clip when discontinuity detected.
  Takes,

  ///< Same as Takes but mutes the previously recorded clip when a new one
  ///< is created.
  TakesMuted,
};
Q_ENUM_NS (RecordingMode)

} // namespace zrythm::controllers::recording
