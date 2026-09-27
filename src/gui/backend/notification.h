// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QDateTime>
#include <QObject>
#include <QString>
#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::gui
{

/**
 * @brief A single user-facing message posted through a NotificationCenter.
 *
 * Notifications are created by NotificationCenter::post() and owned by the
 * center for the center's lifetime; models and views hold bare pointers.
 * Removing a notification from the history does not delete it; the object is
 * destroyed with the center.
 */
class Notification : public QObject
{
  Q_OBJECT
  Q_PROPERTY (
    zrythm::gui::Notification::Severity severity READ severity CONSTANT FINAL)
  Q_PROPERTY (QString title READ title CONSTANT FINAL)
  Q_PROPERTY (QString detail READ detail CONSTANT FINAL)
  Q_PROPERTY (QString contextTag READ contextTag CONSTANT FINAL)
  Q_PROPERTY (QDateTime timestamp READ timestamp CONSTANT FINAL)
  Q_PROPERTY (
    bool acknowledged READ isAcknowledged WRITE setAcknowledged NOTIFY
      acknowledgedChanged FINAL)
  Q_PROPERTY (QString actionLabel READ actionLabel CONSTANT FINAL)
  Q_PROPERTY (QString actionId READ actionId CONSTANT FINAL)
  QML_ELEMENT
  QML_UNCREATABLE ("Notifications are created by NotificationCenter::post()")

public:
  /**
   * How urgent the message is and which surface presents it.
   *
   * Info/Success/Warning/Error are presented as toasts by the visible
   * window; Critical opens a modal dialog.
   */
  enum Severity
  {
    Info,
    Success,
    Warning,
    Error,
    Critical,
  };
  Q_ENUM (Severity)

  /**
   * @brief Creates a notification.
   *
   * @param severity Urgency and presenting surface.
   * @param title Short user-facing summary.
   * @param detail Optional longer explanation.
   * @param context_tag Groups occurrences for coalescing.
   * @param action_label Optional label of the toast action button.
   * @param action_id Id the hosting window resolves to a callable.
   * @param parent Owning center.
   */
  Notification (
    Severity       severity,
    const QString &title,
    const QString &detail,
    const QString &context_tag,
    const QString &action_label,
    const QString &action_id,
    QObject *      parent = nullptr);

  Severity         severity () const { return severity_; }
  const QString   &title () const { return title_; }
  const QString   &detail () const { return detail_; }
  const QString   &contextTag () const { return context_tag_; }
  const QDateTime &timestamp () const { return timestamp_; }
  bool             isAcknowledged () const { return acknowledged_; }
  void             setAcknowledged (bool acknowledged);
  const QString   &actionLabel () const { return action_label_; }
  const QString   &actionId () const { return action_id_; }

  /**
   * @brief Returns true if this notification coalesces with @p other:
   * they share severity, title and context tag, so a visible toast
   * presenting one of them absorbs the other.
   */
  Q_INVOKABLE bool coalescesWith (const zrythm::gui::Notification * other) const
  {
    return severity_ == other->severity_ && title_ == other->title_
           && context_tag_ == other->context_tag_;
  }

Q_SIGNALS:
  void acknowledgedChanged (bool acknowledged);

private:
  Severity  severity_ = Info;
  QString   title_;
  QString   detail_;
  QString   context_tag_;
  QDateTime timestamp_;
  bool      acknowledged_ = false;
  QString   action_label_;
  QString   action_id_;
};

} // namespace zrythm::gui
