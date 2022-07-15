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

#include "audio/region.h"
#include "audio/track.h"
#include "gui/widgets/dialogs/track_icon_chooser_dialog.h"
#include "gui/widgets/main_window.h"
#include "project.h"
#include "utils/color.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

enum
{
  COL_LABEL,
  COL_PIXBUF,
};

/**
 * Runs the widget and processes the result, then
 * closes the dialog.
 *
 * @return Whether the color was set or not.
 */
bool
track_icon_chooser_dialog_widget_run (
  TrackIconChooserDialogWidget * self)
{
  int res =
    z_gtk_dialog_run (GTK_DIALOG (self->dialog), false);
  bool icon_set = false;
  switch (res)
    {
    case GTK_RESPONSE_ACCEPT:
      icon_set = true;
      break;
    default:
      break;
    }

  if (icon_set && self->selected_icon)
    {
      /* if changed, apply the change */
      if (!string_is_equal (
            self->selected_icon, self->track->icon_name))
        {
          track_set_icon (
            self->track, self->selected_icon, F_UNDOABLE,
            F_PUBLISH_EVENTS);
        }
    }
  gtk_window_destroy (GTK_WINDOW (self->dialog));

  g_free_and_null (self->selected_icon);

  return icon_set;
}

static void
on_item_activated (
  GtkIconView *                  icon_view,
  GtkTreePath *                  path,
  TrackIconChooserDialogWidget * self)
{
  if (self->selected_icon)
    {
      g_free (self->selected_icon);
      self->selected_icon = NULL;
    }

  GtkTreeIter iter;
  gtk_tree_model_get_iter (self->icon_model, &iter, path);
  gtk_tree_model_get (
    GTK_TREE_MODEL (self->icon_model), &iter, COL_LABEL,
    &self->selected_icon, -1);
}

static GtkListStore *
create_list_store (void)
{
  GtkListStore * store =
    gtk_list_store_new (2, G_TYPE_STRING, GDK_TYPE_PIXBUF);

  /* TODO */
#if 0
  GtkIconTheme * icon_theme =
    gtk_icon_theme_get_default ();

  GtkTreeIter iter;
  GList * list =
    gtk_icon_theme_list_icons (
      icon_theme, "TrackTypes");
  for (GList * l = list; l != NULL; l = l->next)
    {
      char * icon_name = (char *) l->data;
      GdkPixbuf * pixbuf =
        gtk_icon_theme_load_icon (
          icon_theme, icon_name, 16, 0, NULL);
      if (pixbuf)
        {
          gtk_list_store_append (
            store, &iter);
          gtk_list_store_set (
            store, &iter,
            COL_LABEL, icon_name,
            COL_PIXBUF, pixbuf,
            -1);
        }
      else
        {
          g_warning (
            "no pixbuf loaded for %s", icon_name);
        }
      g_free (l->data);
    }
  g_list_free (list);
#endif

  return store;
}

/**
 * Creates a new dialog.
 */
TrackIconChooserDialogWidget *
track_icon_chooser_dialog_widget_new (Track * track)
{
  g_return_val_if_fail (IS_TRACK (track), NULL);

  char * str = g_strdup_printf (_ ("%s icon"), track->name);
  TrackIconChooserDialogWidget * self =
    object_new (TrackIconChooserDialogWidget);
  self->dialog = GTK_DIALOG (gtk_dialog_new_with_buttons (
    str, GTK_WINDOW (MAIN_WINDOW),
    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
    _ ("_Cancel"), GTK_RESPONSE_REJECT, _ ("_Select"),
    GTK_RESPONSE_ACCEPT, NULL));
  g_free (str);
  gtk_widget_add_css_class (
    GTK_WIDGET (self->dialog), "track-icon-chooser-dialog");

  /* add icon view */
  self->icon_model = GTK_TREE_MODEL (create_list_store ());
  self->icon_view = GTK_ICON_VIEW (
    gtk_icon_view_new_with_model (self->icon_model));
  gtk_widget_set_visible (GTK_WIDGET (self->icon_view), true);
  gtk_icon_view_set_text_column (self->icon_view, COL_LABEL);
  gtk_icon_view_set_pixbuf_column (
    self->icon_view, COL_PIXBUF);
  gtk_icon_view_set_activate_on_single_click (
    self->icon_view, true);
  g_signal_connect (
    self->icon_view, "item-activated",
    G_CALLBACK (on_item_activated), self);
  GtkBox * content_area = GTK_BOX (
    gtk_dialog_get_content_area (GTK_DIALOG (self->dialog)));
  GtkScrolledWindow * scroll =
    GTK_SCROLLED_WINDOW (gtk_scrolled_window_new ());
  gtk_scrolled_window_set_min_content_width (scroll, 480);
  gtk_scrolled_window_set_min_content_height (scroll, 360);
  gtk_widget_set_visible (GTK_WIDGET (scroll), true);
  gtk_scrolled_window_set_child (
    GTK_SCROLLED_WINDOW (scroll),
    GTK_WIDGET (self->icon_view));
  gtk_box_append (content_area, GTK_WIDGET (scroll));

  self->track = track;

  return self;
}
