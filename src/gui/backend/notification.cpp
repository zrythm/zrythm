// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <exception>
#include <type_traits>

#include "utils/format_qt.h"

#include "gui/backend/notification.h"
#include "utils/logger.h"

namespace zrythm::gui
{

Notification::Notification (
  Severity       severity,
  const QString &title,
  const QString &detail,
  const QString &context_tag,
  const QString &action_label,
  ActionCallback action_callback)
    : QObject (), severity_ (severity), title_ (title), detail_ (detail),
      context_tag_ (context_tag), timestamp_ (QDateTime::currentDateTime ()),
      action_label_ (action_label), action_callback_ (std::move (action_callback))
{
}

QJSValue
Notification::actionCallback () const
{
  const auto * js_callback = std::get_if<QJSValue> (&action_callback_);
  return js_callback ? *js_callback : QJSValue ();
}

void
Notification::triggerAction ()
{
  std::visit (
    [] (auto &&callback) {
      using Callback = std::decay_t<decltype (callback)>;

      if constexpr (std::is_same_v<Callback, std::function<void ()>>)
        {
          if (!callback)
            return;

          // Exceptions are reported, not propagated
          try
            {
              callback ();
            }
          catch (const std::exception &e)
            {
              z_warning ("notification action failed: {}", e.what ());
            }
          catch (...)
            {
              z_warning ("notification action failed: unknown exception");
            }
        }
      else if constexpr (std::is_same_v<Callback, QJSValue>)
        {
          if (!callback.isCallable ())
            return;

          // Exceptions in the callback are returned, not thrown;
          // report them instead of failing silently
          const QJSValue result = callback.call ();
          if (result.isError ())
            z_warning ("notification action failed: {}", result.toString ());
        }
      // std::monostate: no action to run
    },
    action_callback_);
}

void
Notification::setAcknowledged (bool acknowledged)
{
  if (acknowledged_ == acknowledged)
    return;

  acknowledged_ = acknowledged;
  Q_EMIT acknowledgedChanged (acknowledged_);
}

} // namespace zrythm::gui
