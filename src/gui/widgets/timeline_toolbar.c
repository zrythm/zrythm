/*
 * Copyright (C) 2019-2022 Alexandros Theodotou <alex at zrythm dot org>
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

#include "actions/undo_manager.h"
#include "actions/undoable_action.h"
#include "gui/accel.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/playhead_scroll_buttons.h"
#include "gui/widgets/quantize_box.h"
#include "gui/widgets/range_action_buttons.h"
#include "gui/widgets/snap_box.h"
#include "gui/widgets/snap_grid.h"
#include "gui/widgets/timeline_toolbar.h"
#include "gui/widgets/zoom_buttons.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/resources.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  TimelineToolbarWidget,
  timeline_toolbar_widget,
  GTK_TYPE_BOX)

void
timeline_toolbar_widget_refresh (
  TimelineToolbarWidget * self)
{
  /* enable/disable merge button */
  bool sensitive =
    arranger_selections_can_be_merged (
      (ArrangerSelections *) TL_SELECTIONS);
  g_debug (
    "settings merge button sensitivity %d",
    sensitive);
  gtk_widget_set_sensitive (
    GTK_WIDGET (self->merge_btn), sensitive);
}

void
timeline_toolbar_widget_setup (
  TimelineToolbarWidget * self)
{
  snap_box_widget_setup (
    self->snap_box, SNAP_GRID_TIMELINE);
  quantize_box_widget_setup (
    self->quantize_box, QUANTIZE_OPTIONS_TIMELINE);
}

static void
timeline_toolbar_widget_init (
  TimelineToolbarWidget * self)
{
  g_type_ensure (QUANTIZE_BOX_WIDGET_TYPE);
  g_type_ensure (RANGE_ACTION_BUTTONS_WIDGET_TYPE);
  g_type_ensure (SNAP_BOX_WIDGET_TYPE);
  g_type_ensure (
    PLAYHEAD_SCROLL_BUTTONS_WIDGET_TYPE);
  g_type_ensure (ZOOM_BUTTONS_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

#define SET_TOOLTIP(x) \
  z_gtk_set_tooltip_for_actionable ( \
    GTK_ACTIONABLE (self->x), \
    gtk_widget_get_tooltip_text ( \
      GTK_WIDGET (self->x)))

  SET_TOOLTIP (event_viewer_toggle);
  SET_TOOLTIP (musical_mode_toggle);
  SET_TOOLTIP (merge_btn);

#undef SET_TOOLTIP

  zoom_buttons_widget_setup (
    self->zoom_buttons, true);
}

static void
timeline_toolbar_widget_class_init (
  TimelineToolbarWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "timeline_toolbar.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, TimelineToolbarWidget, x)

  BIND_CHILD (snap_box);
  BIND_CHILD (quantize_box);
  BIND_CHILD (event_viewer_toggle);
  BIND_CHILD (musical_mode_toggle);
  BIND_CHILD (range_action_buttons);
  BIND_CHILD (zoom_buttons);
  BIND_CHILD (merge_btn);

#undef BIND_CHILD
}
