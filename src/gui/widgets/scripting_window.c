/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include "config.h"

#ifdef HAVE_GUILE

#include "gui/widgets/scripting_window.h"
#include "guile/guile.h"
#include "project.h"
#include "utils/io.h"
#include "utils/resources.h"
#include "utils/ui.h"

#include <gtk/gtk.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <libguile.h>
#pragma GCC diagnostic pop

G_DEFINE_TYPE (
  ScriptingWindowWidget,
  scripting_window_widget,
  GTK_TYPE_WINDOW)

/**
 * Function that runs in guile mode.
 */
static void*
guile_mode_func (void* data)
{
  ScriptingWindowWidget * self =
    (ScriptingWindowWidget *) data;

  char * tmp_dir =
    g_dir_make_tmp (
      "zrythm_script_XXXXXX", NULL);
  GtkTextIter start_iter, end_iter;
  gtk_text_buffer_get_start_iter (
    self->textbuf, &start_iter);
  gtk_text_buffer_get_end_iter (
    self->textbuf, &end_iter);
  char * script_content =
    gtk_text_buffer_get_text (
      self->textbuf, &start_iter, &end_iter, false);
  char * full_path =
    g_build_filename (tmp_dir, "script.scm", NULL);
  GError *err = NULL;
  g_file_set_contents (
    full_path, script_content, -1, &err);
  if (err != NULL)
    {
      g_warning (
        "Unable to write project file: %s",
        err->message);
      g_error_free (err);
      return NULL;
    }

  guile_define_modules ();

  /* receive output */
  SCM out_port = scm_open_output_string ();
  scm_set_current_output_port (out_port);
  scm_set_current_error_port (out_port);

  /* load a file called script.scm with the following
   * content:
   *
   * (define zrythm-script
   *   (lambda ()
   *     (display "script called") (newline)))
   */
  scm_c_primitive_load (full_path);
  SCM func =
    scm_variable_ref (
      scm_c_lookup ("zrythm-script"));
  scm_call_0 (func);

  SCM str_scm = scm_get_output_string (out_port);
  char * str =
    scm_to_locale_string (str_scm);
  gtk_label_set_text (self->output, str);

  return NULL;
}

static void
on_execute_clicked (
  GtkButton *      spin,
  ScriptingWindowWidget * self)
{
  g_message ("clicked");

  scm_with_guile (&guile_mode_func, self);
}

/**
 * Creates a bounce dialog.
 */
ScriptingWindowWidget *
scripting_window_widget_new ()
{
  ScriptingWindowWidget * self =
    g_object_new (SCRIPTING_WINDOW_WIDGET_TYPE, NULL);

  g_signal_connect (
    G_OBJECT (self->execute_btn), "clicked",
    G_CALLBACK (on_execute_clicked), self);

  return self;
}

static void
scripting_window_widget_class_init (
  ScriptingWindowWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "scripting_window.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, ScriptingWindowWidget, x)

  BIND_CHILD (editor);
  BIND_CHILD (execute_btn);
  BIND_CHILD (output);
  BIND_CHILD (textbuf);
}

static void
scripting_window_widget_init (
  ScriptingWindowWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
#endif
