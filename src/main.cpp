// SPDX-FileCopyrightText: © 2018-2021, 2024-2026 Alexandros Theodotou
// <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <iostream>
#include <ranges>

#include "gui/backend/zrythm_application.h"
#include "utils/logger.h"

#include <QCommandLineParser>
#include <QStringList>

/**
 * Application entry point.
 */
int
main (int argc, char ** argv)
{
  // GLib integration causes issues so disable (but honor env variable if set)
  if (!qEnvironmentVariableIsSet ("QT_NO_GLIB"))
    {
      qputenv ("QT_NO_GLIB", "1");
    }

#if defined(Q_OS_LINUX)
  // Use the XDG desktop portal for native file dialogs. This is the only
  // theme plugin shipped with the bundled Qt; without a portal the
  // dialog falls back to Qt's built-in one.
  qputenv ("QT_QPA_PLATFORMTHEME", "xdgdesktopportal");
#endif

  // handled before constructing the application: the checks verify data
  // the application constructor itself requires
  QCommandLineParser verify_parser;
  verify_parser.addOption (
    { QStringLiteral ("verify-installation"),
      QStringLiteral ("Verify that the installation is complete, then exit") });
  QStringList args;
  for (const auto i : std::views::iota (0, argc))
    {
      args << QString::fromUtf8 (argv[i]);
    }
  verify_parser.parse (args);
  if (verify_parser.isSet ("verify-installation"))
    {
      const auto result = zrythm::gui::ZrythmApplication::verify_installation ();
      if (!result)
        {
          std::cerr
            << "installation check failed: " << result.error ().view ()
            << std::endl;
          return EXIT_FAILURE;
        }
      std::cout << "installation OK" << std::endl;
      return EXIT_SUCCESS;
    }

  try
    {
      zrythm::gui::ZrythmApplication app (argc, argv);
      return app.exec ();
    }
  catch (const std::exception &e)
    {
      // startup errors are logged and shown as a dialog by the
      // ZrythmApplication constructor before rethrowing
      z_critical ("Unhandled exception: {}", e.what ());
      return EXIT_FAILURE;
    }
}
