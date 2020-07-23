/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Zrythm test helper.
 */

#ifndef __TEST_HELPERS_ZRYTHM_H__
#define __TEST_HELPERS_ZRYTHM_H__

#include "zrythm-test-config.h"

#include <signal.h>

#include "audio/chord_track.h"
#include "audio/engine_dummy.h"
#include "audio/marker_track.h"
#include "audio/tracklist.h"
#include <project.h>
#include "utils/backtrace.h"
#include "utils/cairo.h"
#include "utils/objects.h"
#include "utils/flags.h"
#include "utils/io.h"
#include "utils/log.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib.h>
#include <glib/gi18n.h>

/**
 * @addtogroup tests
 *
 * @{
 */

/** Time to run fishbowl, in seconds */
#define DEFAULT_FISHBOWL_TIME 20

/** Compares 2 Position pointers. */
#define g_assert_cmppos(a,b) \
  g_assert_cmpint ( \
    (a)->bars, ==, (b)->bars); \
  g_assert_cmpint ( \
    (a)->beats, ==, (b)->beats); \
  g_assert_cmpint ( \
    (a)->sixteenths, ==, (b)->sixteenths); \
  g_assert_cmpint ( \
    (a)->ticks, ==, (b)->ticks); \
  g_assert_cmpfloat_with_epsilon ( \
    (a)->sub_tick, (b)->sub_tick, 0.0001); \
  g_assert_cmpfloat_with_epsilon ( \
    (a)->total_ticks, (b)->total_ticks, 0.0001); \
  g_assert_cmpint ( \
    (a)->frames, ==, (b)->frames)

#ifndef G_APPROX_VALUE
#define G_APPROX_VALUE(a, b, epsilon) \
  (((a) > (b) ? (a) - (b) : (b) - (a)) < (epsilon))
#endif

#ifndef g_assert_cmpfloat_with_epsilon
#define g_assert_cmpfloat_with_epsilon( \
  n1,n2,epsilon) \
  G_STMT_START { \
    double __n1 = (n1), __n2 = (n2), __epsilon = (epsilon); \
    if (G_APPROX_VALUE (__n1,  __n2, __epsilon)) ; else \
      g_assertion_message_cmpnum (G_LOG_DOMAIN, __FILE__, __LINE__, G_STRFUNC, \
      #n1 " == " #n2 " (+/- " #epsilon ")", __n1, "==", __n2, 'f'); \
  } G_STMT_END
#endif

static void
segv_handler (int sig)
{
  char prefix[200];
#ifdef _WOE32
  strcpy (
    prefix, _("Error - Backtrace:\n"));
#else
  sprintf (
    prefix,
    _("Error: %s - Backtrace:\n"), strsignal (sig));
#endif
  char * bt = backtrace_get (prefix, 100);

  g_warning ("%s", bt);

  exit (sig);
}

/**
 * To be called by every test's main to initialize
 * Zrythm to default values.
 */
void
test_helper_zrythm_init ()
{
  object_free_w_func_and_null (
    zrythm_free, ZRYTHM);

  ZRYTHM = zrythm_new (false, true);

  /* init logging to custom file */
  char * tmp_log_dir =
    g_build_filename (
      g_get_tmp_dir (), "zrythm_test_logs", NULL);
  io_mkdir (tmp_log_dir);
  char * tmp_log_file_template =
    g_build_filename (
      tmp_log_dir, "XXXXXX.log", NULL);
  g_free (tmp_log_dir);
  int tmp_log_file =
    g_mkstemp (tmp_log_file_template);
  g_free (tmp_log_file_template);
  log_init_with_file (LOG, true, tmp_log_file);
  log_init_writer_idle (LOG, 1);

  ZRYTHM->create_project_path =
    g_dir_make_tmp (
      "zrythm_test_project_XXXXXX", NULL);
  project_load (NULL, 0);

  /* set a segv handler */
  signal (SIGSEGV, segv_handler);
}

/**
 * To be called by every test's main at the end to
 * clean up (TODO).
 */
void
test_helper_zrythm_cleanup ()
{
  object_free_w_func_and_null (
    zrythm_free, ZRYTHM);
}

/**
 * To be called after test_helper_zrythm_init() to
 * initialize the UI (GTK).
 */
void
test_helper_zrythm_gui_init (
  int argc, char *argv[])
{
  ZRYTHM->have_ui = 1;
  if (gtk_init_check (&argc, &argv))
    {
      gtk_init (&argc, &argv);
    }
  else
    {
      g_test_skip ("No display found");
      exit (77);
    }

  /* set theme */
  g_object_set (gtk_settings_get_default (),
                "gtk-theme-name",
                "Matcha-dark-sea",
                NULL);
  g_message ("set theme");

  gtk_icon_theme_add_resource_path (
    gtk_icon_theme_get_default (),
    "/org/zrythm/Zrythm/app/icons/breeze-icons");
  gtk_icon_theme_add_resource_path (
    gtk_icon_theme_get_default (),
    "/org/zrythm/Zrythm/app/icons/fork-awesome");
  gtk_icon_theme_add_resource_path (
    gtk_icon_theme_get_default (),
    "/org/zrythm/Zrythm/app/icons/font-awesome");
  gtk_icon_theme_add_resource_path (
    gtk_icon_theme_get_default (),
    "/org/zrythm/Zrythm/app/icons/zrythm");
  gtk_icon_theme_add_resource_path (
    gtk_icon_theme_get_default (),
    "/org/zrythm/Zrythm/app/icons/ext");
  gtk_icon_theme_add_resource_path (
    gtk_icon_theme_get_default (),
    "/org/zrythm/Zrythm/app/icons/gnome-builder");

  g_message ("set resource paths");

  // set default css provider
  GtkCssProvider * css_provider =
    gtk_css_provider_new();
  gtk_css_provider_load_from_resource (
    css_provider,
    "/org/zrythm/Zrythm/app/theme.css");
  gtk_style_context_add_provider_for_screen (
          gdk_screen_get_default (),
          GTK_STYLE_PROVIDER (css_provider),
          800);
  g_object_unref (css_provider);
  g_message ("set default css provider");

  CAIRO_CACHES = z_cairo_caches_new ();
  UI_CACHES = ui_caches_new ();
}

/**
 * @}
 */

#endif
