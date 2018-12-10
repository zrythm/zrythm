/*
 * gui/widgets/center_dock.c - Main window widget
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

#include "audio/transport.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/left_dock_edge.h"
#include "gui/widgets/right_dock_edge.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/snap_grid.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/tracklist.h"
#include "utils/resources.h"

G_DEFINE_TYPE (CenterDockWidget,
               center_dock_widget,
               DZL_TYPE_DOCK_BIN)

static gboolean
key_release_cb (GtkWidget      * widget,
                 GdkEventKey * event,
                 gpointer       data)
{
  if (event && event->keyval == GDK_KEY_space)
    {
      if (TRANSPORT->play_state == PLAYSTATE_ROLLING)
        transport_request_pause ();
      else if (TRANSPORT->play_state == PLAYSTATE_PAUSED)
        transport_request_roll ();
    }

  return FALSE;
}

/*static void*/
/*center_dock_widget_add (GtkContainer * container,*/
                        /*GtkWidget *    widget)*/
/*{*/
  /*CenterDockWidget * self = (CenterDockWidget *) container;*/

/*}*/

/*static GtkWidget **/
/*center_dock_widget_create_edge (DzlDockBin *     dock_bin,*/
                                /*GtkPositionType  edge)*/
/*{*/
  /*g_assert (DZL_IS_DOCK_BIN (dock_bin));*/
  /*g_assert (edge >= GTK_POS_LEFT);*/
  /*g_assert (edge <= GTK_POS_BOTTOM);*/

  /*if (edge == GTK_POS_LEFT)*/
    /*return g_object_new (LEFT_DOCK_EDGE_WIDGET_TYPE,*/
                         /*"edge", edge,*/
                         /*"reveal-child", FALSE,*/
                         /*"visible", TRUE,*/
                         /*NULL);*/

  /*if (edge == GTK_POS_RIGHT)*/
    /*return g_object_new (RIGHT_DOCK_EDGE_WIDGET_TYPE,*/
                         /*"edge", edge,*/
                         /*"reveal-child", FALSE,*/
                         /*"visible", FALSE,*/
                         /*NULL);*/

  /*if (edge == GTK_POS_BOTTOM)*/
    /*return g_object_new (BOT_DOCK_EDGE_WIDGET_TYPE,*/
                         /*"edge", edge,*/
                         /*"reveal-child", FALSE,*/
                         /*"visible", TRUE,*/
                         /*NULL);*/

  /*return DZL_DOCK_BIN_CLASS (center_dock_widget_parent_class)->create_edge (dock_bin, edge);*/
/*}*/

static void
center_dock_widget_init (CenterDockWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  GValue a = G_VALUE_INIT;
  g_value_init (&a, G_TYPE_BOOLEAN);
  g_value_set_boolean (&a,
                       1);
  g_object_set_property (G_OBJECT (self),
                         "left-visible",
                         &a);
  g_object_set_property (G_OBJECT (self),
                         "right-visible",
                         &a);
  g_object_set_property (G_OBJECT (self),
                         "bottom-visible",
                         &a);

  /* setup tracklist */
  self->tracklist = tracklist_widget_new (PROJECT->tracklist);
  gtk_container_add (GTK_CONTAINER (self->tracklist_viewport),
                     GTK_WIDGET (self->tracklist));
  gtk_widget_set_size_request (GTK_WIDGET (self->tracklist_header),
                               -1,
                               60);
  tracklist_widget_show (self->tracklist);

  /* setup ruler */
  self->ruler = ruler_widget_new ();
  gtk_container_add (GTK_CONTAINER (self->ruler_viewport),
                     GTK_WIDGET (self->ruler));
  gtk_scrolled_window_set_hadjustment (self->ruler_scroll,
                                       gtk_scrolled_window_get_hadjustment (
                                           GTK_SCROLLED_WINDOW (
                                               self->timeline_scroll)));
  gtk_widget_show_all (GTK_WIDGET (self->ruler_viewport));

  /* setup timeline */
  self->timeline = timeline_arranger_widget_new (
    &PROJECT->snap_grid_timeline);
  gtk_container_add (GTK_CONTAINER (self->timeline_viewport),
                     GTK_WIDGET (self->timeline));
  gtk_scrolled_window_set_min_content_width (self->timeline_scroll, 400);
  gtk_scrolled_window_set_vadjustment (self->timeline_scroll,
            gtk_scrolled_window_get_vadjustment (self->tracklist_scroll));
  gtk_widget_show_all (GTK_WIDGET (self->timeline_viewport));
  gtk_widget_show_all (GTK_WIDGET (self->timeline));
  /*gtk_widget_show_all (GTK_WIDGET (self->timeline->bg));*/

  /* setup bot toolbar */
  self->snap_grid_midi = snap_grid_widget_new (&PROJECT->snap_grid_midi);
  gtk_box_pack_start (GTK_BOX (self->snap_grid_midi_box),
                      GTK_WIDGET (self->snap_grid_midi),
                      1, 1, 0);

  // set icons
  gtk_tool_button_set_icon_widget (
    self->instrument_add,
    resources_get_icon ("plus.svg"));

  /* set events */
  g_signal_connect (G_OBJECT (self), "key_release_event",
                    G_CALLBACK (key_release_cb), self);

  /* add edges */
  /*gtk_container_add (GTK_CONTAINER (self),*/
                     /*center_dock_widget_create_edge (*/
                       /*DZL_DOCK_BIN (self),*/
                       /*GTK_POS_LEFT));*/
}


static void
center_dock_widget_class_init (CenterDockWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);

  /*DzlDockBinClass *dock_bin_class = DZL_DOCK_BIN_CLASS (klass);*/
  /*dock_bin_class->create_edge = center_dock_widget_create_edge;*/

  /*GtkContainerClass * container_class = GTK_CONTAINER_CLASS (klass);*/
  /*container_class->add = center_dock_widget_add;*/

  resources_set_class_template (
    GTK_WIDGET_CLASS (klass),
    "center_dock.ui");

  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    CenterDockWidget,
    editor_top);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    CenterDockWidget,
    tracklist_timeline);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    CenterDockWidget,
    tracklist_top);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    CenterDockWidget,
    tracklist_scroll);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    CenterDockWidget,
    tracklist_viewport);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    CenterDockWidget,
    tracklist_header);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    CenterDockWidget,
    timeline_ruler);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    CenterDockWidget,
    ruler_scroll);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    CenterDockWidget,
    ruler_viewport);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    CenterDockWidget,
    timeline_scroll);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    CenterDockWidget,
    timeline_viewport);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    CenterDockWidget,
    instruments_toolbar);
  gtk_widget_class_bind_template_child (
    klass,
    CenterDockWidget,
    snap_grid_midi_box);
  gtk_widget_class_bind_template_child (
    klass,
    CenterDockWidget,
    instrument_add);
  gtk_widget_class_bind_template_child (
    klass,
    CenterDockWidget,
    left_dock_edge);
}

