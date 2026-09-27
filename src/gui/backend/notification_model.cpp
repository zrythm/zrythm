// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>

#include "gui/backend/notification_model.h"

namespace zrythm::gui
{

NotificationModel::NotificationModel (QObject * parent)
    : QAbstractListModel (parent)
{
}

int
NotificationModel::rowCount (const QModelIndex &parent) const
{
  if (parent.isValid ())
    return 0;

  return static_cast<int> (notifications_.size ());
}

QVariant
NotificationModel::data (const QModelIndex &index, int role) const
{
  if (!index.isValid () || index.row () >= rowCount ())
    return {};

  const Notification * notification = notifications_[index.row ()];
  switch (role)
    {
    case SeverityRole:
      return static_cast<int> (notification->severity ());
    case TitleRole:
      return notification->title ();
    case DetailRole:
      return notification->detail ();
    case ContextTagRole:
      return notification->contextTag ();
    case TimestampRole:
      return notification->timestamp ();
    case AcknowledgedRole:
      return notification->isAcknowledged ();
    case ActionLabelRole:
      return notification->actionLabel ();
    case ActionIdRole:
      return notification->actionId ();
    case NotificationRole:
      return QVariant::fromValue (const_cast<Notification *> (notification));
    default:
      return {};
    }
}

QHash<int, QByteArray>
NotificationModel::roleNames () const
{
  QHash<int, QByteArray> roles;
  roles[SeverityRole] = "severity";
  roles[TitleRole] = "title";
  roles[DetailRole] = "detail";
  roles[ContextTagRole] = "contextTag";
  roles[TimestampRole] = "timestamp";
  roles[AcknowledgedRole] = "acknowledged";
  roles[ActionLabelRole] = "actionLabel";
  roles[ActionIdRole] = "actionId";
  roles[NotificationRole] = "notification";
  return roles;
}

void
NotificationModel::add_notification (Notification * notification)
{
  beginInsertRows (QModelIndex (), 0, 0);
  notifications_.insert (notifications_.begin (), notification);
  endInsertRows ();

  if (static_cast<int> (notifications_.size ()) > MAX_NOTIFICATIONS)
    {
      const auto first_removed = MAX_NOTIFICATIONS;
      const auto last_removed = static_cast<int> (notifications_.size ()) - 1;
      beginRemoveRows (QModelIndex (), first_removed, last_removed);
      notifications_.resize (MAX_NOTIFICATIONS);
      endRemoveRows ();
    }

  // Acknowledgement is set on the notification itself; the model reacts
  // so views and derived counts follow
  connect (
    notification, &Notification::acknowledgedChanged, this,
    [this, notification] () {
      const auto it = std::ranges::find (notifications_, notification);
      if (it == notifications_.end ())
        return;

      const auto row = static_cast<int> (it - notifications_.begin ());
      Q_EMIT dataChanged (index (row, 0), index (row, 0), { AcknowledgedRole });
      Q_EMIT unacknowledgedAttentionChanged ();
    });

  Q_EMIT unacknowledgedAttentionChanged ();
}

void
NotificationModel::set_all_acknowledged ()
{
  for (auto * notification : notifications_)
    notification->setAcknowledged (true);
}

void
NotificationModel::clear_all ()
{
  if (notifications_.empty ())
    return;

  beginResetModel ();
  notifications_.clear ();
  endResetModel ();
  Q_EMIT unacknowledgedAttentionChanged ();
}

int
NotificationModel::unacknowledgedAttentionCount () const
{
  return static_cast<int> (std::ranges::count_if (
    notifications_, [] (const Notification * notification) {
      return !notification->isAcknowledged ()
             && (notification->severity () == Notification::Severity::Warning
                 || notification->severity () == Notification::Severity::Error);
    }));
}

Notification::Severity
NotificationModel::highestUnacknowledgedAttentionSeverity () const
{
  const auto has_unacknowledged = [this] (Notification::Severity severity) {
    return std::ranges::any_of (
      notifications_, [severity] (const Notification * notification) {
        return !notification->isAcknowledged ()
               && notification->severity () == severity;
      });
  };
  if (has_unacknowledged (Notification::Severity::Error))
    return Notification::Severity::Error;
  if (has_unacknowledged (Notification::Severity::Warning))
    return Notification::Severity::Warning;
  return Notification::Severity::Info;
}

std::span<Notification * const>
NotificationModel::entries () const
{
  return notifications_;
}

} // namespace zrythm::gui
