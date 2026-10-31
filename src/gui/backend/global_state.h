// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
/* SPDX-License-Identifier: LicenseRef-ZrythmLicense */

#pragma once

#include "gui/backend/zrythm_application.h"

#include <QCoreApplication>

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
    /* the application instance is not always a ZrythmApplication (test
     * harnesses create a plain QGuiApplication); qobject_cast from the
     * declared return type of instance () returns null in that case */
    return qobject_cast<zrythm::gui::ZrythmApplication *> (
      QCoreApplication::instance ());
  }
};
