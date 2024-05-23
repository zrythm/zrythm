// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/snap_grid.h"
#include "gui/widgets/popovers/snap_grid_popover.h"
#include "gui/widgets/snap_grid.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/resources.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

G_DEFINE_TYPE (SnapGridWidget, snap_grid_widget, GTK_TYPE_WIDGET)

static void
set_label (SnapGridWidget * self)
{
  SnapGrid * sg = self->snap_grid;
  char *     snap_str = snap_grid_stringize (sg);

  char new_str[600];
  if (sg->snap_to_grid)
    {
      if (sg->length_type == NoteLengthType::NOTE_LENGTH_LINK)
        {
          sprintf (new_str, "%s - 🔗", snap_str);
        }
      else if (sg->length_type == NoteLengthType::NOTE_LENGTH_LAST_OBJECT)
        {
          sprintf (new_str, _ ("%s - Last object"), snap_str);
        }
      else
        {
          char * default_str = snap_grid_stringize (sg);
          sprintf (new_str, "%s - %s", snap_str, default_str);
          g_free (default_str);
        }
    }
  else
    {
      sprintf (new_str, "%s", _ ("Off"));
    }
  adw_button_content_set_label (self->content, new_str);

  g_free (snap_str);
}

void
snap_grid_widget_refresh (SnapGridWidget * self)
{
  set_label (self);
}

static void
create_popover (GtkMenuButton * menu_btn, gpointer user_data)
{
  SnapGridWidget * self = Z_SNAP_GRID_WIDGET (user_data);
  self->popover = snap_grid_popover_widget_new (self);
  gtk_menu_button_set_popover (menu_btn, GTK_WIDGET (self->popover));
}

void
snap_grid_widget_setup (SnapGridWidget * self, SnapGrid * snap_grid)
{
  self->snap_grid = snap_grid;
  gtk_menu_button_set_create_popup_func (
    GTK_MENU_BUTTON (self->menu_btn), create_popover, self, NULL);

  set_label (self);
}

static void
snap_grid_widget_class_init (SnapGridWidgetClass * klass)
{
  GtkWidgetClass * wklass = GTK_WIDGET_CLASS (klass);
  gtk_widget_class_set_layout_manager_type (wklass, GTK_TYPE_BIN_LAYOUT);
}

static void
snap_grid_widget_init (SnapGridWidget * self)
{
  self->menu_btn = GTK_MENU_BUTTON (gtk_menu_button_new ());
  gtk_widget_set_parent (GTK_WIDGET (self->menu_btn), GTK_WIDGET (self));

  self->content = ADW_BUTTON_CONTENT (adw_button_content_new ());
  adw_button_content_set_icon_name (self->content, "snap-to-grid");
  gtk_menu_button_set_child (
    GTK_MENU_BUTTON (self->menu_btn), GTK_WIDGET (self->content));
  gtk_widget_set_tooltip_text (GTK_WIDGET (self), _ ("Snap/Grid Settings"));
}
