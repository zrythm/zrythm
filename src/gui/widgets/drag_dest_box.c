// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Box used as destination for DnD.
 */

#include "actions/mixer_selections_action.h"
#include "actions/tracklist_selections.h"
#include "dsp/channel.h"
#include "dsp/modulator_track.h"
#include "dsp/port_connections_manager.h"
#include "dsp/track.h"
#include "dsp/tracklist.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/backend/wrapped_object_with_change_signal.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/drag_dest_box.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/right_dock_edge.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"
#include "project.h"
#include "settings/plugin_settings.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/objects.h"
#include "utils/resources.h"
#include "utils/string.h"
#include "utils/symap.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (DragDestBoxWidget, drag_dest_box_widget, GTK_TYPE_BOX)

static void
on_dnd_leave_value_ready (
  GObject *      source_object,
  GAsyncResult * res,
  gpointer       user_data)
{
  GdkDrop *      drop = GDK_DROP (source_object);
  GError *       err = NULL;
  const GValue * value = gdk_drop_read_value_finish (drop, res, &err);
  if (err)
    {
      g_message ("error: %s", err->message);
      return;
    }

  if (!G_VALUE_HOLDS (value, WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE))
    return;

  WrappedObjectWithChangeSignal * wrapped_obj = g_value_get_object (value);
  if (wrapped_obj->type == WRAPPED_OBJECT_TYPE_TRACK)
    {
      /* unhighlight bottom part of last track */
      Track * track = tracklist_get_last_track (
        TRACKLIST, TRACKLIST_PIN_OPTION_UNPINNED_ONLY, true);
      track_widget_do_highlight (track->widget, 0, 0, 0);
    }
}

static void
on_dnd_leave (GtkDropTarget * drop_target, DragDestBoxWidget * self)
{
  GdkDrop * drop = gtk_drop_target_get_current_drop (drop_target);
  if (!drop)
    return;

  gdk_drop_read_value_async (
    drop, WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE, 0, NULL,
    on_dnd_leave_value_ready, self);
}

static void
on_dnd_motion_value_ready (
  GObject *      source_object,
  GAsyncResult * res,
  gpointer       user_data)
{
  GdkDrop *      drop = GDK_DROP (source_object);
  GError *       err = NULL;
  const GValue * value = gdk_drop_read_value_finish (drop, res, &err);
  if (err)
    {
      g_message ("error: %s", err->message);
      return;
    }

  SupportedFile *    supported_file = NULL;
  Track *            dropped_track = NULL;
  Plugin *           pl = NULL;
  PluginDescriptor * pl_descr = NULL;
  if (G_VALUE_HOLDS (value, WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE))
    {
      WrappedObjectWithChangeSignal * wrapped_obj = g_value_get_object (value);
      if (wrapped_obj->type == WRAPPED_OBJECT_TYPE_SUPPORTED_FILE)
        {
          supported_file = (SupportedFile *) wrapped_obj->obj;
        }
      else if (wrapped_obj->type == WRAPPED_OBJECT_TYPE_PLUGIN)
        {
          pl = (Plugin *) wrapped_obj->obj;
        }
      else if (wrapped_obj->type == WRAPPED_OBJECT_TYPE_PLUGIN_DESCR)
        {
          pl_descr = (PluginDescriptor *) wrapped_obj->obj;
        }
      else if (wrapped_obj->type == WRAPPED_OBJECT_TYPE_TRACK)
        {
          dropped_track = (Track *) wrapped_obj->obj;
        }
    }

  bool has_files = false;
  if (
    G_VALUE_HOLDS (value, GDK_TYPE_FILE_LIST)
    || G_VALUE_HOLDS (value, G_TYPE_FILE))
    {
      has_files = true;
    }

  if (has_files)
    {
      /* defer to drag_data_received */
      /*self->defer_drag_motion_status = 1;*/

      /*gtk_drag_get_data (*/
      /*widget, context, target, time);*/

      return;
    }
  else if (supported_file)
    {

      return;
    }
  else if (pl_descr)
    {
      /*gtk_drag_highlight (widget);*/

      return;
    }
  else if (pl)
    {
    }
  else if (dropped_track)
    {
      /*gtk_drag_unhighlight (widget);*/

      /* highlight bottom part of last track */
      Track * track = tracklist_get_last_track (
        TRACKLIST, TRACKLIST_PIN_OPTION_UNPINNED_ONLY, true);
      int track_height = gtk_widget_get_height (GTK_WIDGET (track->widget));
      track_widget_do_highlight (track->widget, 0, track_height - 1, 1);
    }
  else
    {
      /*gtk_drag_unhighlight (widget);*/
    }
}

static GdkDragAction
on_dnd_motion (
  GtkDropTarget * drop_target,
  gdouble         x,
  gdouble         y,
  gpointer        user_data)
{
  DragDestBoxWidget * self = Z_DRAG_DEST_BOX_WIDGET (user_data);

  /* request value */
  GdkDrop * drop = gtk_drop_target_get_current_drop (drop_target);
  /* FIXME just use gtk_drop_target_get_value() */
  gdk_drop_read_value_async (
    drop, G_TYPE_OBJECT, 0, NULL, on_dnd_motion_value_ready, self);

  return GDK_ACTION_MOVE;
}

static gboolean
on_dnd_drop (
  GtkDropTarget * drop_target,
  const GValue *  value,
  double          x,
  double          y,
  gpointer        data)
{
  DragDestBoxWidget * self = Z_DRAG_DEST_BOX_WIDGET (data);

  GdkDragAction action = z_gtk_drop_target_get_selected_action (drop_target);

  SupportedFile *    file = NULL;
  PluginDescriptor * pd = NULL;
  Plugin *           pl = NULL;
  Track *            track = NULL;
  if (G_VALUE_HOLDS (value, WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE))
    {
      WrappedObjectWithChangeSignal * wrapped_obj = g_value_get_object (value);
      if (wrapped_obj->type == WRAPPED_OBJECT_TYPE_SUPPORTED_FILE)
        {
          file = (SupportedFile *) wrapped_obj->obj;
        }
      else if (wrapped_obj->type == WRAPPED_OBJECT_TYPE_PLUGIN)
        {
          pl = (Plugin *) wrapped_obj->obj;
        }
      else if (wrapped_obj->type == WRAPPED_OBJECT_TYPE_PLUGIN_DESCR)
        {
          pd = (PluginDescriptor *) wrapped_obj->obj;
        }
      else if (wrapped_obj->type == WRAPPED_OBJECT_TYPE_TRACK)
        {
          track = (Track *) wrapped_obj->obj;
        }
    }

  if (
    G_VALUE_HOLDS (value, GDK_TYPE_FILE_LIST)
    || G_VALUE_HOLDS (value, G_TYPE_FILE) || file)
    {
      char ** uris = NULL;
      if (G_VALUE_HOLDS (value, G_TYPE_FILE))
        {
          GFile *        gfile = g_value_get_object (value);
          GStrvBuilder * uris_builder = g_strv_builder_new ();
          char *         uri = g_file_get_uri (gfile);
          g_strv_builder_add (uris_builder, uri);
          uris = g_strv_builder_end (uris_builder);
        }
      else if (G_VALUE_HOLDS (value, GDK_TYPE_FILE_LIST))
        {
          GStrvBuilder * uris_builder = g_strv_builder_new ();
          GSList *       l;
          for (l = g_value_get_boxed (value); l; l = l->next)
            {
              char * uri = g_file_get_uri (l->data);
              g_strv_builder_add (uris_builder, uri);
              g_free (uri);
            }
          uris = g_strv_builder_end (uris_builder);
        }

      if (!zrythm_app_check_and_show_trial_limit_error (zrythm_app))
        {
          GError * err = NULL;
          bool     success = tracklist_import_files (
            TRACKLIST, uris, file, NULL, NULL, -1, NULL, NULL, &err);
          if (!success)
            {
              HANDLE_ERROR_LITERAL (err, _ ("Failed to import files"));
            }
        }
      return true;
    }
  else if (pd)
    {
      if (
        self->type == DRAG_DEST_BOX_TYPE_MIXER
        || self->type == DRAG_DEST_BOX_TYPE_TRACKLIST)
        {
          PluginSetting * setting = plugin_setting_new_default (pd);

          if (!zrythm_app_check_and_show_trial_limit_error (zrythm_app))
            {
              plugin_setting_activate (setting);
            }

          plugin_setting_free (setting);
        }
      else
        {
          PluginSetting * setting = plugin_setting_new_default (pd);
          GError *        err = NULL;
          bool            ret = mixer_selections_action_perform_create (
            Z_PLUGIN_SLOT_MODULATOR, track_get_name_hash (P_MODULATOR_TRACK),
            P_MODULATOR_TRACK->num_modulators, setting, 1, &err);
          if (!ret)
            {
              HANDLE_ERROR (err, "%s", _ ("Failed to create plugin"));
            }
          plugin_setting_free (setting);
        }

      return true;
    }
  else if (pl)
    {
      /* NOTE this is a cloned pointer, don't use
       * it */
      g_warn_if_fail (pl);

      GError * err = NULL;
      bool     ret;
      if (action == GDK_ACTION_COPY)
        {
          ret = mixer_selections_action_perform_copy (
            MIXER_SELECTIONS, PORT_CONNECTIONS_MGR, Z_PLUGIN_SLOT_INSERT, 0, 0,
            &err);
        }
      else if (action == GDK_ACTION_MOVE)
        {
          ret = mixer_selections_action_perform_move (
            MIXER_SELECTIONS, PORT_CONNECTIONS_MGR, Z_PLUGIN_SLOT_INSERT, 0, 0,
            &err);
        }
      else
        g_return_val_if_reached (true);

      if (!ret)
        {
          HANDLE_ERROR (err, "%s", _ ("Failed to move or copy plugin"));
        }

      return true;
    }
  else if (track)
    {
      tracklist_selections_select_foldable_children (TRACKLIST_SELECTIONS);
      int pos = tracklist_get_last_pos (
        TRACKLIST, TRACKLIST_PIN_OPTION_UNPINNED_ONLY, true);
      pos++;

      GError * err = NULL;
      bool     ret = false;
      if (action == GDK_ACTION_COPY)
        {
          if (
            tracklist_selections_contains_uncopyable_track (TRACKLIST_SELECTIONS))
            {
              g_message (
                "cannot copy - track selection "
                "contains uncopyable track");
              return false;
            }
          if (!zrythm_app_check_and_show_trial_limit_error (zrythm_app))
            {
              ret = tracklist_selections_action_perform_copy (
                TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, pos, &err);
            }
        }
      else if (action == GDK_ACTION_MOVE)
        {
          ret = tracklist_selections_action_perform_move (
            TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, pos, &err);
        }
      else
        g_return_val_if_reached (true);

      if (!ret)
        {
          HANDLE_ERROR (err, "%s", _ ("Failed to move or copy track"));
        }

      return true;
    }

  /* drag was not accepted */
  return false;
}

static void
show_context_menu (DragDestBoxWidget * self, double x, double y)
{
  GMenu * menu = tracklist_widget_generate_add_track_menu ();

  z_gtk_show_context_menu_from_g_menu (self->popover_menu, x, y, menu);
}

static void
on_right_click (
  GtkGestureClick * gesture,
  gint              n_press,
  gdouble           x,
  gdouble           y,
  gpointer          user_data)
{
  DragDestBoxWidget * self = Z_DRAG_DEST_BOX_WIDGET (user_data);

  if (n_press == 1)
    {
      show_context_menu (self, x, y);
    }
}

static void
on_click_pressed (
  GtkGestureClick *   gesture,
  gint                n_press,
  gdouble             x,
  gdouble             y,
  DragDestBoxWidget * self)
{
  mixer_selections_clear (MIXER_SELECTIONS, F_PUBLISH_EVENTS);
  tracklist_selections_select_last_visible (TRACKLIST_SELECTIONS);

  PROJECT->last_selection = Z_PROJECT_SELECTION_TYPE_TRACKLIST;
  EVENTS_PUSH (ET_PROJECT_SELECTION_TYPE_CHANGED, NULL);
}

static void
setup_dnd (DragDestBoxWidget * self)
{
  GtkDropTarget * drop_target =
    gtk_drop_target_new (G_TYPE_INVALID, GDK_ACTION_COPY | GDK_ACTION_MOVE);
  GType types[] = {
    GDK_TYPE_FILE_LIST, G_TYPE_FILE, WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE
  };
  gtk_drop_target_set_gtypes (drop_target, types, G_N_ELEMENTS (types));
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (drop_target));

  /* connect signal */
  g_signal_connect (drop_target, "motion", G_CALLBACK (on_dnd_motion), self);
  g_signal_connect (drop_target, "drop", G_CALLBACK (on_dnd_drop), self);
  g_signal_connect (drop_target, "leave", G_CALLBACK (on_dnd_leave), self);
}

/**
 * Creates a drag destination box widget.
 */
DragDestBoxWidget *
drag_dest_box_widget_new (
  GtkOrientation  orientation,
  int             spacing,
  DragDestBoxType type)
{
  /* create */
  DragDestBoxWidget * self = g_object_new (DRAG_DEST_BOX_WIDGET_TYPE, NULL);

  self->type = type;

  switch (type)
    {
    case DRAG_DEST_BOX_TYPE_MIXER:
    case DRAG_DEST_BOX_TYPE_MODULATORS:
      gtk_widget_set_size_request (GTK_WIDGET (self), 160, -1);
      break;
    case DRAG_DEST_BOX_TYPE_TRACKLIST:
      gtk_widget_set_size_request (GTK_WIDGET (self), -1, 160);
      break;
    }

  /* make expandable */
  gtk_widget_set_vexpand (GTK_WIDGET (self), true);
  gtk_widget_set_hexpand (GTK_WIDGET (self), true);

  setup_dnd (self);

  return self;
}

/**
 * GTK boilerplate.
 */
static void
drag_dest_box_widget_init (DragDestBoxWidget * self)
{
  self->popover_menu = GTK_POPOVER_MENU (gtk_popover_menu_new_from_model (NULL));
  gtk_box_append (GTK_BOX (self), GTK_WIDGET (self->popover_menu));

  self->click = GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  g_signal_connect (
    G_OBJECT (self->click), "pressed", G_CALLBACK (on_click_pressed), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (self->click));

  self->right_click = GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (self->right_click), GDK_BUTTON_SECONDARY);
  g_signal_connect (
    G_OBJECT (self->right_click), "pressed", G_CALLBACK (on_right_click), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (self->right_click));

#if 0
  self->drag =
    GTK_GESTURE_DRAG (gtk_gesture_drag_new ());
  g_signal_connect (
    G_OBJECT(self->drag), "drag-begin",
    G_CALLBACK (drag_begin),  self);
  g_signal_connect (
    G_OBJECT(self->drag), "drag-update",
    G_CALLBACK (drag_update),  self);
  g_signal_connect (
    G_OBJECT(self->drag), "drag-end",
    G_CALLBACK (drag_end),  self);
  gtk_widget_add_controller (
    GTK_WIDGET (self),
    GTK_EVENT_CONTROLLER (self->drag));

  GtkEventControllerMotion * motion_controller =
    GTK_EVENT_CONTROLLER_MOTION (
      gtk_event_controller_motion_new ());
  gtk_widget_add_controller (
    GTK_WIDGET (self),
    GTK_EVENT_CONTROLLER (motion_controller));
#endif
}

static void
drag_dest_box_widget_class_init (DragDestBoxWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (klass, "drag-dest-box");
}
