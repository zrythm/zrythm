// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/qquick/keyboard_state.h"

#include <QGuiApplication>
#include <QKeyEvent>
#include <QThread>

namespace zrythm::gui::qquick
{

KeyboardState::KeyboardState (QObject * parent) : QObject (parent)
{
  qApp->installEventFilter (this);
}

bool
KeyboardState::eventFilter (QObject * obj, QEvent * ev)
{
  // App-wide event filters run in the RECEIVER's thread, not this object's.
  // Key and window-activation events are only delivered to GUI-thread
  // objects, so the writes below never race with QML reads of the held
  // flags. If that ever changes (e.g. synthetic key events posted to
  // worker-thread objects), the flags would need synchronization.
  Q_ASSERT (QThread::currentThread () == qApp->thread ());

  if (ev->type () == QEvent::WindowDeactivate)
    {
      // Modifier state can't be tracked while another window has focus
      if (shift_held_ || ctrl_held_ || alt_held_)
        {
          shift_held_ = ctrl_held_ = alt_held_ = false;
          Q_EMIT modifierHeldChanged ();
        }
    }
  else if (ev->type () == QEvent::KeyPress || ev->type () == QEvent::KeyRelease)
    {
      const auto * kev = static_cast<const QKeyEvent *> (ev);
      const bool   held = ev->type () == QEvent::KeyPress;
      bool         changed = false;
      switch (kev->key ())
        {
        case Qt::Key_Shift:
          changed = shift_held_ != held;
          shift_held_ = held;
          break;
        case Qt::Key_Control:
          changed = ctrl_held_ != held;
          ctrl_held_ = held;
          break;
        case Qt::Key_Alt:
          changed = alt_held_ != held;
          alt_held_ = held;
          break;
        default:
          break;
        }
      if (changed)
        Q_EMIT modifierHeldChanged ();
    }
  return QObject::eventFilter (obj, ev);
}

} // namespace zrythm::gui::qquick
