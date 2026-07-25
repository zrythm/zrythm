// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QObject>
#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::gui::qquick
{

/**
 * @brief QML singleton tracking global keyboard modifier state.
 *
 * Modifier state is application-global and must be tracked regardless of
 * keyboard focus (e.g. toggling snapping with Shift while hovering an
 * arranger without having clicked it), so an application-wide event filter
 * is used instead of QML key handlers.
 */
class KeyboardState : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON
  Q_PROPERTY (bool shiftHeld READ shiftHeld NOTIFY modifierHeldChanged FINAL)
  Q_PROPERTY (bool ctrlHeld READ ctrlHeld NOTIFY modifierHeldChanged FINAL)
  Q_PROPERTY (bool altHeld READ altHeld NOTIFY modifierHeldChanged FINAL)

public:
  explicit KeyboardState (QObject * parent = nullptr);

  bool shiftHeld () const { return shift_held_; }
  bool ctrlHeld () const { return ctrl_held_; }
  bool altHeld () const { return alt_held_; }

  Q_SIGNAL void modifierHeldChanged ();

protected:
  bool eventFilter (QObject * obj, QEvent * ev) override;

private:
  bool shift_held_{};
  bool ctrl_held_{};
  bool alt_held_{};
};

} // namespace zrythm::gui::qquick
