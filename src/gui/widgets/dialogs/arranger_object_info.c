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

#include "gui/backend/arranger_object.h"
#include "gui/widgets/dialogs/arranger_object_info.h"
#include "project.h"
#include "utils/io.h"
#include "utils/resources.h"
#include "utils/ui.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

G_DEFINE_TYPE (
  ArrangerObjectInfoDialogWidget,
  arranger_object_info_dialog_widget,
  GTK_TYPE_DIALOG)

static void
set_values (
  ArrangerObjectInfoDialogWidget * self,
  ArrangerObject *                 obj)
{
  self->obj = obj;

  char         tmp[600];
  const char * name =
    arranger_object_get_name (obj);
  if (name)
    {
      gtk_label_set_text (self->name_lbl, name);
    }
  else
    {
      gtk_label_set_text (self->name_lbl, "");
    }
  ZRegion * region =
    arranger_object_get_region (obj);
  if (region)
    {
      sprintf (
        tmp, "%s [tr %u, ln %d, at %d, idx %d]",
        region->name, region->id.track_name_hash,
        region->id.lane_pos, region->id.at_idx,
        region->id.idx);
      gtk_label_set_text (self->owner_lbl, tmp);
    }
  else
    {
      gtk_label_set_text (self->owner_lbl, "");
    }
}

/**
 * Creates a new arranger_object_info dialog.
 */
ArrangerObjectInfoDialogWidget *
arranger_object_info_dialog_widget_new (
  ArrangerObject * object)
{
  ArrangerObjectInfoDialogWidget * self =
    g_object_new (
      ARRANGER_OBJECT_INFO_DIALOG_WIDGET_TYPE, NULL);

  set_values (self, object);

  return self;
}

static void
arranger_object_info_dialog_widget_class_init (
  ArrangerObjectInfoDialogWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "arranger_object_info_dialog.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, ArrangerObjectInfoDialogWidget, x)

  BIND_CHILD (name_lbl);
  BIND_CHILD (type_lbl);
  BIND_CHILD (owner_lbl);
}

static void
arranger_object_info_dialog_widget_init (
  ArrangerObjectInfoDialogWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_window_set_title (
    GTK_WINDOW (self), _ ("Arranger object info"));
  gtk_window_set_icon_name (
    GTK_WINDOW (self), "zrythm");
}
