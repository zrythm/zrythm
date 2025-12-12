// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/app_settings.h"

#include <QTranslator>
#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::gui
{

class TranslationManager : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  explicit TranslationManager (
    utils::AppSettings &app_settings,
    QObject *           parent = nullptr);

  TranslationManager * get_instance ();

  Q_INVOKABLE static QString getSystemLocale ();

  Q_INVOKABLE void loadTranslation (const QString &locale);

private:
  utils::AppSettings &app_settings_;
  QTranslator         translator_;
  bool                translator_loaded_ = false;
};

} // namespace zrythm::gui
