// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
/* SPDX-License-Identifier: LicenseRef-ZrythmLicense */

#pragma once

#include "gui/backend/zrythm_application.h"

class GlobalState : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON
public:
  Q_PROPERTY (
    zrythm::gui::ZrythmApplication * application READ application CONSTANT FINAL)

public:
  GlobalState (QObject * parent = nullptr) : QObject (parent) { }
  zrythm::gui::ZrythmApplication * application () const
  {
    return qobject_cast<zrythm::gui::ZrythmApplication *> (qApp);
  }
};
