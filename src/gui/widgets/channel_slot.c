// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/mixer_selections_action.h"
#include "actions/undo_manager.h"
#include "actions/undoable_action.h"
#include "audio/channel.h"
#include "audio/engine.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/backend/wrapped_object_with_change_signal.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/channel_slot.h"
#include "gui/widgets/left_dock_edge.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/mixer.h"
#include "plugins/lv2_plugin.h"
#include "project.h"
#include "utils/cairo.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/resources.h"
#include "utils/symap.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  ChannelSlotWidget,
  channel_slot_widget,
  GTK_TYPE_WIDGET)

#define ELLIPSIZE_PADDING 2

static Plugin *
get_plugin (ChannelSlotWidget * self)
{
  g_return_val_if_fail (
    self->type == PLUGIN_SLOT_INSTRUMENT
      || IS_TRACK (self->track),
    NULL);

  Plugin * plugin = NULL;
  switch (self->type)
    {
    case PLUGIN_SLOT_INSERT:
      return self->track->channel
        ->inserts[self->slot_index];
      break;
    case PLUGIN_SLOT_MIDI_FX:
      return self->track->channel
        ->midi_fx[self->slot_index];
      break;
    case PLUGIN_SLOT_INSTRUMENT:
      if (self->track && self->track->channel)
        return self->track->channel->instrument;
      break;
    case PLUGIN_SLOT_MODULATOR:
      return self->track
        ->modulators[self->slot_index];
      break;
    default:
      g_return_val_if_reached (NULL);
      break;
    }

  return plugin;
}

static void
recreate_pango_layouts (ChannelSlotWidget * self)
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
channel_slot_snapshot (
  GtkWidget *   widget,
  GtkSnapshot * snapshot)
{
  ChannelSlotWidget * self =
    Z_CHANNEL_SLOT_WIDGET (widget);

  int width =
    gtk_widget_get_allocated_width (widget);
  int height =
    gtk_widget_get_allocated_height (widget);

  GtkStyleContext * context =
    gtk_widget_get_style_context (widget);

  gtk_snapshot_render_background (
    snapshot, context, 0, 0, width, height);

  int      padding = 2;
  Plugin * plugin = get_plugin (self);
  GdkRGBA  bg, fg;
  if (plugin)
    {
      fg = UI_COLOR_BLACK;
      if (!plugin_is_enabled (plugin, false))
        {
          bg.red = 0.6f;
          bg.green = 0.6f;
          bg.blue = 0.6f;
          bg.alpha = 1.f;
          if (plugin->visible)
            {
              bg.red += 0.1f;
              bg.green += 0.1f;
              bg.blue += 0.1f;
            }
        }
      else if (plugin->visible)
        bg = UI_COLORS->bright_green;
      else
        bg = UI_COLORS->darkish_green;
    }
  else
    {
      bg = Z_GDK_RGBA_INIT (0.1f, 0.1f, 0.1f, 0.9f);
      fg = Z_GDK_RGBA_INIT (0.3f, 0.3f, 0.3f, 1);
    }

  /* fill background */
  gtk_snapshot_append_color (
    snapshot, &bg,
    &GRAPHENE_RECT_INIT (
      padding, padding, width - padding * 2,
      height - padding * 2));

  if (!self->empty_slot_layout || !self->pl_name_layout)
    {
      recreate_pango_layouts (self);
    }

#define MAX_LEN 400
  char txt[MAX_LEN];

  if (plugin)
    {
      const PluginDescriptor * descr =
        plugin->setting->descr;

      /* fill text */
      if (plugin->instantiation_failed)
        {
          snprintf (
            txt, MAX_LEN, "(!) %s", descr->name);
        }
      else
        {
          strncpy (txt, descr->name, MAX_LEN);
          txt[MAX_LEN - 1] = '\0';
        }
      int w, h;
      z_cairo_get_text_extents_for_widget (
        widget, self->pl_name_layout, txt, &w, &h);
      pango_layout_set_markup (
        self->pl_name_layout, txt, -1);
      gtk_snapshot_save (snapshot);
      gtk_snapshot_translate (
        snapshot,
        &GRAPHENE_POINT_INIT (
          width / 2.f - w / 2.f,
          height / 2.f - h / 2.f));
      gtk_snapshot_append_layout (
        snapshot, self->pl_name_layout, &fg);
      gtk_snapshot_restore (snapshot);

      /* update tooltip */
      if (
        !self->pl_name
        || !g_strcmp0 (descr->name, self->pl_name))
        {
          if (self->pl_name)
            g_free (self->pl_name);
          self->pl_name = g_strdup (descr->name);
          gtk_widget_set_tooltip_text (
            widget, self->pl_name);
        }
    }
  else
    {
      /* fill text */
      int w, h;
      if (self->type == PLUGIN_SLOT_INSTRUMENT)
        {
          snprintf (
            txt, MAX_LEN, "%s",
            _ ("No instrument"));
        }
      else
        {
          snprintf (
            txt, MAX_LEN, _ ("Slot #%d"),
            self->slot_index + 1);
        }
      z_cairo_get_text_extents_for_widget (
        widget, self->empty_slot_layout, txt, &w,
        &h);
      pango_layout_set_markup (
        self->empty_slot_layout, txt, -1);
      gtk_snapshot_save (snapshot);
      gtk_snapshot_translate (
        snapshot,
        &GRAPHENE_POINT_INIT (
          width / 2.f - w / 2.f,
          height / 2.f - h / 2.f));
      gtk_snapshot_append_layout (
        snapshot, self->empty_slot_layout, &fg);
      gtk_snapshot_restore (snapshot);

      /* update tooltip */
      if (self->pl_name)
        {
          g_free (self->pl_name);
          self->pl_name = NULL;
          gtk_widget_set_tooltip_text (widget, txt);
        }
    }
#undef MAX_LEN
}

static gboolean
on_dnd_drop (
  GtkDropTarget * drop_target,
  const GValue *  value,
  double          x,
  double          y,
  gpointer        data)
{
  if (!G_VALUE_HOLDS (
        value,
        WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE))
    {
      g_message ("invalid DND type");
      return false;
    }

  ChannelSlotWidget * self =
    Z_CHANNEL_SLOT_WIDGET (data);

  GdkDragAction action =
    z_gtk_drop_target_get_selected_action (
      drop_target);

  g_debug (
    "channel slot: dnd drop %s",
    action == GDK_ACTION_COPY ? "COPY"
    : action == GDK_ACTION_MOVE
      ? "MOVE"
      : "LINK");

  WrappedObjectWithChangeSignal * wrapped_obj =
    g_value_get_object (value);
  Plugin *           pl = NULL;
  PluginDescriptor * descr = NULL;
  if (
    wrapped_obj->type
    == WRAPPED_OBJECT_TYPE_PLUGIN_DESCR)
    {
      descr = (PluginDescriptor *) wrapped_obj->obj;
    }
  else if (
    wrapped_obj->type == WRAPPED_OBJECT_TYPE_PLUGIN)
    {
      pl = (Plugin *) wrapped_obj->obj;
    }

  bool plugin_invalid = false;
  if (pl)
    {
      /* if plugin not at original position */
      Track * orig_track = plugin_get_track (pl);
      if (
        self->track != orig_track
        || self->slot_index != pl->id.slot
        || self->type != pl->id.slot_type)
        {
          if (plugin_descriptor_is_valid_for_slot_type (
                pl->setting->descr, self->type,
                self->track->type))
            {
              bool     ret;
              GError * err = NULL;
              if (action == GDK_ACTION_COPY)
                {
                  ret = mixer_selections_action_perform_copy (
                    MIXER_SELECTIONS,
                    PORT_CONNECTIONS_MGR,
                    self->type,
                    track_get_name_hash (
                      self->track),
                    self->slot_index, &err);
                }
              else if (action == GDK_ACTION_MOVE)
                {
                  ret = mixer_selections_action_perform_move (
                    MIXER_SELECTIONS,
                    PORT_CONNECTIONS_MGR,
                    self->type,
                    track_get_name_hash (
                      self->track),
                    self->slot_index, &err);
                }
              else
                g_return_val_if_reached (false);

              if (!ret)
                {
                  HANDLE_ERROR (
                    err, "%s",
                    _ ("Failed to move or copy "
                       "plugins"));
                }
            }
          else
            {
              plugin_invalid = true;
              descr = pl->setting->descr;
            }
        }
    }
  else if (descr)
    {
      /* validate */
      if (plugin_descriptor_is_valid_for_slot_type (
            descr, self->type, self->track->type))
        {
          PluginSetting * setting =
            plugin_setting_new_default (descr);
          GError * err = NULL;
          bool     ret =
            mixer_selections_action_perform_create (
              self->type,
              track_get_name_hash (self->track),
              self->slot_index, setting, 1, &err);
          if (!ret)
            {
              HANDLE_ERROR (
                err,
                _ ("Failed to create plugin %s"),
                setting->descr->name);
            }
          plugin_setting_free (setting);
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
        _ ("Plugin %s cannot be added to this slot"),
        descr->name);
      ui_show_error_message (
        MAIN_WINDOW, false, msg);
    }

  gtk_widget_queue_draw (GTK_WIDGET (self));

  return true;
}

/**
 * Control not pressed, no plugin exists,
 * not same channel */
static inline void
select_no_ctrl_no_pl_no_ch (
  ChannelSlotWidget * self)
{
  mixer_selections_clear (
    MIXER_SELECTIONS, F_PUBLISH_EVENTS);
}

/**
 * Control not pressed, no plugin exists,
 * same channel */
static inline void
select_no_ctrl_no_pl_ch (ChannelSlotWidget * self)
{
  mixer_selections_clear (
    MIXER_SELECTIONS, F_PUBLISH_EVENTS);
}

/**
 * Control not pressed, plugin exists,
 * not same channel */
static inline void
select_no_ctrl_pl_no_ch (ChannelSlotWidget * self)
{
  mixer_selections_clear (
    MIXER_SELECTIONS, F_NO_PUBLISH_EVENTS);

  mixer_selections_add_slot (
    MIXER_SELECTIONS, self->track, self->type,
    self->slot_index, F_NO_CLONE);
}

/**
 * Control not pressed, plugin exists,
 * same channel */
static inline void
select_no_ctrl_pl_ch (ChannelSlotWidget * self)
{
  /* if plugin is not selected, make it the only
   * selection otherwise do nothing */
  if (!mixer_selections_contains_slot (
        MIXER_SELECTIONS, self->type,
        self->slot_index))
    {
      mixer_selections_clear (
        MIXER_SELECTIONS, F_NO_PUBLISH_EVENTS);

      mixer_selections_add_slot (
        MIXER_SELECTIONS, self->track, self->type,
        self->slot_index, F_NO_CLONE);
    }
}

/**
 * Control pressed, no plugin exists, not
 * same channel */
static inline void
select_ctrl_no_pl_no_ch (ChannelSlotWidget * self)
{
  mixer_selections_clear (
    MIXER_SELECTIONS, F_PUBLISH_EVENTS);
}

/**
 * Control pressed, no plugin exists,
 * same channel */
static inline void
select_ctrl_no_pl_ch (ChannelSlotWidget * self)
{
  mixer_selections_clear (
    MIXER_SELECTIONS, F_PUBLISH_EVENTS);
}

/**
 * Control pressed, plugin exists,
 * not same channel */
static inline void
select_ctrl_pl_no_ch (ChannelSlotWidget * self)
{
  /* make it the only selection */
  mixer_selections_clear (
    MIXER_SELECTIONS, F_NO_PUBLISH_EVENTS);
  mixer_selections_add_slot (
    MIXER_SELECTIONS, self->track, self->type,
    self->slot_index, F_NO_CLONE);
}

/**
 * Control pressed, plugin exists, same channel */
static inline void
select_ctrl_pl_ch (ChannelSlotWidget * self)
{
  /* if already selected, deselect it, otherwise
   * add it to selections */
  if (mixer_selections_contains_slot (
        MIXER_SELECTIONS, self->type,
        self->slot_index))
    {
      mixer_selections_remove_slot (
        MIXER_SELECTIONS, self->slot_index,
        self->type, F_PUBLISH_EVENTS);
      /*self->deselected = 1;*/
    }
  else
    {
      mixer_selections_add_slot (
        MIXER_SELECTIONS, self->track, self->type,
        self->slot_index, F_NO_CLONE);
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
select_plugin (ChannelSlotWidget * self, int ctrl)
{
  bool pl = false, ch = false;

  /* if plugin exists */
  Plugin * plugin = get_plugin (self);
  if (plugin)
    pl = true;

  /* if same channel as selections */
  g_return_if_fail (self->track);
  if (
    self->track->channel
    && track_get_name_hash (self->track)
         == MIXER_SELECTIONS->track_name_hash)
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
  if (self->track)
    {
      tracklist_selections_select_single (
        TRACKLIST_SELECTIONS, self->track,
        F_PUBLISH_EVENTS);
    }
}

static void
drag_end (
  GtkGestureDrag *    gesture,
  gdouble             offset_x,
  gdouble             offset_y,
  ChannelSlotWidget * self)
{
  GdkModifierType state =
    gtk_event_controller_get_current_event_state (
      GTK_EVENT_CONTROLLER (gesture));

  Plugin * pl = get_plugin (self);
  if (pl && self->n_press == 2)
    {
      bool new_visible = !pl->visible;
      g_message (
        "%s: setting plugin %s visible %d",
        __func__, pl->setting->descr->name,
        new_visible);
      g_warn_if_fail (pl->instantiated);
      pl->visible = new_visible;
      EVENTS_PUSH (
        ET_PLUGIN_VISIBILITY_CHANGED, pl);
    }
  else if (self->n_press == 1)
    {
      int ctrl = 0;
      /* if control click */
      if (state & GDK_CONTROL_MASK)
        ctrl = 1;

      select_plugin (self, ctrl);

      /*self->deselected = 0;*/
      /*self->reselected = 0;*/
    }

  if (pl)
    {
      g_return_if_fail (self->track);
      g_return_if_fail (self->track->channel);
      g_return_if_fail (
        self->track->channel->widget);
      self->track->channel->widget
        ->last_plugin_press =
        g_get_monotonic_time ();
    }
  g_message (
    "%s: drag end %d press", __func__,
    self->n_press);
}

static void
on_press (
  GtkGestureClick *   gesture,
  gint                n_press,
  gdouble             x,
  gdouble             y,
  ChannelSlotWidget * self)
{
  self->n_press = n_press;
  g_message ("pressed %d", n_press);

  if (
    self->open_plugin_inspector_on_click
    && self->type != PLUGIN_SLOT_INSTRUMENT)
    {
      if (self->type == PLUGIN_SLOT_INSERT)
        {
          PROJECT->last_selection =
            SELECTION_TYPE_INSERT;
        }
      else if (self->type == PLUGIN_SLOT_MIDI_FX)
        {
          PROJECT->last_selection =
            SELECTION_TYPE_MIDI_FX;
        }
      EVENTS_PUSH (
        ET_PROJECT_SELECTION_TYPE_CHANGED, NULL);
    }
}

static bool
tick_cb (
  GtkWidget *         widget,
  GdkFrameClock *     frame_clock,
  ChannelSlotWidget * self)
{
  if (!gtk_widget_is_visible (widget))
    {
      return G_SOURCE_CONTINUE;
    }

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
  ChannelSlotWidget * self,
  double              x,
  double              y)
{
  MW_MIXER->paste_slot = self;

  switch (self->type)
    {
    case PLUGIN_SLOT_INSERT:
      PROJECT->last_selection =
        SELECTION_TYPE_INSERT;
      break;
    case PLUGIN_SLOT_INSTRUMENT:
      PROJECT->last_selection =
        SELECTION_TYPE_INSTRUMENT;
      break;
    case PLUGIN_SLOT_MIDI_FX:
      PROJECT->last_selection =
        SELECTION_TYPE_MIDI_FX;
      break;
    case PLUGIN_SLOT_MODULATOR:
      PROJECT->last_selection =
        SELECTION_TYPE_MODULATOR;
      break;
    default:
      g_return_if_reached ();
      break;
    }
  EVENTS_PUSH (
    ET_PROJECT_SELECTION_TYPE_CHANGED, NULL);

  GMenu *     menu = g_menu_new ();
  GMenuItem * menuitem;

  Plugin * pl = get_plugin (self);

  if (pl)
    {
      GMenu * plugin_submenu = g_menu_new ();

      /* add bypass option */
      char tmp[500];
      sprintf (
        tmp, "app.plugin-toggle-enabled::%p", pl);
      menuitem = z_gtk_create_menu_item (
        _ ("Bypass"), NULL, tmp);
      g_menu_append_item (plugin_submenu, menuitem);

      /* add inspect option */
      menuitem = z_gtk_create_menu_item (
        _ ("Inspect"), NULL, "app.plugin-inspect");
      g_menu_append_item (plugin_submenu, menuitem);

      g_menu_append_section (
        menu, _ ("Plugin"),
        G_MENU_MODEL (plugin_submenu));
    }

  GMenu * edit_submenu = g_menu_new ();
  if (self->type != PLUGIN_SLOT_INSTRUMENT)
    {
      menuitem = CREATE_CUT_MENU_ITEM ("app.cut");
      g_menu_append_item (edit_submenu, menuitem);
      menuitem = CREATE_COPY_MENU_ITEM ("app.copy");
      g_menu_append_item (edit_submenu, menuitem);
      menuitem =
        CREATE_PASTE_MENU_ITEM ("app.paste");
      g_menu_append_item (edit_submenu, menuitem);
    }

  /* if plugin exists */
  if (pl && self->type != PLUGIN_SLOT_INSTRUMENT)
    {
      /* add delete item */
      menuitem = CREATE_DELETE_MENU_ITEM (
        "app.mixer-selections-delete");
      g_menu_append_item (edit_submenu, menuitem);
    }

  g_menu_append_section (
    menu, _ ("Edit"), G_MENU_MODEL (edit_submenu));

  if (self->type != PLUGIN_SLOT_INSTRUMENT)
    {
      GMenu * select_submenu = g_menu_new ();

      menuitem = CREATE_CLEAR_SELECTION_MENU_ITEM (
        "app.clear-selection");
      g_menu_append_item (select_submenu, menuitem);
      menuitem = CREATE_SELECT_ALL_MENU_ITEM (
        "app.select-all");
      g_menu_append_item (select_submenu, menuitem);

      g_menu_append_section (
        menu, _ ("Select"),
        G_MENU_MODEL (select_submenu));
    }

  z_gtk_show_context_menu_from_g_menu (
    self->popover_menu, x, y, menu);
}

static void
on_right_click (
  GtkGestureClick *   gesture,
  gint                n_press,
  gdouble             x,
  gdouble             y,
  ChannelSlotWidget * self)
{
  if (n_press != 1)
    return;

  GdkModifierType state =
    gtk_event_controller_get_current_event_state (
      GTK_EVENT_CONTROLLER (gesture));

  select_plugin (self, state & GDK_CONTROL_MASK);

  show_context_menu (self, x, y);
}

static GdkDragAction
on_dnd_motion (
  GtkDropTarget * drop_target,
  gdouble         x,
  gdouble         y,
  gpointer        user_data)
{
  /*ChannelSlotWidget * self =*/
  /*Z_CHANNEL_SLOT_WIDGET (user_data);*/

  g_message ("motion");

  return GDK_ACTION_MOVE;
}

static void
on_motion_enter (
  GtkEventControllerMotion * motion_controller,
  gdouble                    x,
  gdouble                    y,
  gpointer                   user_data)
{
  ChannelSlotWidget * self =
    Z_CHANNEL_SLOT_WIDGET (user_data);
  gtk_widget_set_state_flags (
    GTK_WIDGET (self), GTK_STATE_FLAG_PRELIGHT, 0);
}

static void
on_motion_leave (
  GtkEventControllerMotion * motion_controller,
  gpointer                   user_data)
{
  ChannelSlotWidget * self =
    Z_CHANNEL_SLOT_WIDGET (user_data);
  gtk_widget_unset_state_flags (
    GTK_WIDGET (self), GTK_STATE_FLAG_PRELIGHT);
}

static void
finalize (ChannelSlotWidget * self)
{
  if (self->pl_name)
    g_free (self->pl_name);
  if (self->empty_slot_layout)
    g_object_unref (self->empty_slot_layout);
  if (self->pl_name_layout)
    g_object_unref (self->pl_name_layout);

  G_OBJECT_CLASS (channel_slot_widget_parent_class)
    ->finalize (G_OBJECT (self));
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
  Track *             track)
{
  self->track = track;
  channel_slot_widget_set_state_flags (
    self, GTK_STATE_FLAG_SELECTED,
    track->channel->instrument
      && plugin_is_selected (
        track->channel->instrument));
}

static GdkContentProvider *
on_dnd_drag_prepare (
  GtkDragSource *     source,
  double              x,
  double              y,
  ChannelSlotWidget * self)
{
  Plugin * pl = get_plugin (self);
  WrappedObjectWithChangeSignal * wrapped_obj =
    wrapped_object_with_change_signal_new (
      pl, WRAPPED_OBJECT_TYPE_PLUGIN);
  GdkContentProvider * content_providers[] = {
    gdk_content_provider_new_typed (
      WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE,
      wrapped_obj),
  };

  return gdk_content_provider_new_union (
    content_providers,
    G_N_ELEMENTS (content_providers));
}

static void
setup_dnd (ChannelSlotWidget * self)
{
  if (self->type != PLUGIN_SLOT_INSTRUMENT)
    {
      /* set as drag source for plugin */
      GtkDragSource * drag_source =
        gtk_drag_source_new ();
      gtk_drag_source_set_actions (
        drag_source,
        GDK_ACTION_COPY | GDK_ACTION_MOVE);
      g_signal_connect (
        drag_source, "prepare",
        G_CALLBACK (on_dnd_drag_prepare), self);
      gtk_widget_add_controller (
        GTK_WIDGET (self),
        GTK_EVENT_CONTROLLER (drag_source));
    }

  /* set as drag dest for both plugins and
   * plugin descriptors */
  GtkDropTarget * drop_target = gtk_drop_target_new (
    G_TYPE_INVALID,
    GDK_ACTION_MOVE | GDK_ACTION_COPY);
  gtk_drop_target_set_preload (drop_target, true);
  GType types[] = {
    WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE
  };
  gtk_drop_target_set_gtypes (
    drop_target, types, G_N_ELEMENTS (types));
  g_signal_connect (
    drop_target, "drop", G_CALLBACK (on_dnd_drop),
    self);
  g_signal_connect (
    drop_target, "motion",
    G_CALLBACK (on_dnd_motion), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self),
    GTK_EVENT_CONTROLLER (drop_target));
}

/**
 * Creates a new ChannelSlot widget and binds it to
 * the given value.
 */
ChannelSlotWidget *
channel_slot_widget_new (
  int            slot_index,
  Track *        track,
  PluginSlotType type,
  bool           open_plugin_inspector_on_click)
{
  ChannelSlotWidget * self =
    g_object_new (CHANNEL_SLOT_WIDGET_TYPE, NULL);
  self->slot_index = slot_index;
  self->type = type;
  self->track = track;
  self->open_plugin_inspector_on_click =
    open_plugin_inspector_on_click;

  setup_dnd (self);

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
    channel_slot_widget_new (
      -1, NULL, PLUGIN_SLOT_INSTRUMENT, false);

  return self;
}

static void
channel_slot_widget_init (ChannelSlotWidget * self)
{
  gtk_widget_set_size_request (
    GTK_WIDGET (self), -1, 20);

  gtk_widget_set_hexpand (GTK_WIDGET (self), true);

  self->pl_name = NULL;
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (self), _ ("empty slot"));

  self->popover_menu = GTK_POPOVER_MENU (
    gtk_popover_menu_new_from_model (NULL));
  gtk_widget_set_parent (
    GTK_WIDGET (self->popover_menu),
    GTK_WIDGET (self));

  self->click =
    GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  gtk_event_controller_set_propagation_phase (
    GTK_EVENT_CONTROLLER (self->click),
    GTK_PHASE_CAPTURE);
  g_signal_connect (
    G_OBJECT (self->click), "pressed",
    G_CALLBACK (on_press), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self),
    GTK_EVENT_CONTROLLER (self->click));

  self->right_mouse_mp =
    GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (self->right_mouse_mp),
    GDK_BUTTON_SECONDARY);
  g_signal_connect (
    G_OBJECT (self->right_mouse_mp), "pressed",
    G_CALLBACK (on_right_click), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self),
    GTK_EVENT_CONTROLLER (self->right_mouse_mp));

  self->drag =
    GTK_GESTURE_DRAG (gtk_gesture_drag_new ());
  gtk_event_controller_set_propagation_phase (
    GTK_EVENT_CONTROLLER (self->drag),
    GTK_PHASE_CAPTURE);
  g_signal_connect (
    G_OBJECT (self->drag), "drag-end",
    G_CALLBACK (drag_end), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self),
    GTK_EVENT_CONTROLLER (self->drag));

  GtkEventController * motion_controller =
    gtk_event_controller_motion_new ();
  g_signal_connect (
    G_OBJECT (motion_controller), "enter",
    G_CALLBACK (on_motion_enter), self);
  g_signal_connect (
    G_OBJECT (motion_controller), "leave",
    G_CALLBACK (on_motion_leave), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self), motion_controller);

  gtk_widget_add_tick_callback (
    GTK_WIDGET (self), (GtkTickCallback) tick_cb,
    self, NULL);
}

static void
dispose (ChannelSlotWidget * self)
{
  gtk_widget_unparent (
    GTK_WIDGET (self->popover_menu));

  G_OBJECT_CLASS (channel_slot_widget_parent_class)
    ->dispose (G_OBJECT (self));
}

static void
channel_slot_widget_class_init (
  ChannelSlotWidgetClass * klass)
{
  GtkWidgetClass * wklass =
    GTK_WIDGET_CLASS (klass);
  wklass->snapshot = channel_slot_snapshot;
  gtk_widget_class_set_css_name (
    wklass, "channel-slot");

  gtk_widget_class_set_layout_manager_type (
    wklass, GTK_TYPE_BIN_LAYOUT);

  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->finalize = (GObjectFinalizeFunc) finalize;
  oklass->dispose = (GObjectFinalizeFunc) dispose;
}
