// clang-format off
// SPDX-FileCopyrightText: © 2018-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// clang-format on

#include "zrythm-config.h"

#include "utils/log.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>

/**
 * Application entry point.
 */
int
main (int argc, char ** argv)
{
#ifdef _WOE32
  /* force old GL renderer on windows until this is fixed:
   * https://gitlab.gnome.org/GNOME/gtk/-/issues/6401 */
  g_setenv ("GSK_RENDERER", "gl", false);
#endif

  LOG = log_new ();

  /* send activate signal */
  zrythm_app = zrythm_app_new (argc, (const char **) argv);

  int ret = g_application_run (G_APPLICATION (zrythm_app), argc, argv);
  g_object_unref (zrythm_app);

  log_free (LOG);

  return ret;
}
