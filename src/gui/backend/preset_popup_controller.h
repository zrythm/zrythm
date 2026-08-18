// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QObject>

namespace zrythm::gui
{

/**
 * @brief Invokable surface passed to windowed preset popup scenes.
 *
 * The popup scene's signals are QML-declared, so C++ cannot connect to
 * them with member-function-pointer connects; instead the scene calls
 * these invokables on its `popupHost` property, and the controller
 * re-emits them as regular C++ signals for the host window backend.
 *
 * Must outlive the popup scene's root item.
 */
class PresetPopupController final : public QObject
{
  Q_OBJECT

public:
  using QObject::QObject;

  Q_INVOKABLE void presetPopupActivated (int index)
  {
    Q_EMIT activated (index);
  }

  Q_INVOKABLE void presetPopupDismissed () { Q_EMIT dismissed (); }

Q_SIGNALS:
  /** The user picked an item. */
  void activated (int index);
  /** The user dismissed the popup without selecting (Escape). */
  void dismissed ();
};

} // namespace zrythm::gui
