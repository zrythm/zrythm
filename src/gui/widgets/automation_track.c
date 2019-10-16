/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/automatable.h"
#include "audio/automation_track.h"
#include "audio/automation_tracklist.h"
#include "audio/audio_bus_track.h"
#include "audio/channel.h"
#include "audio/engine.h"
#include "audio/instrument_track.h"
#include "audio/track.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/automatable_selector_button.h"
#include "gui/widgets/automation_track.h"
#include "gui/widgets/automation_tracklist.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/instrument_track.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/region.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_panel.h"
#include "gui/widgets/track.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/resources.h"

G_DEFINE_TYPE (AutomationTrackWidget,
               automation_track_widget,
               GTK_TYPE_GRID)

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
 * Adds automation track for automation track.
 */
/*static void*/
/*add_automation_track (*/
  /*AutomationTracklistWidget * atlw,*/
  /*AutomationTrack *           at)*/
/*{*/
  /*automation_tracklist_widget_add_automation_track (*/
    /*atlw,*/
    /*at,*/
    /*automation_tracklist_widget_get_automation_track_widget_index (*/
      /*atlw,*/
      /*at->widget) + 1);*/
/*}*/


static void
on_add_track_clicked (
  GtkWidget *             widget,
  AutomationTrackWidget * self)
{
  /* get next non visible automation track and add
   * its widget via
   * track_widget_add_automatoin_track_widget */
  AutomationTracklist * atl =
    track_get_automation_tracklist (
      self->at->track);
  AutomationTrack * at =
    automation_tracklist_get_first_invisible_at (
      atl);
  g_return_if_fail (at);

  if (!at->created)
    at->created = 1;
  at->visible = 1;

  EVENTS_PUSH (ET_AUTOMATION_TRACK_ADDED, at);
}

static void
on_remove_track_clicked (
  GtkWidget *             widget,
  AutomationTrackWidget * self)
{
  /* don't remove if last visible */
  AutomationTracklist * atl =
    track_get_automation_tracklist (
      self->at->track);
  if (automation_tracklist_get_num_visible (
        atl) == 1)
    return;

  self->at->visible = 0;

  EVENTS_PUSH (
    ET_AUTOMATION_TRACK_REMOVED,
    self->at);
}

/**
 * Sets selected automation track on combo box.
 */
/*static void*/
/*set_active_al (*/
  /*AutomationTrackWidget * self,*/
  /*GtkTreeModel *         model,*/
  /*AutomationTrack *       al)*/
/*{*/
  /*GtkTreeIter iter;*/
  /*GtkTreePath * path;*/
  /*Automatable * a = al->at->automatable;*/
  /*switch (a->type)*/
    /*{*/
    /*case AUTOMATABLE_TYPE_CHANNEL_FADER:*/
      /*path =*/
        /*gtk_tree_path_new_from_indices (*/
          /*CHANNEL_FADER_INDEX,*/
          /*-1);*/
      /*break;*/
    /*case AUTOMATABLE_TYPE_PLUGIN_CONTROL:*/
      /*path =*/
        /*gtk_tree_path_new_from_indices (*/
          /*a->slot_index + PLUGIN_START_INDEX,*/
          /*a->control->index + PLUGIN_CONTROL_START_INDEX,*/
          /*-1);*/
      /*break;*/
    /*case AUTOMATABLE_TYPE_PLUGIN_ENABLED:*/
      /*path =*/
        /*gtk_tree_path_new_from_indices (*/
          /*a->slot_index + PLUGIN_START_INDEX,*/
          /*PLUGIN_ENABLED_INDEX,*/
          /*-1);*/
      /*break;*/
    /*case AUTOMATABLE_TYPE_CHANNEL_MUTE:*/
      /*path =*/
        /*gtk_tree_path_new_from_indices (*/
          /*CHANNEL_MUTE_INDEX,*/
          /*-1);*/
      /*break;*/
    /*case AUTOMATABLE_TYPE_CHANNEL_PAN:*/
      /*path =*/
        /*gtk_tree_path_new_from_indices (*/
          /*CHANNEL_PAN_INDEX,*/
          /*-1);*/
      /*break;*/
    /*}*/

  /*gtk_tree_model_get_iter (model, &iter, path);*/
  /*gtk_tree_path_free (path);*/
  /*gtk_combo_box_set_active_iter (*/
    /*GTK_COMBO_BOX (self->selector),*/
    /*&iter);*/

/*}*/

/*static void*/
/*on_at_selector_changed (*/
  /*GtkComboBox *          widget,*/
  /*AutomationTrackWidget * self)*/
/*{*/
  /*GtkTreeIter iter;*/
  /*gtk_combo_box_get_active_iter (widget, &iter);*/
  /*GtkTreeModel * model =*/
    /*gtk_combo_box_get_model (widget);*/
  /*GValue value = G_VALUE_INIT;*/
  /*gtk_tree_model_get_value (*/
    /*model, &iter,*/
    /*COLUMN_AUTOMATABLE, &value);*/
  /*Automatable * a = g_value_get_pointer (&value);*/
  /*Track * track = self->al->at->track;*/

  /*[> get selected automation track <]*/
  /*AutomationTrack * at =*/
    /*automation_tracklist_get_at_from_automatable (*/
      /*track_get_automation_tracklist (track),*/
      /*a);*/

  /* if this automation track is not already
   * in a visible track */
  /*if (!at->al || !at->al->visible)*/
    /*automation_track_update_automation_track (*/
      /*self->al,*/
      /*at);*/
  /*else*/
    /*{*/
      /*set_active_al (self,*/
                     /*self->selector_model,*/
                     /*self->al);*/
      /*ui_show_notification (*/
        /*"Selected automatable already has an "*/
        /*"automation track");*/
    /*}*/

/*}*/

/*static GtkTreeModel **/
/*create_automatables_store (Track * track)*/
/*{*/
  /*GtkTreeIter iter, iter2;*/
  /*GtkTreeStore *store;*/
  /*[>gint i;<]*/

  /*store = gtk_tree_store_new (NUM_COLUMNS,*/
                              /*G_TYPE_STRING,*/
                              /*G_TYPE_POINTER);*/

  /*ChannelTrack * ct = (ChannelTrack *) track;*/
  /*for (int i = 0; i < ct->channel->num_automatables; i++)*/
    /*{*/
      /*Automatable * a = ct->channel->automatables[i];*/
      /*gtk_tree_store_append (store, &iter, NULL);*/
      /*gtk_tree_store_set (store, &iter,*/
                          /*COLUMN_LABEL, a->label,*/
                          /*COLUMN_AUTOMATABLE, a,*/
                          /*-1);*/
    /*}*/

  /*for (int i = 0; i < STRIP_SIZE; i++)*/
    /*{*/
      /*Plugin * plugin = ct->channel->plugins[i];*/

      /*if (plugin)*/
        /*{*/
          /*[> add category: <slot>:<plugin name> <]*/
          /*gtk_tree_store_append (store, &iter, NULL);*/
          /*char * label =*/
            /*g_strdup_printf (*/
              /*"%d:%s",*/
              /*i,*/
              /*plugin->descr->name);*/
          /*gtk_tree_store_set (store, &iter,*/
                              /*COLUMN_LABEL, label,*/
                              /*-1);*/
          /*g_free (label);*/
          /*for (int j = 0; j < plugin->num_automatables; j++)*/
            /*{*/
              /*Automatable * a = plugin->automatables[j];*/

              /*[> add automatable <]*/
              /*gtk_tree_store_append (*/
                /*store, &iter2, &iter);*/
              /*gtk_tree_store_set (*/
                /*store, &iter2,*/
                /*COLUMN_LABEL, a->label,*/
                /*COLUMN_AUTOMATABLE, a,*/
                /*-1);*/
            /*}*/
        /*}*/
    /*}*/

  /*return GTK_TREE_MODEL (store);*/
/*}*/


/**
 * Returns the y pixels from the value based on the
 * allocation of the automation track.
 */
int
automation_track_widget_get_y_px_from_normalized_val (
  AutomationTrackWidget * self,
  float                  normalized_val)
{
  int allocated_h =
    gtk_widget_get_allocated_height (
      GTK_WIDGET (self));
  int point =
    allocated_h -
    (int) (normalized_val * (float) allocated_h);

  return point;
}

void
automation_track_widget_update_current_val (
  AutomationTrackWidget * self)
{
  char * val =
    g_strdup_printf (
      "%.2f",
      (double)
      automatable_get_val (
        self->at->automatable));
  gtk_label_set_text (self->current_val, val);
  g_free (val);
}

void
automation_track_widget_refresh (
  AutomationTrackWidget * self)
{
  /*g_signal_handler_block (*/
    /*self->selector,*/
    /*self->selector_changed_cb_id);*/
  automatable_selector_button_widget_setup (
    self->selector, self);
  /*g_signal_handler_unblock (*/
    /*self->selector,*/
    /*self->selector_changed_cb_id);*/


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

  automation_track_widget_update_current_val (
    self);
}

static void
on_destroy (
  AutomationTrackWidget * self)
{
  AutomationTrack * at = self->at;

  g_object_unref (self);

  at->widget = NULL;
}

/**
 * Creates a new Fader widget and binds it to the given value.
 */
AutomationTrackWidget *
automation_track_widget_new (
  AutomationTrack * at)
{
  AutomationTrackWidget * self =
    g_object_new (
      AUTOMATION_TRACK_WIDGET_TYPE,
      NULL);

  self->at = at;
  at->widget = self;

  automatable_selector_button_widget_setup (
    self->selector, self);

  /* connect signals */
  g_signal_connect (
    self, "size-allocate",
    G_CALLBACK (size_allocate_cb), NULL);
  /*self->selector_changed_cb_id =*/
    /*g_signal_connect (*/
      /*self->selector, "changed",*/
      /*G_CALLBACK (on_at_selector_changed), self);*/

  gtk_widget_set_vexpand (
    GTK_WIDGET (self), 1);
  gtk_widget_set_visible (
    GTK_WIDGET (self), 1);

  g_object_ref (self);

  return self;
}

/**
 * Gets the float value at the given Y coordinate
 * relative to the AutomationTrackWidget.
 */
float
automation_track_widget_get_fvalue_at_y (
  AutomationTrackWidget * self,
  double                 _start_y)
{
  Automatable * a = self->at->automatable;

  GtkAllocation allocation;
  gtk_widget_get_allocation (
    GTK_WIDGET (self),
    &allocation);

  gint wx, valy;
  gtk_widget_translate_coordinates(
            GTK_WIDGET (MW_TIMELINE),
            GTK_WIDGET (self),
            0,
            (int) _start_y,
            &wx,
            &valy);

  /* get ratio from widget */
  int widget_size = allocation.height;
  int widget_value = widget_size - valy;
  float widget_ratio =
    CLAMP (
      (float) widget_value / (float) widget_size,
      0.f, 1.f);
  float automatable_value =
    automatable_normalized_val_to_real (
      a, widget_ratio);

  return automatable_value;
}

/**
 * Returns the y pixels of the automation point in
 * the automation track.
 */
double
automation_track_widget_get_y (
  AutomationTrackWidget * self,
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
automation_track_widget_init (
  AutomationTrackWidget * self)
{
  gtk_widget_destroy (
    GTK_WIDGET (g_object_new (
      AUTOMATABLE_SELECTOR_BUTTON_WIDGET_TYPE,
      NULL)));

  gtk_widget_init_template (GTK_WIDGET (self));

  g_signal_connect (
    self, "destroy",
    G_CALLBACK (on_destroy), NULL);
}

static void
automation_track_widget_class_init (
  AutomationTrackWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass,
    "automation_track.ui");
  gtk_widget_class_set_css_name (
    klass,
    "automation-track");

  gtk_widget_class_bind_template_child (
    klass,
    AutomationTrackWidget,
    selector);
  gtk_widget_class_bind_template_child (
    klass,
    AutomationTrackWidget,
    value_box);
  gtk_widget_class_bind_template_child (
    klass,
    AutomationTrackWidget,
    mute_toggle);
  gtk_widget_class_bind_template_child (
    klass,
    AutomationTrackWidget,
    current_val);
  gtk_widget_class_bind_template_callback (
    klass,
    on_add_track_clicked);
  gtk_widget_class_bind_template_callback (
    klass,
    on_remove_track_clicked);
}
