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

#include "audio/channel.h"
#include "audio/engine.h"
#include "plugins/lv2_plugin.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/channel_slot.h"
#include "gui/widgets/main_window.h"
#include "project.h"
#include "utils/ui.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (ChannelSlotWidget, channel_slot_widget, GTK_TYPE_DRAWING_AREA)

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
  ChannelSlotWidget * channel_slot =
    Z_CHANNEL_SLOT_WIDGET (widget);
  Channel * channel = channel_slot->channel;

  PluginDescriptor * descr = *(gpointer *) gtk_selection_data_get_data (data);

  /* stop engine */
  int prev_run =
    g_atomic_int_get (&AUDIO_ENGINE->run);
  g_atomic_int_set (&AUDIO_ENGINE->run, 0);
  g_usleep (5000);

  Plugin * plugin = plugin_create_from_descr (descr);

  if (plugin_instantiate (plugin) < 0)
    {
      GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT;
      GtkWidget * dialog =
        gtk_message_dialog_new (
          GTK_WINDOW (MAIN_WINDOW),
          flags,
          GTK_MESSAGE_ERROR,
          GTK_BUTTONS_CLOSE,
          "Error instantiating plugin “%s”. Please see log for details.",
          plugin->descr->name);
      gtk_dialog_run (GTK_DIALOG (dialog));
      gtk_widget_destroy (dialog);
      plugin_free (plugin);
      return;
    }

  /* add to specific channel */
  zix_sem_wait (&AUDIO_ENGINE->port_operation_lock);
  channel_add_plugin (
    channel, channel_slot->slot_index, plugin);
  zix_sem_post (&AUDIO_ENGINE->port_operation_lock);

  g_atomic_int_set (&AUDIO_ENGINE->run, prev_run);

  if (g_settings_get_int (
        S_PREFERENCES,
        "open-plugin-uis-on-instantiate"))
    {
      plugin->visible = 1;
      EVENTS_PUSH (ET_PLUGIN_VISIBILITY_CHANGED,
                   plugin);
    }
  gtk_widget_queue_draw (widget);
}

static int
draw_cb (GtkWidget * widget, cairo_t * cr, void* data)
{
  guint width, height;
  GtkStyleContext *context;
  ChannelSlotWidget * self = (ChannelSlotWidget *) widget;
  context = gtk_widget_get_style_context (widget);

  width = gtk_widget_get_allocated_width (widget);
  height = gtk_widget_get_allocated_height (widget);

  gtk_render_background (
    context, cr, 0, 0, width, height);


  int padding = 2;
  cairo_text_extents_t te;
  Plugin * plugin = self->channel->plugins[self->slot_index];
  if (plugin)
    {
      GdkRGBA c;
      if (!plugin->enabled)
        /* matcha */
        gdk_rgba_parse (&c,
                        "#2eb398");
      if (plugin->visible)
        /* bright green */
        gdk_rgba_parse (&c,
                        "#1DDD6A");
      else
        /* darkish green */
        gdk_rgba_parse (&c,
                        "#1A884c");

      /* fill background */
      cairo_set_source_rgba (
        cr, c.red, c.green, c.blue, 1.0);
      cairo_move_to (cr, padding, padding);
      cairo_line_to (cr, padding, height - padding);
      cairo_line_to (cr, width - padding, height - padding);
      cairo_line_to (cr, width - padding, padding);
      cairo_fill(cr);

      /* fill text */
      cairo_set_source_rgba (cr, 0.0, 0.0, 0.0, 1.0);
      cairo_select_font_face (cr, "Arial",
                              CAIRO_FONT_SLANT_NORMAL,
                              CAIRO_FONT_WEIGHT_BOLD);
      cairo_set_font_size (cr, 12.0);
      cairo_text_extents (cr, plugin->descr->name, &te);
      cairo_move_to (cr, 20,
                     te.height / 2 - te.y_bearing);
      cairo_show_text (cr, plugin->descr->name);
    }
  else
    {
      /* fill background */
      cairo_set_source_rgba (cr, 0.1, 0.1, 0.1, 1.0);
      cairo_move_to (cr, padding, padding);
      cairo_line_to (cr, padding, height - padding);
      cairo_line_to (cr, width - padding, height - padding);
      cairo_line_to (cr, width - padding, padding);
      cairo_fill(cr);

      /* fill text */
      const char * txt = "empty slot";
      cairo_set_source_rgba (cr, 0.3, 0.3, 0.3, 1.0);
      cairo_select_font_face (cr, "Arial",
                              CAIRO_FONT_SLANT_ITALIC,
                              CAIRO_FONT_WEIGHT_NORMAL);
      cairo_set_font_size (cr, 12.0);
      cairo_text_extents (cr, txt, &te);
      cairo_move_to (cr, 20,
                     te.height / 2 - te.y_bearing);
      cairo_show_text (cr, txt);

    }

  //highlight if grabbed or if mouse is hovering over me
  /*if (self->hover)*/
    /*{*/
      /*cairo_set_source_rgba (cr, 0.8, 0.8, 0.8, 0.12 );*/
      /*cairo_new_sub_path (cr);*/
      /*cairo_arc (cr, x + width - radius, y + radius, radius, -90 * degrees, 0 * degrees);*/
      /*cairo_arc (cr, x + width - radius, y + height - radius, radius, 0 * degrees, 90 * degrees);*/
      /*cairo_arc (cr, x + radius, y + height - radius, radius, 90 * degrees, 180 * degrees);*/
      /*cairo_arc (cr, x + radius, y + radius, radius, 180 * degrees, 270 * degrees);*/
      /*cairo_close_path (cr);*/
      /*cairo_fill (cr);*/
    /*}*/
  return FALSE;
}

static gboolean
button_press_cb (GtkWidget * widget,
		 GdkEventButton * event,
		 gpointer data)
{
  if (event->type == GDK_2BUTTON_PRESS)
    {
      ChannelSlotWidget * self = (ChannelSlotWidget *) data;
      Plugin * plugin = self->channel->plugins[self->slot_index];
      if (plugin)
	{
	  if (plugin->descr->protocol == PROT_LV2)
	    {
	      plugin->visible = !plugin->visible;
	      EVENTS_PUSH (ET_PLUGIN_VISIBILITY_CHANGED,
	                   plugin);
	    }
	  else
	    {
	      plugin_open_ui (plugin);

	    }
	}
    }
  return FALSE;
}

static void
on_drag_data_get (GtkWidget        *widget,
               GdkDragContext   *context,
               GtkSelectionData *data,
               guint             info,
               guint             time,
               gpointer          user_data)
{
  g_message ("aaaaaaaaa");
}

static gboolean
on_motion (
  GtkWidget * widget,
  GdkEvent *event,
  ChannelSlotWidget * self)
{
  if (gdk_event_get_event_type (event) ==
        GDK_ENTER_NOTIFY)
    {
      gtk_widget_set_state_flags (
        GTK_WIDGET (self),
        GTK_STATE_FLAG_PRELIGHT, 0);
      bot_bar_change_status (
        _("Channel Slot - Double click to open "
          "plugin - Click and drag to move plugin"));
    }
  else if (gdk_event_get_event_type (event) ==
             GDK_LEAVE_NOTIFY)
    {
      gtk_widget_unset_state_flags (
        GTK_WIDGET (self),
        GTK_STATE_FLAG_PRELIGHT);
      bot_bar_change_status ("");
    }

  return FALSE;
}

/**
 * Creates a new ChannelSlot widget and binds it to the given value.
 */
ChannelSlotWidget *
channel_slot_widget_new (int slot_index,
                         ChannelWidget * cw)
{
  ChannelSlotWidget * self = g_object_new (CHANNEL_SLOT_WIDGET_TYPE, NULL);
  self->slot_index = slot_index;
  self->channel = cw->channel;

  gtk_widget_set_size_request (GTK_WIDGET (self), -1, 24);

  /* connect signals */
  g_signal_connect (G_OBJECT (self), "draw",
                    G_CALLBACK (draw_cb), self);
  g_signal_connect (GTK_WIDGET (self),
                    "drag-data-received",
                    G_CALLBACK(on_drag_data_received), NULL);
  g_signal_connect (G_OBJECT(self), "button_press_event",
                    G_CALLBACK (button_press_cb),  self);
  g_signal_connect (GTK_WIDGET (cw->slot_boxes[slot_index]),
                    "drag-data-get",
                    G_CALLBACK (on_drag_data_get),
                    self);

  GtkTargetEntry entries[2];
  entries[0].target = TARGET_ENTRY_PLUGIN;
  entries[0].flags = GTK_TARGET_SAME_APP;
  entries[0].info = TARGET_ENTRY_ID_PLUGIN;
  entries[1].target = TARGET_ENTRY_PLUGIN_DESCR;
  entries[1].flags = GTK_TARGET_SAME_APP;
  entries[1].info = TARGET_ENTRY_ID_PLUGIN_DESCR;
  gtk_drag_source_set (
    GTK_WIDGET (cw->slot_boxes[slot_index]),
    GDK_MODIFIER_MASK,
    entries,
    1,
    GDK_ACTION_COPY);
  gtk_drag_dest_set (
    GTK_WIDGET (self),
    GTK_DEST_DEFAULT_ALL,
    entries,
    2,
    GDK_ACTION_COPY);

  return self;
}

static void
channel_slot_widget_init (ChannelSlotWidget * self)
{
  /* make it able to notify */
  gtk_widget_add_events (
    GTK_WIDGET (self),
    GDK_BUTTON_PRESS_MASK |
    GDK_ENTER_NOTIFY_MASK |
    GDK_LEAVE_NOTIFY_MASK);

  g_signal_connect (
    G_OBJECT (self), "enter-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT(self), "leave-notify-event",
    G_CALLBACK (on_motion),  self);
}

static void
channel_slot_widget_class_init (
  ChannelSlotWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (
    klass, "channel-slot");
}
