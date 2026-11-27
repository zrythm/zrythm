// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <memory>

#include "utils/app_settings.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QQmlContext>
#include <QQmlEngine>
#include <QtQuickTest>

#include "helpers/in_memory_settings_backend.h"

namespace
{
// Exposed to the tests as the context property "testUtils".
class QmlTestSetup final : public QObject
{
  Q_OBJECT

public:
  Q_INVOKABLE static QString clipboardText ()
  {
    return QGuiApplication::clipboard ()->text ();
  }

  Q_INVOKABLE static void
  setEnvironmentVariable (const QString &name, const QString &value)
  {
    const auto name_bytes = name.toUtf8 ();
    qputenv (name_bytes.constData (), value.toUtf8 ());
  }

  Q_INVOKABLE static void unsetEnvironmentVariable (const QString &name)
  {
    qunsetenv (name.toUtf8 ().constData ());
  }

private Q_SLOTS:
  void qmlEngineAvailable (QQmlEngine * engine)
  {
    settings_ = std::make_unique<zrythm::utils::AppSettings> (
      std::make_unique<zrythm::test_helpers::InMemorySettingsBackend> (),
      zrythm::utils::UserDirectories{}, this);
    engine->rootContext ()->setContextProperty (
      "testAppSettings", settings_.get ());
    engine->rootContext ()->setContextProperty ("testUtils", this);
  }

private:
  std::unique_ptr<zrythm::utils::AppSettings> settings_;
};
}

QUICK_TEST_MAIN_WITH_SETUP (qml_tests, QmlTestSetup)

#include "qml_test_main.moc"
