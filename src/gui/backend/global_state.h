// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "gui/backend/backend/zrythm.h"
#include "gui/backend/zrythm_application.h"

class GlobalState : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON
public:
  Q_PROPERTY (Zrythm * zrythm READ zrythm CONSTANT FINAL)
  Q_PROPERTY (
    zrythm::gui::ZrythmApplication * application READ application CONSTANT FINAL)

public:
  GlobalState (QObject * parent = nullptr) : QObject (parent) { }
  Zrythm *                 zrythm () const { return Zrythm::getInstance (); }
  gui::ZrythmApplication * application () const
  {
    return qobject_cast<gui::ZrythmApplication *> (qApp);
  }
};
