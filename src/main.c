/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
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
#ifdef _WIN32
#include <windows.h>
#include <dbghelp.h>
#else
#include <execinfo.h>
#endif
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "plugins/vst/vst_windows.h"
#include "plugins/vst/vst_x11.h"
#include "gui/widgets/main_window.h"
#include "utils/gtk.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "utils/ui.h"
#include "zrythm.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#ifdef HAVE_LIBGTOP
#include <glibtop.h>
#endif

#include <audec/audec.h>
#include <suil/suil.h>
#include <fftw3.h>
#ifdef HAVE_X11
#include <X11/Xlib.h>
#endif

#define BACKTRACE_SIZE 100

/** SIGSEGV handler. */
static void
handler (int sig)
{
  char message[12000];
  char current_line[2000];

#ifdef _WIN32
  unsigned int   i;
  void         * stack[ 100 ];
  unsigned short frames;
  SYMBOL_INFO  * symbol;
  HANDLE         process;

  process = GetCurrentProcess();

  SymInitialize (process, NULL, TRUE);

  frames =
    CaptureStackBackTrace (0, 100, stack, NULL);
  symbol =
    (SYMBOL_INFO *)
    calloc (
      sizeof (SYMBOL_INFO) + 256 * sizeof( char ),
      1);
  symbol->MaxNameLen = 255;
  symbol->SizeOfStruct = sizeof (SYMBOL_INFO);

  sprintf (
    message,
    _("Error - Backtrace:\n"), sig);
  for (i = 0; i < frames; i++)
    {
      SymFromAddr (
        process, (DWORD64) (stack[i]), 0,
        symbol);

      sprintf (
        current_line,
        "%u: %s - 0x%0X\n", frames - i - 1,
        symbol->Name, symbol->Address);
      strcat (message, current_line);
    }
  free (symbol);
#else
  void *array[BACKTRACE_SIZE];
  char ** strings;

  /* get void*'s for all entries on the stack */
  int size = backtrace (array, BACKTRACE_SIZE);

  /* print out all the frames to stderr */
  const char * signal_string = strsignal (sig);
  sprintf (
    message,
    _("Error: %s - Backtrace:\n"), signal_string);
  strings = backtrace_symbols (array, size);
  for (int i = 0; i < size; i++)
    {
      sprintf (current_line, "%s\n", strings[i]);
      strcat (message, current_line);
    }
  free (strings);
#endif

  g_message ("%s", message);

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
  char * ver = zrythm_get_version (0);
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
      "> Context, distro, etc.\n\n"
      "# Backtrace\n%s\n",
      ver, message);
  g_free (ver);
  char * report_template_escaped =
    g_uri_escape_string (
      report_template, NULL, FALSE);
  char * atag =
    g_strdup_printf (
      "<a href=\"%s?issue[description]=%s\">",
      NEW_ISSUE_URL,
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
    "%s", message);
  GtkLabel * label =
    z_gtk_message_dialog_get_label (
      GTK_MESSAGE_DIALOG (dialog), 1);
  gtk_label_set_selectable (
    label, 1);

  /* wrap the backtrace in a scrolled window */
  GtkBox * box =
    GTK_BOX (
      gtk_message_dialog_get_message_area (
        GTK_MESSAGE_DIALOG (dialog)));
  GtkWidget * secondary_area =
    z_gtk_container_get_nth_child (
      GTK_CONTAINER (box), 1);
  gtk_container_remove (
    GTK_CONTAINER (box), secondary_area);
  GtkWidget * scrolled_window =
    gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_min_content_height (
    GTK_SCROLLED_WINDOW (scrolled_window), 360);
  gtk_container_add (
    GTK_CONTAINER (scrolled_window),
    secondary_area);
  gtk_container_add (
    GTK_CONTAINER (box), scrolled_window);
  gtk_widget_show_all (GTK_WIDGET (box));

  /* run the dialog */
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);

  g_free (report_template);
  g_free (atag);
  g_free (markup);
  g_free (report_template_escaped);

  exit(1);
}

static void
print_version ()
{
  char * ver = zrythm_get_version (0);
  fprintf (
    stdout,
    "Zrythm %s\nbuilt with %s %s\n"
    "%s\n%s\n%s\n%s\n",
    ver,
    COMPILER, COMPILER_VERSION,
    "Copyright (C) 2018-2020 Alexandros Theodotou",
    "License AGPLv3+: GNU AGPL version 3 or later <https://www.gnu.org/licenses/agpl.html>",
    "This is free software: you are free to change and redistribute it.",
    "There is NO WARRANTY, to the extent permitted by law.");
  g_free (ver);
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
    "Write comments and bugs to %s\n"
    "Support this project at https://liberapay.com/Zrythm\n"
    "Website https://www.zrythm.org\n"),
    ISSUE_TRACKER_URL);
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

  char * ver = zrythm_get_version (0);
  fprintf (
    stdout,
    _("Zrythm-%s Copyright (C) 2018-2020 Alexandros Theodotou\n\n"
    "Zrythm comes with ABSOLUTELY NO WARRANTY!\n\n"
    "This is free software, and you are welcome to redistribute it\n"
    "under certain conditions. See the file `COPYING' for details.\n\n"
    "Write comments and bugs to %s\n"
    "Support this project at https://liberapay.com/Zrythm\n\n"),
    ver, ISSUE_TRACKER_URL);
  g_free (ver);

  char * cur_dir = g_get_current_dir ();
  g_message (
    "Running Zrythm in %s", cur_dir);
  g_free (cur_dir);

  g_message ("GTK_THEME=%s", getenv ("GTK_THEME"));

  /* install segfault handler */
  g_message ("Installing signal handler...");
  signal(SIGSEGV, handler);

#ifdef HAVE_X11
  /* init xlib threads */
  g_message ("Initing X threads...");
  XInitThreads ();

  /* init VST UI engine */
  g_message ("Initing VST UI engine...");
  g_return_val_if_fail (vst_x11_init (0) == 0, -1);;
#endif

#ifdef _WIN32
  /* init VST UI engine */
  g_message ("Initing VST UI engine...");
  g_return_val_if_fail (
    vst_windows_init () == 0, -1);;
#endif

  /* init suil */
  g_message ("Initing suil...");
  suil_init(&argc, &argv, SUIL_ARG_NONE);

  /* init fftw */
  g_message ("Making fftw planner thread safe...");
  fftw_make_planner_thread_safe ();
  fftwf_make_planner_thread_safe ();

  /* init audio decoder */
  g_message ("Initing audio decoder...");
  audec_init ();

  /* init glibtop */
#ifdef HAVE_LIBGTOP
  g_message ("Initing libgtop...");
  glibtop_init ();
#endif

  /* init random */
  g_message ("Initing random...");
#ifdef _WIN32
  srand ((unsigned int) time (NULL));
#else
  srandom ((unsigned int) time (NULL));
#endif

  /* init object utils */
  g_message ("Initing object utils...");
  object_utils_init ();

  /* init math coefficients */
  g_message ("Initing math coefficients...");
  math_init ();

  /* send activate signal */
  g_message ("Initing Zrythm app...");
  zrythm_app = zrythm_app_new ();
  g_message ("running Zrythm...");

  return
    g_application_run (
      G_APPLICATION (zrythm_app), argc, argv);
}
