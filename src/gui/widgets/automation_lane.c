/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

/** \file
 */

#include "audio/automatable.h"
#include "audio/automation_lane.h"
#include "audio/automation_track.h"
#include "audio/automation_tracklist.h"
#include "audio/bus_track.h"
#include "audio/channel.h"
#include "audio/instrument_track.h"
#include "audio/track.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/automation_lane.h"
#include "gui/widgets/automation_tracklist.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/instrument_track.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/region.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/track.h"
#include "utils/arrays.h"
#include "utils/resources.h"

G_DEFINE_TYPE (AutomationLaneWidget,
               automation_lane_widget,
               GTK_TYPE_GRID)

#define GET_TRACK(self) Track * track = self->al->at->track

/**
 * Enums for identifying combobox entries.
 */
enum
{
  CHANNEL_FADER_INDEX,
  CHANNEL_PAN_INDEX,
  CHANNEL_MUTE_INDEX,
  PLUGIN_START_INDEX
};
enum
{
  PLUGIN_ENABLED_INDEX,
  PLUGIN_CONTROL_START_INDEX
};
enum
{
  COLUMN_LABEL,
  COLUMN_AUTOMATABLE,
  NUM_COLUMNS
};

static void
size_allocate_cb (GtkWidget * widget, GtkAllocation * allocation, void * data)
{
  gtk_widget_queue_draw (GTK_WIDGET (MW_TIMELINE));
  gtk_widget_queue_allocate (GTK_WIDGET (MW_TIMELINE));
}

/**
 * Adds automation lane for automation track.
 */
/*static void*/
/*add_automation_lane (*/
  /*AutomationTracklistWidget * atlw,*/
  /*AutomationTrack *           at)*/
/*{*/
  /*automation_tracklist_widget_add_automation_track (*/
    /*atlw,*/
    /*at,*/
    /*automation_tracklist_widget_get_automation_lane_widget_index (*/
      /*atlw,*/
      /*at->widget) + 1);*/
/*}*/


static void
on_add_lane_clicked (
  GtkWidget * widget,
  void * data)
{
  AutomationLaneWidget * self =
    Z_AUTOMATION_LANE_WIDGET (data);

  /* get next non visible automation track and add its widget via
   * track_widget_add_automatoin_track_widget */
  GET_TRACK (self);
  AutomationTracklist * atl =
    track_get_automation_tracklist (track);
  AutomationTrack * at =
    automation_tracklist_get_first_invisible_at (
      atl);
  if (!at)
    return;

  if (!at->al)
    {
      at->al = automation_lane_new (at);
      array_append (atl->automation_lanes,
                    atl->num_automation_lanes,
                    at->al);
      g_message ("atl num auto lanes %d",
                 atl->num_automation_lanes);
    }
  else
    {
      at->al->visible = 1;
    }
  g_message ("the at is %s",
             at->automatable->label);

  automation_tracklist_widget_refresh (
    atl->widget);
}

/**
 * Sets selected automation lane on combo box.
 */
static void
set_active_al (
  AutomationLaneWidget * self,
  GtkTreeModel *         model,
  AutomationLane *       al)
{
  GtkTreeIter iter;
  GtkTreePath * path;
  Automatable * a = al->at->automatable;
  switch (a->type)
    {
    case AUTOMATABLE_TYPE_CHANNEL_FADER:
      path =
        gtk_tree_path_new_from_indices (
          CHANNEL_FADER_INDEX,
          -1);
      break;
    case AUTOMATABLE_TYPE_PLUGIN_CONTROL:
      path =
        gtk_tree_path_new_from_indices (
          a->slot_index + PLUGIN_START_INDEX,
          a->control->index + PLUGIN_CONTROL_START_INDEX,
          -1);
      break;
    case AUTOMATABLE_TYPE_PLUGIN_ENABLED:
      path =
        gtk_tree_path_new_from_indices (
          a->slot_index + PLUGIN_START_INDEX,
          PLUGIN_ENABLED_INDEX,
          -1);
      break;
    case AUTOMATABLE_TYPE_CHANNEL_MUTE:
      path =
        gtk_tree_path_new_from_indices (
          CHANNEL_MUTE_INDEX,
          -1);
      break;
    case AUTOMATABLE_TYPE_CHANNEL_PAN:
      path =
        gtk_tree_path_new_from_indices (
          CHANNEL_PAN_INDEX,
          -1);
      break;
    }

  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_path_free (path);
  gtk_combo_box_set_active_iter (
    GTK_COMBO_BOX (self->selector),
    &iter);

}

static void
on_at_selector_changed (
  GtkComboBox *          widget,
  AutomationLaneWidget * self)
{
  GtkTreeIter iter;
  gtk_combo_box_get_active_iter (widget, &iter);
  GtkTreeModel * model =
    gtk_combo_box_get_model (widget);
  GValue value = G_VALUE_INIT;
  gtk_tree_model_get_value (
    model, &iter,
    COLUMN_AUTOMATABLE, &value);
  Automatable * a = g_value_get_pointer (&value);
  Track * track = self->al->at->track;

  /* get selected automation track */
  AutomationTrack * at =
    automation_tracklist_get_at_from_automatable (
      track_get_automation_tracklist (track),
      a);

  /* if this automation track is not already
   * in a visible lane */
  if (!at->al || !at->al->visible)
    automation_lane_update_automation_track (
      self->al,
      at);
  else
    {
      set_active_al (self,
                     self->selector_model,
                     self->al);
      ui_show_notification (
        "Selected automatable already has an "
        "automation lane");
    }

}

static GtkTreeModel *
create_automatables_store (Track * track)
{
  GtkTreeIter iter, iter2;
  GtkTreeStore *store;
  /*gint i;*/

  store = gtk_tree_store_new (NUM_COLUMNS,
                              G_TYPE_STRING,
                              G_TYPE_POINTER);

  ChannelTrack * ct = (ChannelTrack *) track;
  for (int i = 0; i < ct->channel->num_automatables; i++)
    {
      Automatable * a = ct->channel->automatables[i];
      gtk_tree_store_append (store, &iter, NULL);
      gtk_tree_store_set (store, &iter,
                          COLUMN_LABEL, a->label,
                          COLUMN_AUTOMATABLE, a,
                          -1);
    }

  for (int i = 0; i < STRIP_SIZE; i++)
    {
      Plugin * plugin = ct->channel->plugins[i];

      if (plugin)
        {
          /* add category: <slot>:<plugin name> */
          gtk_tree_store_append (store, &iter, NULL);
          char * label =
            g_strdup_printf (
              "%d:%s",
              i,
              plugin->descr->name);
          gtk_tree_store_set (store, &iter,
                              COLUMN_LABEL, label,
                              -1);
          g_free (label);
          for (int j = 0; j < plugin->num_automatables; j++)
            {
              Automatable * a = plugin->automatables[j];

              /* add automatable */
              gtk_tree_store_append (
                store, &iter2, &iter);
              gtk_tree_store_set (
                store, &iter2,
                COLUMN_LABEL, a->label,
                COLUMN_AUTOMATABLE, a,
                -1);
            }
        }
    }

  return GTK_TREE_MODEL (store);
}

static void
setup_combo_box (AutomationLaneWidget * self)
{
  self->selector_model =
    create_automatables_store (self->al->at->track);
  gtk_combo_box_set_model (self->selector,
                           self->selector_model);
  gtk_cell_layout_clear (
    GTK_CELL_LAYOUT (self->selector));
  GtkCellRenderer* renderer =
    gtk_cell_renderer_text_new ();
  g_object_set (G_OBJECT (renderer),
                "ellipsize",
                PANGO_ELLIPSIZE_END,
                NULL);
  gtk_cell_layout_pack_start (
    GTK_CELL_LAYOUT (self->selector),
    renderer,
    TRUE);
  gtk_cell_layout_set_attributes (
    GTK_CELL_LAYOUT (self->selector),
    renderer,
    "text", COLUMN_LABEL,
    NULL);

  set_active_al (self,
                 self->selector_model,
                 self->al);
}

void
automation_lane_widget_refresh (
  AutomationLaneWidget * self)
{
  g_message ("updating automation track widget");

  g_signal_handler_block (
    self->selector,
    self->selector_changed_cb_id);
  setup_combo_box (self);
  g_signal_handler_unblock (
    self->selector,
    self->selector_changed_cb_id);


  /* TODO do below for better performance */

  /* remove all automation points/curves from
   * arranger */
  /*z_gtk_container_remove_children_of_type (*/
    /*Z_ARRANGER_WIDGET (MW_TIMELINE),*/
    /*AUTOMATION_POINT_WIDGET_TYPE);*/
  /*z_gtk_container_remove_children_of_type (*/
    /*Z_ARRANGER_WIDGET (MW_TIMELINE),*/
    /*AUTOMATION_CURVE_WIDGET_TYPE);*/

  /* add automation points/curves for the currently
   * selected automatable */
}

/**
 * Creates a new Fader widget and binds it to the given value.
 */
AutomationLaneWidget *
automation_lane_widget_new (
  AutomationLane * al)
{
  AutomationLaneWidget * self =
    g_object_new (
      AUTOMATION_LANE_WIDGET_TYPE,
      NULL);

  self->al = al;
  al->widget = self;

  setup_combo_box (self);

  /* connect signals */
  g_signal_connect (
    self, "size-allocate",
    G_CALLBACK (size_allocate_cb), NULL);
  self->selector_changed_cb_id =
    g_signal_connect (
      self->selector, "changed",
      G_CALLBACK (on_at_selector_changed), self);

  gtk_widget_set_vexpand (
    GTK_WIDGET (self), 1);
  gtk_widget_set_visible (
    GTK_WIDGET (self), 1);

  return self;
}

float
automation_lane_widget_get_fvalue_at_y (
  AutomationLaneWidget * self,
  double                 _start_y)
{
  Automatable * a = self->al->at->automatable;

  GtkAllocation allocation;
  gtk_widget_get_allocation (
    GTK_WIDGET (self),
    &allocation);

  gint wx, valy;
  gtk_widget_translate_coordinates(
            GTK_WIDGET (MW_TIMELINE),
            GTK_WIDGET (self),
            0,
            _start_y,
            &wx,
            &valy);
  /*int miny = allocation.height;*/

  /* get ratio from widget */
  int widget_size = allocation.height;
  /*int widget_value = widget_size - (_start_y - maxy);*/
  int widget_value = widget_size - valy;
  /*g_message ("widget_size %d value %d", widget_size, widget_value);*/
  float widget_ratio = (float) widget_value / widget_size;
  /*g_message ("widget_ratio %f", widget_ratio);*/

  /*g_message ("getting f value at y, automatable %s",*/
             /*a->label);*/
  float max = automatable_get_maxf (a);
  float min = automatable_get_minf (a);
  /*g_message ("automatable max %f min %f", max, min);*/
  float automatable_size = automatable_get_sizef (a);
  float automatable_value = min + widget_ratio * automatable_size;
  automatable_value = MIN (automatable_value, max);
  automatable_value = MAX (automatable_value, min);
  g_message ("automatable value %f", automatable_value);

  return automatable_value;
}

double
automation_lane_widget_get_y (
  AutomationLaneWidget * self,
  AutomationPointWidget * ap_widget)
{
  gint wx, wy;
  gtk_widget_translate_coordinates(
            GTK_WIDGET (ap_widget),
            GTK_WIDGET (self),
            0,
            0,
            &wx,
            &wy);

  return wy;
}

static void
automation_lane_widget_init (AutomationLaneWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

static void
automation_lane_widget_class_init (
  AutomationLaneWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass,
    "automation_lane.ui");
  gtk_widget_class_set_css_name (
    klass,
    "automation-lane");

  gtk_widget_class_bind_template_child (
    klass,
    AutomationLaneWidget,
    selector);
  gtk_widget_class_bind_template_child (
    klass,
    AutomationLaneWidget,
    value_box);
  gtk_widget_class_bind_template_child (
    klass,
    AutomationLaneWidget,
    mute_toggle);
  gtk_widget_class_bind_template_callback (
    klass,
    on_add_lane_clicked);
}
