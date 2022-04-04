// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * Copyright (C) 2018-2021 Alexandros Theodotou <alex at zrythm dot org>
 */

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
  LOG = log_new ();

  /* send activate signal */
  zrythm_app =
    zrythm_app_new (argc, (const char **) argv);

  int ret = g_application_run (
    G_APPLICATION (zrythm_app), argc, argv);
  g_object_unref (zrythm_app);

  log_free (LOG);

  return ret;
}
