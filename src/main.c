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
#include "plugins/lv2/suil.h"
#include "utils/objects.h"
#include "zrythm.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#ifdef HAVE_LIBGTOP
#include <glibtop.h>
#endif

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

  // get void*'s for all entries on the stack
  size = backtrace(array, 20);

  // print out all the frames to stderr
  g_message ("Error: signal %d - Backtrace:", sig);
  strings = backtrace_symbols (array, size);

  for (int i = 0; i < size; i++)
    g_message ("%s", strings[i]);

  free (strings);
#endif
  exit(1);
}

static void
print_version ()
{
  fprintf (
    stdout,
    "zrythm%s\n",
    PACKAGE_VERSION);
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
    _("Zrythm-%s Copyright (C) 2018-2019 Alexandros Theodotou et al.\n\n"
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
