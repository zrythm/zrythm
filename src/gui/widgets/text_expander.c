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

#include "gui/widgets/main_window.h"
#include "gui/widgets/text_expander.h"
#include "project.h"
#include "utils/gtk.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#define TEXT_EXPANDER_WIDGET_TYPE \
  (text_expander_widget_get_type ())
G_DEFINE_TYPE (
  TextExpanderWidget,
  text_expander_widget,
  EXPANDER_BOX_WIDGET_TYPE)

static gboolean
on_focus (
  GtkWidget * widget,
  TextExpanderWidget * self)
{
  g_message ("text focused");
  self->has_focus = true;

  return FALSE;
}

static gboolean
on_focus_out (
  GtkWidget *widget,
  GdkEvent  *event,
  TextExpanderWidget * self)
{
  g_message ("text focus out");
  self->has_focus = false;

  if (self->setter && self->obj)
    {
      GtkTextIter start_iter, end_iter;
      gtk_text_buffer_get_start_iter (
        GTK_TEXT_BUFFER (self->buffer), &start_iter);
      gtk_text_buffer_get_end_iter (
        GTK_TEXT_BUFFER (self->buffer), &end_iter);
      char * content =
        gtk_text_buffer_get_text (
          GTK_TEXT_BUFFER (self->buffer),
          &start_iter, &end_iter, false);
      self->setter (self->obj, content);
    }

  return FALSE;
}

/**
 * Refreshes the text.
 */
void
text_expander_widget_refresh (
  TextExpanderWidget * self)
{
  if (self->getter && self->obj)
    {
      gtk_text_buffer_set_text (
        GTK_TEXT_BUFFER (self->buffer),
        self->getter (self->obj), -1);
    }
}

/**
 * Sets up the TextExpanderWidget.
 */
void
text_expander_widget_setup (
  TextExpanderWidget * self,
  GenericStringGetter  getter,
  GenericStringSetter  setter,
  void *               obj)
{
  self->getter = getter;
  self->setter = setter;
  self->obj = obj;

  text_expander_widget_refresh (self);
}

static void
text_expander_widget_class_init (
  TextExpanderWidgetClass * klass)
{
}

static void
text_expander_widget_init (
  TextExpanderWidget * self)
{
  self->scroll =
    GTK_SCROLLED_WINDOW (
      gtk_scrolled_window_new (
        NULL, NULL));
  gtk_widget_set_vexpand (
    GTK_WIDGET (self->scroll), 1);
  gtk_widget_set_visible (
    GTK_WIDGET (self->scroll), 1);
  gtk_scrolled_window_set_shadow_type (
    self->scroll, GTK_SHADOW_ETCHED_IN);
  gtk_widget_set_size_request (
    GTK_WIDGET (self->scroll), -1, 124);

  self->viewport =
    GTK_VIEWPORT (
      gtk_viewport_new (NULL, NULL));
  gtk_widget_set_visible (
    GTK_WIDGET (self->viewport), 1);
  gtk_container_add (
    GTK_CONTAINER (self->scroll),
    GTK_WIDGET (self->viewport));

  GtkSourceLanguageManager * manager =
    gtk_source_language_manager_get_default ();
  GtkSourceLanguage * lang =
    gtk_source_language_manager_get_language (
      manager, "ini");
  self->buffer =
    gtk_source_buffer_new_with_language (lang);
  self->editor =
    GTK_SOURCE_VIEW (
      gtk_source_view_new_with_buffer (
      self->buffer));
  gtk_container_add (
    GTK_CONTAINER (self->viewport),
    GTK_WIDGET (self->editor));
  gtk_widget_set_visible (
    GTK_WIDGET (self->editor), true);
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

  expander_box_widget_add_content (
    Z_EXPANDER_BOX_WIDGET (self),
    GTK_WIDGET (self->scroll));

  expander_box_widget_set_icon_name (
    Z_EXPANDER_BOX_WIDGET (self),
    "gnome-builder-xml-comment-symbolic-light");

  g_signal_connect (
    G_OBJECT (self->editor), "focus-out-event",
    G_CALLBACK (on_focus_out), self);
  g_signal_connect (
    G_OBJECT (self->editor), "grab-focus",
    G_CALLBACK (on_focus), self);
}

