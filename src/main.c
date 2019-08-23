/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU General Affero Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <ctype.h>
#include <execinfo.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "ext/audio_decoder/ad.h"
#include "gui/widgets/main_window.h"
#include "plugins/lv2/suil.h"
#include "utils/gtk.h"
#include "utils/objects.h"
#include "utils/ui.h"
#include "zrythm.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#ifdef HAVE_LIBGTOP
#include <glibtop.h>
#endif

#include <curl/curl.h>
#include <fftw3.h>

/** SIGSEGV handler. */
static void
handler (int sig) {
#ifdef _WIN32
    /* TODO */
#else
  void *array[20];
  size_t size;
  char ** strings;

  /* get void*'s for all entries on the stack */
  size = backtrace(array, 20);

  /* print out all the frames to stderr */
  char * str, * tmp, * tmp2;
  str =
    g_strdup_printf (
      _("Error: signal %d - Backtrace:"), sig);
  strings = backtrace_symbols (array, size);

  for (int i = 0; i < size; i++)
    {
      tmp2 = str;
      tmp = g_strdup_printf ("%s", strings[i]);
      str =
        g_strconcat (
          str, "\n", tmp, NULL);
      g_free (tmp);
      g_free (tmp2);
    }

  g_message ("%s", str);

  GtkDialogFlags flags =
    GTK_DIALOG_DESTROY_WITH_PARENT;
  GtkWidget * dialog =
    gtk_message_dialog_new_with_markup (
      GTK_WINDOW (MAIN_WINDOW),
      flags,
      GTK_MESSAGE_ERROR,
      GTK_BUTTONS_CLOSE,
      NULL);

  /* %23 is hash, %0A is new line */
  char * report_template =
    g_strdup_printf (
      "# Steps to reproduce\n"
      "> Write a list of steps to reproduce the "
      "bug\n\n"
      "# What happens?\n"
      "> Please tell us what happened\n\n"
      "# What is expected?\n"
      "> What is expected to happen?\n\n"
      "# Version\n%s\n\n"
      "# Other info\n"
      "> Context, etc.\n\n"
      "# Backtrace\n%s\n",
      PACKAGE_VERSION,
      str
      );
  CURL *curl = curl_easy_init();
  char * report_template_escaped =
    curl_easy_escape (
      curl, report_template, 0);
  char * atag =
    g_strdup_printf (
      "<a href=\"https://savannah.nongnu.org/support/?func=additem&amp;group=zrythm&amp;prefill[summary]=Bug Report (enter details)&amp;prefill[details]=%s\">",
      report_template_escaped);
  char * markup =
    g_strdup_printf (
      _("Zrythm has crashed. Please help us fix "
        "this by "
        "%ssubmitting a bug report%s."),
      atag,
      "</a>");

  gtk_message_dialog_set_markup (
    GTK_MESSAGE_DIALOG (dialog),
    markup);
  gtk_message_dialog_format_secondary_markup (
    GTK_MESSAGE_DIALOG (dialog),
    str);
  GtkLabel * label =
    z_gtk_message_dialog_get_label (
      GTK_MESSAGE_DIALOG (dialog), 1);
  gtk_label_set_selectable (
    label, 1);
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);

  free (strings);
  g_free (str);
  g_free (report_template);
  g_free (atag);
  g_free (markup);
  curl_free (report_template_escaped);

  curl_easy_cleanup (curl);
#endif
  exit(1);
}

static void
print_version ()
{
  fprintf (
    stdout,
    "Zrythm %s\nbuilt with %s %s\n"
    "%s\n%s\n%s\n%s\n",
    PACKAGE_VERSION,
    COMPILER, COMPILER_VERSION,
    "Copyright (C) 2018-2019 Alexandros Theodotou",
    "License AGPLv3+: GNU AGPL version 3 or later <https://www.gnu.org/licenses/agpl.html>",
    "This is free software: you are free to change and redistribute it.",
    "There is NO WARRANTY, to the extent permitted by law.");
}

static void
print_help ()
{
  fprintf (
    stdout,
    _("Usage: zrythm [ OPTIONS ] [ PROJECT-NAME ]\n\n"
    "Options:\n"
    "  -h, --help      display this help and exit\n"
    "  -v, --version   output version information and exit\n\n"
    "Examples:\n"
    "  zrythm          run normally\n\n"
    "Write comments and bugs to https://savannah.nongnu.org/support/?group=zrythm\n"
    "Support this project at https://liberapay.com/Zrythm\n"
    "Website https://www.zrythm.org\n"));
}

/**
 * main
 */
int
main (int    argc,
      char **argv)
{
  int c, option_index;
  static struct option long_options[] =
    {
      {"version", no_argument, 0, 'v'},
      {"help", no_argument, 0, 'h'},
      {0, 0, 0, 0}
    };
  opterr = 0;

  while (1)
    {
      c =
        getopt_long (
          argc, argv, "vh",
          long_options, &option_index);

      /* Detect the end of the options. */
      if (c == -1)
        break;

      switch (c)
        {
        case 'v':
          print_version ();
          return 0;
        case 'h':
          print_help ();
          return 0;
          break;
        case '?':
          /* getopt_long already printed an error
           * message */
          return 1;
        default:
          abort ();
        }
    }

  fprintf (
    stdout,
    _("Zrythm-%s Copyright (C) 2018-2019 Alexandros Theodotou\n\n"
    "Zrythm comes with ABSOLUTELY NO WARRANTY!\n\n"
    "This is free software, and you are welcome to redistribute it\n"
    "under certain conditions. Look at the file `COPYING' within this\n"
    "distribution for details.\n\n"
    "Write comments and bugs to https://savannah.nongnu.org/support/?group=zrythm\n"
    "Support this project at https://liberapay.com/Zrythm\n\n"),
    PACKAGE_VERSION);

  /* unset GTK_THEME */
  g_message ("GTK_THEME=%s", getenv ("GTK_THEME"));
  putenv ("GTK_THEME=");

  /* install segfault handler */
  g_message ("Installing signal handler...");
  signal(SIGSEGV, handler);

  /* init suil */
  g_message ("Initing suil...");
  suil_init(&argc, &argv, SUIL_ARG_NONE);

  /* init fftw */
  g_message ("Making fftw planner thread safe...");
  fftw_make_planner_thread_safe ();
  fftwf_make_planner_thread_safe ();

  /* init audio decoder */
  g_message ("Initing audio decoder...");
  ad_init ();

  /* init glibtop */
#ifdef HAVE_LIBGTOP
  g_message ("Initing libgtop...");
  glibtop_init ();
#endif

  /* init random */
  g_message ("Initing random...");
#ifdef _WIN32
  srand (time (NULL));
#else
  srandom (time (NULL));
#endif

  /* init object utils */
  g_message ("Initing object utils...");
  object_utils_init ();

  /* send activate signal */
  g_message ("Initing Zrythm app...");
  zrythm_app = zrythm_app_new ();
  g_message ("running Zrythm...");
  return g_application_run (
    G_APPLICATION (zrythm_app),
    argc,
    argv);
}
