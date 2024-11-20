// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "zrythm-config.h"

#include <QtQmlIntegration>

namespace zrythm::gui
{

class TranslationManager : public QObject
{
  Q_OBJECT
  QML_ELEMENT

public:
  explicit TranslationManager (QObject * parent = nullptr);

  TranslationManager * get_instance ();

  Q_INVOKABLE static QString getSystemLocale ();

  Q_INVOKABLE void loadTranslation (const QString &locale);

private:
  QTranslator translator_;
  bool        translator_loaded_ = false;
};

} // namespace zrythm::gui