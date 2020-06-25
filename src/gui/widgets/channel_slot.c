/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include "actions/copy_plugins_action.h"
#include "actions/create_plugins_action.h"
#include "actions/delete_plugins_action.h"
#include "actions/move_plugins_action.h"
#include "actions/undoable_action.h"
#include "actions/undo_manager.h"
#include "audio/channel.h"
#include "audio/engine.h"
#include "plugins/lv2_plugin.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/channel_slot.h"
#include "gui/widgets/left_dock_edge.h"
#include "gui/widgets/main_window.h"
#include "project.h"
#include "utils/cairo.h"
#include "utils/gtk.h"
#include "utils/flags.h"
#include "utils/symap.h"
#include "utils/ui.h"
#include "utils/resources.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  ChannelSlotWidget, channel_slot_widget,
  GTK_TYPE_DRAWING_AREA)

#define ELLIPSIZE_PADDING 2

static Plugin *
get_plugin (
  ChannelSlotWidget * self)
{
  Plugin * plugin = NULL;
  switch (self->type)
    {
    case PLUGIN_SLOT_INSERT:
      return
        self->channel->inserts[self->slot_index];
      break;
    case PLUGIN_SLOT_MIDI_FX:
      return
        self->channel->midi_fx[self->slot_index];
      break;
    case PLUGIN_SLOT_INSTRUMENT:
      if (self->channel)
        {
          return self->channel->instrument;
        }
      break;
    }

  return plugin;
}

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
  Plugin * plugin = get_plugin (self);
  if (plugin)
    {
      GdkRGBA bg, fg;
      fg = UI_COLOR_BLACK;
      if (!plugin_is_enabled (plugin))
        {
          bg.red = 0.6;
          bg.green = 0.6;
          bg.blue = 0.6;
          bg.alpha = 1.0;
          if (plugin->visible)
            {
              bg.red += 0.1;
              bg.green += 0.1;
              bg.blue += 0.1;
            }
        }
      else if (plugin->visible)
        bg = UI_COLORS->bright_green;
      else
        bg = UI_COLORS->darkish_green;

      /* fill background */
      cairo_set_source_rgba (
        cr, bg.red, bg.green, bg.blue, 1.0);
      cairo_rectangle (
        cr, padding, padding,
        width - padding * 2, height - padding * 2);
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
      cairo_rectangle (
        cr, padding, padding,
        width - padding * 2, height - padding * 2);
      cairo_fill(cr);

      /* fill text */
      cairo_set_source_rgba (
        cr, 0.3, 0.3, 0.3, 1.0);
      int w, h;
      char text[400];
      if (self->type == PLUGIN_SLOT_INSTRUMENT)
        {
          sprintf (text, "%s", _("No instrument"));
        }
      else
        {
          sprintf (
            text, _("Slot #%d"),
            self->slot_index + 1);
        }
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
            widget, text);
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

  bool plugin_invalid = false;
  Plugin * pl = NULL;
  PluginDescriptor * descr = NULL;
  if (atom == plugin_atom)
    {
      Plugin * received_pl = NULL;
      const guchar *my_data =
        gtk_selection_data_get_data (data);
      memcpy (
        &received_pl, my_data,
        sizeof (received_pl));
      pl = plugin_find (&received_pl->id);
      g_warn_if_fail (pl);

      /* if plugin not at original position */
      Channel * orig_ch =
        plugin_get_channel (pl);
      if (self->channel != orig_ch ||
          self->slot_index != pl->id.slot ||
          self->type != pl->id.slot_type)
        {
          if (plugin_descriptor_is_valid_for_slot_type (
                pl->descr, self->type,
                self->channel->track->type))
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
                      self->type,
                      self->channel->track,
                      self->slot_index);
                }
              else if (action == GDK_ACTION_MOVE)
                {
                  ua =
                    move_plugins_action_new (
                      MIXER_SELECTIONS,
                      self->type,
                      self->channel->track,
                      self->slot_index);
                }
              g_warn_if_fail (ua);

              undo_manager_perform (
                UNDO_MANAGER, ua);
            }
          else
            {
              plugin_invalid = true;
              descr = pl->descr;
            }
        }
    }
  else if (atom == plugin_descr_atom)
    {
      gdk_drag_status (
        context, GDK_ACTION_COPY, time);
      const guchar *my_data =
        gtk_selection_data_get_data (data);
      memcpy (&descr, my_data, sizeof (descr));
      g_warn_if_fail (descr);

      /* validate */
      if (plugin_descriptor_is_valid_for_slot_type (
            descr, self->type,
            self->channel->track->type))
        {
          UndoableAction * ua =
            create_plugins_action_new (
              descr, self->type,
              self->channel->track_pos,
              self->slot_index, 1);

          undo_manager_perform (
            UNDO_MANAGER, ua);
        }
      else
        {
          plugin_invalid = true;
        }
    }

  if (plugin_invalid)
    {
      char msg[400];
      sprintf (
        msg,
        _("Plugin %s cannot be added to this slot"),
        descr->name);
      ui_show_error_message (
        MAIN_WINDOW, msg);
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
    self->type,
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
        self->type,
        self->slot_index))
    {
      mixer_selections_clear (
        MIXER_SELECTIONS,
        F_NO_PUBLISH_EVENTS);

      mixer_selections_add_slot (
        MIXER_SELECTIONS,
        self->channel,
        self->type,
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
    self->type,
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
        self->type,
        self->slot_index))
    {
      mixer_selections_remove_slot (
        MIXER_SELECTIONS,
        self->slot_index,
        self->type,
        F_PUBLISH_EVENTS);
      /*self->deselected = 1;*/
    }
  else
    {
      mixer_selections_add_slot (
        MIXER_SELECTIONS,
        self->channel,
        self->type,
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
  bool pl = false, ch = false;

  /* if plugin exists */
  Plugin * plugin = get_plugin (self);
  if (plugin)
    pl = true;

  /* if same channel as selections */
  if (self->channel &&
      self->channel->track_pos ==
        MIXER_SELECTIONS->track_pos)
    ch = true;

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
  if (self->channel)
    {
      Track * track =
        channel_get_track (self->channel);
      tracklist_selections_select_single (
        TRACKLIST_SELECTIONS, track);
    }
}

static void
drag_end (
  GtkGestureDrag *    gesture,
  gdouble             offset_x,
  gdouble             offset_y,
  ChannelSlotWidget * self)
{
  GdkModifierType state_mask =
    ui_get_state_mask (
      GTK_GESTURE (gesture));

  Plugin * pl = get_plugin (self);
  if (self->n_press == 2)
    {
      g_message ("opening plugin ");
      if (pl)
        {
          pl->visible = !pl->visible;
          EVENTS_PUSH (
            ET_PLUGIN_VISIBILITY_CHANGED, pl);
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

  if (pl)
    {
      self->channel->widget->last_plugin_press =
        g_get_monotonic_time ();
    }
  g_message ("drag end %d", self->n_press);
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
  g_message ("multipress %d", n_press);

  if (self->open_plugin_inspector_on_click &&
      self->type != PLUGIN_SLOT_INSTRUMENT)
    {
      PROJECT->last_selection =
        SELECTION_TYPE_PLUGIN;
    }
}

static void
on_plugin_delete (
  GtkMenuItem *       menu_item,
  ChannelSlotWidget * self)
{
  UndoableAction * ua =
    delete_plugins_action_new (MIXER_SELECTIONS);
  undo_manager_perform (UNDO_MANAGER, ua);
  EVENTS_PUSH (ET_PLUGINS_REMOVED, self->channel);
}

static void
on_plugin_bypass_activate (
  GtkMenuItem * menuitem,
  Plugin *      pl)
{
  plugin_set_enabled (
    pl, !plugin_is_enabled (pl), true);
}

static void
on_plugin_inspect_activate (
  GtkMenuItem * menuitem,
  Plugin *      pl)
{
  PROJECT->last_selection = SELECTION_TYPE_PLUGIN;
  left_dock_edge_widget_refresh (
    MW_LEFT_DOCK_EDGE);
}

static bool
tick_cb (
  GtkWidget *         widget,
  GdkFrameClock *     frame_clock,
  ChannelSlotWidget * self)
{
  Plugin * pl = get_plugin (self);
  bool is_selected = pl && plugin_is_selected (pl);
  if (is_selected != self->was_selected)
    {
      channel_slot_widget_set_state_flags (
        self, GTK_STATE_FLAG_SELECTED, is_selected);
    }

  return G_SOURCE_CONTINUE;
}

static void
show_context_menu (
  ChannelSlotWidget * self)
{
  GtkWidget *menu;
  GtkMenuItem * menuitem;

  menu = gtk_menu_new();

  Plugin * pl = get_plugin (self);

#define CREATE_SEPARATOR \
  menuitem = \
    GTK_MENU_ITEM (gtk_separator_menu_item_new ()); \
  gtk_widget_show_all (GTK_WIDGET (menuitem))

#define ADD_TO_SHELL \
  gtk_menu_shell_append ( \
    GTK_MENU_SHELL(menu), GTK_WIDGET (menuitem))

  bool needs_sep = false;

  if (pl)
    {
      /* add bypass option */
      menuitem =
        GTK_MENU_ITEM (
          gtk_check_menu_item_new_with_label (
            _("Bypass")));
      gtk_check_menu_item_set_active (
        GTK_CHECK_MENU_ITEM (menuitem),
        !plugin_is_enabled (pl));
      g_signal_connect (
        G_OBJECT (menuitem), "activate",
        G_CALLBACK (on_plugin_bypass_activate), pl);
      ADD_TO_SHELL;

      /* add inspect option */
      menuitem =
        GTK_MENU_ITEM (
          gtk_menu_item_new_with_label (
            _("Inspect")));
      g_signal_connect (
        G_OBJECT (menuitem), "activate",
        G_CALLBACK (on_plugin_inspect_activate),
        pl);
      ADD_TO_SHELL;

      needs_sep = true;
    }

  if (self->type != PLUGIN_SLOT_INSTRUMENT)
    {
      if (needs_sep)
        {
          CREATE_SEPARATOR;
          ADD_TO_SHELL;
        }

      menuitem = CREATE_CUT_MENU_ITEM (NULL);
      ADD_TO_SHELL;
      menuitem = CREATE_COPY_MENU_ITEM (NULL);
      ADD_TO_SHELL;
      menuitem = CREATE_PASTE_MENU_ITEM (NULL);
      ADD_TO_SHELL;
    }

  /* if plugin exists */
  if (pl && self->type != PLUGIN_SLOT_INSTRUMENT)
    {
      /* add delete item */
      menuitem = CREATE_DELETE_MENU_ITEM (NULL);
      g_signal_connect (
        G_OBJECT (menuitem), "activate",
        G_CALLBACK (on_plugin_delete), self);
      ADD_TO_SHELL;

      needs_sep = true;
    }

  if (self->type != PLUGIN_SLOT_INSTRUMENT)
    {
      if (needs_sep)
        {
          CREATE_SEPARATOR;
          ADD_TO_SHELL;
        }

      menuitem =
        CREATE_CLEAR_SELECTION_MENU_ITEM (NULL);
      ADD_TO_SHELL;
      menuitem = CREATE_SELECT_ALL_MENU_ITEM (NULL);
      ADD_TO_SHELL;

      needs_sep = true;
    }

#undef ADD_TO_SHELL

  gtk_widget_show_all(menu);
  gtk_menu_attach_to_widget (
    GTK_MENU (menu),
    GTK_WIDGET (self), NULL);

  gtk_menu_popup_at_pointer (GTK_MENU (menu), NULL);
}

static void
on_right_click (
  GtkGestureMultiPress *gesture,
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
  Plugin* pl = get_plugin (self);

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
on_drag_motion (
  GtkWidget *widget,
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
    gdk_drag_status (
      context, GDK_ACTION_COPY, time);
  else
    gdk_drag_status (
      context, GDK_ACTION_MOVE, time);
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
    z_cairo_create_pango_layout_from_string (
      (GtkWidget *) self, "Arial Italic 7.5",
      PANGO_ELLIPSIZE_END, ELLIPSIZE_PADDING);

  self->pl_name_layout =
    z_cairo_create_pango_layout_from_string (
      (GtkWidget *) self, "Arial Bold 7.5",
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
 * Sets or unsets state flags and redraws the
 * widget.
 *
 * @param set True to set, false to unset.
 */
void
channel_slot_widget_set_state_flags (
  ChannelSlotWidget * self,
  GtkStateFlags       flags,
  bool                set)
{
  if (set)
    {
      gtk_widget_set_state_flags (
        GTK_WIDGET (self), flags, 0);
    }
  else
    {
      gtk_widget_unset_state_flags (
        GTK_WIDGET (self), flags);
    }
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

void
channel_slot_widget_set_instrument (
  ChannelSlotWidget * self,
  Channel *           ch)
{
  self->channel = ch;
  channel_slot_widget_set_state_flags (
    self, GTK_STATE_FLAG_SELECTED,
    ch->instrument &&
    plugin_is_selected (ch->instrument));
}

/**
 * Creates a new ChannelSlot widget and binds it to
 * the given value.
 */
ChannelSlotWidget *
channel_slot_widget_new (
  int       slot_index,
  Channel * ch,
  PluginSlotType type,
  bool      open_plugin_inspector_on_click)
{
  ChannelSlotWidget * self =
    g_object_new (
      CHANNEL_SLOT_WIDGET_TYPE, NULL);
  self->slot_index = slot_index;
  self->type = type;
  self->channel = ch;
  self->open_plugin_inspector_on_click =
    open_plugin_inspector_on_click;

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

/**
 * Creates a new ChannelSlot widget whose track
 * and plugin can change.
 */
ChannelSlotWidget *
channel_slot_widget_new_instrument (void)
{
  ChannelSlotWidget * self =
    g_object_new (
      CHANNEL_SLOT_WIDGET_TYPE, NULL);
  self->slot_index = -1;
  self->type = PLUGIN_SLOT_INSTRUMENT;
  self->channel = NULL;
  self->open_plugin_inspector_on_click = false;

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
    GTK_WIDGET (self), -1, 20);

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

  gtk_widget_add_tick_callback (
    GTK_WIDGET (self), (GtkTickCallback) tick_cb,
    self, NULL);
}

static void
channel_slot_widget_class_init (
  ChannelSlotWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (
    klass, "channel-slot");

  GObjectClass * oklass =
    G_OBJECT_CLASS (klass);
  oklass->finalize =
    (GObjectFinalizeFunc) finalize;
}
