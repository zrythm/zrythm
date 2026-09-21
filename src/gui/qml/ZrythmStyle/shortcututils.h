// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QKeySequence>
#include <QObject>
#include <QVariant>
#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::gui
{

/**
 * @brief Helpers for displaying keyboard shortcuts in QML.
 *
 * When an Action's shortcut is assigned from a StandardKey enum (e.g.
 * `shortcut: StandardKey.Save`), QML reads the value back as the raw enum
 * number instead of resolved text, so the conversion to display text must
 * happen in C++.
 */
class ShortcutUtils : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

public:
  /**
   * @brief Converts a shortcut value to native display text.
   *
   * String values are returned as-is (QML already resolves string-assigned
   * shortcuts). Numbers are interpreted as QKeySequence::StandardKey enum
   * values and resolved against the current platform bindings. Anything
   * else yields an empty string.
   */
  Q_INVOKABLE static QString displayText (const QVariant &shortcut)
  {
    switch (shortcut.typeId ())
      {
      case QMetaType::QString:
        return shortcut.toString ();
      case QMetaType::Int:
      case QMetaType::UInt:
      case QMetaType::LongLong:
        {
          const auto standardKey =
            static_cast<QKeySequence::StandardKey> (shortcut.toInt ());
          return QKeySequence (standardKey).toString (QKeySequence::NativeText);
        }
      default:
        return {};
      }
  }
};
}
