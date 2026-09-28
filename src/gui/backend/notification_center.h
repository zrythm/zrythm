// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <functional>
#include <vector>

#include "gui/backend/notification.h"
#include "gui/backend/notification_model.h"
#include "utils/qt.h"

#include <QJSValue>
#include <QObject>
#include <QString>
#include <QVector>
#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::gui
{

/**
 * @brief Routes user-facing messages to the visible window and the history.
 *
 * The center owns every notification it posts for its own lifetime.
 *
 * post() is callable from any thread and delivers on the GUI thread. It is
 * not real-time-safe (it allocates); realtime contexts keep using the
 * realtime logger, which stays developer-facing.
 */
class NotificationCenter : public QObject
{
  Q_OBJECT
  Q_PROPERTY (
    zrythm::gui::NotificationModel * history READ history CONSTANT FINAL)
  QML_ELEMENT

public:
  explicit NotificationCenter (QObject * parent = nullptr);

  /**
   * @brief Posts a message to the user.
   *
   * Callable from any thread. Every post is logged once, on the calling
   * thread, at info level for Info/Success and warning level otherwise
   * (user-facing messages are expected conditions, not defects: the
   * error log level aborts with a backtrace and is reserved for bugs).
   *
   * @param context_tag Groups occurrences for coalescing: posts sharing
   *   severity, title and tag merge into one visible toast with a count.
   * @param action_label Label of the toast action button; requires
   *   @p action_callback.
   * @param action_callback Callable invoked when the action button is
   *   activated. Only carried by GUI-thread posts.
   */
  void post (
    Notification::Severity severity,
    const QString         &title,
    const QString         &detail = {},
    const QString         &context_tag = {},
    const QString         &action_label = {},
    const QJSValue        &action_callback = {});

  /**
   * @brief Posts a message with a C++ action callback.
   *
   * Callable from any thread; the callback is carried through the
   * queued delivery and invoked on the GUI thread when the action is
   * activated. Capture receivers via QPointer if they may be destroyed
   * before activation.
   *
   * @param action_label Label of the toast action button; requires
   *   @p action_callback.
   */
  void post (
    Notification::Severity severity,
    const QString         &title,
    const QString         &detail,
    const QString         &context_tag,
    const QString         &action_label,
    std::function<void ()> action_callback);

  Q_INVOKABLE void postInfo (
    const QString  &title,
    const QString  &detail,
    const QString  &context_tag = {},
    const QString  &action_label = {},
    const QJSValue &action_callback = {});
  Q_INVOKABLE void postSuccess (
    const QString  &title,
    const QString  &detail,
    const QString  &context_tag = {},
    const QString  &action_label = {},
    const QJSValue &action_callback = {});
  Q_INVOKABLE void postWarning (
    const QString  &title,
    const QString  &detail,
    const QString  &context_tag = {},
    const QString  &action_label = {},
    const QJSValue &action_callback = {});
  Q_INVOKABLE void postError (
    const QString  &title,
    const QString  &detail,
    const QString  &context_tag = {},
    const QString  &action_label = {},
    const QJSValue &action_callback = {});
  Q_INVOKABLE void postCritical (
    const QString  &title,
    const QString  &detail,
    const QString  &context_tag = {},
    const QString  &action_label = {},
    const QJSValue &action_callback = {});

  /** Marks every retained notification as acknowledged. */
  Q_INVOKABLE void acknowledgeAll ();

  /** Removes every retained notification from the history. */
  Q_INVOKABLE void clearHistory ();

  /**
   * @brief Returns the retained critical notifications that have not
   * been presented to the user yet.
   */
  Q_INVOKABLE QVector<zrythm::gui::Notification *>
              unacknowledgedCriticals () const;

  NotificationModel * history () const;

Q_SIGNALS:
  /**
   * @brief Emitted on the GUI thread for every posted notification, after
   * it is added to the history.
   */
  void notificationPosted (zrythm::gui::Notification * notification);

private:
  void post_internal (
    Notification::Severity       severity,
    const QString               &title,
    const QString               &detail,
    const QString               &context_tag,
    const QString               &action_label,
    Notification::ActionCallback action_callback);

  /** Logs, validates and dispatches a post for either callback carrier. */
  void post_dispatch (
    Notification::Severity       severity,
    const QString               &title,
    const QString               &detail,
    const QString               &context_tag,
    const QString               &action_label,
    Notification::ActionCallback action_callback);

  /** Announces, stores and emits an already created notification. */
  void finalize_post (utils::QObjectUniquePtr<Notification> notification);

  NotificationModel * history_ = nullptr;

  /** Owns every posted notification for the center's lifetime. */
  std::vector<utils::QObjectUniquePtr<Notification>> owned_notifications_;
};

} // namespace zrythm::gui
