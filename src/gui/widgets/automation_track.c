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
#include "audio/track.h"
#include "gui/widgets/automation_track.h"
#include "gui/widgets/main_window.h"

G_DEFINE_TYPE (AutomationTrackWidget, automation_track_widget, GTK_TYPE_GRID)

static void
size_allocate_cb (GtkWidget * widget, GtkAllocation * allocation, void * data)
{
  gtk_widget_queue_draw (GTK_WIDGET (MAIN_WINDOW->timeline));
  gtk_widget_queue_allocate (GTK_WIDGET (MAIN_WINDOW->timeline));
}

static void
on_add_lane_clicked (GtkWidget * widget, void * data)
{

}

static GtkTreeModel *
create_automatables_store (Track * track)
{
  GtkTreeIter iter, iter2;
  GtkTreeStore *store;
  gint i;

  store = gtk_tree_store_new (2, G_TYPE_STRING, G_TYPE_POINTER);

  for (int i = 0; i < track->num_automatables; i++)
    {
      Automatable * a = track->automatables[i];
      gtk_tree_store_append (store, &iter, NULL);
      gtk_tree_store_set (store, &iter,
                          0, a->label,
                          1, a,
                          -1);
    }

  for (int i = 0; i < STRIP_SIZE; i++)
    {
      Plugin * plugin = track->channel->strip[i];
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

/**
 * Creates a new Fader widget and binds it to the given value.
 */
AutomationTrackWidget *
automation_track_widget_new (AutomationTrack * automation_track)
{
  AutomationTrackWidget * self = g_object_new (
                            AUTOMATION_TRACK_WIDGET_TYPE,
                            NULL);

  self->automation_track = automation_track;

  GtkTreeModel * model = create_automatables_store (automation_track->track);
  gtk_combo_box_set_model (self->selector,
                           model);
  GtkCellRenderer* renderer = gtk_cell_renderer_text_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (self->selector), renderer, TRUE);
    gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (self->selector), renderer,
                                    "text", 0,
                                    NULL);
    /*gtk_cell_layout_set_cell_data_func (GTK_CELL_LAYOUT (combo)*/
                                        /*renderer,*/
                                        /*is_capital_sensitive,*/
                                        /*NULL, NULL);*/

    GtkTreeIter iter;
    GtkTreePath * path = gtk_tree_path_new_from_indices (0, -1);
    gtk_tree_model_get_iter (model, &iter, path);
    gtk_tree_path_free (path);
    gtk_combo_box_set_active_iter (GTK_COMBO_BOX (self->selector), &iter);
  /*self->value = digital_meter_widget_new (DIGITAL_METER_TYPE_VALUE,*/
                                          /*NULL,*/

  /*gtk_box_pack_start (self->value_box,*/
                      /*GTK_WIDGET (self->color),*/
                      /*1,*/
                      /*1,*/
                      /*0);*/



  GtkWidget *image = gtk_image_new_from_resource (
          "/online/alextee/zrythm/mute.svg");
  gtk_button_set_image (GTK_BUTTON (self->mute_toggle), image);
  gtk_button_set_label (GTK_BUTTON (self->mute_toggle),
                        "");
  g_signal_connect (self, "size-allocate",
                    G_CALLBACK (size_allocate_cb), NULL);

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
  gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass),
                                        on_add_lane_clicked);
}



