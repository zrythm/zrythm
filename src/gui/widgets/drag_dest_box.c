/*
 * gui/widgets/drag_dest_box.c - A dnd destination box used by mixer and tracklist
 *                                widgets
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

#include "audio/channel.h"
#include "audio/mixer.h"
#include "audio/track.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/drag_dest_box.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/mixer.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"
#include "utils/gtk.h"
#include "utils/ui.h"

G_DEFINE_TYPE (DragDestBoxWidget, drag_dest_box_widget, GTK_TYPE_BOX)

static int counter = 0;

static void
on_drag_data_received (GtkWidget        *widget,
               GdkDragContext   *context,
               gint              x,
               gint              y,
               GtkSelectionData *data,
               guint             info,
               guint             time,
               gpointer          user_data)
{
  DragDestBoxWidget * self = DRAG_DEST_BOX_WIDGET (widget);
  if (self->type == DRAG_DEST_BOX_TYPE_MIXER ||
      self->type == DRAG_DEST_BOX_TYPE_TRACKLIST)
    {
      Plugin_Descriptor * descr = *(gpointer *) gtk_selection_data_get_data (data);
      Plugin * plugin = plugin_create_from_descr (descr);

      if (plugin_instantiate (plugin) < 0)
        {
          char * message = g_strdup_printf ("Error instantiating plugin “%s”. Please see log for details.",
                                            plugin->descr->name);

          ui_show_error_message (GTK_WINDOW (MAIN_WINDOW), message);
          g_free (message);
          plugin_free (plugin);
          return;
        }

      Channel * new_channel = channel_create (CT_MIDI,
                                              g_strdup_printf ("%s %d",
                                                               descr->name,
                                                               counter++));
      mixer_add_channel_and_init_track (new_channel);
      channel_add_plugin (new_channel,
                          0,
                          plugin);

      /* create channel widget */
      mixer_widget_add_channel (MAIN_WINDOW->mixer, new_channel);

      /* create track widget */
      tracklist_widget_add_track (MAIN_WINDOW->tracklist,
                                  new_channel->track,
                                  MAIN_WINDOW->tracklist->num_visible);

      track_widget_show (new_channel->track->widget);
      channel_widget_show (new_channel->widget);
    }
}

/**
 * Creates a drag destination box widget.
 */
DragDestBoxWidget *
drag_dest_box_widget_new (GtkOrientation  orientation,
                          int             spacing,
                          DragDestBoxType type)
{
  /* create */
  DragDestBoxWidget * self = g_object_new (
                            DRAG_DEST_BOX_WIDGET_TYPE,
                            "orientation",
                            orientation,
                            "spacing",
                            spacing,
                            NULL);
  self->type = type;

  /* set as drag dest */
  gtk_drag_dest_set (GTK_WIDGET (self),
                            GTK_DEST_DEFAULT_ALL,
                            WIDGET_MANAGER->entries,
                            WIDGET_MANAGER->num_entries,
                            GDK_ACTION_COPY);

  /* connect signal */
  g_signal_connect (GTK_WIDGET (self),
                    "drag-data-received",
                    G_CALLBACK(on_drag_data_received), NULL);

  return self;
}

/**
 * GTK boilerplate.
 */
static void
drag_dest_box_widget_init (DragDestBoxWidget * self)
{
}
static void
drag_dest_box_widget_class_init (DragDestBoxWidgetClass * klass)
{
}


