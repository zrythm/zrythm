/*
 * gui/widgets/automation_track.c - AutomationTrack
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

/** \file
 */

#include "audio/automatable.h"
#include "audio/automation_track.h"
#include "audio/channel.h"
#include "audio/instrument_track.h"
#include "audio/track.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/automation_track.h"
#include "gui/widgets/automation_tracklist.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/instrument_track.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/region.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/track.h"

G_DEFINE_TYPE (AutomationTrackWidget, automation_track_widget, GTK_TYPE_PANED)

static void
size_allocate_cb (GtkWidget * widget, GtkAllocation * allocation, void * data)
{
  gtk_widget_queue_draw (GTK_WIDGET (MW_TIMELINE));
  gtk_widget_queue_allocate (GTK_WIDGET (MW_TIMELINE));
}

static void
add_automation_track (AutomationTracklistWidget * atlw,
                      AutomationTrack *       at)
{
  automation_tracklist_widget_add_automation_track (
    atlw,
    at,
    automation_tracklist_widget_get_automation_track_widget_index (
      atlw,
      at->widget) + 1);
}

static void
on_add_lane_clicked (GtkWidget * widget, void * data)
{
  AutomationTrackWidget * self = AUTOMATION_TRACK_WIDGET (data);

  /* get next non visible automation track and add its widget via
   * track_widget_add_automatoin_track_widget */
  InstrumentTrack * it;
  switch (self->at->track->type)
    {
    case TRACK_TYPE_INSTRUMENT:
      it = (InstrumentTrack *) self->at->track;
      for (int i = 0; i < it->num_automation_tracks; i++)
        {
          AutomationTrack * at =
            it->automation_tracks[i];
          if (!at->widget)
            {
              add_automation_track (self->at->track->widget->ins_tw->automation_tracklist_widget,
                                    at);
              break;
            }
        }
      break;
    case TRACK_TYPE_MASTER:
      /* TODO */
    case TRACK_TYPE_AUDIO:
    case TRACK_TYPE_CHORD:
    case TRACK_TYPE_BUS:
      break;
    }
}

/**
 * For testing.
 */
static void
on_show (GtkWidget *widget,
         gpointer   user_data)
{
  /*AutomationTrackWidget * self = AUTOMATION_TRACK_WIDGET (user_data);*/
  g_message ("showing");
}


void
on_at_selector_changed (GtkComboBox * widget,
               gpointer     user_data)
{
  AutomationTrackWidget * self = AUTOMATION_TRACK_WIDGET (user_data);

  g_message ("selector changed");
  GtkTreeIter iter;
  gtk_combo_box_get_active_iter (widget, &iter);
  GtkTreeModel * model = gtk_combo_box_get_model (widget);
  GValue value = G_VALUE_INIT;
  gtk_tree_model_get_value (model, &iter, 1, &value);
  Automatable * a = g_value_get_pointer (&value);
  automation_track_set_automatable (self->at, a);
}

static GtkTreeModel *
create_automatables_store (Track * track)
{
  GtkTreeIter iter, iter2;
  GtkTreeStore *store;
  /*gint i;*/

  store = gtk_tree_store_new (2, G_TYPE_STRING, G_TYPE_POINTER);

  int num_automatables;
  Automatable ** automatables;
  InstrumentTrack * it;
  switch (track->type)
    {
    case TRACK_TYPE_INSTRUMENT:
      it = (InstrumentTrack *) track;
      num_automatables = it->channel->num_automatables;
      automatables = it->channel->automatables;
      break;
    case TRACK_TYPE_MASTER:
    case TRACK_TYPE_AUDIO:
    case TRACK_TYPE_CHORD:
    case TRACK_TYPE_BUS:
      break;
    }
  for (int i = 0; i < num_automatables; i++)
    {
      Automatable * a = automatables[i];
      gtk_tree_store_append (store, &iter, NULL);
      gtk_tree_store_set (store, &iter,
                          0, a->label,
                          1, a,
                          -1);
    }

  for (int i = 0; i < STRIP_SIZE; i++)
    {
      Plugin * plugin = NULL;
      InstrumentTrack * it;
      switch (track->type)
        {
        case TRACK_TYPE_INSTRUMENT:
          it = (InstrumentTrack *) track;
          plugin = it->channel->strip[i];
          break;
        case TRACK_TYPE_MASTER:
        case TRACK_TYPE_AUDIO:
        case TRACK_TYPE_CHORD:
        case TRACK_TYPE_BUS:
          break;
        }

      if (plugin)
        {
          gtk_tree_store_append (store, &iter, NULL);
          char * label = g_strdup_printf ("%d:%s", i, plugin->descr->name);
          gtk_tree_store_set (store, &iter, 0, label, -1);
          g_free (label);
          for (int j = 0; j < plugin->num_automatables; j++)
            {
              Automatable * a = plugin->automatables[j];
              gtk_tree_store_append (store, &iter2, &iter);
              gtk_tree_store_set (store, &iter2,
                                  0, a->label,
                                  1, a,
                                  -1);
            }
        }
    }

  return GTK_TREE_MODEL (store);
}

static void
setup_combo_box (AutomationTrackWidget * self)
{
  GtkTreeModel * model = create_automatables_store (self->at->track);
  gtk_combo_box_set_model (self->selector,
                           model);
  gtk_cell_layout_clear (GTK_CELL_LAYOUT (self->selector));
  GtkCellRenderer* renderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (self->selector), renderer, TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (self->selector), renderer,
                                  "text", 0,
                                  NULL);

  GtkTreeIter iter;
  /* FIXME find the associated automatable */
  GtkTreePath * path = gtk_tree_path_new_from_indices (0, -1);
  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_path_free (path);
  gtk_combo_box_set_active_iter (GTK_COMBO_BOX (self->selector), &iter);
}

void
automation_track_widget_update (AutomationTrackWidget * self)
{
  g_message ("updating automation track widget");
  setup_combo_box (self);
}

/**
 * Creates a new Fader widget and binds it to the given value.
 */
AutomationTrackWidget *
automation_track_widget_new (AutomationTrack * automation_track)
{
  AutomationTrackWidget * self = g_object_new (
                            AUTOMATION_TRACK_WIDGET_TYPE,
                            NULL);

  self->at = automation_track;
  automation_track->widget = self;

  setup_combo_box (self);

  GtkWidget *image = gtk_image_new_from_resource (
          "/online/alextee/zrythm/mute.svg");
  gtk_button_set_image (GTK_BUTTON (self->mute_toggle), image);
  gtk_button_set_label (GTK_BUTTON (self->mute_toggle),
                        "");

  /* connect signals */
  g_signal_connect (self, "size-allocate",
                    G_CALLBACK (size_allocate_cb), NULL);
  g_signal_connect (self, "show",
                    G_CALLBACK (on_show), self);

  return self;
}

static void
automation_track_widget_init (AutomationTrackWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

static void
automation_track_widget_class_init (AutomationTrackWidgetClass * klass)
{
  gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass),
                                               "/online/alextee/zrythm/ui/automation_track.ui");

  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        AutomationTrackWidget,
                                        selector);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        AutomationTrackWidget,
                                        value_box);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        AutomationTrackWidget,
                                        mute_toggle);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        AutomationTrackWidget,
                                        at_grid);
  gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass),
                                        on_add_lane_clicked);
}

float
automation_track_widget_get_fvalue_at_y (AutomationTrackWidget * at_widget,
                                         double                  _start_y)
{
  Automatable * a = at_widget->at->automatable;

  GtkAllocation allocation;
  gtk_widget_get_allocation (GTK_WIDGET (at_widget->at_grid),
                             &allocation);

  gint wx, valy;
  gtk_widget_translate_coordinates(
            GTK_WIDGET (MW_TIMELINE),
            GTK_WIDGET (at_widget->at_grid),
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
automation_track_widget_get_y (AutomationTrackWidget * at_widget,
                               AutomationPointWidget * ap_widget)
{
  gint wx, wy;
  gtk_widget_translate_coordinates(
            GTK_WIDGET (ap_widget),
            GTK_WIDGET (at_widget),
            0,
            0,
            &wx,
            &wy);

  return wy;
}
