// SPDX-FileCopyrightText: © 2018-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "gui/backend/zrythm_application.h"

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

  zrythm::gui::ZrythmApplication app (argc, argv);
  return app.exec ();
}
