/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "gui/backend/timeline_selections.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/chord_object.h"
#include "gui/widgets/digital_meter.h"
#include "gui/widgets/region.h"
#include "gui/widgets/selection_info.h"
#include "gui/widgets/timeline_selection_info.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (TimelineSelectionInfoWidget,
               timeline_selection_info_widget,
               GTK_TYPE_STACK)

void
timeline_selection_info_widget_refresh (
  TimelineSelectionInfoWidget * self,
  TimelineSelections * ts)
{
  GtkWidget * fo =
    timeline_selections_get_first_object (
      ts, 0);

  selection_info_widget_clear (
    self->selection_info);
  gtk_stack_set_visible_child (
    GTK_STACK (self),
    GTK_WIDGET (self->no_selection_label));

  if (Z_IS_REGION_WIDGET (fo))
    {
      REGION_WIDGET_GET_PRIVATE (fo);
      Region * r = rw_prv->region;

      DigitalMeterWidget * dm =
        digital_meter_widget_new_for_position (
          r,
          region_get_start_pos,
          region_set_start_pos,
          24, _("start"));
      digital_meter_set_draw_line (dm, 1);

      selection_info_widget_add_info_with_text (
        self->selection_info,
        _("Start Position"),
        dm);
      gtk_stack_set_visible_child (
        GTK_STACK (self),
        GTK_WIDGET (self->selection_info));
    }
  else if (Z_IS_AUTOMATION_POINT_WIDGET (fo))
    {

    }
  else if (Z_IS_CHORD_OBJECT_WIDGET (fo))
    {

    }
}

static void
timeline_selection_info_widget_class_init (
  TimelineSelectionInfoWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (
    klass, "timeline-selection-info");
}

static void
timeline_selection_info_widget_init (
  TimelineSelectionInfoWidget * self)
{
  self->no_selection_label =
    GTK_LABEL (
      gtk_label_new (_("No object selected")));
  gtk_widget_set_visible (
    GTK_WIDGET (self->no_selection_label), 1);
  self->selection_info =
    g_object_new (SELECTION_INFO_WIDGET_TYPE, NULL);
  gtk_widget_set_visible (
    GTK_WIDGET (self->selection_info), 1);

  gtk_stack_add_named (
    GTK_STACK (self),
    GTK_WIDGET (self->no_selection_label),
    "no-selection");
  gtk_stack_add_named (
    GTK_STACK (self),
    GTK_WIDGET (self->selection_info),
    "selection-info");

  gtk_widget_set_size_request (
    GTK_WIDGET (self),
    -1,
    51);
}
