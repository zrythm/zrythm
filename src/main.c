/*
 * Copyright (C) 2018-2021 Alexandros Theodotou <alex at zrythm dot org>
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

#ifdef HAVE_GUILE
#include "guile/project_generator.h"
#endif
#include "gui/widgets/main_window.h"
#include "gui/widgets/dialogs/bug_report_dialog.h"
#include "utils/backtrace.h"
#include "utils/flags.h"
#include "utils/gdb.h"
#include "guile/guile.h"
#include "utils/gtk.h"
#include "utils/log.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "utils/ui.h"
#include "utils/valgrind.h"
#include "project.h"
#include "settings/settings.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include "ext/whereami/whereami.h"

#ifdef HAVE_GTK_SOURCE_VIEW_4
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <gtksourceview/gtksource.h>
#pragma GCC diagnostic pop
#endif

#include <audec/audec.h>
#include <fftw3.h>
#ifdef HAVE_LSP_DSP
#include <lsp-plug.in/dsp/dsp.h>
#endif
#include <suil/suil.h>
#ifdef HAVE_X11
#include <X11/Xlib.h>
#endif

static bool pretty_print = false;
static bool dummy = false;
static bool compress = false;
static bool print_settings_flag = false;
static char * arg0 = NULL;
static char * audio_backend = NULL;
static char * midi_backend = NULL;
static char * buf_size = NULL;
static char * samplerate = NULL;
static char * output_file = NULL;
static char * project_file = NULL;
static char * convert_file = NULL;

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
  char * bt =
    backtrace_get_with_lines (prefix, 100);

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

  exit (EXIT_FAILURE);
}

static bool
print_version (
  const gchar * option_name,
  const gchar * value,
  gpointer      data,
  GError **     error)
{
  char ver_with_caps[2000];
  zrythm_get_version_with_capabilities (
    ver_with_caps);
  fprintf (
    stdout,
    "%s\n%s\n%s\n%s\n",
    ver_with_caps,
    "Copyright © 2018-2021 The Zrythm contributors",
    "This is free software; see the source for copying conditions.",
    "There is NO "
    "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.");

  exit (EXIT_SUCCESS);
}

/**
 * Checks that the file exists and exits if it
 * doesn't.
 */
static void
verify_file_exists (
  const char * file)
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
static void
verify_output_exists (void)
{
  if (!output_file)
    {
      char str[600];
      sprintf (
        str,
        "%s\n",
        _("An output file was not specified. Please "
        "pass one with `--output=FILE`."));
      fprintf (stderr, "%s", str);
      exit (EXIT_FAILURE);
    }
}

static bool
reset_to_factory (
  const gchar * option_name,
  const gchar * value,
  gpointer      data,
  GError **     error)
{
  settings_reset_to_factory (1, 1);

  exit (EXIT_SUCCESS);
}

static bool
print_settings (void)
{
  localization_init (false, false);
  settings_print (pretty_print);

  exit (EXIT_SUCCESS);
}

static void
convert_project (void)
{
  char * output;
  size_t output_size;
  char * err_msg = NULL;
  if (compress)
    {
      verify_output_exists ();

      err_msg =
        project_compress (
          &output_file, NULL,
          PROJECT_COMPRESS_FILE,
          convert_file, -1,
          PROJECT_COMPRESS_FILE);
    }
  else
    {
      err_msg =
        project_decompress (
          &output, &output_size,
          PROJECT_DECOMPRESS_DATA,
          convert_file, -1,
          PROJECT_DECOMPRESS_FILE);
    }

  if (err_msg)
    {
      fprintf (
        stderr,
        _("Project failed to decompress: %s\n"),
        err_msg);
      g_free (err_msg);
      exit (EXIT_FAILURE);
    }
  else
    {
      if (!compress)
        {
          output =
            realloc (
              output,
              output_size + sizeof (char));
          output[output_size] = '\0';
          fprintf (stdout, "%s\n", output);
        }
      exit (EXIT_SUCCESS);
    }

  fprintf (stdout, "%s\n", _("Unknown operation"));
  exit (EXIT_FAILURE);
}

static bool
parse_convert (
  const gchar * option_name,
  const gchar * value,
  gpointer      data,
  GError **     error)
{
  if (string_is_equal (
        option_name, "--yaml-to-zpj"))
    {
      compress = true;
    }

  verify_file_exists (value);
  convert_file = g_strdup (value);

  return true;
}

static bool
gen_project (
  const gchar * option_name,
  const gchar * value,
  gpointer      data,
  GError **     error)
{
  verify_output_exists ();
  verify_file_exists (value);
#ifdef HAVE_GUILE
  zrythm_app =
    g_object_new (
      ZRYTHM_APP_TYPE,
      "resource-base-path", "/org/zrythm/Zrythm",
      "flags", G_APPLICATION_HANDLES_OPEN,
      NULL);
  ZRYTHM =
    zrythm_new (
      arg0, false, false, F_NOT_OPTIMIZED);
  ZRYTHM->generating_project = true;
  int script_res =
    guile_project_generator_generate_project_from_file (
      value, output_file);
  exit (script_res);
#else
  fprintf (
    stderr,
    _("libguile is required for this option\n"));
  exit (EXIT_FAILURE);
#endif
}

/**
 * main
 */
int
main (int    argc,
      char **argv)
{
  arg0 = argv[0];

  LOG = log_new ();

  GOptionEntry entries[] =
    {
      { "version", 'v',  G_OPTION_FLAG_NO_ARG,
        G_OPTION_ARG_CALLBACK, print_version,
        __("Print version information"),
        NULL },
      { "zpj-to-yaml", 0,
        G_OPTION_FLAG_FILENAME,
        G_OPTION_ARG_CALLBACK, parse_convert,
        __("Convert ZPJ-FILE to YAML"),
        "ZPJ-FILE" },
      { "yaml-to-zpj", 0,
        G_OPTION_FLAG_FILENAME,
        G_OPTION_ARG_CALLBACK, parse_convert,
        __("Convert YAML-PROJECT-FILE to the .zpj format"),
        "YAML-PROJECT-FILE" },
      { "gen-project", 0,
        G_OPTION_FLAG_FILENAME,
        G_OPTION_ARG_CALLBACK, gen_project,
        __("Generate a project from SCRIPT-FILE"),
        "SCRIPT-FILE" },
      { "pretty", 0, G_OPTION_FLAG_NONE,
        G_OPTION_ARG_NONE, &pretty_print,
        __("Print output in user-friendly way"),
        NULL },
      { "print-settings", 'p', G_OPTION_FLAG_NONE,
        G_OPTION_ARG_NONE, &print_settings_flag,
        __("Print current settings"), NULL },
      { "reset-to-factory", 0,
        G_OPTION_FLAG_NO_ARG,
        G_OPTION_ARG_CALLBACK, reset_to_factory,
        __("Reset to factory settings"), NULL },
      { "audio-backend", 0, G_OPTION_FLAG_NONE,
        G_OPTION_ARG_STRING, &audio_backend,
        __("Override the audio backend to use"),
        "BACKEND" },
      { "midi-backend", 0, G_OPTION_FLAG_NONE,
        G_OPTION_ARG_STRING, &midi_backend,
        __("Override the MIDI backend to use"),
        "BACKEND" },
      { "dummy", 0, G_OPTION_FLAG_NONE,
        G_OPTION_ARG_NONE, &dummy,
        __("Shorthand for --midi-backend=none "
        "--audio-backend=none"),
        NULL },
      { "buf-size", 0, G_OPTION_FLAG_NONE,
        G_OPTION_ARG_STRING, &buf_size,
        "Override the buffer size to use for the "
        "audio backend, if applicable",
        "BUF_SIZE" },
      { "samplerate", 0, G_OPTION_FLAG_NONE,
        G_OPTION_ARG_STRING, &samplerate,
        "Override the samplerate to use for the "
        "audio backend, if applicable",
        "SAMPLERATE" },
      { "output", 'o', G_OPTION_FLAG_NONE,
        G_OPTION_ARG_STRING, &output_file,
        "File or directory to output to", "FILE" },
      { NULL },
    };

  /* parse options */
  GError *error = NULL;
  char context_str[500];
  sprintf (
    context_str, __("[PROJECT-FILE] - Run %s"), PROGRAM_NAME);
  GOptionContext * context =
    g_option_context_new (context_str);
  g_option_context_add_main_entries (
    context, entries, GETTEXT_PACKAGE);
  g_option_context_add_group (
    context, gtk_get_option_group (false));
  char examples[8000];
  sprintf (
    examples,
    __("Examples:\n"
    "  --zpj-to-yaml a.zpj > b.yaml        Convert a a.zpj to YAML and save to b.yaml\n"
    "  --gen-project a.scm -o myproject    Generate myproject from a.scm\n"
    "  -p --pretty                         Pretty-print current settings\n\n"
    "Please report issues to %s\n"),
    ISSUE_TRACKER_URL);
  g_option_context_set_description (
    context, examples);
  char * null_terminated_args[argc + 1];
  for (int i = 0; i < argc; i++)
    {
      null_terminated_args[i] = argv[i];
    }
  null_terminated_args[argc] = NULL;
  char ** allocated_args =
    g_strdupv (null_terminated_args);
  if (!g_option_context_parse_strv (
         context, &allocated_args, &error))
    {
      g_error (
        "option parsing failed: %s",
        error->message);
    }

  if (dummy)
    {
      audio_backend = "none";
      midi_backend = "none";
    }

  if (convert_file)
    {
      convert_project ();
    }
  else if (print_settings_flag)
    {
      print_settings ();
    }

  char * ver = zrythm_get_version (0);
  fprintf (
    stdout,
    _("%s-%s Copyright (C) 2018-2021 The Zrythm contributors\n\n"
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
  signal (SIGABRT, segv_handler);

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

#ifdef HAVE_LSP_DSP
  /* init lsp dsp */
  g_message ("Initing LSP DSP...");
  lsp_dsp_init();

  /* output information about the system */
  lsp_dsp_info_t * info = lsp_dsp_info();
  if (info)
    {
      printf("Architecture:   %s\n", info->arch);
      printf("Processor:      %s\n", info->cpu);
      printf("Model:          %s\n", info->model);
      printf("Features:       %s\n", info->features);
      free(info);
    }
  else
    {
      g_warning ("Failed to get system info");
    }

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
  char * exe_path = NULL;
  int length, dirname_length;
  length =
    wai_getExecutablePath (NULL, 0, &dirname_length);
  if (length > 0)
  {
    exe_path = (char *) malloc (length + 1);
    wai_getExecutablePath (
      exe_path, length, &dirname_length);
    exe_path[length] = '\0';
  }
  g_message ("executable path found: %s", exe_path);
  zrythm_app =
    zrythm_app_new (
      exe_path ? exe_path : argv[0],
      audio_backend, midi_backend, buf_size,
      samplerate);

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

  log_free (LOG);

  return ret;
}
