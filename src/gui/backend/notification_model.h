// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <span>

#include "gui/backend/notification.h"

#include <QAbstractListModel>
#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::gui
{

/**
 * @brief Retained notification history, newest first.
 *
 * Every posted occurrence is stored as its own row (the history does not
 * coalesce; only visible toasts do). Notifications are owned by the
 * NotificationCenter and outlive their rows.
 */
class NotificationModel : public QAbstractListModel
{
  Q_OBJECT
  Q_PROPERTY (
    int unacknowledgedAttentionCount READ unacknowledgedAttentionCount NOTIFY
      unacknowledgedAttentionChanged FINAL)
  Q_PROPERTY (
    zrythm::gui::Notification::Severity highestUnacknowledgedAttentionSeverity
      READ highestUnacknowledgedAttentionSeverity NOTIFY
        unacknowledgedAttentionChanged FINAL)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  enum Roles
  {
    SeverityRole = Qt::UserRole + 1,
    TitleRole,
    DetailRole,
    ContextTagRole,
    TimestampRole,
    AcknowledgedRole,
    ActionLabelRole,
    NotificationRole,
  };

  explicit NotificationModel (QObject * parent = nullptr);

  int rowCount (const QModelIndex &parent = QModelIndex ()) const override;
  QVariant
  data (const QModelIndex &index, int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames () const override;

  /** Adds @p notification at the front. */
  void add_notification (Notification * notification);

  /** Marks every retained notification as acknowledged. */
  void set_all_acknowledged ();

  /** Drops every retained notification from the history. */
  void clear_all ();

  /**
   * @brief Number of retained unacknowledged notifications with
   * Warning or Error severity.
   */
  int unacknowledgedAttentionCount () const;

  /**
   * @brief Highest unacknowledged Warning or Error severity, or Info
   * when the count is zero.
   */
  Notification::Severity highestUnacknowledgedAttentionSeverity () const;

  std::span<Notification * const> entries () const;

  static constexpr int MAX_NOTIFICATIONS = 100;

Q_SIGNALS:
  void unacknowledgedAttentionChanged ();

private:
  std::vector<Notification *> notifications_;
};

} // namespace zrythm::gui
