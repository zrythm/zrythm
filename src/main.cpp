// SPDX-FileCopyrightText: © 2018-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include <QFontDatabase>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "zrythm_application.h"
// #include <kddockwidgets/KDDockWidgets.h>

/**
 * Application entry point.
 */
int
main (int argc, char ** argv)
{
  ZrythmApplication app (argc, argv);

  // Create and show the main window
  QQmlApplicationEngine engine;

  engine.addImportPath (":/org.zrythm.Zrythm/imports");

  const QUrl url (
    u"qrc:/org.zrythm.Zrythm/imports/Zrythm/resources/ui/main_window.qml"_qs);
  engine.load (url);

  if (!engine.rootObjects ().isEmpty ())
    {
      qDebug () << "QML file loaded successfully";
    }
  else
    {
      qDebug () << "Failed to load QML file";
    }

  // Load custom font
  int fontId = QFontDatabase::addApplicationFont (
    "/usr/share/fonts/cantarell/Cantarell-VF.otf");
  if (fontId == -1)
    {
      qWarning () << "Failed to load custom font";
    }

  // start the event loop
  return app.exec ();
}
