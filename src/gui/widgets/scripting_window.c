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

#include "zrythm-config.h"

#ifdef HAVE_GUILE

#include "gui/widgets/scripting_window.h"
#include "guile/guile.h"
#include "project.h"
#include "utils/io.h"
#include "utils/resources.h"
#include "utils/ui.h"

#include <gtk/gtk.h>

#include <glib/gi18n.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <libguile.h>
#pragma GCC diagnostic pop

G_DEFINE_TYPE (
  ScriptingWindowWidget,
  scripting_window_widget,
  GTK_TYPE_WINDOW)

static SCM out_port;
static SCM error_out_port;

static const char * example_script =
";;; This is an example GNU Guile script using modules provided by Zrythm\n\
;;; See https://www.gnu.org/software/guile/ for more info about GNU Guile\n\
;;; See https://manual.zrythm.org/en/scripting/intro.html for a list of\n\
;;; modules provided by Zrythm\n"
#if 0
"(use-modules (zrythm)\n\
             (audio position))\n\
(define zrythm-script\n\
  (lambda ()\n\
    (display (zrythm-get-ver))\n\
    (let ((mypos (position-new 2 1 1 34 0)))\n\
      (position-print mypos))))";
#endif
"(use-modules (audio track)\n\
             (audio tracklist))\n\
(define zrythm-script\n\
  (lambda ()\n\
    ;; loop through all tracks and print them\n\
    (let ((num-tracks (tracklist-get-num-tracks)))\n\
      (let loop ((i 0))\n\
        (when (< i num-tracks)\n\
          (let ((track (tracklist-get-track-at-pos i)))\n\
            (display (track-get-name track))\n\
            (newline))\n\
          (loop (+ i 1)))))))";

static SCM
call_proc (void * data)
{
  char * full_path = (char *) data;

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

  return SCM_BOOL_T;
}

static SCM
eval_handler (
  void * handler_data, SCM key, SCM args)
{
  SCM stack = *(SCM *) handler_data;

  /* Put the code which you want to handle an error
   * after the stack has been unwound here. */

  scm_print_exception (
    error_out_port, SCM_BOOL_F,
    key, args);
  scm_display_backtrace (
    stack, error_out_port, SCM_BOOL_F, SCM_BOOL_F);

  return SCM_BOOL_F;
}

static SCM
preunwind_proc (
  void * handler_data,
  SCM    key,
  SCM    parameters)
{
  /* Capture the stack here: */
  *(SCM *) handler_data =
    scm_make_stack (SCM_BOOL_T, SCM_EOL);

  return SCM_BOOL_T;
}

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
    GTK_TEXT_BUFFER (self->buffer), &start_iter);
  gtk_text_buffer_get_end_iter (
    GTK_TEXT_BUFFER (self->buffer), &end_iter);
  char * script_content =
    gtk_text_buffer_get_text (
      GTK_TEXT_BUFFER (self->buffer),
      &start_iter, &end_iter, false);
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
  out_port = scm_open_output_string ();
  error_out_port = scm_open_output_string ();
  scm_set_current_output_port (out_port);
  scm_set_current_error_port (error_out_port);

  SCM captured_stack = SCM_BOOL_F;

  SCM ret =
    scm_c_catch (
      SCM_BOOL_T, call_proc, full_path, eval_handler,
      &captured_stack, preunwind_proc,
      &captured_stack);

  SCM str_scm = scm_get_output_string (out_port);
  char * str =
    scm_to_locale_string (str_scm);
  str_scm =
    scm_get_output_string (error_out_port);
  char * err_str =
    scm_to_locale_string (str_scm);
  char * markup;
  if (ret == SCM_BOOL_T)
    {
      markup =
        g_markup_printf_escaped (
          "<span foreground=\"green\" size=\"large\">"
          "<b>%s</b></span>\n%s",
          _("Script execution successful"),
          str);
    }
  else
    {
      markup =
        g_markup_printf_escaped (
          "<span foreground=\"red\" size=\"large\">"
          "<b>%s</b></span>\n%s",
          _("Script execution failed"),
          err_str);
    }
  gtk_label_set_markup (
    self->output, markup);
  g_free (markup);

  return NULL;
}

static void
on_execute_clicked (
  GtkButton *      spin,
  ScriptingWindowWidget * self)
{
  scm_with_guile (&guile_mode_func, self);
}

/**
 * Creates a bounce dialog.
 */
ScriptingWindowWidget *
scripting_window_widget_new ()
{
  ScriptingWindowWidget * self =
    g_object_new (
      SCRIPTING_WINDOW_WIDGET_TYPE,
      "title", _("Scripting Interface"),
      "icon-name", "zrythm",
      NULL);

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

  BIND_CHILD (execute_btn);
  BIND_CHILD (output);
  BIND_CHILD (source_viewport);
}

static void
scripting_window_widget_init (
  ScriptingWindowWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  GtkSourceLanguageManager * manager =
    gtk_source_language_manager_get_default ();
  GtkSourceLanguage * lang =
    gtk_source_language_manager_get_language (
      manager, "scheme");
  self->buffer =
    gtk_source_buffer_new_with_language (lang);
  self->editor =
    GTK_SOURCE_VIEW (
      gtk_source_view_new_with_buffer (
      self->buffer));
  gtk_container_add (
    GTK_CONTAINER (self->source_viewport),
    GTK_WIDGET (self->editor));
  gtk_widget_set_visible (
    GTK_WIDGET (self->editor), true);
  gtk_text_buffer_set_text (
    GTK_TEXT_BUFFER (self->buffer),
    example_script, -1);
  gtk_source_view_set_show_line_numbers (
    self->editor, true);
  gtk_source_view_set_auto_indent (
    self->editor, true);
  gtk_source_view_set_indent_on_tab (
    self->editor, true);
  gtk_source_view_set_tab_width (
    self->editor, 2);
  gtk_source_view_set_indent_width (
    self->editor, 2);
  gtk_source_view_set_insert_spaces_instead_of_tabs (
    self->editor, true);
  gtk_source_view_set_smart_backspace (
    self->editor, true);

  /* set style */
  GtkSourceStyleSchemeManager * style_mgr =
    gtk_source_style_scheme_manager_get_default ();
  gtk_source_style_scheme_manager_prepend_search_path (
    style_mgr, CONFIGURE_SOURCEVIEW_STYLES_DIR);
  gtk_source_style_scheme_manager_force_rescan (
    style_mgr);
  GtkSourceStyleScheme * scheme =
    gtk_source_style_scheme_manager_get_scheme (
      style_mgr, "monokai-extended-zrythm");
  gtk_source_buffer_set_style_scheme (
    self->buffer, scheme);

  gtk_label_set_selectable (self->output, true);
}
#endif
