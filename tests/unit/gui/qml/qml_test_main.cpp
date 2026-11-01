// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <memory>

#include "utils/app_settings.h"

#include <QQmlContext>
#include <QQmlEngine>
#include <QtQuickTest>

#include "helpers/in_memory_settings_backend.h"

namespace
{
class QmlTestSetup final : public QObject
{
  Q_OBJECT

private Q_SLOTS:
  void qmlEngineAvailable (QQmlEngine * engine)
  {
    settings_ = std::make_unique<zrythm::utils::AppSettings> (
      std::make_unique<zrythm::test_helpers::InMemorySettingsBackend> (), this);
    engine->rootContext ()->setContextProperty (
      "testAppSettings", settings_.get ());
  }

private:
  std::unique_ptr<zrythm::utils::AppSettings> settings_;
};
}

QUICK_TEST_MAIN_WITH_SETUP (qml_tests, QmlTestSetup)

#include "qml_test_main.moc"
