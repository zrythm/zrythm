// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/translation_manager.h"
#include "gui/backend/zrythm_application.h"

#include <QtQml/QQmlContext>

namespace zrythm::gui
{

TranslationManager::TranslationManager (QObject * parent) : QObject (parent)
{
  QTimer::singleShot (0, this, [this] {
    auto locale = SettingsManager::uiLocale ();
    loadTranslation (locale);
  });
}

TranslationManager *
TranslationManager::get_instance ()
{
  return dynamic_cast<ZrythmApplication *> (qApp)->translationManager ();
}

QString
TranslationManager::getSystemLocale ()
{
  return QLocale ().name ();
}

void
TranslationManager::loadTranslation (const QString &locale)
{
  // Load system locale by default
  QString locale_to_use = locale.isEmpty () ? getSystemLocale () : locale;
  z_debug ("Loading translation for locale: {}", locale_to_use);

  // Remove existing translation if any
  if (translator_loaded_)
    {
      QCoreApplication::removeTranslator (&translator_);
    }

  // Load new translation
  const auto success =
    translator_.load (QString (u":/i18n/zrythm_%1"_s).arg (locale_to_use));

  if (success)
    {
      QCoreApplication::installTranslator (&translator_);
      translator_loaded_ = true;
      auto * engine =
        dynamic_cast<ZrythmApplication *> (qApp)->get_qml_engine ();
      if (engine != nullptr)
        {
          z_debug ("Retranslating...");
          engine->retranslate ();
        }
      else
        {
          z_warning ("Failed to retranslate - engine not found");
        }
      SettingsManager::get_instance ()->set_uiLocale (locale);
      z_info ("Loaded translation for locale: {}", locale_to_use);
    }
  else
    {
      z_warning ("Failed to load translation for locale: {}", locale_to_use);
    }
}

} // namespace zrythm::gui
