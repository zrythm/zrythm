// SPDX-FileCopyrightText: © 2018-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/mixer_selections_action.h"
#include "actions/undo_manager.h"
#include "actions/undoable_action.h"
#include "dsp/channel.h"
#include "dsp/engine.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/backend/wrapped_object_with_change_signal.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/channel_slot.h"
#include "gui/widgets/channel_slot_activate_button.h"
#include "gui/widgets/left_dock_edge.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/mixer.h"
#include "project.h"
#include "utils/cairo.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/objects.h"
#include "utils/resources.h"
#include "utils/symap.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (ChannelSlotWidget, channel_slot_widget, GTK_TYPE_WIDGET)

#define ELLIPSIZE_PADDING 2

Plugin *
channel_slot_widget_get_plugin (ChannelSlotWidget * self)
{
  g_return_val_if_fail (
    self->type == ZPluginSlotType::Z_PLUGIN_SLOT_INSTRUMENT
      || IS_TRACK (self->track),
    NULL);

  Plugin * plugin = NULL;
  switch (self->type)
    {
    case ZPluginSlotType::Z_PLUGIN_SLOT_INSERT:
      return self->track->channel->inserts[self->slot_index];
      break;
    case ZPluginSlotType::Z_PLUGIN_SLOT_MIDI_FX:
      return self->track->channel->midi_fx[self->slot_index];
      break;
    case ZPluginSlotType::Z_PLUGIN_SLOT_INSTRUMENT:
      if (self->track && self->track->channel)
        return self->track->channel->instrument;
      break;
    case ZPluginSlotType::Z_PLUGIN_SLOT_MODULATOR:
      return self->track->modulators[self->slot_index];
      break;
    default:
      g_return_val_if_reached (NULL);
      break;
    }

  return plugin;
}

static void
update_pango_layouts (ChannelSlotWidget * self, bool force)
{
  if (!self->txt_layout || force)
    {
      object_free_w_func_and_null (g_object_unref, self->txt_layout);
      PangoLayout * layout =
        gtk_widget_create_pango_layout (GTK_WIDGET (self), NULL);
      pango_layout_set_ellipsize (layout, PANGO_ELLIPSIZE_END);

      self->txt_layout = layout;
    }

  int btn_width = gtk_widget_get_width (GTK_WIDGET (self->activate_btn));

  pango_layout_set_width (
    self->txt_layout,
    pango_units_from_double (MAX (
      gtk_widget_get_width (GTK_WIDGET (self)) - ELLIPSIZE_PADDING * 2 - btn_width,
      1)));
}

static void
channel_slot_snapshot (GtkWidget * widget, GtkSnapshot * snapshot)
{
  ChannelSlotWidget * self = Z_CHANNEL_SLOT_WIDGET (widget);

  update_pango_layouts (self, false);

  int width = gtk_widget_get_width (widget);
  int btn_width = gtk_widget_get_width (GTK_WIDGET (self->activate_btn));
  int height = gtk_widget_get_height (widget);

  Plugin * plugin = channel_slot_widget_get_plugin (self);

#define MAX_LEN 400
  char txt[MAX_LEN];

  if (plugin)
    {
      const PluginDescriptor * descr = plugin->setting->descr;

      /* fill text */
      if (plugin->instantiation_failed)
        {
          snprintf (txt, MAX_LEN, "(!) %s", descr->name);
        }
      else
        {
          strncpy (txt, descr->name, MAX_LEN);
          txt[MAX_LEN - 1] = '\0';
        }
      int w, h;
      z_cairo_get_text_extents_for_widget (
        widget, self->txt_layout, txt, &w, &h);
      pango_layout_set_markup (self->txt_layout, txt, -1);
      gtk_snapshot_save (snapshot);
      {
        graphene_point_t tmp_pt = GRAPHENE_POINT_INIT (
          (float) btn_width + (float) (width - btn_width) / 2.f - (float) w / 2.f,
          (float) height / 2.f - (float) h / 2.f);
        gtk_snapshot_translate (snapshot, &tmp_pt);
      }
      GdkRGBA tmp_color = Z_GDK_RGBA_INIT (1, 1, 1, 1);
      gtk_snapshot_append_layout (snapshot, self->txt_layout, &tmp_color);
      gtk_snapshot_restore (snapshot);

      /* update tooltip */
      if (!self->pl_name || !g_strcmp0 (descr->name, self->pl_name))
        {
          if (self->pl_name)
            g_free (self->pl_name);
          self->pl_name = g_strdup (descr->name);
          gtk_widget_set_tooltip_text (widget, self->pl_name);
        }
    }
  else /* else if no plugin */
    {
      /* fill text */
      int w, h;
      if (self->type == ZPluginSlotType::Z_PLUGIN_SLOT_INSTRUMENT)
        {
          snprintf (txt, MAX_LEN, "%s", _ ("No instrument"));
        }
      else
        {
          snprintf (txt, MAX_LEN, _ ("Slot #%d"), self->slot_index + 1);
        }
      z_cairo_get_text_extents_for_widget (
        widget, self->txt_layout, txt, &w, &h);
      pango_layout_set_markup (self->txt_layout, txt, -1);
      gtk_snapshot_save (snapshot);
      {
        graphene_point_t tmp_pt = GRAPHENE_POINT_INIT (
          (float) btn_width + (float) (width - btn_width) / 2.f - (float) w / 2.f,
          (float) height / 2.f - (float) h / 2.f);
        gtk_snapshot_translate (snapshot, &tmp_pt);
      }
      GdkRGBA tmp_color = Z_GDK_RGBA_INIT (1, 1, 1, 0.55);
      gtk_snapshot_append_layout (snapshot, self->txt_layout, &tmp_color);
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

  GTK_WIDGET_CLASS (GTK_WIDGET_GET_CLASS (self->activate_btn))
    ->snapshot (GTK_WIDGET (self), snapshot);
}

static gboolean
on_dnd_drop (
  GtkDropTarget * drop_target,
  const GValue *  value,
  double          x,
  double          y,
  gpointer        data)
{
  if (!G_VALUE_HOLDS (value, WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE))
    {
      g_message ("invalid DND type");
      return false;
    }

  ChannelSlotWidget * self = Z_CHANNEL_SLOT_WIDGET (data);

  GdkDragAction action = z_gtk_drop_target_get_selected_action (drop_target);

  g_debug (
    "channel slot: dnd drop %s",
    action == GDK_ACTION_COPY ? "COPY"
    : action == GDK_ACTION_MOVE
      ? "MOVE"
      : "LINK");

  WrappedObjectWithChangeSignal * wrapped_obj =
    static_cast<WrappedObjectWithChangeSignal *> (g_value_get_object (value));
  Plugin *           pl = NULL;
  PluginDescriptor * descr = NULL;
  if (wrapped_obj->type == WrappedObjectType::WRAPPED_OBJECT_TYPE_PLUGIN_DESCR)
    {
      descr = (PluginDescriptor *) wrapped_obj->obj;
    }
  else if (wrapped_obj->type == WrappedObjectType::WRAPPED_OBJECT_TYPE_PLUGIN)
    {
      pl = (Plugin *) wrapped_obj->obj;
    }

  channel_handle_plugin_import (
    self->track->channel, pl, MIXER_SELECTIONS, descr, self->slot_index,
    self->type, action == GDK_ACTION_COPY, true);

  gtk_widget_queue_draw (GTK_WIDGET (self));

  return true;
}

/**
 * Control not pressed, no plugin exists, not same channel */
static inline void
select_no_ctrl_no_pl_no_ch (ChannelSlotWidget * self, bool fire_events)
{
  mixer_selections_clear (MIXER_SELECTIONS, fire_events);
}

/**
 * Control not pressed, no plugin exists, same channel */
static inline void
select_no_ctrl_no_pl_ch (ChannelSlotWidget * self, bool fire_events)
{
  mixer_selections_clear (MIXER_SELECTIONS, fire_events);
}

/**
 * Control not pressed, plugin exists, not same channel */
static inline void
select_no_ctrl_pl_no_ch (ChannelSlotWidget * self, bool fire_events)
{
  mixer_selections_clear (MIXER_SELECTIONS, F_NO_PUBLISH_EVENTS);

  mixer_selections_add_slot (
    MIXER_SELECTIONS, self->track, self->type, self->slot_index, F_NO_CLONE,
    fire_events);
}

/**
 * Control not pressed, plugin exists, same channel */
static inline void
select_no_ctrl_pl_ch (ChannelSlotWidget * self, bool fire_events)
{
  /* if plugin is not selected, make it the only
   * selection otherwise do nothing */
  if (!mixer_selections_contains_slot (
        MIXER_SELECTIONS, self->type, self->slot_index))
    {
      mixer_selections_clear (MIXER_SELECTIONS, F_NO_PUBLISH_EVENTS);

      mixer_selections_add_slot (
        MIXER_SELECTIONS, self->track, self->type, self->slot_index, F_NO_CLONE,
        fire_events);
    }
}

/**
 * Control pressed, no plugin exists, not same channel */
static inline void
select_ctrl_no_pl_no_ch (ChannelSlotWidget * self, bool fire_events)
{
  mixer_selections_clear (MIXER_SELECTIONS, fire_events);
}

/**
 * Control pressed, no plugin exists, same channel */
static inline void
select_ctrl_no_pl_ch (ChannelSlotWidget * self, bool fire_events)
{
  mixer_selections_clear (MIXER_SELECTIONS, fire_events);
}

/**
 * Control pressed, plugin exists, not same channel */
static inline void
select_ctrl_pl_no_ch (ChannelSlotWidget * self, bool fire_events)
{
  /* make it the only selection */
  mixer_selections_clear (MIXER_SELECTIONS, F_NO_PUBLISH_EVENTS);
  mixer_selections_add_slot (
    MIXER_SELECTIONS, self->track, self->type, self->slot_index, F_NO_CLONE,
    fire_events);
}

/**
 * Control pressed, plugin exists, same channel */
static inline void
select_ctrl_pl_ch (ChannelSlotWidget * self, bool fire_events)
{
  /* if already selected, deselect it, otherwise
   * add it to selections */
  if (mixer_selections_contains_slot (
        MIXER_SELECTIONS, self->type, self->slot_index))
    {
      mixer_selections_remove_slot (
        MIXER_SELECTIONS, self->slot_index, self->type, fire_events);
      /*self->deselected = 1;*/
    }
  else
    {
      mixer_selections_add_slot (
        MIXER_SELECTIONS, self->track, self->type, self->slot_index, F_NO_CLONE,
        fire_events);
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
select_plugin (ChannelSlotWidget * self, bool ctrl, bool fire_events)
{
  bool pl = false, ch = false;

  /* if plugin exists */
  Plugin * plugin = channel_slot_widget_get_plugin (self);
  if (plugin)
    pl = true;

  /* if same channel as selections */
  g_return_if_fail (self->track);
  if (
    self->track->channel
    && track_get_name_hash (self->track) == MIXER_SELECTIONS->track_name_hash)
    ch = true;

  if (!ctrl && !pl && !ch)
    select_no_ctrl_no_pl_no_ch (self, fire_events);
  else if (!ctrl && !pl && ch)
    select_no_ctrl_no_pl_ch (self, fire_events);
  else if (!ctrl && pl && !ch)
    select_no_ctrl_pl_no_ch (self, fire_events);
  else if (!ctrl && pl && ch)
    select_no_ctrl_pl_ch (self, fire_events);
  else if (ctrl && !pl && !ch)
    select_ctrl_no_pl_no_ch (self, fire_events);
  else if (ctrl && !pl && ch)
    select_ctrl_no_pl_ch (self, fire_events);
  else if (ctrl && pl && !ch)
    select_ctrl_pl_no_ch (self, fire_events);
  else if (ctrl && pl && ch)
    select_ctrl_pl_ch (self, fire_events);

  /* select channel */
  if (self->track)
    {
      tracklist_selections_select_single (
        TRACKLIST_SELECTIONS, self->track, fire_events);
    }
}

static void
drag_end (
  GtkGestureDrag *    gesture,
  gdouble             offset_x,
  gdouble             offset_y,
  ChannelSlotWidget * self)
{
  GdkModifierType state = gtk_event_controller_get_current_event_state (
    GTK_EVENT_CONTROLLER (gesture));

  Plugin * pl = channel_slot_widget_get_plugin (self);
  if (pl && self->n_press == 2)
    {
      bool new_visible = !pl->visible;
      g_message (
        "%s: setting plugin %s visible %d", __func__, pl->setting->descr->name,
        new_visible);
      g_warn_if_fail (pl->instantiated);
      pl->visible = new_visible;
      EVENTS_PUSH (EventType::ET_PLUGIN_VISIBILITY_CHANGED, pl);
    }
  else if (self->n_press == 1)
    {
      int ctrl = 0;
      /* if control click */
      if (state & GDK_CONTROL_MASK)
        ctrl = 1;

      select_plugin (self, ctrl, F_PUBLISH_EVENTS);

      /*self->deselected = 0;*/
      /*self->reselected = 0;*/
    }

  if (pl)
    {
      g_return_if_fail (self->track);
      g_return_if_fail (self->track->channel);
      g_return_if_fail (self->track->channel->widget);
      self->track->channel->widget->last_plugin_press = g_get_monotonic_time ();
    }
  g_message ("%s: drag end %d press", __func__, self->n_press);
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
    && self->type != ZPluginSlotType::Z_PLUGIN_SLOT_INSTRUMENT)
    {
      if (self->type == ZPluginSlotType::Z_PLUGIN_SLOT_INSERT)
        {
          PROJECT->last_selection =
            ZProjectSelectionType::Z_PROJECT_SELECTION_TYPE_INSERT;
        }
      else if (self->type == ZPluginSlotType::Z_PLUGIN_SLOT_MIDI_FX)
        {
          PROJECT->last_selection =
            ZProjectSelectionType::Z_PROJECT_SELECTION_TYPE_MIDI_FX;
        }
      EVENTS_PUSH (EventType::ET_PROJECT_SELECTION_TYPE_CHANGED, NULL);
    }
}

static bool
tick_cb (GtkWidget * widget, GdkFrameClock * frame_clock, ChannelSlotWidget * self)
{
  if (!gtk_widget_is_visible (widget))
    {
      return G_SOURCE_CONTINUE;
    }

  Plugin * pl = channel_slot_widget_get_plugin (self);
  if (pl)
    {
      gtk_widget_set_sensitive (GTK_WIDGET (self->activate_btn), true);
      if (
        gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->activate_btn))
        != plugin_is_enabled (pl, false))
        {
          g_signal_handler_block (
            self->activate_btn, self->activate_btn->toggled_id);
          gtk_toggle_button_set_active (
            GTK_TOGGLE_BUTTON (self->activate_btn),
            plugin_is_enabled (pl, false));
          g_signal_handler_unblock (
            self->activate_btn, self->activate_btn->toggled_id);
        }
    }
  else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->activate_btn)))
    {
      /* disable toggle button if no plugin */
      g_signal_handler_block (
        self->activate_btn, self->activate_btn->toggled_id);
      gtk_toggle_button_set_active (
        GTK_TOGGLE_BUTTON (self->activate_btn), false);
      g_signal_handler_unblock (
        self->activate_btn, self->activate_btn->toggled_id);
      gtk_widget_set_sensitive (GTK_WIDGET (self->activate_btn), false);
    }

  Plugin * plugin = channel_slot_widget_get_plugin (self);
  bool     empty = plugin == NULL;
  self->was_empty = empty;
  gtk_widget_remove_css_class (GTK_WIDGET (self), "empty");
  gtk_widget_remove_css_class (GTK_WIDGET (self), "disabled");
  gtk_widget_unset_state_flags (
    GTK_WIDGET (self),
    static_cast<GtkStateFlags> (
      static_cast<int> (GTK_STATE_FLAG_SELECTED)
      | static_cast<int> (GTK_STATE_FLAG_CHECKED)));
  if (plugin)
    {
      bool is_selected = plugin_is_selected (plugin);
      if (is_selected)
        {
          gtk_widget_set_state_flags (
            GTK_WIDGET (self), GTK_STATE_FLAG_SELECTED, false);
        }
      if (!plugin_is_enabled (plugin, false))
        {
          gtk_widget_add_css_class (GTK_WIDGET (self), "disabled");
        }
      if (plugin->visible)
        {
          gtk_widget_set_state_flags (
            GTK_WIDGET (self), GTK_STATE_FLAG_CHECKED, false);
        }
    }
  else
    {
      gtk_widget_add_css_class (GTK_WIDGET (self), "empty");
    }

  gtk_widget_set_visible (
    GTK_WIDGET (self->bridge_icon),
    plugin
      && plugin->setting->bridge_mode != ZCarlaBridgeMode::Z_CARLA_BRIDGE_NONE);
  if (plugin)
    {
      switch (plugin->setting->bridge_mode)
        {
        case ZCarlaBridgeMode::Z_CARLA_BRIDGE_FULL:
          gtk_image_set_from_icon_name (self->bridge_icon, "css.gg-remote");
          gtk_widget_set_tooltip_text (
            GTK_WIDGET (self->bridge_icon), _ ("Bridged (Full)"));
          break;
        case ZCarlaBridgeMode::Z_CARLA_BRIDGE_UI:
          gtk_image_set_from_icon_name (
            self->bridge_icon, "material-design-remote-desktop");
          gtk_widget_set_tooltip_text (
            GTK_WIDGET (self->bridge_icon), _ ("Bridged (UI)"));
          break;
        case ZCarlaBridgeMode::Z_CARLA_BRIDGE_NONE:
          break;
        }
    }

  return G_SOURCE_CONTINUE;
}

static void
show_context_menu (ChannelSlotWidget * self, double x, double y)
{
  MW_MIXER->paste_slot = self;

  switch (self->type)
    {
    case ZPluginSlotType::Z_PLUGIN_SLOT_INSERT:
      PROJECT->last_selection =
        ZProjectSelectionType::Z_PROJECT_SELECTION_TYPE_INSERT;
      break;
    case ZPluginSlotType::Z_PLUGIN_SLOT_INSTRUMENT:
      PROJECT->last_selection =
        ZProjectSelectionType::Z_PROJECT_SELECTION_TYPE_INSTRUMENT;
      break;
    case ZPluginSlotType::Z_PLUGIN_SLOT_MIDI_FX:
      PROJECT->last_selection =
        ZProjectSelectionType::Z_PROJECT_SELECTION_TYPE_MIDI_FX;
      break;
    case ZPluginSlotType::Z_PLUGIN_SLOT_MODULATOR:
      PROJECT->last_selection =
        ZProjectSelectionType::Z_PROJECT_SELECTION_TYPE_MODULATOR;
      break;
    default:
      g_return_if_reached ();
      break;
    }
  EVENTS_PUSH (EventType::ET_PROJECT_SELECTION_TYPE_CHANGED, NULL);

  GMenu *     menu = g_menu_new ();
  GMenuItem * menuitem;

  Plugin * pl = channel_slot_widget_get_plugin (self);

  if (pl)
    {
      GMenu * plugin_submenu = g_menu_new ();

      /* bypass */
      char tmp[500];
      sprintf (tmp, "app.plugin-toggle-enabled::%p", pl);
      g_menu_append (plugin_submenu, _ ("Bypass"), tmp);

      /* inspect */
      g_menu_append (plugin_submenu, _ ("Inspect"), "app.plugin-inspect");

      /* change load behavior */
      GMenu * load_behavior_menu = g_menu_new ();
      sprintf (tmp, "app.plugin-change-load-behavior::%p,%s", pl, "normal");
      g_menu_append (load_behavior_menu, _ ("Normal"), tmp);
      sprintf (tmp, "app.plugin-change-load-behavior::%p,%s", pl, "ui");
      g_menu_append (load_behavior_menu, _ ("Bridge UI"), tmp);
      sprintf (tmp, "app.plugin-change-load-behavior::%p,%s", pl, "full");
      g_menu_append (load_behavior_menu, _ ("Bridge Full"), tmp);
      g_menu_append_submenu (
        plugin_submenu, _ ("Change Load Behavior"),
        G_MENU_MODEL (load_behavior_menu));

      g_menu_append_section (menu, _ ("Plugin"), G_MENU_MODEL (plugin_submenu));
    }

  GMenu * edit_submenu = g_menu_new ();
  if (self->type != ZPluginSlotType::Z_PLUGIN_SLOT_INSTRUMENT)
    {
      menuitem = CREATE_CUT_MENU_ITEM ("app.cut");
      g_menu_append_item (edit_submenu, menuitem);
      menuitem = CREATE_COPY_MENU_ITEM ("app.copy");
      g_menu_append_item (edit_submenu, menuitem);
      menuitem = CREATE_PASTE_MENU_ITEM ("app.paste");
      g_menu_append_item (edit_submenu, menuitem);
    }

  /* if plugin exists */
  if (pl && self->type != ZPluginSlotType::Z_PLUGIN_SLOT_INSTRUMENT)
    {
      /* add delete item */
      menuitem = CREATE_DELETE_MENU_ITEM ("app.mixer-selections-delete");
      g_menu_append_item (edit_submenu, menuitem);
    }

  g_menu_append_section (menu, NULL, G_MENU_MODEL (edit_submenu));

  if (self->type != ZPluginSlotType::Z_PLUGIN_SLOT_INSTRUMENT)
    {
      GMenu * select_submenu = g_menu_new ();

      menuitem = CREATE_CLEAR_SELECTION_MENU_ITEM ("app.clear-selection");
      g_menu_append_item (select_submenu, menuitem);
      menuitem = CREATE_SELECT_ALL_MENU_ITEM ("app.select-all");
      g_menu_append_item (select_submenu, menuitem);

      g_menu_append_section (menu, NULL, G_MENU_MODEL (select_submenu));
    }

  z_gtk_show_context_menu_from_g_menu (self->popover_menu, x, y, menu);
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

  GdkModifierType state = gtk_event_controller_get_current_event_state (
    GTK_EVENT_CONTROLLER (gesture));

  select_plugin (self, state & GDK_CONTROL_MASK, F_NO_PUBLISH_EVENTS);

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
  ChannelSlotWidget * self = Z_CHANNEL_SLOT_WIDGET (user_data);
  gtk_widget_set_state_flags (GTK_WIDGET (self), GTK_STATE_FLAG_PRELIGHT, 0);
}

static void
on_motion_leave (GtkEventControllerMotion * motion_controller, gpointer user_data)
{
  ChannelSlotWidget * self = Z_CHANNEL_SLOT_WIDGET (user_data);
  gtk_widget_unset_state_flags (GTK_WIDGET (self), GTK_STATE_FLAG_PRELIGHT);
}

static void
finalize (ChannelSlotWidget * self)
{
  if (self->pl_name)
    g_free (self->pl_name);
  if (self->txt_layout)
    g_object_unref (self->txt_layout);

  G_OBJECT_CLASS (channel_slot_widget_parent_class)->finalize (G_OBJECT (self));
}

void
channel_slot_widget_set_instrument (ChannelSlotWidget * self, Track * track)
{
  self->track = track;
}

static GdkContentProvider *
on_dnd_drag_prepare (
  GtkDragSource *     source,
  double              x,
  double              y,
  ChannelSlotWidget * self)
{
  Plugin *                        pl = channel_slot_widget_get_plugin (self);
  WrappedObjectWithChangeSignal * wrapped_obj =
    wrapped_object_with_change_signal_new (
      pl, WrappedObjectType::WRAPPED_OBJECT_TYPE_PLUGIN);
  GdkContentProvider * content_providers[] = {
    gdk_content_provider_new_typed (
      WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE, wrapped_obj),
  };

  return gdk_content_provider_new_union (
    content_providers, G_N_ELEMENTS (content_providers));
}

static void
setup_dnd (ChannelSlotWidget * self)
{
  if (self->type != ZPluginSlotType::Z_PLUGIN_SLOT_INSTRUMENT)
    {
      /* set as drag source for plugin */
      GtkDragSource * drag_source = gtk_drag_source_new ();
      gtk_drag_source_set_actions (
        drag_source,
        static_cast<GdkDragAction> (
          static_cast<int> (GDK_ACTION_COPY)
          | static_cast<int> (GDK_ACTION_MOVE)));
      g_signal_connect (
        drag_source, "prepare", G_CALLBACK (on_dnd_drag_prepare), self);
      gtk_widget_add_controller (
        GTK_WIDGET (self), GTK_EVENT_CONTROLLER (drag_source));
    }

  /* set as drag dest for both plugins and
   * plugin descriptors */
  GtkDropTarget * drop_target = gtk_drop_target_new (
    G_TYPE_INVALID,
    static_cast<GdkDragAction> (
      static_cast<int> (GDK_ACTION_MOVE) | static_cast<int> (GDK_ACTION_COPY)));
  gtk_drop_target_set_preload (drop_target, true);
  GType types[] = { WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE };
  gtk_drop_target_set_gtypes (drop_target, types, G_N_ELEMENTS (types));
  g_signal_connect (drop_target, "drop", G_CALLBACK (on_dnd_drop), self);
  g_signal_connect (drop_target, "motion", G_CALLBACK (on_dnd_motion), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (drop_target));
}

/**
 * Creates a new ChannelSlot widget and binds it to
 * the given value.
 */
ChannelSlotWidget *
channel_slot_widget_new (
  int             slot_index,
  Track *         track,
  ZPluginSlotType type,
  bool            open_plugin_inspector_on_click)
{
  ChannelSlotWidget * self = static_cast<ChannelSlotWidget *> (
    g_object_new (CHANNEL_SLOT_WIDGET_TYPE, NULL));
  self->slot_index = slot_index;
  self->type = type;
  self->track = track;
  self->open_plugin_inspector_on_click = open_plugin_inspector_on_click;

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
  ChannelSlotWidget * self = channel_slot_widget_new (
    -1, NULL, ZPluginSlotType::Z_PLUGIN_SLOT_INSTRUMENT, false);

  return self;
}

static void
on_css_changed (GtkWidget * widget, GtkCssStyleChange * change)
{
  ChannelSlotWidget * self = Z_CHANNEL_SLOT_WIDGET (widget);
  GTK_WIDGET_CLASS (channel_slot_widget_parent_class)
    ->css_changed (widget, change);
  update_pango_layouts (self, true);
}

static void
dispose (ChannelSlotWidget * self)
{
  gtk_widget_unparent (GTK_WIDGET (self->popover_menu));
  gtk_widget_unparent (GTK_WIDGET (self->activate_btn));
  gtk_widget_unparent (GTK_WIDGET (self->bridge_icon));

  G_OBJECT_CLASS (channel_slot_widget_parent_class)->dispose (G_OBJECT (self));
}

static void
channel_slot_widget_init (ChannelSlotWidget * self)
{
  gtk_widget_set_size_request (GTK_WIDGET (self), -1, 20);

  gtk_widget_set_hexpand (GTK_WIDGET (self), true);
  gtk_widget_set_focusable (GTK_WIDGET (self), true);
  gtk_widget_set_focus_on_click (GTK_WIDGET (self), true);

  /* this is needed for border radius to take effect */
  gtk_widget_set_overflow (GTK_WIDGET (self), GTK_OVERFLOW_HIDDEN);

  self->pl_name = NULL;
  gtk_widget_set_tooltip_text (GTK_WIDGET (self), _ ("empty slot"));

  self->popover_menu = GTK_POPOVER_MENU (gtk_popover_menu_new_from_model (NULL));
  gtk_widget_set_parent (GTK_WIDGET (self->popover_menu), GTK_WIDGET (self));

  self->click = GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  gtk_event_controller_set_propagation_phase (
    GTK_EVENT_CONTROLLER (self->click), GTK_PHASE_CAPTURE);
  g_signal_connect (
    G_OBJECT (self->click), "pressed", G_CALLBACK (on_press), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (self->click));

  self->right_mouse_mp = GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (self->right_mouse_mp), GDK_BUTTON_SECONDARY);
  g_signal_connect (
    G_OBJECT (self->right_mouse_mp), "released", G_CALLBACK (on_right_click),
    self);
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (self->right_mouse_mp));

  self->drag = GTK_GESTURE_DRAG (gtk_gesture_drag_new ());
  gtk_event_controller_set_propagation_phase (
    GTK_EVENT_CONTROLLER (self->drag), GTK_PHASE_CAPTURE);
  g_signal_connect (
    G_OBJECT (self->drag), "drag-end", G_CALLBACK (drag_end), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (self->drag));

  GtkEventController * motion_controller = gtk_event_controller_motion_new ();
  g_signal_connect (
    G_OBJECT (motion_controller), "enter", G_CALLBACK (on_motion_enter), self);
  g_signal_connect (
    G_OBJECT (motion_controller), "leave", G_CALLBACK (on_motion_leave), self);
  gtk_widget_add_controller (GTK_WIDGET (self), motion_controller);

  self->activate_btn = channel_slot_activate_button_widget_new (self);
  gtk_widget_set_parent (GTK_WIDGET (self->activate_btn), GTK_WIDGET (self));

  gtk_accessible_update_property (
    GTK_ACCESSIBLE (self), GTK_ACCESSIBLE_PROPERTY_LABEL, "Channel slot", -1);

  gtk_widget_add_tick_callback (
    GTK_WIDGET (self), (GtkTickCallback) tick_cb, self, NULL);

  self->bridge_icon =
    GTK_IMAGE (gtk_image_new_from_icon_name ("iconoir-bridge3d"));
  gtk_widget_set_parent (GTK_WIDGET (self->bridge_icon), GTK_WIDGET (self));
  gtk_widget_set_halign (GTK_WIDGET (self->bridge_icon), GTK_ALIGN_END);
  gtk_widget_set_valign (GTK_WIDGET (self->bridge_icon), GTK_ALIGN_END);
  gtk_widget_set_hexpand (GTK_WIDGET (self->bridge_icon), true);
  gtk_image_set_pixel_size (self->bridge_icon, 16);
  gtk_widget_set_visible (GTK_WIDGET (self->bridge_icon), false);
}

static void
channel_slot_widget_class_init (ChannelSlotWidgetClass * klass)
{
  GtkWidgetClass * wklass = GTK_WIDGET_CLASS (klass);
  wklass->snapshot = channel_slot_snapshot;
  wklass->css_changed = on_css_changed;
  gtk_widget_class_set_css_name (wklass, "channel-slot");
  gtk_widget_class_set_accessible_role (wklass, GTK_ACCESSIBLE_ROLE_GROUP);

  gtk_widget_class_set_layout_manager_type (wklass, GTK_TYPE_BOX_LAYOUT);

  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->finalize = (GObjectFinalizeFunc) finalize;
  oklass->dispose = (GObjectFinalizeFunc) dispose;
}
