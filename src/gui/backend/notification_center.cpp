// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/format_qt.h"

#include "gui/backend/notification_center.h"
#include "utils/logger.h"

#include <QAccessible>
#include <QMetaObject>
#include <QThread>

namespace zrythm::gui
{

namespace
{

std::string_view
severity_name (Notification::Severity severity)
{
  switch (severity)
    {
    case Notification::Info:
      return "Info";
    case Notification::Success:
      return "Success";
    case Notification::Warning:
      return "Warning";
    case Notification::Error:
      return "Error";
    case Notification::Critical:
      return "Critical";
    }
  std::unreachable ();
}

void
log_post (
  Notification::Severity severity,
  const QString         &title,
  const QString         &detail,
  const QString         &context_tag)
{
  auto base =
    fmt::format ("[notification] [{}] {}", severity_name (severity), title);
  if (!detail.isEmpty ())
    base += fmt::format (" — {}", detail);
  if (!context_tag.isEmpty ())
    base += fmt::format (" [{}]", context_tag);

  switch (severity)
    {
    case Notification::Info:
    case Notification::Success:
      z_info ("{}", base);
      break;
    case Notification::Warning:
    case Notification::Error:
    case Notification::Critical:
      z_warning ("{}", base);
      break;
    }
}

void
warn_incomplete_action ()
{
  z_warning (
    "notification actions need both a label and a callback; the action "
    "was dropped");
}

} // namespace

NotificationCenter::NotificationCenter (QObject * parent)
    : QObject (parent), history_ (new NotificationModel (this))
{
}

void
NotificationCenter::post (
  Notification::Severity severity,
  const QString         &title,
  const QString         &detail,
  const QString         &context_tag,
  const QString         &action_label,
  const QJSValue        &action_callback)
{
  // QML closures must not cross threads: the closure is never copied
  // or inspected off the GUI thread — an off-GUI-thread post drops the
  // action and the closure dies in the caller's frame
  if (QThread::currentThread () != thread ())
    {
      if (!action_label.isEmpty ())
        {
          z_warning (
            "notification actions must be posted on the GUI thread; the "
            "action was dropped");
        }
      post_dispatch (severity, title, detail, context_tag, QString (), {});
      return;
    }

  post_dispatch (
    severity, title, detail, context_tag, action_label, action_callback);
}

void
NotificationCenter::post (
  Notification::Severity severity,
  const QString         &title,
  const QString         &detail,
  const QString         &context_tag,
  const QString         &action_label,
  std::function<void ()> action_callback)
{
  post_dispatch (
    severity, title, detail, context_tag, action_label,
    std::move (action_callback));
}

void
NotificationCenter::post_dispatch (
  Notification::Severity       severity,
  const QString               &title,
  const QString               &detail,
  const QString               &context_tag,
  const QString               &action_label,
  Notification::ActionCallback action_callback)
{
  log_post (severity, title, detail, context_tag);

  QString    effective_action_label = action_label;
  const bool has_callback = std::visit (
    [] (const auto &callback) {
      using Callback = std::decay_t<decltype (callback)>;
      if constexpr (std::is_same_v<Callback, QJSValue>)
        return callback.isCallable ();
      else if constexpr (std::is_same_v<Callback, std::function<void ()>>)
        return static_cast<bool> (callback);
      else
        return false;
    },
    action_callback);
  const bool has_label = !effective_action_label.isEmpty ();

  // Actions render on toasts only: a Critical post opens a modal
  // dialog, which has no action button
  if (severity == Notification::Critical && (has_label || has_callback))
    {
      z_warning ("notification actions are toast-only; the action was dropped");
      effective_action_label.clear ();
      action_callback = {};
    }
  // Actions need both a label and a callback; half an action is a bug
  // in the caller, so it is dropped loudly
  else if (has_label != has_callback)
    {
      warn_incomplete_action ();
      effective_action_label.clear ();
      action_callback = {};
    }

  // C++ callbacks are carried through the queued delivery and invoked
  // on the GUI thread
  if (QThread::currentThread () == thread ())
    {
      post_internal (
        severity, title, detail, context_tag, effective_action_label,
        std::move (action_callback));
    }
  else
    {
      QMetaObject::invokeMethod (
        this,
        [this, severity, title, detail, context_tag, effective_action_label,
         action_callback] () {
          post_internal (
            severity, title, detail, context_tag, effective_action_label,
            action_callback);
        },
        Qt::QueuedConnection);
    }
}

void
NotificationCenter::postInfo (
  const QString  &title,
  const QString  &detail,
  const QString  &context_tag,
  const QString  &action_label,
  const QJSValue &action_callback)
{
  post (
    Notification::Info, title, detail, context_tag, action_label,
    action_callback);
}

void
NotificationCenter::postSuccess (
  const QString  &title,
  const QString  &detail,
  const QString  &context_tag,
  const QString  &action_label,
  const QJSValue &action_callback)
{
  post (
    Notification::Success, title, detail, context_tag, action_label,
    action_callback);
}

void
NotificationCenter::postWarning (
  const QString  &title,
  const QString  &detail,
  const QString  &context_tag,
  const QString  &action_label,
  const QJSValue &action_callback)
{
  post (
    Notification::Warning, title, detail, context_tag, action_label,
    action_callback);
}

void
NotificationCenter::postError (
  const QString  &title,
  const QString  &detail,
  const QString  &context_tag,
  const QString  &action_label,
  const QJSValue &action_callback)
{
  post (
    Notification::Error, title, detail, context_tag, action_label,
    action_callback);
}

void
NotificationCenter::postCritical (
  const QString  &title,
  const QString  &detail,
  const QString  &context_tag,
  const QString  &action_label,
  const QJSValue &action_callback)
{
  post (
    Notification::Critical, title, detail, context_tag, action_label,
    action_callback);
}

void
NotificationCenter::acknowledgeAll ()
{
  history_->set_all_acknowledged ();
}

void
NotificationCenter::clearHistory ()
{
  history_->clear_all ();
}

QVector<Notification *>
NotificationCenter::unacknowledgedCriticals () const
{
  QVector<Notification *> result;
  for (auto * notification : history_->entries ())
    {
      if (
        notification->severity () == Notification::Critical
        && !notification->isAcknowledged ())
        {
          result.append (notification);
        }
    }
  return result;
}

NotificationModel *
NotificationCenter::history () const
{
  return history_;
}

void
NotificationCenter::post_internal (
  Notification::Severity       severity,
  const QString               &title,
  const QString               &detail,
  const QString               &context_tag,
  const QString               &action_label,
  Notification::ActionCallback action_callback)
{
  finalize_post (
    utils::make_qobject_unique<Notification> (
      severity, title, detail, context_tag, action_label,
      std::move (action_callback)));
}

void
NotificationCenter::finalize_post (
  utils::QObjectUniquePtr<Notification> notification)
{
  if (QAccessible::isActive ())
    {
      const QString text =
        notification->detail ().isEmpty ()
          ? notification->title ()
          : notification->title () + ". " + notification->detail ();
      QAccessible::updateAccessibility (
        new QAccessibleAnnouncementEvent (notification.get (), text));
    }

  auto * notification_ptr = notification.get ();
  owned_notifications_.push_back (std::move (notification));
  history_->add_notification (notification_ptr);
  Q_EMIT notificationPosted (notification_ptr);
}

} // namespace zrythm::gui
