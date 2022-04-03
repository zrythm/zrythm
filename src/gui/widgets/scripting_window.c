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

#  include "gui/widgets/scripting_window.h"
#  include "project.h"
#  include "settings/settings.h"
#  include "utils/gtk.h"
#  include "utils/io.h"
#  include "utils/resources.h"
#  include "utils/ui.h"

#  include <glib/gi18n.h>
#  include <gtk/gtk.h>

#  include "guile/guile.h"

G_DEFINE_TYPE (
  ScriptingWindowWidget,
  scripting_window_widget,
  GTK_TYPE_WINDOW)

static void
on_execute_clicked (
  GtkButton *             spin,
  ScriptingWindowWidget * self)
{
  GtkTextIter start_iter, end_iter;
  gtk_text_buffer_get_start_iter (
    GTK_TEXT_BUFFER (self->buffer), &start_iter);
  gtk_text_buffer_get_end_iter (
    GTK_TEXT_BUFFER (self->buffer), &end_iter);
  char * script_content = gtk_text_buffer_get_text (
    GTK_TEXT_BUFFER (self->buffer), &start_iter,
    &end_iter, false);

  char * markup =
    (char *) guile_run_script (script_content);
  g_free (script_content);

  gtk_label_set_markup (self->output, markup);
  g_free (markup);
}

/**
 * Creates a bounce dialog.
 */
ScriptingWindowWidget *
scripting_window_widget_new ()
{
  ScriptingWindowWidget * self = g_object_new (
    SCRIPTING_WINDOW_WIDGET_TYPE, "title",
    _ ("Scripting Interface"), "icon-name",
    "zrythm", NULL);

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

#  define BIND_CHILD(x) \
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

  char * filename = g_settings_get_string (
    S_P_SCRIPTING_GENERAL, "default-script");
  char * dir =
    zrythm_get_dir (ZRYTHM_DIR_USER_SCRIPTS);
  char * full_path =
    g_build_filename (dir, filename, NULL);
  if (!g_file_test (full_path, G_FILE_TEST_EXISTS))
    {
      g_free (dir);
      g_free (full_path);
      dir = zrythm_get_dir (
        ZRYTHM_DIR_SYSTEM_SCRIPTSDIR);
      full_path =
        g_build_filename (dir, filename, NULL);

      if (!g_file_test (
            full_path, G_FILE_TEST_EXISTS))
        {
          g_free (full_path);
          full_path = g_build_filename (
            dir, "print-all-tracks", NULL);

          if (!g_file_test (
                full_path, G_FILE_TEST_EXISTS))
            {
              g_critical (
                "File not found: %s", full_path);
              return;
            }
        }
    }

  char *   script_contents = NULL;
  GError * err = NULL;
  bool     ret = g_file_get_contents (
        full_path, &script_contents, NULL, &err);
  if (!ret)
    {
      g_critical (
        "error reading <%s> contents: %s",
        full_path, err->message);
      return;
    }

  g_free (full_path);
  g_free (dir);
  g_free (filename);

  GtkSourceLanguageManager * manager =
    z_gtk_source_language_manager_get ();
  GtkSourceLanguage * lang =
    gtk_source_language_manager_get_language (
      manager, "scheme");
  g_return_if_fail (lang);
  self->buffer =
    gtk_source_buffer_new_with_language (lang);
  self->editor = GTK_SOURCE_VIEW (
    gtk_source_view_new_with_buffer (self->buffer));
  gtk_viewport_set_child (
    GTK_VIEWPORT (self->source_viewport),
    GTK_WIDGET (self->editor));
  gtk_text_buffer_set_text (
    GTK_TEXT_BUFFER (self->buffer), script_contents,
    -1);
  gtk_source_view_set_show_line_numbers (
    self->editor, true);
  gtk_source_view_set_auto_indent (
    self->editor, true);
  gtk_source_view_set_indent_on_tab (
    self->editor, true);
  gtk_source_view_set_tab_width (self->editor, 2);
  gtk_source_view_set_indent_width (self->editor, 2);
  gtk_source_view_set_insert_spaces_instead_of_tabs (
    self->editor, true);
  gtk_source_view_set_smart_backspace (
    self->editor, true);

  g_free (script_contents);

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
