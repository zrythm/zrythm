// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <cstdint>

#include <QObject>
#include <QtQmlIntegration/QtQmlIntegration>

namespace zrythm::controllers::recording
{
Q_NAMESPACE
QML_ELEMENT

enum class RecordingMode : std::uint8_t
{
  ///< Unimplemented
  OverwriteEvents,

  ///< Unimplemented
  MergeEvents,

  ///< New region when discontinuity detected.
  Takes,

  ///< Same as Takes but mutes the previously recorded region when a new one
  ///< is created.
  TakesMuted,
};
Q_ENUM_NS (RecordingMode)

} // namespace zrythm::controllers::recording
