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
#include "audio/audio_bus_track.h"
#include "audio/audio_group_track.h"
#include "audio/master_track.h"
#include "audio/instrument_track.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "audio/region.h"
#include "gui/backend/tracklist_selections.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/custom_button.h"
#include "gui/widgets/editable_label.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_activity_bar.h"
#include "gui/widgets/mixer.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_bg.h"
#include "gui/widgets/timeline_panel.h"
#include "gui/widgets/track.h"
#include "gui/widgets/track_top_grid.h"
#include "gui/widgets/tracklist.h"
#include "project.h"
#include "utils/cairo.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/resources.h"
#include "utils/ui.h"

#include <gtk/gtk.h>

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  TrackWidget, track_widget, GTK_TYPE_BOX)

#define ICON_NAME_RECORD "z-media-record"
#define ICON_NAME_SOLO "solo"
#define ICON_NAME_MUTE "mute"
#define ICON_NAME_SHOW_UI "instrument"
#define ICON_NAME_SHOW_AUTOMATION_LANES \
  "z-node-type-cusp"
#define ICON_NAME_SHOW_TRACK_LANES \
  "z-format-justify-fill"
#define ICON_NAME_LOCK "z-object-unlocked"
#define ICON_NAME_FREEZE "snowflake-o"

static void
draw_color_area (
  TrackWidget * self,
  cairo_t *     cr,
  int           width,
  int           height)
{
  cairo_surface_t * surface =
    z_cairo_get_surface_from_icon_name (
      self->icon_name, 16, 0);

  gdk_cairo_set_source_rgba (
    cr, &self->track->color);
  cairo_rectangle (
    cr, 0, 0, 18, height);
  cairo_fill (cr);

  GdkRGBA c2, c3;
  ui_get_contrast_color (
    &self->track->color, &c2);
  ui_get_contrast_color (
    &c2, &c3);

  /* add shadow in the back */
  cairo_set_source_rgba (
    cr, c3.red,
    c3.green, c3.blue, 0.4);
  cairo_mask_surface(
    cr, surface, 2, 2);
  cairo_fill(cr);

  /* add main icon */
  cairo_set_source_rgba (
    cr, c2.red, c2.green, c2.blue, 1);
  /*cairo_set_source_surface (*/
    /*self->cached_cr, surface, 1, 1);*/
  cairo_mask_surface(
    self->cached_cr, surface, 1, 1);
  cairo_fill (self->cached_cr);
}

static int
track_draw_cb (
  GtkWidget *   widget,
  cairo_t *     cr,
  TrackWidget * self)
{
  if (self->redraw)
    {
      GtkStyleContext *context =
        gtk_widget_get_style_context (widget);

      int width =
        gtk_widget_get_allocated_width (widget);
      int height =
        gtk_widget_get_allocated_height (widget);

      z_cairo_reset_caches (
        &self->cached_cr,
        &self->cached_surface, width,
        height, cr);

      gtk_render_background (
        context, self->cached_cr, 0, 0,
        width, height);

      draw_color_area (self, cr, width, height);

      self->redraw = 0;
    }

  cairo_set_source_surface (
    cr, self->cached_surface, 0, 0);
  cairo_paint (cr);

  return FALSE;
}

static gboolean
on_motion (
  GtkWidget *      widget,
  GdkEventMotion * event,
  TrackWidget *    self)
{
  int height =
    gtk_widget_get_allocated_height (widget);

  /* show resize cursor */
  g_message ("y %f height %d",
             event->y, height);
  if (self->bg_hovered)
    {
      if (height - event->y < 12)
        {
          self->resize = 1;
          ui_set_cursor_from_name (
            widget, "s-resize");
        }
      else
        {
          self->resize = 0;
          ui_set_pointer_cursor (widget);
        }
    }

  if (event->type == GDK_ENTER_NOTIFY)
    {
      g_message ("enter");
      self->bg_hovered = 1;
      self->resize = 0;
    }
  else if (event->type == GDK_LEAVE_NOTIFY)
    {
      g_message ("leave");
      ui_set_pointer_cursor (widget);
      self->bg_hovered = 0;
      self->resize = 0;
    }
  else
    {
      self->bg_hovered = 1;
    }

  return FALSE;
}

/**
 * Wrapper.
 */
void
track_widget_force_redraw (
  TrackWidget * self)
{
  g_return_if_fail (self);
  self->redraw = 1;
  /*gtk_widget_queue_draw (*/
    /*(GtkWidget *) self->drawing_area);*/
}

/**
 * Returns if cursor is in top half of the track.
 *
 * Used by timeline to determine if it will select
 * objects or range.
 */
int
track_widget_is_cursor_in_top_half (
  TrackWidget * self,
  double        y)
{
  gint wx, wy;
  gtk_widget_translate_coordinates (
    GTK_WIDGET (MW_TIMELINE),
    GTK_WIDGET (self),
    0,
    (int) y,
    &wx,
    &wy);

  /* if bot half */
  if (wy >= self->track->main_height / 2 &&
      wy <= self->track->main_height)
    {
      return 0;
    }
  else /* if top half */
    {
      return 1;
    }
}

static TrackLane *
get_lane_at_y (
  TrackWidget * self,
  double        y)
{
  Track * track = self->track;

  if (!track->lanes_visible)
    return NULL;

  TrackLane * lane = NULL;
  for (int i = 0; i < track->num_lanes; i++)
    {
      lane = track->lanes[i];

      g_message ("checking %d", i);
      if (lane->widget &&
          ui_is_child_hit (
            GTK_WIDGET (self),
            GTK_WIDGET (lane->widget),
            0, 1, 0, y, 0, 0))
        {
          return lane;
        }
    }

  return NULL;
}

/**
 * Info to pass when selecting a MIDI channel from
 * the context menu.
 */
typedef struct MidiChSelectionInfo
{
  /** Either one of these should be set. */
  Track *     track;
  TrackLane * lane;

  /** MIDI channel (1-16), or 0 for lane to
   * inherit. */
  midi_byte_t ch;
} MidiChSelectionInfo;

static void
on_midi_ch_selected (
  GtkMenuItem *         menu_item,
  MidiChSelectionInfo * info)
{
  if (info->lane)
    {
      info->lane->midi_ch = info->ch;
    }
  if (info->track)
    {
      info->track->midi_ch = info->ch;
    }
  free (info);
}

static void
show_context_menu (
  TrackWidget * self,
  double        y)
{
  GtkWidget *menu;
  GtkMenuItem *menuitem;
  menu = gtk_menu_new();
  Track * track = self->track;
  TrackLane * lane =
    get_lane_at_y (self, y);

#define APPEND(mi) \
  gtk_menu_shell_append ( \
    GTK_MENU_SHELL (menu), \
    GTK_WIDGET (menuitem));

  int num_selected =
    TRACKLIST_SELECTIONS->num_tracks;

  if (num_selected > 0)
    {
      char * str;

      if (track->type != TRACK_TYPE_MASTER &&
          track->type != TRACK_TYPE_CHORD &&
          track->type != TRACK_TYPE_MARKER)
        {
          /* delete track */
          if (num_selected == 1)
            str =
              g_strdup (_("_Delete Track"));
          else
            str =
              g_strdup (_("_Delete Tracks"));
          menuitem =
            z_gtk_create_menu_item (
              str,
              "z-delete",
              0,
              NULL,
              0,
              "win.delete-selected-tracks");
          g_free (str);
          APPEND (menuitem);

          /* duplicate track */
          if (num_selected == 1)
            str =
              g_strdup (_("_Duplicate Track"));
          else
            str =
              g_strdup (_("_Duplicate Tracks"));
          menuitem =
            z_gtk_create_menu_item (
              str,
              "z-edit-duplicate",
              0,
              NULL,
              0,
              "win.duplicate-selected-tracks");
          g_free (str);
          APPEND (menuitem);
        }

      /* add regions */
      if (track->type == TRACK_TYPE_INSTRUMENT)
        {
          menuitem =
            z_gtk_create_menu_item (
              _("Add Region"),
              "z-gtk-add",
              0,
              NULL,
              0,
              "win.duplicate-selected-tracks");
          APPEND (menuitem);
        }

      menuitem =
        z_gtk_create_menu_item (
          num_selected == 1 ?
            _("Hide Track") :
            _("Hide Tracks"),
          "z-gnumeric-column-hide",
          0,
          NULL,
          0,
          "win.hide-selected-tracks");
      APPEND (menuitem);

      menuitem =
        z_gtk_create_menu_item (
          num_selected == 1 ?
            _("Pin/Unpin Track") :
            _("Pin/Unpin Tracks"),
          "z-window-pin",
          0,
          NULL,
          0,
          "win.pin-selected-tracks");
      APPEND (menuitem);
    }

  /* add midi channel selectors */
  if (track_has_piano_roll (track))
    {
      menuitem =
        GTK_MENU_ITEM (
          gtk_menu_item_new_with_label (
            _("Track MIDI Ch")));

      GtkMenu * submenu =
        GTK_MENU (gtk_menu_new ());
      gtk_widget_set_visible (
        GTK_WIDGET (submenu), 1);
      GtkMenuItem * submenu_item;
      for (int i = 1; i <= 16; i++)
        {
          char * lbl =
            g_strdup_printf (
              _("%sMIDI Channel %d"),
              i == track->midi_ch ? "* " : "",
              i);
          submenu_item =
            GTK_MENU_ITEM (
              gtk_menu_item_new_with_label (lbl));
          g_free (lbl);

          MidiChSelectionInfo * info =
            calloc (
              1, sizeof (MidiChSelectionInfo));
          info->track = track;
          info->ch = (midi_byte_t) i;
          g_signal_connect (
            G_OBJECT (submenu_item), "activate",
            G_CALLBACK (on_midi_ch_selected),
            info);

          gtk_menu_shell_append (
            GTK_MENU_SHELL (submenu),
            GTK_WIDGET (submenu_item));
          gtk_widget_set_visible (
            GTK_WIDGET (submenu_item), 1);
        }

      gtk_menu_item_set_submenu (
        menuitem, GTK_WIDGET (submenu));
      gtk_widget_set_visible (
        GTK_WIDGET (menuitem), 1);

      APPEND (menuitem);

      if (lane)
        {
          char * lbl =
            g_strdup_printf (
              _("Lane %d MIDI Ch"),
              lane->pos);
          menuitem =
            GTK_MENU_ITEM (
              gtk_menu_item_new_with_label (
                lbl));
          g_free (lbl);

          submenu =
            GTK_MENU (gtk_menu_new ());
          gtk_widget_set_visible (
            GTK_WIDGET (submenu), 1);
          for (int i = 0; i <= 16; i++)
            {
              if (i == 0)
                lbl =
                  g_strdup_printf (
                    _("%sInherit"),
                    lane->midi_ch == i ? "* " : "");
              else
                lbl =
                  g_strdup_printf (
                    _("%sMIDI Channel %d"),
                    lane->midi_ch == i ? "* " : "",
                    i);
              submenu_item =
                GTK_MENU_ITEM (
                  gtk_menu_item_new_with_label (
                    lbl));
              g_free (lbl);

              MidiChSelectionInfo * info =
                calloc (
                  1, sizeof (MidiChSelectionInfo));
              info->lane = lane;
              info->ch = (midi_byte_t) i;
              g_signal_connect (
                G_OBJECT (submenu_item), "activate",
                G_CALLBACK (on_midi_ch_selected),
                info);

              gtk_menu_shell_append (
                GTK_MENU_SHELL (submenu),
                GTK_WIDGET (submenu_item));
              gtk_widget_set_visible (
                GTK_WIDGET (submenu_item), 1);
            }

          gtk_menu_item_set_submenu (
            menuitem, GTK_WIDGET (submenu));
          gtk_widget_set_visible (
            GTK_WIDGET (menuitem), 1);

          APPEND (menuitem);
        }
    }


#undef APPEND

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
  TrackWidget *         self)
{

  GdkModifierType state_mask =
    ui_get_state_mask (GTK_GESTURE (gesture));

  Track * track = self->track;
  if (!track_is_selected (track))
    {
      if (state_mask & GDK_SHIFT_MASK ||
          state_mask & GDK_CONTROL_MASK)
        {
          track_select (
            track, F_SELECT, 0, 1);
        }
      else
        {
          track_select (
            track, F_SELECT, 1, 1);
        }
    }
  if (n_press == 1)
    {
      show_context_menu (self, y);
    }
}

static void
multipress_pressed (
  GtkGestureMultiPress *gesture,
  gint                  n_press,
  gdouble               x,
  gdouble               y,
  gpointer              user_data)
{
  TrackWidget * self =
    Z_TRACK_WIDGET (user_data);

  /* FIXME should do this via focus on click
   * property */
  /*if (!gtk_widget_has_focus (GTK_WIDGET (self)))*/
    /*gtk_widget_grab_focus (GTK_WIDGET (self));*/

  GdkModifierType state_mask =
    ui_get_state_mask (GTK_GESTURE (gesture));

  Track * track = self->track;

  PROJECT->last_selection =
    SELECTION_TYPE_TRACK;

  track_select (
    track,
    track_is_selected (track) &&
    state_mask & GDK_CONTROL_MASK ?
      F_NO_SELECT: F_SELECT,
    (state_mask & GDK_SHIFT_MASK ||
      state_mask & GDK_CONTROL_MASK) ?
      0 : 1,
    1);
}

static void
on_drag_begin (GtkGestureDrag *gesture,
               gdouble         start_x,
               gdouble         start_y,
               TrackWidget * self)
{
  self->selected_in_dnd = 0;
  self->dragged = 0;

  if (self->resize)
    {
      /* start resizing */
      self->resizing = 1;
    }
  else
    {
      Track * track = self->track;
      self->selected_in_dnd = 1;
      MW_MIXER->start_drag_track = track;

      if (self->n_press == 1)
        {
          int ctrl = 0, selected = 0;

          ctrl = self->ctrl_held_at_start;

          if (tracklist_selections_contains_track (
                TRACKLIST_SELECTIONS,
                track))
            selected = 1;

          /* no control & not selected */
          if (!ctrl && !selected)
            {
              tracklist_selections_select_single (
                TRACKLIST_SELECTIONS,
                track);
            }
          else if (!ctrl && selected)
            { }
          else if (ctrl && !selected)
            tracklist_selections_add_track (
              TRACKLIST_SELECTIONS,
              track);
        }
    }

  self->start_x = start_x;
  self->start_y = start_y;
}

static void
on_drag_update (
  GtkGestureDrag * gesture,
  gdouble         offset_x,
  gdouble         offset_y,
  TrackWidget * self)
{
  g_message ("drag_update");

  self->dragged = 1;

  if (self->resizing)
    {
      /* resize */
      Track * track = self->track;
      track->main_height += (int) offset_y;
      g_message ("resizing");
      gtk_widget_set_size_request (
        GTK_WIDGET (self), -1,
        track_get_full_visible_height (track));
    }
  else
    {
      /* start dnd */
      char * entry_track =
        g_strdup (TARGET_ENTRY_TRACK);
      GtkTargetEntry entries[] = {
        {
          entry_track, GTK_TARGET_SAME_APP,
          symap_map (ZSYMAP, TARGET_ENTRY_TRACK),
        },
      };
      GtkTargetList * target_list =
        gtk_target_list_new (
          entries, G_N_ELEMENTS (entries));

      gtk_drag_begin_with_coordinates (
        (GtkWidget *) self, target_list,
        GDK_ACTION_MOVE | GDK_ACTION_COPY,
        (int) gtk_gesture_single_get_button (
          GTK_GESTURE_SINGLE (gesture)),
        NULL,
        (int) (self->start_x + offset_x),
        (int) (self->start_y + offset_y));

      g_free (entry_track);
    }
}

static void
on_drag_end (
  GtkGestureDrag *gesture,
  gdouble         offset_x,
  gdouble         offset_y,
  TrackWidget * self)
{
  self->resizing = 0;
}

static void
on_drag_data_get (
  GtkWidget        *widget,
  GdkDragContext   *context,
  GtkSelectionData *data,
  guint             info,
  guint             time,
  TrackWidget * self)
{
  /* Not really needed since the selections are
   * used. just send master */
  gtk_selection_data_set (
    data,
    gdk_atom_intern_static_string (
      TARGET_ENTRY_TRACK),
    32,
    (const guchar *) &P_MASTER_TRACK,
    sizeof (P_MASTER_TRACK));
}

/**
 * Highlights/unhighlights the Tracks
 * appropriately.
 *
 * @param highlight 1 to highlight top or bottom,
 *   0 to unhighlight all.
 */
void
track_widget_do_highlight (
  TrackWidget * self,
  gint          x,
  gint          y,
  const int     highlight)
{
  if (highlight)
    {
      /* if we are closer to the start of selection
       * than the end */
      int h =
        gtk_widget_get_allocated_height (
          GTK_WIDGET (self));
      if (y < h / 2)
        {
          /* highlight top */
          gtk_drag_highlight (
            GTK_WIDGET (
              self->highlight_top_box));
          gtk_widget_set_size_request (
            GTK_WIDGET (
              self->highlight_top_box),
            -1, 2);

          /* unhilight bot */
          gtk_drag_unhighlight (
            GTK_WIDGET (
              self->highlight_bot_box));
          gtk_widget_set_size_request (
            GTK_WIDGET (
              self->highlight_bot_box),
            -1, -1);
        }
      else
        {
          /* highlight bot */
          gtk_drag_highlight (
            GTK_WIDGET (
              self->highlight_bot_box));
          gtk_widget_set_size_request (
            GTK_WIDGET (
              self->highlight_bot_box),
            -1, 2);

          /* unhilight top */
          gtk_drag_unhighlight (
            GTK_WIDGET (
              self->highlight_top_box));
          gtk_widget_set_size_request (
            GTK_WIDGET (
              self->highlight_top_box),
            -1, -1);
        }
    }
  else
    {
      gtk_drag_unhighlight (
        GTK_WIDGET (
          self->highlight_top_box));
      gtk_widget_set_size_request (
        GTK_WIDGET (self->highlight_top_box),
        -1, -1);
      gtk_drag_unhighlight (
        GTK_WIDGET (
          self->highlight_bot_box));
      gtk_widget_set_size_request (
        GTK_WIDGET (self->highlight_bot_box),
        -1, -1);
    }
}

/**
 * Callback when lanes button is toggled.
 */
void
track_widget_on_show_lanes_toggled (
  TrackWidget * self)
{
  Track * track = self->track;

  /* set visibility flag */
  track->lanes_visible =
    !track->lanes_visible;

  EVENTS_PUSH (ET_TRACK_LANES_VISIBILITY_CHANGED,
               track);
}

void
track_widget_on_show_automation_toggled (
  TrackWidget * self)
{
  Track * track = self->track;

  /* set visibility flag */
  track->bot_paned_visible =
    !track->bot_paned_visible;

  EVENTS_PUSH (ET_TRACK_BOT_PANED_VISIBILITY_CHANGED,
               track);
}

void
track_widget_on_solo_toggled (
  TrackWidget * self)
{
  track_set_soloed (
    self->track, self->track->solo, 1);
}

/**
 * General handler for tracks that have mute
 * buttons.
 */
void
track_widget_on_mute_toggled (
  TrackWidget * self)
{
  track_set_muted (
    self->track, !self->track->mute, 1);
}

void
track_widget_on_record_toggled (
  TrackWidget * self)
{
  Track * track = self->track;
  ChannelTrack * ct = (ChannelTrack *) track;
  Channel * chan = ct->channel;

  /* toggle record flag */
  track_set_recording (track, !track->recording);
  chan->record_set_automatically = 0;
  g_message ("recording %d, %s",
             track->recording,
             track->name);

  EVENTS_PUSH (ET_TRACK_STATE_CHANGED,
               track);
}

/**
 * Add a button.
 *
 * @param top 1 for top, 0 for bottom.
 */
static void
add_button (
  TrackWidget * self,
  int           top,
  const char *  icon_name,
  int           size)
{
}

/**
 * Wrapper for child track widget.
 *
 * Sets color, draw callback, etc.
 */
TrackWidget *
track_widget_new (Track * track)
{
  g_return_val_if_fail (track, NULL);

  TrackWidget * self =
    g_object_new (
      TRACK_WIDGET_TYPE, NULL);

  switch (track->type)
    {
    case TRACK_TYPE_INSTRUMENT:
      strcpy (self->icon_name, "z-audio-midi");
      add_button (
        self, 1, ICON_NAME_RECORD, 12);
      add_button (
        self, 1, ICON_NAME_SOLO, 12);
      add_button (
        self, 1, ICON_NAME_MUTE, 12);
      add_button (
        self, 1, ICON_NAME_SHOW_UI, 12);
      add_button (
        self, 0, ICON_NAME_SHOW_AUTOMATION_LANES,
        12);
      add_button (
        self, 0, ICON_NAME_SHOW_TRACK_LANES, 12);
      add_button (
        self, 0, ICON_NAME_LOCK, 12);
      add_button (
        self, 0, ICON_NAME_FREEZE, 12);
      break;
    case TRACK_TYPE_MIDI:
      strcpy (self->icon_name, "z-audio-midi");
      break;
    case TRACK_TYPE_MASTER:
      strcpy (self->icon_name, "bus");
      break;
    case TRACK_TYPE_CHORD:
      strcpy (self->icon_name, "z-minuet-chords");
      break;
    case TRACK_TYPE_MARKER:
      strcpy (
        self->icon_name,
        "z-kdenlive-show-markers");
      break;
    default:
      break;
    }

  int height =
    track_get_full_visible_height (track);
  g_return_val_if_fail (height > 1, NULL);
  gtk_widget_set_size_request (
    GTK_WIDGET (self->drawing_area),
    -1, height);

  self->track = track;

  return self;
}

static void
on_destroy (
  TrackWidget * self)
{

  Track * track = self->track;

  CustomButtonWidget * cb = NULL;
  for (int i = 0; i < self->num_top_buttons; i++)
    {
      cb = self->top_buttons[i];
      custom_button_widget_free (cb);
    }
  for (int i = 0; i < self->num_bot_buttons; i++)
    {
      cb = self->bot_buttons[i];
      custom_button_widget_free (cb);
    }

  g_object_unref (self);

  track->widget = NULL;
}

static void
track_widget_init (TrackWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_widget_set_vexpand_set (
    (GtkWidget *) self, 1);

  /* set font sizes */
  /*gtk_label_set_max_width_chars (*/
    /*self->top_grid->name->label, 14);*/
  /*gtk_label_set_width_chars (*/
    /*self->top_grid->name->label, 14);*/
  /*gtk_label_set_ellipsize (*/
    /*self->top_grid->name->label, PANGO_ELLIPSIZE_END);*/
  /*gtk_label_set_xalign (*/
    /*self->top_grid->name->label, 0);*/

  self->drag =
    GTK_GESTURE_DRAG (
      gtk_gesture_drag_new (
        GTK_WIDGET (self->drawing_area)));

  self->multipress =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (
        GTK_WIDGET (self->drawing_area)));
  self->right_mouse_mp =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (
        GTK_WIDGET (self->drawing_area)));
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (self->right_mouse_mp),
    GDK_BUTTON_SECONDARY);

  /* make widget able to notify */
  gtk_widget_add_events (
    GTK_WIDGET (self->drawing_area),
    GDK_ALL_EVENTS_MASK);

  g_signal_connect (
    G_OBJECT (self->multipress), "pressed",
    G_CALLBACK (multipress_pressed), self);
  g_signal_connect (
    G_OBJECT (self->right_mouse_mp), "pressed",
    G_CALLBACK (on_right_click), self);
  g_signal_connect (
    G_OBJECT (self->drawing_area),
    "enter-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT (self->drawing_area),
    "leave-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT (self->drawing_area),
    "motion-notify-event",
    G_CALLBACK (on_motion),  self);
  g_signal_connect (
    G_OBJECT (self->drawing_area), "draw",
    G_CALLBACK (track_draw_cb),  self);
  g_signal_connect (
    G_OBJECT (self->drag), "drag-begin",
    G_CALLBACK (on_drag_begin), self);
  g_signal_connect (
    G_OBJECT (self->drag), "drag-update",
    G_CALLBACK (on_drag_update), self);
  g_signal_connect (
    G_OBJECT (self->drag), "drag-end",
    G_CALLBACK (on_drag_end), self);
  g_signal_connect (
    GTK_WIDGET (self),
    "drag-data-get",
    G_CALLBACK (on_drag_data_get), self);
  g_signal_connect (
    G_OBJECT(self), "destroy",
    G_CALLBACK (on_destroy),  NULL);

  g_object_ref (self);

  self->redraw = 1;
}

static void
track_widget_class_init (TrackWidgetClass * _klass)
{
  GtkWidgetClass * klass =
    GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "track.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, TrackWidget, x)

  BIND_CHILD (drawing_area);
  /*BIND_CHILD (color);*/
  /*BIND_CHILD (paned);*/
  /*BIND_CHILD (top_grid);*/
  /*BIND_CHILD (event_box);*/
  BIND_CHILD (highlight_top_box);
  BIND_CHILD (highlight_bot_box);
}
