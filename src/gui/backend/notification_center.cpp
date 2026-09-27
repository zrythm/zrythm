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
  const QString         &context_tag)
{
  const auto log_message = [&] () -> std::string {
    auto base =
      fmt::format ("[notification] [{}] {}", severity_name (severity), title);
    if (!detail.isEmpty ())
      base += fmt::format (" — {}", detail);
    if (!context_tag.isEmpty ())
      base += fmt::format (" [{}]", context_tag);
    return base;
  }();
  switch (severity)
    {
    case Notification::Info:
    case Notification::Success:
      z_info ("{}", log_message);
      break;
    case Notification::Warning:
    case Notification::Error:
    case Notification::Critical:
      z_warning ("{}", log_message);
      break;
    }

  if (QThread::currentThread () == thread ())
    {
      post_internal (severity, title, detail, context_tag);
    }
  else
    {
      QMetaObject::invokeMethod (
        this,
        [this, severity, title, detail, context_tag] () {
          post_internal (severity, title, detail, context_tag);
        },
        Qt::QueuedConnection);
    }
}

void
NotificationCenter::postInfo (const QString &title, const QString &detail)
{
  post (Notification::Info, title, detail);
}

void
NotificationCenter::postSuccess (const QString &title, const QString &detail)
{
  post (Notification::Success, title, detail);
}

void
NotificationCenter::postWarning (const QString &title, const QString &detail)
{
  post (Notification::Warning, title, detail);
}

void
NotificationCenter::postError (const QString &title, const QString &detail)
{
  post (Notification::Error, title, detail);
}

void
NotificationCenter::postCritical (const QString &title, const QString &detail)
{
  post (Notification::Critical, title, detail);
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
  Notification::Severity severity,
  const QString         &title,
  const QString         &detail,
  const QString         &context_tag)
{
  auto notification = utils::make_qobject_unique<Notification> (
    severity, title, detail, context_tag, QString (), QString ());

  if (QAccessible::isActive ())
    {
      const QString text = detail.isEmpty () ? title : title + ". " + detail;
      QAccessible::updateAccessibility (
        new QAccessibleAnnouncementEvent (notification.get (), text));
    }

  auto * notification_ptr = notification.get ();
  owned_notifications_.push_back (std::move (notification));
  history_->add_notification (notification_ptr);
  Q_EMIT notificationPosted (notification_ptr);
}

} // namespace zrythm::gui
