/*
 * Copyright (C) 2020-2021 Alexandros Theodotou <alex at zrythm dot org>
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

G_DEFINE_TYPE (
  ScriptingWindowWidget,
  scripting_window_widget,
  GTK_TYPE_WINDOW)

static void
on_execute_clicked (
  GtkButton *      spin,
  ScriptingWindowWidget * self)
{
  GtkTextIter start_iter, end_iter;
  gtk_text_buffer_get_start_iter (
    GTK_TEXT_BUFFER (self->buffer), &start_iter);
  gtk_text_buffer_get_end_iter (
    GTK_TEXT_BUFFER (self->buffer), &end_iter);
  char * script_content =
    gtk_text_buffer_get_text (
      GTK_TEXT_BUFFER (self->buffer),
      &start_iter, &end_iter, false);

  char * markup =
    (char *) guile_run_script (script_content);
  g_free (script_content);

  gtk_label_set_markup (
    self->output, markup);
  g_free (markup);
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

  char * example_script =
    g_strdup_printf (
      "%s%s%s%s",
      _(";;; This is an example GNU Guile script using modules provided by Zrythm\n\
;;; See https://www.gnu.org/software/guile/ for more info about GNU Guile\n\
;;; See https://manual.zrythm.org/en/scripting/intro.html for a list of\n\
;;; modules provided by Zrythm\n"),
"(use-modules (audio track)\n\
             (audio tracklist)\n\
             (project)\n\
             (zrythm))\n\
(define zrythm-script\n\
  (lambda ()\n",
_("    ;; loop through all tracks and print them\n"),
"    (let* ((prj (zrythm-get-project))\n\
           (tracklist (project-get-tracklist prj))\n\
           (num-tracks (tracklist-get-num-tracks tracklist)))\n\
      (let loop ((i 0))\n\
        (when (< i num-tracks)\n\
          (let ((track (tracklist-get-track-at-pos tracklist i)))\n\
            (display (track-get-name track))\n\
            (newline))\n\
          (loop (+ i 1)))))))");

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

  g_free (example_script);

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
