// SPDX-FileCopyrightText: © 2019-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

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
#include "plugins/plugin_manager.h"
#include "utils/backtrace.h"
#include "utils/cairo.h"
#include "utils/datetime.h"
#include "utils/flags.h"
#include "utils/io.h"
#include "utils/log.h"
#include "utils/objects.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib.h>
#include <glib/gi18n.h>

#include <project.h>

/**
 * @addtogroup tests
 *
 * @{
 */

void
test_helper_zrythm_init (void);
void
test_helper_zrythm_init_optimized (void);
void
test_helper_zrythm_cleanup (void);
void
test_helper_zrythm_gui_init (int argc, char * argv[]);

/** Time to run fishbowl, in seconds */
#define DEFAULT_FISHBOWL_TIME 20

/** Compares 2 Position pointers. */
#define g_assert_cmppos(a, b) \
  g_assert_cmpfloat_with_epsilon ( \
    (a)->ticks, (b)->ticks, 0.0001); \
  g_assert_cmpint ((a)->frames, ==, (b)->frames)

#ifndef G_APPROX_VALUE
#  define G_APPROX_VALUE(a, b, epsilon) \
    (((a) > (b) ? (a) - (b) : (b) - (a)) < (epsilon))
#endif

#ifndef g_assert_cmpfloat_with_epsilon
#  define g_assert_cmpfloat_with_epsilon(n1, n2, epsilon) \
    G_STMT_START \
    { \
      double __n1 = (n1), __n2 = (n2), __epsilon = (epsilon); \
      if (G_APPROX_VALUE (__n1, __n2, __epsilon)) \
        ; \
      else \
        g_assertion_message_cmpnum ( \
          G_LOG_DOMAIN, __FILE__, __LINE__, G_STRFUNC, \
          #n1 " == " #n2 " (+/- " #epsilon ")", __n1, \
          "==", __n2, 'f'); \
    } \
    G_STMT_END
#endif

#ifndef g_assert_cmpfloat
#  define g_assert_cmpfloat(n1, n2) \
    G_STMT_START \
    { \
      double \
        __n1 = (n1), \
        __n2 = (n2), __epsilon = (FLT_EPSILON); \
      if (G_APPROX_VALUE (__n1, __n2, __epsilon)) \
        ; \
      else \
        g_assertion_message_cmpnum ( \
          G_LOG_DOMAIN, __FILE__, __LINE__, G_STRFUNC, \
          #n1 " == " #n2 " (+/- " #epsilon ")", __n1, \
          "==", __n2, 'f'); \
    } \
    G_STMT_END
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored \
  "-Wanalyzer-unsafe-call-within-signal-handler"
static void
segv_handler (int sig)
{
  char prefix[200];
#ifdef _WOE32
  strcpy (prefix, _ ("Error - Backtrace:\n"));
#else
  sprintf (
    prefix, _ ("Error: %s - Backtrace:\n"), strsignal (sig));
#endif
  char * bt = backtrace_get (prefix, 100, false);

  g_warning ("%s", bt);

  exit (sig);
}
#pragma GCC diagnostic pop

static void
_test_helper_zrythm_init (
  bool optimized,
  int  samplerate,
  int  buf_size)
{
  object_free_w_func_and_null (zrythm_free, ZRYTHM);
  object_free_w_func_and_null (log_free, LOG);

  Log ** log_ptr = log_get ();
  Log *  log_obj = log_new ();
  *log_ptr = log_obj;
  LOG = log_obj;

  ZRYTHM = zrythm_new (NULL, false, true, optimized);
  ZRYTHM->undo_stack_len = 64;

  /* init logic - note: will use a random dir in
   * tmp as the user dire */
  zrythm_init_user_dirs_and_files (ZRYTHM);
  zrythm_init_templates (ZRYTHM);

  /* dummy ZrythmApp object for testing */
  ZrythmApp ** zapp_ptr = zrythm_app_get ();
  ZrythmApp *  zapp = object_new (ZrythmApp);
  *zapp_ptr = zapp;
  zapp->gtk_thread = g_thread_self ();
  zrythm_app = zapp;
  zrythm_app->samplerate = samplerate;
  zrythm_app->buf_size = buf_size;

  /* init logging to custom file */
  char * tmp_log_dir = g_build_filename (
    g_get_tmp_dir (), "zrythm_test_logs", NULL);
  io_mkdir (tmp_log_dir);
  char * str_datetime = datetime_get_for_filename ();
  char * log_filepath = g_strdup_printf (
    "%s%slog_%s.log", tmp_log_dir, G_DIR_SEPARATOR_S,
    str_datetime);
  g_free (str_datetime);
  g_free (tmp_log_dir);
  log_init_with_file (LOG, log_filepath);
  log_init_writer_idle (LOG, 1);

  ZRYTHM->create_project_path =
    g_dir_make_tmp ("zrythm_test_project_XXXXXX", NULL);
  project_load (NULL, false);

  /* adaptive snap only supported with UI */
  SNAP_GRID_TIMELINE->snap_adaptive = false;

  /* set a segv handler */
  signal (SIGSEGV, segv_handler);

  /* append lv2 path for test plugins */
  char * tmp = PLUGIN_MANAGER->lv2_path;
  PLUGIN_MANAGER->lv2_path = g_strdup_printf (
    "%s%s%s", PLUGIN_MANAGER->lv2_path,
    G_SEARCHPATH_SEPARATOR_S, TESTS_BUILDDIR);
  g_free (tmp);
}

/**
 * To be called by every test's main to initialize
 * Zrythm to default values.
 */
void
test_helper_zrythm_init ()
{
  _test_helper_zrythm_init (false, 0, 0);
}

void
test_helper_zrythm_init_optimized ()
{
  _test_helper_zrythm_init (true, 0, 0);
}

/**
 * To be called by every test's main at the end to
 * clean up.
 */
void
test_helper_zrythm_cleanup ()
{
  g_assert_nonnull (ZRYTHM->testing_dir);
  io_rmdir (ZRYTHM->testing_dir, true);
  object_free_w_func_and_null (zrythm_free, ZRYTHM);
  object_free_w_func_and_null (log_free, LOG);
}

/**
 * To be called after test_helper_zrythm_init() to
 * initialize the UI (GTK).
 */
void
test_helper_zrythm_gui_init (int argc, char * argv[])
{
  ZRYTHM->have_ui = 1;
  if (gtk_init_check ())
    {
      gtk_init ();
    }
  else
    {
      g_test_skip ("No display found");
      exit (77);
    }

  /* set theme */
  g_object_set (
    gtk_settings_get_default (), "gtk-theme-name",
    "Matcha-dark-sea", NULL);
  g_message ("set theme");

  GdkDisplay *   display = gdk_display_get_default ();
  GtkIconTheme * icon_theme =
    gtk_icon_theme_get_for_display (display);
  gtk_icon_theme_add_resource_path (
    icon_theme, "/org/zrythm/Zrythm/app/icons/breeze-icons");
  gtk_icon_theme_add_resource_path (
    icon_theme, "/org/zrythm/Zrythm/app/icons/fork-awesome");
  gtk_icon_theme_add_resource_path (
    icon_theme, "/org/zrythm/Zrythm/app/icons/font-awesome");
  gtk_icon_theme_add_resource_path (
    icon_theme, "/org/zrythm/Zrythm/app/icons/zrythm");
  gtk_icon_theme_add_resource_path (
    icon_theme, "/org/zrythm/Zrythm/app/icons/ext");
  gtk_icon_theme_add_resource_path (
    icon_theme, "/org/zrythm/Zrythm/app/icons/gnome-builder");

  g_message ("set resource paths");

  // set default css provider
  GtkCssProvider * css_provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_resource (
    css_provider, "/org/zrythm/Zrythm/app/theme.css");
  gtk_style_context_add_provider_for_display (
    display, GTK_STYLE_PROVIDER (css_provider), 800);
  g_object_unref (css_provider);
  g_message ("set default css provider");

  CAIRO_CACHES = z_cairo_caches_new ();
  UI_CACHES = ui_caches_new ();
}

/**
 * @}
 */

#endif
