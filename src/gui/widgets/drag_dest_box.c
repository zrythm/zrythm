/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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

/** \file
 */

#include "actions/copy_plugins_action.h"
#include "actions/copy_tracks_action.h"
#include "actions/create_tracks_action.h"
#include "actions/move_plugins_action.h"
#include "actions/move_tracks_action.h"
#include "audio/channel.h"
#include "audio/modulator.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "gui/accel.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/drag_dest_box.h"
#include "gui/widgets/file_browser.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/right_dock_edge.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/flags.h"
#include "utils/resources.h"
#include "utils/string.h"
#include "utils/symap.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (DragDestBoxWidget,
               drag_dest_box_widget,
               GTK_TYPE_EVENT_BOX)

static void
on_drag_leave (
  GtkWidget      *widget,
  GdkDragContext *context,
  guint           time,
  DragDestBoxWidget * self)
{
  GdkAtom target =
    gtk_drag_dest_find_target (
      widget, context, NULL);

  if (target ==
        GET_ATOM (TARGET_ENTRY_TRACK))
    {
      /* unhighlight bottom part of last track */
      Track * track =
        tracklist_get_last_track (
          TRACKLIST, 0, 1);
      track_widget_do_highlight (
        track->widget, 0, 0, 0);
    }
}

static gboolean
on_drag_motion (
  GtkWidget        *widget,
  GdkDragContext   *context,
  gint              x,
  gint              y,
  guint             time,
  DragDestBoxWidget * self)
{
  GdkAtom target =
    gtk_drag_dest_find_target (
      widget, context, NULL);

  if (target == GDK_NONE)
    {
      gtk_drag_unhighlight (widget);
      gdk_drag_status (
        context, 0, time);
      return TRUE;
    }

  if (target == GET_ATOM (TARGET_ENTRY_URI_LIST))
    {
      /* defer to drag_data_received */
      /*self->defer_drag_motion_status = 1;*/

      /*gtk_drag_get_data (*/
        /*widget, context, target, time);*/

      return TRUE;
    }
  else if (target == GET_ATOM (TARGET_ENTRY_SUPPORTED_FILE))
    {
      return TRUE;
    }
  else if (target ==
           GET_ATOM (TARGET_ENTRY_PLUGIN_DESCR))
    {
      gtk_drag_highlight (widget);
      gdk_drag_status (context,
                       GDK_ACTION_COPY,
                       time);
      return TRUE;
    }
  else if (target ==
           GET_ATOM (TARGET_ENTRY_PLUGIN))
    {
      GdkModifierType mask;
      z_gtk_widget_get_mask (
        widget, &mask);
      if (mask & GDK_CONTROL_MASK)
        gdk_drag_status (
          context, GDK_ACTION_COPY, time);
      else
        gdk_drag_status (
          context, GDK_ACTION_MOVE, time);
      return TRUE;
    }
  else if (target ==
             GET_ATOM (TARGET_ENTRY_TRACK))
    {
      GdkModifierType mask;
      z_gtk_widget_get_mask (
        widget, &mask);
      if (mask & GDK_CONTROL_MASK)
        gdk_drag_status (
          context, GDK_ACTION_COPY, time);
      else
        gdk_drag_status (
          context, GDK_ACTION_MOVE, time);
      gtk_drag_unhighlight (widget);

      /* highlight bottom part of last track */
      Track * track =
        tracklist_get_last_track (
          TRACKLIST, 0, 1);
      int track_height =
        gtk_widget_get_allocated_height (
          GTK_WIDGET (track->widget));
      track_widget_do_highlight (
        track->widget, 0,
        track_height - 1, 1);
    }

  return FALSE;
}

static void
on_drag_data_received (
  GtkWidget        *widget,
  GdkDragContext   *context,
  gint              x,
  gint              y,
  GtkSelectionData *data,
  guint             info,
  guint             time,
  DragDestBoxWidget * self)
{
  GdkAtom target =
    gtk_selection_data_get_target (data);

  if (target == GDK_NONE)
    return;

#define IS_URI_LIST \
  (target == GET_ATOM (TARGET_ENTRY_URI_LIST))
#define IS_SUPPORTED_FILE \
  (target == GET_ATOM (TARGET_ENTRY_SUPPORTED_FILE))

  if (IS_URI_LIST || IS_SUPPORTED_FILE)
    {
      SupportedFile * file = NULL;
      if (IS_SUPPORTED_FILE)
        {
          const guchar *my_data =
            gtk_selection_data_get_data (data);
          memcpy (&file, my_data, sizeof (file));
        }
      else
        {
          char * filepath = NULL;
          char ** uris =
            gtk_selection_data_get_uris (data);
          if (uris)
            {
              char * uri;
              int i = 0;
              while ((uri = uris[i++]) != NULL)
                {
                  /* strip "file://" */
                  if (!string_contains_substr (
                        uri, "file://", 0))
                    continue;

                  if (filepath)
                    g_free (filepath);
                  GError * err = NULL;
                  filepath =
                    g_filename_from_uri (
                      uri, NULL, &err);
                  if (err)
                    {
                      g_warning (
                        "%s", err->message);
                    }

                  /* only accept 1 file for now */
                  break;
                }
              g_strfreev (uris);
            }

          if (filepath)
            {
              file =
                supported_file_new_from_path (
                  filepath);
              g_free (filepath);
            }
        }

      if (file)
        {
          TrackType track_type = 0;
          if (supported_file_type_is_supported (
                file->type) &&
              supported_file_type_is_audio (
                file->type))
            {
              track_type = TRACK_TYPE_AUDIO;
            }
          else if (supported_file_type_is_midi (
                     file->type))
            {
              track_type = TRACK_TYPE_MIDI;
            }
          else
            {
              char * descr =
                supported_file_type_get_description (
                  file->type);
              char * msg =
                g_strdup_printf (
                  _("Unsupported file type %s"),
                  descr);
              g_free (descr);
              if (IS_URI_LIST)
                {
                  supported_file_free (file);
                }
              ui_show_error_message (
                MAIN_WINDOW, msg);
              return;
            }

          UndoableAction * ua =
            create_tracks_action_new (
              track_type, NULL, file,
              TRACKLIST->num_tracks,
              PLAYHEAD, 1);
          if (IS_URI_LIST)
            {
              supported_file_free (file);
            }

          undo_manager_perform (UNDO_MANAGER, ua);
          return;
        }
      else
        {
          ui_show_error_message (
            MAIN_WINDOW,
            _("No file found"));
          return;
        }
    }
  else if (target ==
            GET_ATOM (TARGET_ENTRY_PLUGIN_DESCR))
    {
      PluginDescriptor * pd = NULL;
      const guchar *my_data =
        gtk_selection_data_get_data (data);
      memcpy (&pd, my_data, sizeof (pd));

      if (self->type ==
          DRAG_DEST_BOX_TYPE_MIXER ||
          self->type ==
          DRAG_DEST_BOX_TYPE_TRACKLIST)
        {
          TrackType tt =
            track_get_type_from_plugin_descriptor (
              pd);

          UndoableAction * ua =
            create_tracks_action_new (
              tt, pd, NULL,
              TRACKLIST->num_tracks,
              PLAYHEAD, 1);

          undo_manager_perform (UNDO_MANAGER, ua);
        }
      else
        {
          Track * track =
            TRACKLIST_SELECTIONS->tracks[0];
          Modulator * modulator =
            modulator_new (pd, track);
          track_add_modulator (
            track, modulator);
        }
    }
  else if (target ==
            GET_ATOM (TARGET_ENTRY_PLUGIN))
    {
      /* NOTE this is a cloned pointer, don't use
       * it */
      Plugin * received_pl = NULL;
      const guchar *my_data =
        gtk_selection_data_get_data (data);
      memcpy (
        &received_pl, my_data, sizeof (received_pl));
      Plugin * pl =
        plugin_find (&received_pl->id);
      g_warn_if_fail (pl);

      /* determine if moving or copying */
      GdkDragAction action =
        gdk_drag_context_get_selected_action (
          context);

      UndoableAction * ua = NULL;
      if (action == GDK_ACTION_COPY)
        {
          ua =
            copy_plugins_action_new (
              MIXER_SELECTIONS,
              PLUGIN_SLOT_INSERT,
              NULL, 0);
        }
      else if (action == GDK_ACTION_MOVE)
        {
          ua =
            move_plugins_action_new (
              MIXER_SELECTIONS,
              PLUGIN_SLOT_INSERT,
              NULL, 0);
        }
      g_warn_if_fail (ua);

      undo_manager_perform (
        UNDO_MANAGER, ua);
    }
  else if (target ==
            GET_ATOM (TARGET_ENTRY_TRACK))
    {
      int pos =
        tracklist_get_last_pos (
          TRACKLIST, 0, 1);

      /* determine if moving or copying */
      GdkDragAction action =
        gdk_drag_context_get_selected_action (
          context);

      UndoableAction * ua = NULL;
      if (action == GDK_ACTION_COPY)
        {
          ua =
            copy_tracks_action_new (
              TRACKLIST_SELECTIONS, pos);
        }
      else if (action == GDK_ACTION_MOVE)
        {
          ua =
            move_tracks_action_new (
              TRACKLIST_SELECTIONS, pos);
        }

      g_warn_if_fail (ua);

      undo_manager_perform (
        UNDO_MANAGER, ua);
    }

#undef IS_SUPPORTED_FILE
#undef IS_URI_LIST
}

static void
drag_begin (GtkGestureDrag * gesture,
               gdouble         start_x,
               gdouble         start_y,
               gpointer        user_data)
{
  g_message ("drag");

}

static void
drag_update (GtkGestureDrag * gesture,
               gdouble         offset_x,
               gdouble         offset_y,
               gpointer        user_data)
{

}

static void
drag_end (GtkGestureDrag *gesture,
               gdouble         offset_x,
               gdouble         offset_y,
               gpointer        user_data)
{


}

static gboolean
on_motion (GtkWidget * widget,
           GdkEvent *event,
           DragDestBoxWidget * self)
{
  if (gdk_event_get_event_type (event) ==
      GDK_ENTER_NOTIFY)
    {
      if (self->type ==
            DRAG_DEST_BOX_TYPE_MIXER)
        {
        }
      else if (self->type ==
                 DRAG_DEST_BOX_TYPE_TRACKLIST)
        {
        }
    }
  else if (gdk_event_get_event_type (event) ==
           GDK_LEAVE_NOTIFY)
    {
    }

  return FALSE;
}

static void
show_context_menu (DragDestBoxWidget * self)
{
  GtkWidget *menu;
  GtkWidget * submenu;
  GtkMenuItem * menu_item;
  menu = gtk_menu_new ();

  menu_item =
    z_gtk_create_menu_item (
      _("Add _MIDI Track"),
      NULL,
      ICON_TYPE_GNOME_BUILDER,
      NULL,
      0,
      "win.create-midi-track");
  gtk_menu_shell_append (
    GTK_MENU_SHELL(menu),
    GTK_WIDGET (menu_item));

  menu_item =
    z_gtk_create_menu_item (
      _("Add Audio Track"),
      NULL,
      ICON_TYPE_GNOME_BUILDER,
      NULL,
      0,
      "win.create-audio-track");
  gtk_menu_shell_append (
    GTK_MENU_SHELL(menu),
    GTK_WIDGET (menu_item));

  submenu = gtk_menu_new ();
  menu_item =
    z_gtk_create_menu_item (
      _(track_type_to_string (
          TRACK_TYPE_AUDIO_BUS)),
      NULL, ICON_TYPE_GNOME_BUILDER, NULL, 0,
      "win.create-audio-bus-track");
  gtk_menu_shell_append (
    GTK_MENU_SHELL (submenu),
    GTK_WIDGET (menu_item));
  menu_item =
    z_gtk_create_menu_item (
      _(track_type_to_string (
          TRACK_TYPE_MIDI_BUS)),
      NULL, ICON_TYPE_GNOME_BUILDER, NULL, 0,
      "win.create-midi-bus-track");
  gtk_menu_shell_append (
    GTK_MENU_SHELL (submenu),
    GTK_WIDGET (menu_item));
  menu_item =
    GTK_MENU_ITEM (
      gtk_menu_item_new_with_label (
        _("Add FX Track")));
  gtk_menu_item_set_submenu (
    GTK_MENU_ITEM (menu_item),
    submenu);
  gtk_menu_shell_append (
    GTK_MENU_SHELL (menu),
    GTK_WIDGET (menu_item));

  submenu = gtk_menu_new ();
  menu_item =
    z_gtk_create_menu_item (
      _(track_type_to_string (
          TRACK_TYPE_AUDIO_GROUP)),
      NULL, ICON_TYPE_GNOME_BUILDER, NULL, 0,
      "win.create-audio-group-track");
  gtk_menu_shell_append (
    GTK_MENU_SHELL (submenu),
    GTK_WIDGET (menu_item));
  menu_item =
    z_gtk_create_menu_item (
      _(track_type_to_string (
          TRACK_TYPE_MIDI_GROUP)),
      NULL, ICON_TYPE_GNOME_BUILDER, NULL, 0,
      "win.create-midi-group-track");
  gtk_menu_shell_append (
    GTK_MENU_SHELL (submenu),
    GTK_WIDGET (menu_item));
  menu_item =
    GTK_MENU_ITEM (
      gtk_menu_item_new_with_label (
        _("Add Group Track")));
  gtk_menu_item_set_submenu (
    GTK_MENU_ITEM (menu_item),
    submenu);
  gtk_menu_shell_append (
    GTK_MENU_SHELL (menu),
    GTK_WIDGET (menu_item));

  gtk_widget_show_all(menu);
  gtk_menu_attach_to_widget (GTK_MENU (menu),
                             GTK_WIDGET (self),
                             NULL);
  gtk_menu_popup_at_pointer (GTK_MENU(menu), NULL);
}

static void
on_right_click (GtkGestureMultiPress *gesture,
               gint                  n_press,
               gdouble               x,
               gdouble               y,
               gpointer              user_data)
{
  DragDestBoxWidget * self =
    Z_DRAG_DEST_BOX_WIDGET (user_data);

  if (n_press == 1)
    {
      show_context_menu (self);
    }
}

static void
multipress_pressed (
  GtkGestureMultiPress * gesture,
  gint                   n_press,
  gdouble                x,
  gdouble                y,
  DragDestBoxWidget *    self)
{
  GdkModifierType state_mask;
  ui_get_modifier_type_from_gesture (
    GTK_GESTURE_SINGLE (gesture),
    &state_mask);

  mixer_selections_clear (
    MIXER_SELECTIONS,
    F_PUBLISH_EVENTS);
  tracklist_selections_select_last_visible (
    TRACKLIST_SELECTIONS);
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
  DragDestBoxWidget * self =
    g_object_new (DRAG_DEST_BOX_WIDGET_TYPE,
                  NULL);

  self->type = type;

  if (type == DRAG_DEST_BOX_TYPE_MIXER)
    {
      gtk_widget_set_size_request (
        GTK_WIDGET (self),
        20,
        -1);
    }
  else if (type == DRAG_DEST_BOX_TYPE_TRACKLIST)
    {
      gtk_widget_set_size_request (
        GTK_WIDGET (self),
        -1,
        160);
    }

  /* make expandable */
  gtk_widget_set_vexpand (
    GTK_WIDGET (self), 1);
  gtk_widget_set_hexpand (
    GTK_WIDGET (self), 1);

  /* set as drag dest */
  char * entry_track =
    g_strdup (TARGET_ENTRY_TRACK);
  char * entry_plugin_descr =
    g_strdup (TARGET_ENTRY_PLUGIN_DESCR);
  char * entry_uri_list =
    g_strdup (TARGET_ENTRY_URI_LIST);
  char * entry_supported_file =
    g_strdup (TARGET_ENTRY_SUPPORTED_FILE);
  char * entry_plugin =
    g_strdup (TARGET_ENTRY_PLUGIN);
  GtkTargetEntry entries[] = {
    {
      entry_plugin_descr, GTK_TARGET_SAME_APP,
      symap_map (
        ZSYMAP, TARGET_ENTRY_PLUGIN_DESCR),
    },
    {
      entry_uri_list, GTK_TARGET_SAME_APP,
      symap_map (ZSYMAP, TARGET_ENTRY_URI_LIST),
    },
    {
      entry_uri_list, GTK_TARGET_OTHER_APP,
      symap_map (ZSYMAP, TARGET_ENTRY_URI_LIST),
    },
    {
      entry_supported_file, GTK_TARGET_SAME_APP,
      symap_map (ZSYMAP, TARGET_ENTRY_SUPPORTED_FILE),
    },
    {
      entry_plugin, GTK_TARGET_SAME_APP,
      symap_map (ZSYMAP, TARGET_ENTRY_PLUGIN),
    },
    {
      entry_track, GTK_TARGET_SAME_APP,
      symap_map (ZSYMAP, TARGET_ENTRY_TRACK),
    }
  };
  /* disable for now */
  (void) entries;
#if 0
  gtk_drag_dest_set (
    GTK_WIDGET (self), GTK_DEST_DEFAULT_ALL,
    entries, G_N_ELEMENTS (entries), GDK_ACTION_COPY);
#endif
  g_free (entry_track);
  g_free (entry_plugin);
  g_free (entry_plugin_descr);
  g_free (entry_uri_list);

  /* connect signal */
  g_signal_connect (
    GTK_WIDGET (self), "drag-motion",
    G_CALLBACK(on_drag_motion), self);
  g_signal_connect (
    GTK_WIDGET (self), "drag-data-received",
    G_CALLBACK(on_drag_data_received), self);

  /* show */
  gtk_widget_set_visible (GTK_WIDGET (self),
                          1);

  return self;
}

/**
 * GTK boilerplate.
 */
static void
drag_dest_box_widget_init (DragDestBoxWidget * self)
{
  self->multipress =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (
        GTK_WIDGET (self)));
  self->right_mouse_mp =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (
        GTK_WIDGET (self)));
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (self->right_mouse_mp),
    GDK_BUTTON_SECONDARY);
  self->drag =
    GTK_GESTURE_DRAG (
      gtk_gesture_drag_new (GTK_WIDGET (self)));

  /* make widget able to notify */
  gtk_widget_add_events (
    GTK_WIDGET (self),
    GDK_ALL_EVENTS_MASK);

  /* connect signals */
  g_signal_connect (
    G_OBJECT (self->multipress), "pressed",
    G_CALLBACK (multipress_pressed), self);
  g_signal_connect (
    G_OBJECT (self->right_mouse_mp), "pressed",
    G_CALLBACK (on_right_click), self);
  g_signal_connect (
    G_OBJECT(self->drag), "drag-begin",
    G_CALLBACK (drag_begin),  self);
  g_signal_connect (
    G_OBJECT(self->drag), "drag-update",
    G_CALLBACK (drag_update),  self);
  g_signal_connect (
    G_OBJECT(self->drag), "drag-end",
    G_CALLBACK (drag_end),  self);
  g_signal_connect (
    G_OBJECT (self), "enter-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT(self), "leave-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    GTK_WIDGET (self), "drag-leave",
    G_CALLBACK (on_drag_leave), self);
}

static void
drag_dest_box_widget_class_init (
  DragDestBoxWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (klass,
                                 "drag-dest-box");
}
