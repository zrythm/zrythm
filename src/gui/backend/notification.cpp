// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/notification.h"

namespace zrythm::gui
{

Notification::Notification (
  Severity       severity,
  const QString &title,
  const QString &detail,
  const QString &context_tag,
  const QString &action_label,
  const QString &action_id,
  QObject *      parent)
    : QObject (parent), severity_ (severity), title_ (title), detail_ (detail),
      context_tag_ (context_tag), timestamp_ (QDateTime::currentDateTime ()),
      action_label_ (action_label), action_id_ (action_id)
{
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
