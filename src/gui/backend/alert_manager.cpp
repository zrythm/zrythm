// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/alert_manager.h"

namespace zrythm::gui
{

AlertManager::AlertManager (QObject * parent) : QObject (parent) { }

void
AlertManager::showAlert (const QString &title, const QString &message)
{
  Q_EMIT alertRequested (title, message);
}
}
