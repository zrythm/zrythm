// SPDX-FileCopyrightText: © 2018-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "zrythm_app.h"

#include "gtk_wrapper.h"

/**
 * Application entry point.
 */
int
main (int argc, char ** argv)
{
  Glib::init ();

  /* send activate signal */
  zrythm_app = zrythm_app_new (argc, (const char **) argv);

  int ret = g_application_run (G_APPLICATION (zrythm_app.get ()), argc, argv);

  return ret;
}
