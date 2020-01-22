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

/**
 * \file
 */

#include "actions/copy_plugins_action.h"
#include "actions/create_plugins_action.h"
#include "actions/delete_plugins_action.h"
#include "actions/move_plugins_action.h"
#include "actions/undoable_action.h"
#include "actions/undo_manager.h"
#include "audio/channel.h"
#include "audio/engine.h"
#include "plugins/lv2_plugin.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/channel_slot.h"
#include "gui/widgets/main_window.h"
#include "project.h"
#include "utils/cairo.h"
#include "utils/gtk.h"
#include "utils/flags.h"
#include "utils/ui.h"
#include "utils/resources.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  ChannelSlotWidget, channel_slot_widget,
  GTK_TYPE_DRAWING_AREA)

#define ELLIPSIZE_PADDING 2

static int
channel_slot_draw_cb (
  GtkWidget * widget,
  cairo_t * cr,
  ChannelSlotWidget * self)
{
  GtkStyleContext *context =
  gtk_widget_get_style_context (widget);

  int width =
    gtk_widget_get_allocated_width (widget);
  int height =
    gtk_widget_get_allocated_height (widget);

  gtk_render_background (
    context, cr, 0, 0, width, height);

  int padding = 2;
  Plugin * plugin =
    self->channel->plugins[self->slot_index];
  if (plugin)
    {
      GdkRGBA bg, fg;
      fg = UI_COLOR_BLACK;
      if (!plugin->enabled)
        bg = UI_COLORS->matcha;
      if (plugin->visible)
        bg = UI_COLORS->bright_green;
      else
        bg = UI_COLORS->darkish_green;

      /* fill background */
      cairo_set_source_rgba (
        cr, bg.red, bg.green, bg.blue, 1.0);
      cairo_move_to (cr, padding, padding);
      cairo_line_to (cr, padding, height - padding);
      cairo_line_to (cr, width - padding, height - padding);
      cairo_line_to (cr, width - padding, padding);
      cairo_fill(cr);

      /* fill text */
      cairo_set_source_rgba (
        cr, fg.red, fg.green, fg.blue, 1.0);
      int w, h;
      z_cairo_get_text_extents_for_widget (
        widget, self->pl_name_layout,
        plugin->descr->name, &w, &h);
      z_cairo_draw_text_full (
        cr, widget, self->pl_name_layout,
        plugin->descr->name,
        width / 2 - w / 2,
        height / 2 - h / 2);

      /* update tooltip */
      if (!self->pl_name ||
          !g_strcmp0 (
            plugin->descr->name, self->pl_name))
        {
          if (self->pl_name)
            g_free (self->pl_name);
          self->pl_name =
            g_strdup (plugin->descr->name);
          gtk_widget_set_tooltip_text (
            widget, self->pl_name);
        }
    }
  else
    {
      /* fill background */
      cairo_set_source_rgba (
        cr, 0.1, 0.1, 0.1, 1.0);
      cairo_move_to (cr, padding, padding);
      cairo_line_to (cr, padding, height - padding);
      cairo_line_to (
        cr, width - padding, height - padding);
      cairo_line_to (cr, width - padding, padding);
      cairo_fill(cr);

      /* fill text */
      cairo_set_source_rgba (
        cr, 0.3, 0.3, 0.3, 1.0);
      int w, h;
      char * text = _("empty slot");
      z_cairo_get_text_extents_for_widget (
        widget, self->empty_slot_layout, text,
        &w, &h);
      z_cairo_draw_text_full (
        cr, widget, self->empty_slot_layout,
        text, width / 2 - w / 2,
        height / 2 - h / 2);

      /* update tooltip */
      if (self->pl_name)
        {
          g_free (self->pl_name);
          self->pl_name = NULL;
          gtk_widget_set_tooltip_text (
            widget, _("empty slot"));
        }
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

static void
on_drag_data_received (
  GtkWidget        *widget,
  GdkDragContext   *context,
  gint              x,
  gint              y,
  GtkSelectionData *data,
  guint             info,
  guint             time,
  ChannelSlotWidget * self)
{
  g_message ("drag data received");

  GdkAtom atom =
    gtk_selection_data_get_target (data);
  GdkAtom plugin_atom =
    gdk_atom_intern_static_string (
      TARGET_ENTRY_PLUGIN);
  GdkAtom plugin_descr_atom =
    gdk_atom_intern_static_string (
      TARGET_ENTRY_PLUGIN_DESCR);

  Plugin * pl;
  if (atom == plugin_atom)
    {
      Plugin * received_pl = NULL;
      const guchar *my_data =
        gtk_selection_data_get_data (data);
      memcpy (
        &received_pl, my_data, sizeof (received_pl));
      pl =
        TRACKLIST->tracks[received_pl->track_pos]->
        channel->plugins[received_pl->slot];
      g_warn_if_fail (pl);
      g_warn_if_fail (pl->track);

      /* if plugin not at original position */
      if (self->channel != pl->track->channel ||
          self->slot_index !=
            channel_get_plugin_index (
              pl->track->channel, pl))
        {
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
                  self->channel->track,
                  self->slot_index);
            }
          else if (action == GDK_ACTION_MOVE)
            {
              ua =
                move_plugins_action_new (
                  MIXER_SELECTIONS,
                  self->channel->track,
                  self->slot_index);
            }
          g_warn_if_fail (ua);

          undo_manager_perform (
            UNDO_MANAGER, ua);
        }
    }
  else if (atom == plugin_descr_atom)
    {
      gdk_drag_status (
        context, GDK_ACTION_COPY, time);
      PluginDescriptor * descr = NULL;
      const guchar *my_data =
        gtk_selection_data_get_data (data);
      memcpy (&descr, my_data, sizeof (descr));
      g_warn_if_fail (descr);

      UndoableAction * ua =
        create_plugins_action_new (
          descr, self->channel->track->pos,
          self->slot_index, 1);

      undo_manager_perform (
        UNDO_MANAGER, ua);
    }

  gtk_widget_queue_draw (widget);
}


/**
 * Control not pressed, no plugin exists,
 * not same channel */
static inline void
select_no_ctrl_no_pl_no_ch (
  ChannelSlotWidget * self)
{
  mixer_selections_clear (
    MIXER_SELECTIONS,
    F_PUBLISH_EVENTS);
}

/**
 * Control not pressed, no plugin exists,
 * same channel */
static inline void
select_no_ctrl_no_pl_ch (
  ChannelSlotWidget * self)
{
  mixer_selections_clear (
    MIXER_SELECTIONS,
    F_PUBLISH_EVENTS);
}

/**
 * Control not pressed, plugin exists,
 * not same channel */
static inline void
select_no_ctrl_pl_no_ch (
  ChannelSlotWidget * self)
{
  mixer_selections_clear (
    MIXER_SELECTIONS,
    F_NO_PUBLISH_EVENTS);

  mixer_selections_add_slot (
    MIXER_SELECTIONS,
    self->channel,
    self->slot_index);
}

/**
 * Control not pressed, plugin exists,
 * same channel */
static inline void
select_no_ctrl_pl_ch (
  ChannelSlotWidget * self)
{
  /* if plugin is not selected, make it hte only
   * selection otherwise do nothing */
  if (!mixer_selections_contains_slot (
        MIXER_SELECTIONS,
        self->slot_index))
    {
      mixer_selections_clear (
        MIXER_SELECTIONS,
        F_NO_PUBLISH_EVENTS);

      mixer_selections_add_slot (
        MIXER_SELECTIONS,
        self->channel,
        self->slot_index);
    }
}

/**
 * Control pressed, no plugin exists, not
 * same channel */
static inline void
select_ctrl_no_pl_no_ch (
  ChannelSlotWidget * self)
{
  mixer_selections_clear (
    MIXER_SELECTIONS,
    F_PUBLISH_EVENTS);
}

/**
 * Control pressed, no plugin exists,
 * same channel */
static inline void
select_ctrl_no_pl_ch (
  ChannelSlotWidget * self)
{
  mixer_selections_clear (
    MIXER_SELECTIONS,
    F_PUBLISH_EVENTS);
}

/**
 * Control pressed, plugin exists,
 * not same channel */
static inline void
select_ctrl_pl_no_ch (
  ChannelSlotWidget * self)
{
  /* make it the only selection */
  mixer_selections_clear (
    MIXER_SELECTIONS,
    F_NO_PUBLISH_EVENTS);
  mixer_selections_add_slot (
    MIXER_SELECTIONS,
    self->channel,
    self->slot_index);
}

/**
 * Control pressed, plugin exists, same channel */
static inline void
select_ctrl_pl_ch (
  ChannelSlotWidget * self)
{
  /* if already selected, deselect it, otherwise
   * add it to selections */
  if (mixer_selections_contains_slot (
        MIXER_SELECTIONS,
        self->slot_index))
    {
      mixer_selections_remove_slot (
        MIXER_SELECTIONS,
        self->slot_index,
        F_PUBLISH_EVENTS);
      /*self->deselected = 1;*/
    }
  else
    {
      mixer_selections_add_slot (
        MIXER_SELECTIONS,
        self->channel,
        self->slot_index);
    }
}

/*static void*/
/*drag_update (GtkGestureDrag * gesture,*/
               /*gdouble         offset_x,*/
               /*gdouble         offset_y,*/
               /*ChannelSlotWidget * self)*/
/*{*/
  /*g_message ("drag update");*/

  /*UI_GET_STATE_MASK (gesture);*/
/*}*/

/**
 * Selects the given plugin (either adds it to
 * selections or makes it the only selection).
 *
 * @param ctrl Whether Control is held down or not.
 */
static void
select_plugin (
  ChannelSlotWidget * self,
  int                 ctrl)
{
  int pl = 0, ch = 0;

  /* if plugin exists */
  if (self->channel->plugins[
        self->slot_index])
    pl = 1;

  /* if same channel as selections */
  if (MIXER_SELECTIONS->track &&
      self->channel ==
        MIXER_SELECTIONS->track->channel)
    ch = 1;

  if (!ctrl && !pl && !ch)
    select_no_ctrl_no_pl_no_ch (self);
  else if (!ctrl && !pl && ch)
    select_no_ctrl_no_pl_ch (self);
  else if (!ctrl && pl && !ch)
    select_no_ctrl_pl_no_ch (self);
  else if (!ctrl && pl && ch)
    select_no_ctrl_pl_ch (self);
  else if (ctrl && !pl && !ch)
    select_ctrl_no_pl_no_ch (self);
  else if (ctrl && !pl && ch)
    select_ctrl_no_pl_ch (self);
  else if (ctrl && pl && !ch)
    select_ctrl_pl_no_ch (self);
  else if (ctrl && pl && ch)
    select_ctrl_pl_ch (self);

  /* select channel */
  tracklist_selections_select_single (
    TRACKLIST_SELECTIONS,
    self->channel->track);
}

static void
drag_end (GtkGestureDrag *gesture,
               gdouble         offset_x,
               gdouble         offset_y,
               ChannelSlotWidget * self)
{
  GdkModifierType state_mask =
    ui_get_state_mask (
      GTK_GESTURE (gesture));

  if (self->n_press == 2)
    {
      Plugin * pl =
        self->channel->plugins[self->slot_index];
      g_message ("opening plugin ");
      if (pl)
        {
          if (pl->descr->protocol == PROT_LV2)
            {
              pl->visible = !pl->visible;
              EVENTS_PUSH (
                ET_PLUGIN_VISIBILITY_CHANGED,
                pl);
            }
          else
            {
              plugin_open_ui (pl);
            }
        }
    }
  else if (self->n_press == 1)
    {
      int ctrl = 0;
      /* if control click */
      if (state_mask & GDK_CONTROL_MASK)
        ctrl = 1;

      select_plugin (self, ctrl);

      /*self->deselected = 0;*/
      /*self->reselected = 0;*/
    }
}

static void
multipress_pressed (
  GtkGestureMultiPress *gesture,
  gint                  n_press,
  gdouble               x,
  gdouble               y,
  ChannelSlotWidget *   self)
{
  self->n_press = n_press;

  PROJECT->last_selection =
    SELECTION_TYPE_PLUGIN;
}

static void
on_plugin_delete (
  GtkMenuItem *       menu_item,
  ChannelSlotWidget * self)
{
  UndoableAction * ua =
    delete_plugins_action_new (
      MIXER_SELECTIONS, self->channel->track,
      self->slot_index);
  undo_manager_perform (UNDO_MANAGER, ua);
  EVENTS_PUSH (ET_PLUGINS_REMOVED, self->channel);
}

static void
show_context_menu (
  ChannelSlotWidget * self)
{
  GtkWidget *menu;
  GtkMenuItem * menuitem;

  menu = gtk_menu_new();

  /*int num_selections =*/
    /*MIXER_SELECTIONS->num_slots;*/

  /* TODO split below */
  /*if (num_selections == 0)*/
    /*{*/

    /*}*/
  /*else if (num_selections = 1)*/
    /*{*/


    /*}*/
  /*else*/
    /*{*/

    /*}*/

#define CREATE_SEPARATOR \
  menuitem = \
    GTK_MENU_ITEM (gtk_separator_menu_item_new ()); \
  gtk_widget_show_all (GTK_WIDGET (menuitem))

#define ADD_TO_SHELL \
  gtk_menu_shell_append ( \
    GTK_MENU_SHELL(menu), GTK_WIDGET (menuitem))

  menuitem = CREATE_CUT_MENU_ITEM;
  ADD_TO_SHELL;
  menuitem = CREATE_COPY_MENU_ITEM;
  ADD_TO_SHELL;
  menuitem = CREATE_PASTE_MENU_ITEM;
  ADD_TO_SHELL;
  /* if plugin exists */
  if (self->channel->plugins[self->slot_index])
    {
      /* add delete item */
      menuitem = CREATE_DELETE_MENU_ITEM;
      g_signal_connect (
        G_OBJECT (menuitem), "activate",
        G_CALLBACK (on_plugin_delete), self);
      ADD_TO_SHELL;
    }
  CREATE_SEPARATOR;
  ADD_TO_SHELL;
  menuitem = CREATE_CLEAR_SELECTION_MENU_ITEM;
  ADD_TO_SHELL;
  menuitem = CREATE_SELECT_ALL_MENU_ITEM;
  ADD_TO_SHELL;

#undef ADD_TO_SHELL

  gtk_widget_show_all(menu);
  gtk_menu_attach_to_widget (
    GTK_MENU (menu),
    GTK_WIDGET (self), NULL);

  gtk_menu_popup_at_pointer (GTK_MENU (menu), NULL);
}

static void
on_right_click (GtkGestureMultiPress *gesture,
               gint                  n_press,
               gdouble               x,
               gdouble               y,
               ChannelSlotWidget *   self)
{
  if (n_press != 1)
    return;

  GdkModifierType mod =
    ui_get_state_mask (GTK_GESTURE (gesture));

  select_plugin (self, mod & GDK_CONTROL_MASK);

  show_context_menu (self);
}

static void
on_drag_data_get (
  GtkWidget        *widget,
  GdkDragContext   *context,
  GtkSelectionData *data,
  guint             info,
  guint             time,
  ChannelSlotWidget * self)
{
  Plugin* pl =
    self->channel->plugins[self->slot_index];

  if (!pl)
    return;

  gtk_selection_data_set (
    data,
    gdk_atom_intern_static_string (
      TARGET_ENTRY_PLUGIN),
    32,
    (const guchar *) &pl,
    sizeof (pl));
}

static void
on_drag_motion (GtkWidget *widget,
             GdkDragContext *context,
             gint x,
             gint y,
             guint time,
             ChannelSlotWidget * self)
{
  GdkModifierType mask;
  z_gtk_widget_get_mask (
    widget, &mask);
  if (mask & GDK_CONTROL_MASK)
    gdk_drag_status (context, GDK_ACTION_COPY, time);
  else
    gdk_drag_status (context, GDK_ACTION_MOVE, time);
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
    }
  else if (gdk_event_get_event_type (event) ==
             GDK_LEAVE_NOTIFY)
    {
      gtk_widget_unset_state_flags (
        GTK_WIDGET (self),
        GTK_STATE_FLAG_PRELIGHT);
    }

  return FALSE;
}

static void
recreate_pango_layouts (
  ChannelSlotWidget * self)
{
  if (PANGO_IS_LAYOUT (self->empty_slot_layout))
    g_object_unref (self->empty_slot_layout);
  if (PANGO_IS_LAYOUT (self->pl_name_layout))
    g_object_unref (self->pl_name_layout);

  self->empty_slot_layout =
    z_cairo_create_pango_layout (
      (GtkWidget *) self, "Arial Italic 8",
      PANGO_ELLIPSIZE_END, ELLIPSIZE_PADDING);
  self->pl_name_layout =
    z_cairo_create_pango_layout (
      (GtkWidget *) self, "Arial Bold 8",
      PANGO_ELLIPSIZE_END, ELLIPSIZE_PADDING);
}

static void
on_size_allocate (
  GtkWidget *          widget,
  GdkRectangle *       allocation,
  ChannelSlotWidget * self)
{
  recreate_pango_layouts (self);
}

static void
on_screen_changed (
  GtkWidget *          widget,
  GdkScreen *          previous_screen,
  ChannelSlotWidget * self)
{
  recreate_pango_layouts (self);
}

static void
finalize (
  ChannelSlotWidget * self)
{
  if (self->pl_name)
    g_free (self->pl_name);
  if (self->drag)
    g_object_unref (self->drag);
  if (self->empty_slot_layout)
    g_object_unref (self->empty_slot_layout);
  if (self->pl_name_layout)
    g_object_unref (self->pl_name_layout);

  G_OBJECT_CLASS (
    channel_slot_widget_parent_class)->
      finalize (G_OBJECT (self));
}

/**
 * Creates a new ChannelSlot widget and binds it to the given value.
 */
ChannelSlotWidget *
channel_slot_widget_new (
  int slot_index,
  ChannelWidget * cw)
{
  ChannelSlotWidget * self =
    g_object_new (
      CHANNEL_SLOT_WIDGET_TYPE, NULL);
  self->slot_index = slot_index;
  self->channel = cw->channel;

  char * entry_plugin =
    g_strdup (TARGET_ENTRY_PLUGIN);
  char * entry_plugin_descr =
    g_strdup (TARGET_ENTRY_PLUGIN_DESCR);
  GtkTargetEntry entries[] = {
    {
      entry_plugin, GTK_TARGET_SAME_APP,
      symap_map (ZSYMAP, TARGET_ENTRY_PLUGIN),
    },
    {
      entry_plugin_descr, GTK_TARGET_SAME_APP,
      symap_map (ZSYMAP, TARGET_ENTRY_PLUGIN_DESCR),
    },
  };

  /* set as drag source for plugin */
  gtk_drag_source_set (
    GTK_WIDGET (self),
    GDK_BUTTON1_MASK,
    entries, 1,
    GDK_ACTION_MOVE | GDK_ACTION_COPY);
  /* set as drag dest for both plugins and
   * plugin descriptors */
  gtk_drag_dest_set (
    GTK_WIDGET (self),
    GTK_DEST_DEFAULT_ALL,
    entries, G_N_ELEMENTS (entries),
    GDK_ACTION_MOVE | GDK_ACTION_COPY);
  g_free (entry_plugin);
  g_free (entry_plugin_descr);

  return self;
}

static void
channel_slot_widget_init (
  ChannelSlotWidget * self)
{
  /* make it able to notify */
  gtk_widget_add_events (
    GTK_WIDGET (self),
    GDK_BUTTON_PRESS_MASK |
    GDK_ENTER_NOTIFY_MASK |
    GDK_LEAVE_NOTIFY_MASK);

  gtk_widget_set_size_request (
    GTK_WIDGET (self), -1, 24);

  self->pl_name = NULL;
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (self), _("empty slot"));

  self->multipress =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (
        GTK_WIDGET (self)));
  gtk_event_controller_set_propagation_phase (
    GTK_EVENT_CONTROLLER (self->multipress),
    GTK_PHASE_CAPTURE);
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
  gtk_event_controller_set_propagation_phase (
    GTK_EVENT_CONTROLLER (self->drag),
    GTK_PHASE_CAPTURE);

  /* connect signals */
  g_signal_connect (
    G_OBJECT (self), "draw",
    G_CALLBACK (channel_slot_draw_cb), self);
  g_signal_connect (
    GTK_WIDGET (self), "drag-data-received",
    G_CALLBACK(on_drag_data_received), self);
  g_signal_connect (
    GTK_WIDGET (self), "drag-data-get",
    G_CALLBACK (on_drag_data_get), self);
  g_signal_connect (
    GTK_WIDGET (self), "drag-motion",
    G_CALLBACK (on_drag_motion), self);
  /*g_signal_connect (*/
    /*G_OBJECT (self->drag), "drag-update",*/
    /*G_CALLBACK (drag_update),  self);*/
  g_signal_connect (
    G_OBJECT (self->drag), "drag-end",
    G_CALLBACK (drag_end),  self);
  g_signal_connect (
    G_OBJECT (self), "enter-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT(self), "leave-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT (self->multipress), "pressed",
    G_CALLBACK (multipress_pressed), self);
  g_signal_connect (
    G_OBJECT (self->right_mouse_mp), "pressed",
    G_CALLBACK (on_right_click), self);
  g_signal_connect (
    G_OBJECT (self), "screen-changed",
    G_CALLBACK (on_screen_changed),  self);
  g_signal_connect (
    G_OBJECT (self), "size-allocate",
    G_CALLBACK (on_size_allocate),  self);
}

static void
channel_slot_widget_class_init (
  ChannelSlotWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (
    klass, "channel-slot");

  GObjectClass * oklass =
    G_OBJECT_CLASS (klass);
  oklass->finalize =
    (GObjectFinalizeFunc) finalize;
}
