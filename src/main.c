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
#ifdef _WOE32
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

#include "gui/widgets/main_window.h"
#include "guile/guile.h"
#include "utils/gtk.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "utils/ui.h"
#include "project.h"
#include "zrythm.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#ifdef HAVE_LIBGTOP
#include <glibtop.h>
#endif

#ifdef HAVE_GTK_SOURCE_VIEW_4
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <gtksourceview/gtksource.h>
#pragma GCC diagnostic pop
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
segv_handler (int sig)
{
  char message[12000];
  char current_line[2000];

#ifdef _WOE32
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

  /* call the callback to write queued messages
   * and get last few lines of the log, before
   * logging the backtrace */
  log_idle_cb (LOG);
  char * log =
    log_get_last_n_lines (LOG, 60);
  g_message ("%s", message);
  log_idle_cb (LOG);

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
  char ver_with_caps[2000];
  zrythm_get_version_with_capabilities (
    ver_with_caps);
  char * report_template =
    g_strdup_printf (
      "# Steps to reproduce\n"
      "> Write a list of steps to reproduce the "
      "bug\n\n"
      "# What happens?\n"
      "> Please tell us what happened\n\n"
      "# What is expected?\n"
      "> What is expected to happen?\n\n"
      "# Version\n%s\n"
      "# Other info\n"
      "> Context, distro, etc.\n\n"
      "# Backtrace\n```\n%s```\n\n"
      "# Log\n```\n%s```",
      ver_with_caps, message, log);
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
  char ver_with_caps[2000];
  zrythm_get_version_with_capabilities (
    ver_with_caps);
  fprintf (
    stdout,
    "%s\n%s\n%s\n%s\n",
    ver_with_caps,
    "Copyright © 2018-2020 The Zrythm contributors",
    "This is free software; see the source for copying conditions.",
    "There is NO "
    "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.");
}

static void
print_help ()
{
  fprintf (
    stdout,
    _("Usage: zrythm [ OPTIONS ] [ PROJECT-NAME ]\n\n"
    "Options:\n"
    "  -h, --help      display this help message and exit\n"
    "  --convert-yaml-to-zpj  convert a yaml project to the .zpj format\n"
    "  --convert-zpj-to-yaml  convert a zpj project to the YAML format\n"
    "  -o, --output    specify an output file\n"
    "  -p, --print-settings  print current settings\n"
    "  --pretty        print output in user-friendly way\n"
    "  --reset-to-factory  reset to factory settings\n"
    "  -v, --version   output version information and exit\n\n"
    "Examples:\n"
    "  zrythm -v       print version\n"
    "  zrythm --convert-zpj-to-yaml myproject.zpj --output myproject.yaml  convert a compressed zpj project to YAML\n"
    "  zrythm -p --pretty  pretty-print current settings\n\n"
    "Report bugs to %s\n"),
    ISSUE_TRACKER_URL);
}

/**
 * Checks that the file exists and exits if it
 * doesn't.
 */
void
verify_file_exists (
  char * file)
{
  if (!file ||
      !g_file_test (file, G_FILE_TEST_EXISTS))
    {
      char str[600];
      sprintf (
        str, _("File %s not found."), file);
      strcat (str, "\n");
      fprintf (stderr, "%s", str);
      exit (-1);
    }
}

/**
 * Checks that the output is not NULL and exits if it
 * is.
 */
void
verify_output_exists (
  char * output)
{
  if (!output)
    {
      char str[600];
      sprintf (
        str,
        "%s\n",
        _("An output file was not specified. Please "
        "pass one with `--output [FILE]`."));
      fprintf (stderr, "%s", str);
      exit (-1);
    }
}

/**
 * main
 */
int
main (int    argc,
      char **argv)
{
#define OPT_VERSION 'v'
#define OPT_HELP 'h'
#define OPT_PRINT_SETTINGS 'p'
#define OPT_OUTPUT 'o'
#define OPT_RESET_TO_FACTORY 6492
#define OPT_PRETTY_PRINT 5914
#define OPT_CONVERT_ZPJ_TO_YAML 4198
#define OPT_CONVERT_YAML_TO_ZPJ 35173

  int c, option_index;
  static struct option long_options[] =
    {
      {"version", no_argument, 0, OPT_VERSION},
      {"help", no_argument, 0, OPT_HELP},
      {"convert-zpj-to-yaml", required_argument, 0,
        OPT_CONVERT_ZPJ_TO_YAML},
      {"convert-yaml-to-zpj", required_argument, 0,
        OPT_CONVERT_YAML_TO_ZPJ},
      {"output", required_argument, 0,
        OPT_OUTPUT},
      {"print-settings", no_argument, 0,
        OPT_PRINT_SETTINGS},
      {"reset-to-factory", no_argument,
        0, OPT_RESET_TO_FACTORY},
      {"pretty", no_argument,
        0, OPT_PRETTY_PRINT},
      {0, 0, 0, 0}
    };
  opterr = 0;

  bool pretty_print = false;
  bool print_settings = false;
  bool convert_yaml_to_zpj = false;
  bool convert_zpj_to_yaml = false;
  char * from_file = NULL;
  char * output = NULL;
  while (true)
    {
      c =
        getopt_long (
          argc, argv, "vhpo",
          long_options, &option_index);

      /* Detect the end of the options. */
      if (c == -1)
        break;

      switch (c)
        {
        case OPT_VERSION:
          print_version ();
          return 0;
        case OPT_HELP:
          print_help ();
          return 0;
          break;
        case OPT_CONVERT_ZPJ_TO_YAML:
          convert_zpj_to_yaml = true;
          from_file = optarg;
          break;
        case OPT_CONVERT_YAML_TO_ZPJ:
          convert_yaml_to_zpj = true;
          from_file = optarg;
          break;
        case OPT_OUTPUT:
          output = optarg;
          break;
        case OPT_PRINT_SETTINGS:
          print_settings = true;
          break;
        case OPT_RESET_TO_FACTORY:
          settings_reset_to_factory (1, 1);
          return 0;
          break;
        case OPT_PRETTY_PRINT:
          pretty_print = true;
          break;
        case '?':
          /* getopt_long already printed an error
           * message */
          fprintf (stderr, _("Unknown option\n"));
          return 1;
        default:
          abort ();
        }
    }

  if (print_settings)
    {
      localization_init (false, false);
      settings_print (pretty_print);
      return 0;
    }

  if (convert_yaml_to_zpj)
    {
      verify_output_exists (output);
      verify_file_exists (from_file);
      char * err_msg =
        project_compress (
          &output, NULL,
          PROJECT_COMPRESS_FILE,
          from_file, -1,
          PROJECT_COMPRESS_FILE);
      if (err_msg)
        {
          fprintf (
            stderr,
            _("Project failed to compress: %s\n"),
            err_msg);
          g_free (err_msg);
          return -1;
        }
      else
        {
          fprintf (
            stdout,
            _("Project successfully compressed.\n"));
          return 0;
        }
    }
  else if (convert_zpj_to_yaml)
    {
      verify_output_exists (output);
      verify_file_exists (from_file);
      char * err_msg =
        project_decompress (
          &output, NULL,
          PROJECT_DECOMPRESS_FILE,
          from_file, -1,
          PROJECT_DECOMPRESS_FILE);
      if (err_msg)
        {
          fprintf (
            stderr,
            _("Project failed to decompress: %s\n"),
            err_msg);
          g_free (err_msg);
          return -1;
        }
      else
        {
          fprintf (
            stdout,
            _("Project successfully "
            "decompressed.\n"));
          return 0;
        }
    }

  char * ver = zrythm_get_version (0);
  fprintf (
    stdout,
    _("Zrythm-%s Copyright (C) 2018-2020 The Zrythm contributors\n\n"
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

#ifdef HAVE_GUILE
  guile_init (argc, argv);
#endif

  g_message ("GTK_THEME=%s", getenv ("GTK_THEME"));

  /* install segfault handler */
  g_message ("Installing signal handler...");
  signal(SIGSEGV, segv_handler);

#ifdef HAVE_X11
  /* init xlib threads */
  g_message ("Initing X threads...");
  XInitThreads ();
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
#ifdef _WOE32
  srand ((unsigned int) time (NULL));
#else
  srandom ((unsigned int) time (NULL));
#endif

  /* init gtksourceview */
#ifdef HAVE_GTK_SOURCE_VIEW_4
  gtk_source_init ();
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
