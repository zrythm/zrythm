// SPDX-FileCopyrightText: © 2018-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "gui/cpp/zrythm_application.h"

/**
 * Application entry point.
 */
int
main (int argc, char ** argv)
{
  zrythm::gui::ZrythmApplication app (argc, argv);
  return app.exec ();
}
