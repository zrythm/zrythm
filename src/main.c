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

#include "zrythm-config.h"

#include <ctype.h>
#ifdef _WOE32
#include <windows.h>
#endif
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "gui/widgets/main_window.h"
#include "gui/widgets/dialogs/bug_report_dialog.h"
#include "utils/backtrace.h"
#include "utils/gdb.h"
#include "guile/guile.h"
#include "utils/gtk.h"
#include "utils/log.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "utils/ui.h"
#include "utils/valgrind.h"
#include "project.h"
#include "settings/settings.h"
#include "zrythm.h"
#include "zrythm_app.h"

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

/** SIGSEGV handler. */
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

  /* call the callback to write queued messages
   * and get last few lines of the log, before
   * logging the backtrace */
  log_idle_cb (LOG);
  g_message ("%s", bt);
  log_idle_cb (LOG);

  char str[500];
  sprintf (
    str, _("%s has crashed. "), PROGRAM_NAME);
  GtkWidget * dialog =
    bug_report_dialog_new (
      GTK_WINDOW (MAIN_WINDOW), str, bt);

  /* run the dialog */
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);

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
    _("Usage: %s [ OPTIONS ] [ PROJECT-FILE-PATH ]\n\n"
    "Options:\n"
    "  -h, --help      display this help message and exit\n"
    "  --convert-yaml-to-zpj  convert a yaml project to the .zpj format\n"
    "  --convert-zpj-to-yaml  convert a zpj project to the YAML format\n"
    "  -o, --output    specify an output file\n"
    "  -p, --print-settings  print current settings\n"
    "  --pretty        print output in user-friendly way\n"
    "  --reset-to-factory  reset to factory settings\n"
    "  --gdb           run %s through GDB (debugger)\n"
    "  --callgrind     run %s through callgrind (profiler)\n"
    "  --audio-backend  override the audio backend to use\n"
    "  --midi-backend  override the MIDI backend to use\n"
    "  --dummy         overrides both the audio and MIDI backends to dummy\n"
    "  --buf-size      overrides the buffer size to use for the audio backend, if applicable\n"
    "  --interactive   interactive mode, where applicable\n"
    "  -v, --version   output version information and exit\n\n"
    "Examples:\n"
    "  %s -v       print version\n"
    "  %s --callgrind --dummy --buf-size=8192  profile %s using the dummy backend and a buffer size of 8192\n"
    "  %s --convert-zpj-to-yaml myproject.zpj --output myproject.yaml  convert a compressed zpj project to YAML\n"
    "  %s -p --pretty  pretty-print current settings\n\n"
    "Report bugs to %s\n"),
    PROGRAM_NAME_LOWERCASE, PROGRAM_NAME,
    PROGRAM_NAME, PROGRAM_NAME_LOWERCASE,
    PROGRAM_NAME_LOWERCASE, PROGRAM_NAME,
    PROGRAM_NAME_LOWERCASE, PROGRAM_NAME_LOWERCASE,
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
#define OPT_GDB 4124
#define OPT_CALLGRIND 6843
#define OPT_AUDIO_BACKEND 4811
#define OPT_MIDI_BACKEND 9197
#define OPT_DUMMY 2311
#define OPT_BUF_SIZE 39145
#define OPT_INTERACTIVE 34966

  int c, option_index;
  static struct option long_options[] =
    {
      { "version", no_argument, 0, OPT_VERSION},
      { "help", no_argument, 0, OPT_HELP },
      { "convert-zpj-to-yaml", required_argument, 0,
        OPT_CONVERT_ZPJ_TO_YAML },
      { "convert-yaml-to-zpj", required_argument, 0,
        OPT_CONVERT_YAML_TO_ZPJ },
      { "output", required_argument, 0,
        OPT_OUTPUT },
      { "print-settings", no_argument, 0,
        OPT_PRINT_SETTINGS },
      { "reset-to-factory", no_argument,
        0, OPT_RESET_TO_FACTORY },
      { "pretty", no_argument, 0,
        OPT_PRETTY_PRINT },
      { "gdb", no_argument, 0, OPT_GDB },
      { "callgrind", no_argument, 0,
        OPT_CALLGRIND },
      { "audio-backend", required_argument, 0,
        OPT_AUDIO_BACKEND },
      { "midi-backend", required_argument, 0,
        OPT_MIDI_BACKEND },
      { "dummy", no_argument, 0, OPT_DUMMY },
      { "buf-size", required_argument, 0,
        OPT_BUF_SIZE },
      { "interactive", no_argument, 0,
        OPT_INTERACTIVE },
      { 0, 0, 0, 0 }
    };
  opterr = 0;

  bool pretty_print = false;
  bool print_settings = false;
  bool convert_yaml_to_zpj = false;
  bool convert_zpj_to_yaml = false;
  bool run_gdb = false;
  bool run_callgrind = false;
  bool interactive = false;
  char * from_file = NULL;
  char * output = NULL;
  char * audio_backend = NULL;
  char * midi_backend = NULL;
  char * buf_size = NULL;
  char * project_file = NULL;
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
          return EXIT_SUCCESS;
        case OPT_HELP:
          print_help ();
          return EXIT_SUCCESS;
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
          return EXIT_SUCCESS;
          break;
        case OPT_PRETTY_PRINT:
          pretty_print = true;
          break;
        case OPT_GDB:
          run_gdb = true;
          break;
        case OPT_CALLGRIND:
          run_callgrind = true;
          break;
        case OPT_INTERACTIVE:
          interactive = true;
          break;
        case OPT_AUDIO_BACKEND:
          audio_backend = optarg;
          break;
        case OPT_MIDI_BACKEND:
          midi_backend = optarg;
          break;
        case OPT_DUMMY:
          audio_backend = "none";
          midi_backend = "none";
          break;
        case OPT_BUF_SIZE:
          buf_size = optarg;
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

  /* get last non-option argument as project file */
  for (int index = optind; index < argc; index++)
    {
      project_file = argv[index];
    }

  if (print_settings)
    {
      localization_init (false, false);
      settings_print (pretty_print);
      return EXIT_SUCCESS;
    }
  else if (run_gdb)
    {
#ifdef __linux__
      ZRYTHM = zrythm_new (TRUE, FALSE);
      gdb_exec (argv, true, interactive);
#else
      g_error (
        "This option is not available on your "
        "platform");
#endif
    }
  else if (run_callgrind)
    {
#ifdef __linux__
      ZRYTHM = zrythm_new (TRUE, FALSE);
      valgrind_exec_callgrind (argv);
#else
      g_error (
        "This option is not available on your "
        "platform");
#endif
    }
  else if (convert_yaml_to_zpj)
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
          return EXIT_SUCCESS;
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
          return EXIT_SUCCESS;
        }
    }

  char * ver = zrythm_get_version (0);
  fprintf (
    stdout,
    _("%s-%s Copyright (C) 2018-2020 The Zrythm contributors\n\n"
    "%s comes with ABSOLUTELY NO WARRANTY!\n\n"
    "This is free software, and you are welcome to redistribute it\n"
    "under certain conditions. See the file `COPYING' for details.\n\n"
    "Write comments and bugs to %s\n"
    "Support this project at https://liberapay.com/Zrythm\n\n"),
    PROGRAM_NAME, ver, PROGRAM_NAME,
    ISSUE_TRACKER_URL);
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
  signal (SIGSEGV, segv_handler);

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

  /* init math coefficients */
  g_message ("Initing math coefficients...");
  math_init ();

  /* send activate signal */
  g_message ("Initing Zrythm app...");
  zrythm_app =
    zrythm_app_new (
      audio_backend, midi_backend, buf_size);

  g_message ("running Zrythm...");
  int ret = 0;
  if (project_file)
    {
      int g_argc = 2;
      char * g_argv[3] = {
        argv[0], project_file,
      };

      ret =
        g_application_run (
          G_APPLICATION (zrythm_app),
          g_argc, g_argv);
    }
  else
    {
      ret =
        g_application_run (
          G_APPLICATION (zrythm_app), 0, NULL);
    }
  g_object_unref (zrythm_app);

  return ret;
}
