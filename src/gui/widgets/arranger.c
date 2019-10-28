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

#include "zrythm.h"
#include "actions/actions.h"
#include "actions/arranger_selections.h"
#include "audio/automation_track.h"
#include "audio/channel.h"
#include "audio/chord_track.h"
#include "audio/instrument_track.h"
#include "audio/midi_region.h"
#include "audio/mixer.h"
#include "audio/track.h"
#include "audio/transport.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/arranger_playhead.h"
#include "gui/widgets/audio_arranger.h"
#include "gui/widgets/audio_arranger_bg.h"
#include "gui/widgets/audio_editor_space.h"
#include "gui/widgets/automation_arranger.h"
#include "gui/widgets/automation_arranger_bg.h"
#include "gui/widgets/automation_curve.h"
#include "gui/widgets/automation_editor_space.h"
#include "gui/widgets/automation_track.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/chord_arranger.h"
#include "gui/widgets/chord_arranger_bg.h"
#include "gui/widgets/chord_editor_space.h"
#include "gui/widgets/chord_object.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/foldable_notebook.h"
#include "gui/widgets/inspector.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/marker.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_arranger_bg.h"
#include "gui/widgets/midi_editor_space.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/midi_modifier_arranger_bg.h"
#include "gui/widgets/midi_note.h"
#include "gui/widgets/editor_ruler.h"
#include "gui/widgets/region.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/scale_object.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_bg.h"
#include "gui/widgets/timeline_bot_box.h"
#include "gui/widgets/timeline_minimap.h"
#include "gui/widgets/timeline_panel.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"
#include "gui/widgets/velocity.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/arrays.h"
#include "utils/gtk.h"
#include "utils/flags.h"
#include "utils/resources.h"
#include "utils/ui.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE_WITH_PRIVATE (
  ArrangerWidget,
  arranger_widget,
  GTK_TYPE_OVERLAY)

#define GET_PRIVATE \
  ARRANGER_WIDGET_GET_PRIVATE (self)

/**
 * Gets each specific arranger.
 *
 * Useful boilerplate
 */
#define GET_ARRANGER_ALIASES(self) \
  TimelineArrangerWidget * timeline_arranger = \
    NULL; \
  MidiArrangerWidget * midi_arranger = NULL; \
  MidiModifierArrangerWidget * \
    midi_modifier_arranger = NULL; \
  AudioArrangerWidget * audio_arranger = NULL; \
  ChordArrangerWidget * chord_arranger = NULL; \
  AutomationArrangerWidget * automation_arranger = \
    NULL; \
  if (Z_IS_MIDI_ARRANGER_WIDGET (self)) \
    { \
      midi_arranger = Z_MIDI_ARRANGER_WIDGET (self); \
    } \
  else if (Z_IS_TIMELINE_ARRANGER_WIDGET (self)) \
    { \
      timeline_arranger = \
        Z_TIMELINE_ARRANGER_WIDGET (self); \
    } \
  else if (Z_IS_AUDIO_ARRANGER_WIDGET (self)) \
    { \
      audio_arranger = \
        Z_AUDIO_ARRANGER_WIDGET (self); \
    } \
  else if (Z_IS_MIDI_MODIFIER_ARRANGER_WIDGET (self)) \
    { \
      midi_modifier_arranger = \
        Z_MIDI_MODIFIER_ARRANGER_WIDGET (self); \
    } \
  else if (Z_IS_CHORD_ARRANGER_WIDGET (self)) \
    { \
      chord_arranger = \
        Z_CHORD_ARRANGER_WIDGET (self); \
    } \
  else if (Z_IS_AUTOMATION_ARRANGER_WIDGET (self)) \
    { \
      automation_arranger = \
        Z_AUTOMATION_ARRANGER_WIDGET (self); \
    }

#define FORALL_ARRANGERS(func) \
  func (timeline); \
  func (midi); \
  func (audio); \
  func (automation); \
  func (midi_modifier); \
  func (chord); \

#define ACTION_IS(x) \
  (ar_prv->action == UI_OVERLAY_ACTION_##x)

ArrangerWidgetPrivate *
arranger_widget_get_private (
  ArrangerWidget * self)
{
  return
    arranger_widget_get_instance_private (self);
}

/**
 * Gets called to set the position/size of each overlayed widget.
 */
static gboolean
arranger_get_child_position (
  GtkOverlay   *overlay,
  GtkWidget    *widget,
  GdkRectangle *allocation,
  ArrangerWidget * self)
{
  GET_ARRANGER_ALIASES (self);

#define ELIF_ARR_SET_ALLOCATION(sc) \
  if (sc##_arranger) \
    { \
      sc##_arranger_widget_set_allocation ( \
        sc##_arranger, widget, allocation); \
    }

  if (Z_IS_ARRANGER_PLAYHEAD_WIDGET (widget))
    {
      /* note: -1 (half the width of the playhead
       * widget */
      if (timeline_arranger)
        allocation->x =
          ui_pos_to_px_timeline (
            &TRANSPORT->playhead_pos,
            1) - 1;
      else if ((midi_arranger ||
               midi_modifier_arranger ||
               automation_arranger ||
               audio_arranger) &&
               CLIP_EDITOR->region)
        {
          Region * r = NULL;
          if (automation_arranger)
            {
              r =
                region_at_position (
                  NULL, CLIP_EDITOR->region->at,
                  PLAYHEAD);
            }
          else
            {
              r =
                region_at_position (
                  arranger_object_get_track (
                    (ArrangerObject *)
                    CLIP_EDITOR->region),
                  NULL, PLAYHEAD);
            }
          Position tmp;
          if (r)
            {
              ArrangerObject * obj =
                (ArrangerObject *) r;
              long region_local_frames =
                region_timeline_frames_to_local (
                  r, PLAYHEAD->frames, 1);
              region_local_frames +=
                obj->pos.frames;
              position_from_frames (
                &tmp, region_local_frames);
              allocation->x =
                ui_pos_to_px_editor (
                  &tmp, 1);
            }
          else
            {
              allocation->x =
                ui_pos_to_px_editor (
                  PLAYHEAD, 1);
            }
        }
      allocation->y = 0;
      allocation->width = 2;
      allocation->height =
        gtk_widget_get_allocated_height (
          GTK_WIDGET (self));
    }
  FORALL_ARRANGERS (ELIF_ARR_SET_ALLOCATION)
#undef ELIF_ARR_SET_ALLOCATION

  return TRUE;
}

/**
 * Sets the cursor on the arranger and all of its
 * children.
 */
void
arranger_widget_set_cursor (
  ArrangerWidget * self,
  ArrangerCursor   cursor)
{
  GList *children, *iter;

#define SET_X_CURSOR(x) \
  ui_set_##x##_cursor (self); \
  children = \
    gtk_container_get_children ( \
      GTK_CONTAINER (self)); \
  for (iter = children; \
       iter != NULL; \
       iter = g_list_next (iter)) \
    { \
      ui_set_##x##_cursor ( \
        GTK_WIDGET (iter->data)); \
    } \
  g_list_free (children)


#define SET_CURSOR_FROM_NAME(name) \
  ui_set_cursor_from_name ( \
    GTK_WIDGET (self), name); \
  children = \
    gtk_container_get_children ( \
      GTK_CONTAINER (self)); \
  for (iter = children; \
       iter != NULL; \
       iter = g_list_next (iter)) \
    { \
      ui_set_cursor_from_name ( \
        GTK_WIDGET (iter->data), \
        name); \
    } \
  g_list_free (children)

  switch (cursor)
    {
    case ARRANGER_CURSOR_SELECT:
      SET_X_CURSOR (pointer);
      break;
    case ARRANGER_CURSOR_EDIT:
      SET_X_CURSOR (pencil);
      break;
    case ARRANGER_CURSOR_CUT:
      SET_X_CURSOR (cut_clip);
      break;
    case ARRANGER_CURSOR_RAMP:
      SET_X_CURSOR (line);
      break;
    case ARRANGER_CURSOR_ERASER:
      SET_X_CURSOR (eraser);
      break;
    case ARRANGER_CURSOR_AUDITION:
      SET_X_CURSOR (speaker);
      break;
    case ARRANGER_CURSOR_GRAB:
      SET_X_CURSOR (hand);
      break;
    case ARRANGER_CURSOR_GRABBING:
      SET_CURSOR_FROM_NAME ("grabbing");
      break;
    case ARRANGER_CURSOR_GRABBING_COPY:
      SET_CURSOR_FROM_NAME ("copy");
      break;
    case ARRANGER_CURSOR_GRABBING_LINK:
      SET_CURSOR_FROM_NAME ("link");
      break;
    case ARRANGER_CURSOR_RESIZING_L:
      SET_X_CURSOR (left_resize);
      break;
    case ARRANGER_CURSOR_RESIZING_L_LOOP:
      SET_X_CURSOR (left_resize_loop);
      break;
    case ARRANGER_CURSOR_RESIZING_R:
      SET_X_CURSOR (right_resize);
      break;
    case ARRANGER_CURSOR_RESIZING_R_LOOP:
      SET_X_CURSOR (right_resize_loop);
      break;
    case ARRANGER_CURSOR_RESIZING_UP:
      SET_CURSOR_FROM_NAME ("n-resize");
      break;
    case ARRANGER_CURSOR_RANGE:
      SET_CURSOR_FROM_NAME ("text");
      break;
    default:
      g_warn_if_reached ();
      break;
    }
}

void
arranger_widget_get_hit_widgets_in_range (
  ArrangerWidget *  self,
  GType             type,
  double            start_x,
  double            start_y,
  double            offset_x,
  double            offset_y,
  GtkWidget **      array, ///< array to fill
  int *             array_size) ///< array_size to fill
{
  GList *children, *iter;

  /* go through each overlay child */
  children =
    gtk_container_get_children (
      GTK_CONTAINER (self));
  for (iter = children;
       iter != NULL; iter = g_list_next (iter))
    {
      GtkWidget * widget = GTK_WIDGET (iter->data);

      GtkAllocation allocation;
      gtk_widget_get_allocation (widget,
                                 &allocation);

      gint wx, wy;
      gtk_widget_translate_coordinates(
                GTK_WIDGET (self),
                GTK_WIDGET (widget),
                (int) start_x,
                (int) start_y,
                &wx,
                &wy);

      int x_hit, y_hit;
      if (offset_x < 0)
        {
          x_hit = wx + offset_x <= allocation.width &&
            wx > 0;
        }
      else
        {
          x_hit = wx <= allocation.width &&
            wx + offset_x > 0;
        }
      if (!x_hit) continue;
      if (offset_y < 0)
        {
          y_hit = wy + offset_y <= allocation.height &&
            wy > 0;
        }
      else
        {
          y_hit = wy <= allocation.height &&
            wy + offset_y > 0;
        }
      if (!y_hit) continue;

      if (type == MIDI_NOTE_WIDGET_TYPE &&
          Z_IS_MIDI_NOTE_WIDGET (widget))
        {
          array[(*array_size)++] = widget;
        }
      else if (type == REGION_WIDGET_TYPE &&
               Z_IS_REGION_WIDGET (widget))
        {
          array[(*array_size)++] = widget;
        }
      else if (type ==
                 AUTOMATION_POINT_WIDGET_TYPE &&
               Z_IS_AUTOMATION_POINT_WIDGET (widget))
        {
          array[(*array_size)++] = widget;
        }
      else if (type == CHORD_OBJECT_WIDGET_TYPE &&
               Z_IS_CHORD_OBJECT_WIDGET (widget))
        {
          array[(*array_size)++] = widget;
        }
      else if (type == SCALE_OBJECT_WIDGET_TYPE &&
               Z_IS_SCALE_OBJECT_WIDGET (widget))
        {
          array[(*array_size)++] = widget;
        }
      else if (type == MARKER_WIDGET_TYPE &&
               Z_IS_MARKER_WIDGET (widget))
        {
          array[(*array_size)++] = widget;
        }
      else if (type == VELOCITY_WIDGET_TYPE &&
               Z_IS_VELOCITY_WIDGET (widget))
        {
          array[(*array_size)++] = widget;
        }
    }
  g_list_free (children);
}

/**
 * Returns if the arranger is in a moving-related
 * operation or starting a moving-related operation.
 *
 * Useful to know if we need transient widgets or
 * not.
 */
int
arranger_widget_is_in_moving_operation (
  ArrangerWidget * self)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  if (ar_prv->action ==
        UI_OVERLAY_ACTION_STARTING_MOVING ||
      ar_prv->action ==
        UI_OVERLAY_ACTION_STARTING_MOVING_COPY ||
      ar_prv->action ==
        UI_OVERLAY_ACTION_STARTING_MOVING_LINK ||
      ar_prv->action ==
        UI_OVERLAY_ACTION_MOVING ||
      ar_prv->action ==
        UI_OVERLAY_ACTION_MOVING_COPY ||
      ar_prv->action ==
        UI_OVERLAY_ACTION_MOVING_LINK)
    return 1;

  return 0;
}

/**
 * Used for pushing transients to the bottom on
 * the z axis.
 */
void
arranger_widget_add_overlay_child (
  ArrangerWidget * self,
  ArrangerObject * obj)
{
  gtk_overlay_add_overlay (
    (GtkOverlay *) self, obj->widget);
  arranger_object_set_widget_visibility_and_state (
    obj, 0);
  if (arranger_object_is_transient (obj))
    {
      gtk_overlay_reorder_overlay (
        (GtkOverlay *) self,
        obj->widget, 0);
    }
}

/**
 * Moves the ArrangerSelections by the given
 * amount of ticks.
 *
 * @param ticks_diff Ticks to move by.
 * @param copy_moving 1 if copy-moving.
 */
static void
move_items_x (
  ArrangerWidget * self,
  const long       ticks_diff)
{
  ArrangerSelections * sel =
    arranger_widget_get_selections (self);
  arranger_selections_add_ticks (
    sel, ticks_diff, F_CACHED, AO_UPDATE_NON_TRANS);

  EVENTS_PUSH (
    ET_ARRANGER_SELECTIONS_IN_TRANSIT, sel);
}

/**
 * Refreshes all arranger backgrounds.
 */
void
arranger_widget_refresh_all_backgrounds ()
{
  ArrangerWidgetPrivate * ar_prv;

  ar_prv =
    arranger_widget_get_private (
      (ArrangerWidget *) (MW_TIMELINE));
  gtk_widget_queue_draw (
    GTK_WIDGET (ar_prv->bg));
  ar_prv =
    arranger_widget_get_private (
      (ArrangerWidget *) (MW_MIDI_ARRANGER));
  gtk_widget_queue_draw (
    GTK_WIDGET (ar_prv->bg));
  ar_prv =
    arranger_widget_get_private (
      (ArrangerWidget *) (MW_MIDI_MODIFIER_ARRANGER));
  gtk_widget_queue_draw (
    GTK_WIDGET (ar_prv->bg));
  ar_prv =
    arranger_widget_get_private (
      (ArrangerWidget *) (MW_AUDIO_ARRANGER));
  gtk_widget_queue_draw (
    GTK_WIDGET (ar_prv->bg));
}

static void
select_all_timeline (
  TimelineArrangerWidget *  self,
  int                       select)
{
  int i,j,k;

  arranger_selections_clear (
    (ArrangerSelections *) TL_SELECTIONS);

  /* select everything else */
  Region * r;
  Track * track;
  AutomationTrack * at;
  for (i = 0; i < TRACKLIST->num_tracks; i++)
    {
      track = TRACKLIST->tracks[i];

      if (!track->visible)
        continue;

      AutomationTracklist * atl =
        track_get_automation_tracklist (
          track);

      if (track->lanes_visible)
        {
          TrackLane * lane;
          for (j = 0; j < track->num_lanes; j++)
            {
              lane = track->lanes[j];

              for (k = 0;
                   k < lane->num_regions; k++)
                {
                  r = lane->regions[k];
                  arranger_object_select (
                    (ArrangerObject *) r,
                    select, F_APPEND);
                }
            }
        }

      if (!track->bot_paned_visible)
        continue;

      for (j = 0; j < atl->num_ats; j++)
        {
          at = atl->ats[j];

          if (!at->created ||
              !at->visible)
            continue;

          for (k = 0; k < at->num_regions; k++)
            {
              r = at->regions[k];
              arranger_object_select (
                (ArrangerObject *) r,
                select, F_APPEND);
            }
        }
    }

  /* select scales */
  ScaleObject * scale;
  for (i = 0;
       i < P_CHORD_TRACK->num_scales; i++)
    {
      scale = P_CHORD_TRACK->scales[i];
      arranger_object_select (
        (ArrangerObject *) scale,
        select, F_APPEND);
    }

  /**
   * Deselect range if deselecting all.
   */
  if (!select)
    {
      project_set_has_range (0);
    }
}

static void
select_all_midi (
  MidiArrangerWidget *  self,
  int               select)
{
  if (!CLIP_EDITOR->region)
    return;

  /*midi_arranger_selections_clear (*/
    /*MA_SELECTIONS);*/

  /* select midi notes */
  MidiRegion * mr =
    (MidiRegion *) CLIP_EDITOR_SELECTED_REGION;
  for (int i = 0; i < mr->num_midi_notes; i++)
    {
      MidiNote * midi_note = mr->midi_notes[i];
      arranger_object_select (
        (ArrangerObject *) midi_note,
        select, F_APPEND);
    }
  /*arranger_widget_refresh(&self->parent_instance);*/
}

static void
select_all_chord (
  ChordArrangerWidget *  self,
  int                    select)
{
  arranger_selections_clear (
    (ArrangerSelections *) CHORD_SELECTIONS);

  /* select everything else */
  Region * r = CLIP_EDITOR->region;
  ChordObject * chord;
  for (int i = 0; i < r->num_chord_objects; i++)
    {
      chord = r->chord_objects[i];
      arranger_object_select (
        (ArrangerObject *) chord, select,
        F_APPEND);
    }
}

static void
select_all_midi_modifier (
  MidiModifierArrangerWidget *  self,
  int                           select)
{
  /* TODO */
}

static void
select_all_audio (
  AudioArrangerWidget *  self,
  int                    select)
{
  /* TODO */
}

static void
select_all_automation (
  AutomationArrangerWidget *  self,
  int                         select)
{
  int i;

  arranger_selections_clear (
    (ArrangerSelections *) AUTOMATION_SELECTIONS);

  Region * region = CLIP_EDITOR->region;

  /* select everything else */
  AutomationPoint * ap;
  for (i = 0; i < region->num_aps; i++)
    {
      ap = region->aps[i];
      arranger_object_select (
        (ArrangerObject *) ap, select,
        F_APPEND);
    }

  /**
   * Deselect range if deselecting all.
   */
  if (!select)
    {
      project_set_has_range (0);
    }
}

void
arranger_widget_select_all (
  ArrangerWidget *  self,
  int               select)
{
  GET_ARRANGER_ALIASES (self);

#define ARR_SELECT_ALL(sc) \
  if (sc##_arranger) \
    { \
      select_all_##sc ( \
        sc##_arranger, select); \
    }

  FORALL_ARRANGERS (ARR_SELECT_ALL);

#undef ARR_SELECT_ALL
}

static void
show_context_menu_automation (
  AutomationArrangerWidget * self,
  gdouble              x,
  gdouble              y)
{
  GtkWidget *menu, *menuitem;

  menu = gtk_menu_new();

  menuitem =
    gtk_menu_item_new_with_label ("Do something");

  gtk_menu_shell_append (
    GTK_MENU_SHELL(menu), menuitem);

  gtk_widget_show_all (menu);

  gtk_menu_popup_at_pointer (GTK_MENU(menu), NULL);
}

static void
show_context_menu_timeline (
  TimelineArrangerWidget * self,
  gdouble              x,
  gdouble              y)
{
  GtkWidget *menu, *menuitem;

  ArrangerObject * r_obj =
    arranger_widget_get_hit_arranger_object (
      (ArrangerWidget *) self,
      REGION_WIDGET_TYPE, x, y);
  Region * r = (Region *) r_obj;

  menu = gtk_menu_new();

  if (r)
    {
      if (r->type == REGION_TYPE_MIDI)
        {
          menuitem =
            gtk_menu_item_new_with_label (
              "Export as MIDI file");
          gtk_menu_shell_append (
            GTK_MENU_SHELL(menu), menuitem);
          g_signal_connect (
            menuitem, "activate",
            G_CALLBACK (
              timeline_arranger_on_export_as_midi_file_clicked),
            r);
        }
    }

  gtk_widget_show_all(menu);

  gtk_menu_popup_at_pointer (GTK_MENU(menu), NULL);
}

static void
show_context_menu_midi (
  MidiArrangerWidget * self,
  gdouble              x,
  gdouble              y)
{
  GtkWidget *menu;
  GtkMenuItem * menu_item;

  ArrangerObject * mn_obj =
    arranger_widget_get_hit_arranger_object (
      (ArrangerWidget *) self,
      MIDI_NOTE_WIDGET_TYPE, x, y);
  MidiNote * mn = (MidiNote *) mn_obj;

  if (mn)
    {
      int selected =
        arranger_object_is_selected (
          mn_obj);
      if (!selected)
        {
          arranger_object_select (
            mn_obj,
            F_SELECT, F_NO_APPEND);
        }
    }
  else
    {
      arranger_widget_select_all (
        (ArrangerWidget *) self, F_NO_SELECT);
      arranger_selections_clear (
        (ArrangerSelections *) MA_SELECTIONS);
    }

#define APPEND_TO_MENU \
  gtk_menu_shell_append ( \
    GTK_MENU_SHELL (menu), \
    GTK_WIDGET (menu_item))

  menu = gtk_menu_new();

  menu_item = CREATE_CUT_MENU_ITEM;
  APPEND_TO_MENU;
  menu_item = CREATE_COPY_MENU_ITEM;
  APPEND_TO_MENU;
  menu_item = CREATE_PASTE_MENU_ITEM;
  APPEND_TO_MENU;
  menu_item = CREATE_DELETE_MENU_ITEM;
  APPEND_TO_MENU;
  menu_item = CREATE_DUPLICATE_MENU_ITEM;
  APPEND_TO_MENU;
  menu_item =
    GTK_MENU_ITEM (gtk_separator_menu_item_new ());
  APPEND_TO_MENU;
  menu_item = CREATE_CLEAR_SELECTION_MENU_ITEM;
  APPEND_TO_MENU;
  menu_item = CREATE_SELECT_ALL_MENU_ITEM;
  APPEND_TO_MENU;

#undef APPEND_TO_MENU

  gtk_widget_show_all(menu);

  gtk_menu_popup_at_pointer (GTK_MENU(menu), NULL);
}

static void
show_context_menu_audio (
  AudioArrangerWidget * self,
  gdouble              x,
  gdouble              y)
{
  GtkWidget *menu, *menuitem;

  menu = gtk_menu_new();

  menuitem =
    gtk_menu_item_new_with_label("Do something");

  gtk_menu_shell_append (
    GTK_MENU_SHELL(menu), menuitem);

  gtk_widget_show_all(menu);

  gtk_menu_popup_at_pointer (GTK_MENU(menu), NULL);
}

static void
show_context_menu_chord (
  ChordArrangerWidget * self,
  gdouble              x,
  gdouble              y)
{
}

static void
show_context_menu_midi_modifier (
  MidiModifierArrangerWidget * self,
  gdouble              x,
  gdouble              y)
{
}

static void
show_context_menu (
  ArrangerWidget * self,
  gdouble          x,
  gdouble          y)
{
  GET_ARRANGER_ALIASES (self);

#define SHOW_CONTEXT_MENU(sc) \
  if (sc##_arranger) \
    show_context_menu_##sc ( \
      sc##_arranger, x, y)

  FORALL_ARRANGERS (SHOW_CONTEXT_MENU);

#undef SHOW_CONTEXT_MENU
}

static void
on_right_click (GtkGestureMultiPress *gesture,
               gint                  n_press,
               gdouble               x,
               gdouble               y,
               ArrangerWidget *      self)
{
  if (n_press != 1)
    return;

  show_context_menu (self, x, y);
}

static void
auto_scroll (
  ArrangerWidget * self,
  int              x,
  int              y)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  /* figure out if we should scroll */
  int scroll_h = 0, scroll_v = 0;
  switch (ar_prv->action)
    {
    case UI_OVERLAY_ACTION_MOVING:
    case UI_OVERLAY_ACTION_MOVING_COPY:
    case UI_OVERLAY_ACTION_CREATING_MOVING:
    case UI_OVERLAY_ACTION_SELECTING:
    case UI_OVERLAY_ACTION_RAMPING:
      scroll_h = 1;
      scroll_v = 1;
      break;
    case UI_OVERLAY_ACTION_RESIZING_R:
    case UI_OVERLAY_ACTION_RESIZING_L:
    case UI_OVERLAY_ACTION_STRETCHING_L:
    case UI_OVERLAY_ACTION_CREATING_RESIZING_R:
    case UI_OVERLAY_ACTION_STRETCHING_R:
    case UI_OVERLAY_ACTION_AUTOFILLING:
    case UI_OVERLAY_ACTION_AUDITIONING:
      scroll_h = 1;
      break;
    case UI_OVERLAY_ACTION_RESIZING_UP:
      scroll_v = 1;
      break;
    default:
      break;
    }

  if (!scroll_h && !scroll_v)
    return;

	GtkScrolledWindow *scroll =
		arranger_widget_get_scrolled_window (self);
  g_return_if_fail (scroll);
  int h_scroll_speed = 20;
  int v_scroll_speed = 10;
  int border_distance = 5;
  int scroll_width =
    gtk_widget_get_allocated_width (
      GTK_WIDGET (scroll));
  int scroll_height =
    gtk_widget_get_allocated_height (
      GTK_WIDGET (scroll));
  GtkAdjustment *hadj =
    gtk_scrolled_window_get_hadjustment (
      GTK_SCROLLED_WINDOW (scroll));
  GtkAdjustment *vadj =
    gtk_scrolled_window_get_vadjustment (
      GTK_SCROLLED_WINDOW (scroll));
  int v_delta = 0;
  int h_delta = 0;
  int adj_x =
    (int)
    gtk_adjustment_get_value (hadj);
  int adj_y =
    (int)
    gtk_adjustment_get_value (vadj);
  if (y + border_distance
        >= adj_y + scroll_height)
    {
      v_delta = v_scroll_speed;
    }
  else if (y - border_distance <= adj_y)
    {
      v_delta = - v_scroll_speed;
    }
  if (x + border_distance >= adj_x + scroll_width)
    {
      h_delta = h_scroll_speed;
    }
  else if (x - border_distance <= adj_x)
    {
      h_delta = - h_scroll_speed;
    }
  if (h_delta != 0 && scroll_h)
  {
    gtk_adjustment_set_value (
      hadj,
      gtk_adjustment_get_value (hadj) + h_delta);
  }
  if (v_delta != 0 && scroll_v)
  {
    gtk_adjustment_set_value (
      vadj,
      gtk_adjustment_get_value (vadj) + v_delta);
  }

  return;
}

static gboolean
on_key_release_action (
	GtkWidget *widget,
	GdkEventKey *event,
	ArrangerWidget * self)
{
  GET_PRIVATE;
  ar_prv->key_is_pressed = 0;

  const guint keyval = event->keyval;

  if (z_gtk_keyval_is_ctrl (keyval))
    {
      ar_prv->ctrl_held = 0;
    }

  if (z_gtk_keyval_is_shift (keyval))
    {
      ar_prv->shift_held = 0;
    }
  if (z_gtk_keyval_is_alt (keyval))
    {
      ar_prv->alt_held = 0;
    }

  if (ar_prv->action ==
        UI_OVERLAY_ACTION_STARTING_MOVING)
    {
      if (ar_prv->ctrl_held)
        ar_prv->action =
          UI_OVERLAY_ACTION_MOVING_COPY;
      else
        ar_prv->action =
          UI_OVERLAY_ACTION_MOVING;
    }
  else if (ar_prv->action ==
             UI_OVERLAY_ACTION_MOVING &&
           ar_prv->ctrl_held)
    ar_prv->action =
      UI_OVERLAY_ACTION_MOVING_COPY;
  else if (ar_prv->action ==
             UI_OVERLAY_ACTION_MOVING_COPY &&
           !ar_prv->ctrl_held)
    ar_prv->action =
      UI_OVERLAY_ACTION_MOVING;

  if (Z_IS_TIMELINE_ARRANGER_WIDGET (self))
    {
      timeline_arranger_widget_set_cut_lines_visible (
        (TimelineArrangerWidget *) self);
    }

  arranger_widget_update_visibility (self);

  arranger_widget_refresh_cursor (
    self);

  return TRUE;
}

static gboolean
on_key_action (
  GtkWidget *widget,
  GdkEventKey *event,
  ArrangerWidget * self)
{
  GET_PRIVATE;
  GET_ARRANGER_ALIASES(self);

  int num = 0;

  const guint keyval = event->keyval;

  if (z_gtk_keyval_is_ctrl (keyval))
    {
      ar_prv->ctrl_held = 1;
    }

  if (z_gtk_keyval_is_shift (keyval))
    {
      ar_prv->shift_held = 1;
    }
  if (z_gtk_keyval_is_alt (keyval))
    {
      ar_prv->alt_held = 1;
    }

  if (midi_arranger)
    {
      num =
        MA_SELECTIONS->num_midi_notes;
      if (event->state & GDK_CONTROL_MASK &&
          event->type == GDK_KEY_PRESS &&
          event->keyval == GDK_KEY_d &&
          num > 0)
        {
          /*UndoableAction * duplicate_action =*/
            /*duplicate_midi_arranger_selections_action_new ();*/
          /*undo_manager_perform (*/
            /*UNDO_MANAGER, duplicate_action);*/
          return FALSE;
        }
      if (event->type == GDK_KEY_PRESS &&
          event->keyval == GDK_KEY_Up &&
          num > 0)
        {
          UndoableAction * shift_up_action =
            arranger_selections_action_new_move_midi (
              MA_SELECTIONS, 0, 1);
          undo_manager_perform (
          UNDO_MANAGER, shift_up_action);
          return TRUE;
        }
      if (event->type == GDK_KEY_PRESS &&
          event->keyval == GDK_KEY_Down &&
          num > 0)
        {
          UndoableAction * shift_down_action =
            arranger_selections_action_new_move_midi (
              MA_SELECTIONS, 0, -1);
          undo_manager_perform (
          UNDO_MANAGER, shift_down_action);
          return TRUE;
        }
      if (event->type == GDK_KEY_PRESS &&
          event->keyval == GDK_KEY_Left &&
          num > 0)
        {
          /*UndoableAction * shift_left_action =*/
            /*move_midi_arranger_selections_action_new (-1);*/
          /*undo_manager_perform (*/
          /*UNDO_MANAGER, shift_left_action);*/
          return TRUE;
        }
      if (event->type == GDK_KEY_PRESS &&
          event->keyval == GDK_KEY_Right &&
          num > 0)
        {
          /*UndoableAction * shift_right_action =*/
            /*move_midi_arranger_selections_pos_action_new (1);*/
          /*undo_manager_perform (*/
          /*UNDO_MANAGER, shift_right_action);*/
        }
    }
  if (timeline_arranger)
    {
    }
  if (midi_modifier_arranger)
    {

    }
  if (audio_arranger)
    {

    }
  if (chord_arranger)
    {}
  if (automation_arranger)
    {}

  if (ar_prv->action ==
        UI_OVERLAY_ACTION_STARTING_MOVING)
    {
      if (ar_prv->ctrl_held)
        ar_prv->action =
          UI_OVERLAY_ACTION_MOVING_COPY;
      else
        ar_prv->action =
          UI_OVERLAY_ACTION_MOVING;
    }
  else if (ar_prv->action ==
             UI_OVERLAY_ACTION_MOVING &&
           ar_prv->ctrl_held)
    ar_prv->action =
      UI_OVERLAY_ACTION_MOVING_COPY;
  else if (ar_prv->action ==
             UI_OVERLAY_ACTION_MOVING_COPY &&
           !ar_prv->ctrl_held)
    ar_prv->action =
      UI_OVERLAY_ACTION_MOVING;

  if (timeline_arranger)
    {
      timeline_arranger_widget_set_cut_lines_visible (
        timeline_arranger);
    }

  arranger_widget_update_visibility (self);

  arranger_widget_refresh_cursor (
    self);

  /*if (num > 0)*/
    /*auto_scroll (self);*/

  return FALSE;
}

/**
 * On button press.
 *
 * This merely sets the number of clicks and
 * objects clicked on. It is always called
 * before drag_begin, so the logic is done in drag_begin.
 */
static void
multipress_pressed (
  GtkGestureMultiPress *gesture,
  gint                  n_press,
  gdouble               x,
  gdouble               y,
  ArrangerWidget *      self)
{
  GET_PRIVATE;

  /* set number of presses */
  ar_prv->n_press = n_press;

  /* set modifier button states */
  GdkModifierType state_mask =
    ui_get_state_mask (
      GTK_GESTURE (gesture));
  if (state_mask & GDK_SHIFT_MASK)
    ar_prv->shift_held = 1;
  if (state_mask & GDK_CONTROL_MASK)
    ar_prv->ctrl_held = 1;
}

/**
 * Called when an item needs to be created at the
 * given position.
 */
static void
create_item (ArrangerWidget * self,
             double           start_x,
             double           start_y)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);
  GET_ARRANGER_ALIASES (self);

  /* something will be created */
  Position pos;
  Track * track = NULL;
  AutomationTrack * at = NULL;
  int note, chord_index;
  Region * region = NULL;

  /* get the position */
  arranger_widget_px_to_pos (
    self, start_x, &pos, 1);

  /* snap it */
  if (!ar_prv->shift_held &&
      SNAP_GRID_ANY_SNAP (ar_prv->snap_grid))
    position_snap (
      NULL, &pos, NULL, NULL,
      ar_prv->snap_grid);

  if (timeline_arranger)
    {
      /* figure out if we are creating a region or
       * automation point */
      at =
        timeline_arranger_widget_get_automation_track_at_y (
          timeline_arranger, start_y);
      if (!at)
        track =
          timeline_arranger_widget_get_track_at_y (
            timeline_arranger, start_y);

      /* creating automation point */
      if (at)
        {
          timeline_arranger_widget_create_region (
            timeline_arranger,
            REGION_TYPE_AUTOMATION, track, NULL, at,
            &pos);
        }
      /* double click inside a track */
      else if (track)
        {
          TrackLane * lane =
            timeline_arranger_widget_get_track_lane_at_y (
              timeline_arranger, start_y);
          switch (track->type)
            {
            case TRACK_TYPE_INSTRUMENT:
              timeline_arranger_widget_create_region (
                timeline_arranger,
                REGION_TYPE_MIDI, track,
                lane, NULL, &pos);
              break;
            case TRACK_TYPE_MIDI:
              timeline_arranger_widget_create_region (
                timeline_arranger,
                REGION_TYPE_MIDI, track,
                lane, NULL, &pos);
              break;
            case TRACK_TYPE_AUDIO:
              break;
            case TRACK_TYPE_MASTER:
              break;
            case TRACK_TYPE_CHORD:
              timeline_arranger_widget_create_chord_or_scale (
                timeline_arranger, track,
                start_y, &pos);
            case TRACK_TYPE_AUDIO_BUS:
              break;
            case TRACK_TYPE_AUDIO_GROUP:
              break;
            case TRACK_TYPE_MARKER:
              timeline_arranger_widget_create_marker (
                timeline_arranger, track, &pos);
              break;
            default:
              /* TODO */
              break;
            }
        }
    }
  if (midi_arranger)
    {
      /* find the note and region at x,y */
      note =
        midi_arranger_widget_get_note_at_y (start_y);
      region =
        CLIP_EDITOR->region;

      /* create a note */
      if (region)
        {
          midi_arranger_widget_create_note (
            midi_arranger,
            &pos,
            note,
            (MidiRegion *) region);
        }
    }
  if (midi_modifier_arranger)
    {

    }
  if (audio_arranger)
    {

    }
  if (chord_arranger)
    {
      /* find the chord and region at x,y */
      chord_index =
        chord_arranger_widget_get_chord_at_y (
          start_y);
      region =
        CLIP_EDITOR->region;

      /* create a chord object */
      if (region)
        {
          chord_arranger_widget_create_chord (
            chord_arranger, &pos, chord_index,
            region);
        }
    }
  if (automation_arranger)
    {
      region =
        CLIP_EDITOR->region;

      if (region)
        {
          automation_arranger_widget_create_ap (
            automation_arranger, &pos, start_y,
            region);
        }
    }

  /* something is (likely) added so reallocate */
  gtk_widget_queue_allocate (GTK_WIDGET (self));
}

static void
drag_cancel (
  GtkGesture *       gesture,
  GdkEventSequence * sequence,
  ArrangerWidget *   self)
{
  g_message ("drag cancelled");
}

/**
 * Sets the start pos of the earliest object and
 * the flag whether the earliest object exists.
 */
static void
set_earliest_obj (
  ArrangerWidget * self)
{
  GET_PRIVATE;

  ArrangerSelections * sel =
    arranger_widget_get_selections (self);
  if (arranger_selections_has_any (sel))
    {
      arranger_selections_get_start_pos (
        sel, &ar_prv->earliest_obj_start_pos,
        F_NO_TRANSIENTS, F_GLOBAL);
      arranger_selections_set_cache_poses (sel);
      ar_prv->earliest_obj_exists = 1;
    }
  else
    {
      ar_prv->earliest_obj_exists = 0;
    }
}

/**
 * Checks for the first object hit, sets the
 * appropriate action and selects it.
 *
 * @return If an object was handled or not.
 */
static int
on_drag_begin_handle_hit_object (
  ArrangerWidget * self,
  const double     x,
  const double     y)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);

  ArrangerObjectWidget * obj_w =
    Z_ARRANGER_OBJECT_WIDGET (
      ui_get_hit_child (
        GTK_CONTAINER (self), x, y,
        ARRANGER_OBJECT_WIDGET_TYPE));

  if (!obj_w)
    return 0;

  /* get main object */
  ARRANGER_OBJECT_WIDGET_GET_PRIVATE (obj_w);
  ArrangerObject * obj =
    arranger_object_get_main (
      ao_prv->arranger_object);
  obj_w =
    Z_ARRANGER_OBJECT_WIDGET (obj->widget);

  /* get x as local to the object */
  gint wx, wy;
  gtk_widget_translate_coordinates (
    GTK_WIDGET (self),
    GTK_WIDGET (obj_w),
    (int) x, 0, &wx, &wy);

  /* remember object and pos */
  ar_prv->start_object = obj;
  ar_prv->start_pos_px = x;

  /* get flags */
  int is_resize_l =
    arranger_object_widget_is_resize_l (
      obj_w, wx);
  int is_resize_r =
    arranger_object_widget_is_resize_r (
      obj_w, wx);
  int is_resize_up =
    arranger_object_widget_is_resize_up (
      obj_w, wy);
  int is_resize_loop =
    arranger_object_widget_is_resize_loop (
      obj_w, wy);
  int show_cut_lines =
    arranger_object_widget_should_show_cut_lines (
      obj_w, ar_prv->alt_held);
  int is_selected =
    arranger_object_is_selected (obj);
  ar_prv->start_object_was_selected = is_selected;

  /* select object if unselected */
  switch (P_TOOL)
    {
    case TOOL_SELECT_NORMAL:
    case TOOL_SELECT_STRETCH:
    case TOOL_EDIT:
      /* if ctrl held & not selected, add to
       * selections */
      if (ar_prv->ctrl_held && !is_selected)
        {
          arranger_object_select (
            obj, F_SELECT, F_APPEND);
        }
      /* if ctrl not held & not selected, make it
       * the only
       * selection */
      else if (!ar_prv->ctrl_held && !is_selected)
        {
          arranger_object_select (
            obj, F_SELECT, F_NO_APPEND);
        }
      break;
    case TOOL_CUT:
      /* only select this object */
      arranger_object_select (
        obj, F_SELECT, F_NO_APPEND);
      break;
    default:
      break;
    }

  /* set editor region and show editor if double
   * click */
  if (obj->type == ARRANGER_OBJECT_TYPE_REGION)
    {
      clip_editor_set_region (
        CLIP_EDITOR, (Region *) obj);

      /* if double click bring up piano roll */
      if (ar_prv->n_press == 2 &&
          !ar_prv->ctrl_held)
        {
          /* set the bot panel visible */
          gtk_widget_set_visible (
            GTK_WIDGET (
              MW_BOT_DOCK_EDGE), 1);
          foldable_notebook_widget_set_visibility (
            MW_BOT_FOLDABLE_NOTEBOOK, 1);

          SHOW_CLIP_EDITOR;
        }
    }

#define SET_ACTION(x) \
  ar_prv->action = UI_OVERLAY_ACTION_##x

  /* update arranger action */
  switch (obj->type)
    {
    case ARRANGER_OBJECT_TYPE_REGION:
      switch (P_TOOL)
        {
        case TOOL_ERASER:
          SET_ACTION (ERASING);
          break;
        case TOOL_AUDITION:
          SET_ACTION (AUDITIONING);
          break;
        case TOOL_SELECT_NORMAL:
          if (is_resize_l && is_resize_loop)
            SET_ACTION (RESIZING_L_LOOP);
          else if (is_resize_l)
            SET_ACTION (RESIZING_L);
          else if (is_resize_r && is_resize_loop)
            SET_ACTION (RESIZING_R_LOOP);
          else if (is_resize_r)
            SET_ACTION (RESIZING_R);
          else if (show_cut_lines)
            SET_ACTION (CUTTING);
          else
            SET_ACTION (STARTING_MOVING);
          break;
        case TOOL_SELECT_STRETCH:
          if (is_resize_l)
            SET_ACTION (STRETCHING_L);
          else if (is_resize_r)
            SET_ACTION (STRETCHING_R);
          else
            SET_ACTION (STARTING_MOVING);
          break;
        case TOOL_EDIT:
          if (is_resize_l)
            SET_ACTION (RESIZING_L);
          else if (is_resize_r)
            SET_ACTION (RESIZING_R);
          else
            SET_ACTION (STARTING_MOVING);
          break;
        case TOOL_CUT:
          SET_ACTION (CUTTING);
          break;
        case TOOL_RAMP:
          /* TODO */
          break;
        }
      break;
    case ARRANGER_OBJECT_TYPE_MIDI_NOTE:
      switch (P_TOOL)
        {
        case TOOL_ERASER:
          SET_ACTION (ERASING);
          break;
        case TOOL_AUDITION:
          SET_ACTION (AUDITIONING);
          break;
        case TOOL_SELECT_NORMAL:
        case TOOL_EDIT:
          if ((is_resize_l) &&
              !PIANO_ROLL->drum_mode)
            SET_ACTION (RESIZING_L);
          else if (is_resize_r &&
                   !PIANO_ROLL->drum_mode)
            SET_ACTION (RESIZING_R);
          else
            SET_ACTION (STARTING_MOVING);
          break;
        case TOOL_SELECT_STRETCH:
          if (is_resize_l)
            SET_ACTION (STRETCHING_L);
          else if (is_resize_r)
            SET_ACTION (STRETCHING_R);
          else
            SET_ACTION (STARTING_MOVING);
          break;
        case TOOL_CUT:
          /* TODO */
          break;
        case TOOL_RAMP:
          break;
        }
      break;
    case ARRANGER_OBJECT_TYPE_AUTOMATION_POINT:
      SET_ACTION (STARTING_MOVING);
      break;
    case ARRANGER_OBJECT_TYPE_AUTOMATION_CURVE:
      SET_ACTION (NONE);
      break;
    case ARRANGER_OBJECT_TYPE_VELOCITY:
      if (is_resize_up)
        SET_ACTION (RESIZING_UP);
      else
        SET_ACTION (NONE);
      break;
    case ARRANGER_OBJECT_TYPE_CHORD_OBJECT:
      SET_ACTION (STARTING_MOVING);
      break;
    case ARRANGER_OBJECT_TYPE_SCALE_OBJECT:
      SET_ACTION (STARTING_MOVING);
      break;
    case ARRANGER_OBJECT_TYPE_MARKER:
      SET_ACTION (STARTING_MOVING);
      break;
    default:
      g_return_val_if_reached (0);
    }

#undef SET_ACTION

  return 1;
}

static void
drag_begin (
  GtkGestureDrag *   gesture,
  gdouble            start_x,
  gdouble            start_y,
  ArrangerWidget *   self)
{
  GET_PRIVATE;

  ar_prv->start_x = start_x;
  arranger_widget_px_to_pos (
    self, start_x, &ar_prv->start_pos, 1);
  ar_prv->start_y = start_y;

  if (!gtk_widget_has_focus (GTK_WIDGET (self)))
    gtk_widget_grab_focus (GTK_WIDGET (self));

  /* get current pos */
  arranger_widget_px_to_pos (
    self, ar_prv->start_x,
    &ar_prv->curr_pos, 1);

  /* get difference with drag start pos */
  ar_prv->curr_ticks_diff_from_start =
    position_get_ticks_diff (
      &ar_prv->curr_pos,
      &ar_prv->start_pos,
      NULL);

  /* handle hit object */
  int objects_hit =
    on_drag_begin_handle_hit_object (
      self, start_x, start_y);

  /* if nothing hit */
  if (!objects_hit)
    {
      /* single click */
      if (ar_prv->n_press == 1)
        {
          switch (P_TOOL)
            {
            case TOOL_SELECT_NORMAL:
            case TOOL_SELECT_STRETCH:
              /* selection */
              ar_prv->action =
                UI_OVERLAY_ACTION_STARTING_SELECTION;

              /* deselect all */
              arranger_widget_select_all (self, 0);

              /* if timeline, set either selecting
               * objects or selecting range */
              if (Z_IS_TIMELINE_ARRANGER_WIDGET (
                    self))
                {
                  timeline_arranger_widget_set_select_type (
                    (TimelineArrangerWidget *) self,
                    start_y);
                }
              break;
            case TOOL_EDIT:
              if (!ar_prv->ctrl_held)
                {
                  /* something is created */
                  create_item (
                    self, start_x, start_y);
                }
              else
                {
                  /* autofill */
                }
              break;
            case TOOL_ERASER:
              /* delete selection */
              ar_prv->action =
                UI_OVERLAY_ACTION_STARTING_DELETE_SELECTION;
              break;
            case TOOL_RAMP:
              ar_prv->action =
                UI_OVERLAY_ACTION_STARTING_RAMP;
              break;
            default:
              break;
            }
        }
      /* double click */
      else if (ar_prv->n_press == 2)
        {
          switch (P_TOOL)
            {
            case TOOL_SELECT_NORMAL:
            case TOOL_SELECT_STRETCH:
            case TOOL_EDIT:
              create_item (
                self, start_x, start_y);
              break;
            case TOOL_ERASER:
              /* delete selection */
              ar_prv->action =
                UI_OVERLAY_ACTION_STARTING_DELETE_SELECTION;
              break;
            default:
              break;
            }
        }
    }

  /* set the start pos of the earliest object and
   * the flag whether the earliest object exists */
  set_earliest_obj (self);

  arranger_widget_refresh_cursor (self);
}


static void
drag_update (
  GtkGestureDrag * gesture,
  gdouble         offset_x,
  gdouble         offset_y,
  ArrangerWidget * self)
{
  GET_PRIVATE;

  /* state mask needs to be updated */
  GdkModifierType state_mask =
    ui_get_state_mask (
      GTK_GESTURE (gesture));
  if (state_mask & GDK_SHIFT_MASK)
    ar_prv->shift_held = 1;
  else
    ar_prv->shift_held = 0;
  if (state_mask & GDK_CONTROL_MASK)
    ar_prv->ctrl_held = 1;
  else
    ar_prv->ctrl_held = 0;

  GET_ARRANGER_ALIASES (self);

  /* get current pos */
  arranger_widget_px_to_pos (
    self, ar_prv->start_x + offset_x,
    &ar_prv->curr_pos, 1);

  /* get difference with drag start pos */
  ar_prv->curr_ticks_diff_from_start =
    position_get_ticks_diff (
      &ar_prv->curr_pos,
      &ar_prv->start_pos,
      NULL);

  if (ar_prv->earliest_obj_exists)
    {
      /* add diff to the earliest object's start pos
       * and snap it, then get the diff ticks */
      Position earliest_obj_new_pos;
      position_set_to_pos (
        &earliest_obj_new_pos,
        &ar_prv->earliest_obj_start_pos);
      position_add_ticks (
        &earliest_obj_new_pos,
        ar_prv->curr_ticks_diff_from_start);

      if (position_is_before (
            &earliest_obj_new_pos, &POSITION_START))
        {
          /* stop at 0.0.0.0 */
          position_set_to_pos (
            &earliest_obj_new_pos, &POSITION_START);
        }
      else if (!ar_prv->shift_held &&
          SNAP_GRID_ANY_SNAP (ar_prv->snap_grid))
        {
          position_snap (
            NULL, &earliest_obj_new_pos, NULL,
            NULL, ar_prv->snap_grid);
        }
      ar_prv->adj_ticks_diff =
        position_get_ticks_diff (
          &earliest_obj_new_pos,
          &ar_prv->earliest_obj_start_pos,
          NULL);
    }

  /* set action to selecting if starting selection.
   * this
   * is because drag_update never gets called if
   * it's just
   * a click, so we can check at drag_end and see if
   * anything was selected */
  switch (ar_prv->action)
    {
    case UI_OVERLAY_ACTION_STARTING_SELECTION:
      ar_prv->action =
        UI_OVERLAY_ACTION_SELECTING;
      break;
    case UI_OVERLAY_ACTION_STARTING_DELETE_SELECTION:
      ar_prv->action =
        UI_OVERLAY_ACTION_DELETE_SELECTING;
      break;
    case UI_OVERLAY_ACTION_STARTING_MOVING:
      if (ar_prv->ctrl_held)
        ar_prv->action =
          UI_OVERLAY_ACTION_MOVING_COPY;
      else
        ar_prv->action =
          UI_OVERLAY_ACTION_MOVING;
      break;
    case UI_OVERLAY_ACTION_MOVING:
      if (ar_prv->ctrl_held)
        ar_prv->action =
          UI_OVERLAY_ACTION_MOVING_COPY;
      break;
    case UI_OVERLAY_ACTION_MOVING_COPY:
      if (!ar_prv->ctrl_held)
        ar_prv->action =
          UI_OVERLAY_ACTION_MOVING;
      break;
    case UI_OVERLAY_ACTION_STARTING_RAMP:
      ar_prv->action =
        UI_OVERLAY_ACTION_RAMPING;
      break;
    default:
      break;
    }

  /* update visibility */
  arranger_widget_update_visibility (self);

  switch (ar_prv->action)
    {
    /* if drawing a selection */
    case UI_OVERLAY_ACTION_SELECTING:
      /* find and select objects inside selection */
#define DO_SELECT(sc) \
  if (sc##_arranger) \
    { \
      sc##_arranger_widget_select ( \
        sc##_arranger, offset_x, offset_y, \
        F_NO_DELETE); \
    }
      FORALL_ARRANGERS (DO_SELECT);
#undef DO_SELECT
      EVENTS_PUSH (
        ET_SELECTING_IN_ARRANGER, self);
      break;
    case UI_OVERLAY_ACTION_DELETE_SELECTING:
      /* find and delete objects inside
       * selection */
#define DO_SELECT(sc) \
  if (sc##_arranger) \
    { \
      sc##_arranger_widget_select ( \
        sc##_arranger, offset_x, offset_y, \
        F_DELETE); \
    }
      FORALL_ARRANGERS (DO_SELECT);
#undef DO_SELECT
      EVENTS_PUSH (
        ET_SELECTING_IN_ARRANGER, self);
      break;
    case UI_OVERLAY_ACTION_RESIZING_L_LOOP:
      /* snap selections based on new pos */
      if (timeline_arranger)
        {
          int ret =
            timeline_arranger_widget_snap_regions_l (
              timeline_arranger,
              &ar_prv->curr_pos, 1);
          if (!ret)
            timeline_arranger_widget_snap_regions_l (
              timeline_arranger,
              &ar_prv->curr_pos, 0);
        }
      break;
    case UI_OVERLAY_ACTION_RESIZING_L:
      /* snap selections based on new pos */
      if (timeline_arranger)
        {
          int ret =
            timeline_arranger_widget_snap_regions_l (
              timeline_arranger,
              &ar_prv->curr_pos, 1);
          if (!ret)
            timeline_arranger_widget_snap_regions_l (
              timeline_arranger,
              &ar_prv->curr_pos, 0);
        }
      else if (midi_arranger)
        {
          int ret =
            midi_arranger_widget_snap_midi_notes_l (
              midi_arranger,
              &ar_prv->curr_pos, 1);
          if (!ret)
            midi_arranger_widget_snap_midi_notes_l (
              midi_arranger,
              &ar_prv->curr_pos, 0);
        }
      break;
    case UI_OVERLAY_ACTION_STRETCHING_L:
      /* TODO */
      g_message ("stretching L");
      break;
    case UI_OVERLAY_ACTION_RESIZING_R_LOOP:
      if (timeline_arranger)
        {
          if (timeline_arranger->resizing_range)
            timeline_arranger_widget_snap_range_r (
              &ar_prv->curr_pos);
          else
            {
              int ret =
                timeline_arranger_widget_snap_regions_r (
                  timeline_arranger,
                  &ar_prv->curr_pos, 1);
              if (!ret)
                timeline_arranger_widget_snap_regions_r (
                  timeline_arranger,
                  &ar_prv->curr_pos, 0);
            }
        }
      break;
    case UI_OVERLAY_ACTION_RESIZING_R:
    case UI_OVERLAY_ACTION_CREATING_RESIZING_R:
      if (timeline_arranger)
        {
          if (timeline_arranger->resizing_range)
            timeline_arranger_widget_snap_range_r (
              &ar_prv->curr_pos);
          else
            {
              int ret =
                timeline_arranger_widget_snap_regions_r (
                  timeline_arranger,
                  &ar_prv->curr_pos, 1);
              if (!ret)
                timeline_arranger_widget_snap_regions_r (
                  timeline_arranger,
                  &ar_prv->curr_pos, 0);
            }
        }
      else if (midi_arranger)
        {
          int ret =
            midi_arranger_widget_snap_midi_notes_r (
              midi_arranger,
              &ar_prv->curr_pos, 1);
          if (!ret)
            midi_arranger_widget_snap_midi_notes_r (
              midi_arranger,
              &ar_prv->curr_pos, 0);
        }
      break;
    case UI_OVERLAY_ACTION_STRETCHING_R:
      g_message ("stretching R");
      break;
    case UI_OVERLAY_ACTION_RESIZING_UP:
      if (midi_modifier_arranger)
        {
          midi_modifier_arranger_widget_resize_velocities (
            midi_modifier_arranger,
            offset_y);
        }
      break;
    case UI_OVERLAY_ACTION_MOVING:
    case UI_OVERLAY_ACTION_CREATING_MOVING:
#define MOVE_ITEMS(_arranger) \
      if (_arranger) \
        { \
          move_items_x ( \
            (ArrangerWidget *) _arranger, \
            ar_prv->adj_ticks_diff); \
          _arranger##_widget_move_items_y ( \
            _arranger, \
            offset_y); \
        }
      MOVE_ITEMS (
        timeline_arranger);
      MOVE_ITEMS (
        midi_arranger);
      MOVE_ITEMS (
        automation_arranger);
      MOVE_ITEMS (
        chord_arranger);
      break;
    case UI_OVERLAY_ACTION_MOVING_COPY:
      MOVE_ITEMS (
        timeline_arranger);
      MOVE_ITEMS (
        midi_arranger);
      MOVE_ITEMS (
        automation_arranger);
      MOVE_ITEMS (
        chord_arranger);
#undef MOVE_ITEMS
      break;
    case UI_OVERLAY_ACTION_AUTOFILLING:
      /* TODO */
      g_message ("autofilling");
      break;
    case UI_OVERLAY_ACTION_AUDITIONING:
      /* TODO */
      g_message ("auditioning");
      break;
    case UI_OVERLAY_ACTION_RAMPING:
      /* find and select objects inside selection */
      if (timeline_arranger)
        {
          /* TODO */
        }
      else if (midi_arranger)
        {
          /* TODO */
        }
      else if (midi_modifier_arranger)
        {
          midi_modifier_arranger_widget_ramp (
            midi_modifier_arranger,
            offset_x,
            offset_y);
        }
      else if (audio_arranger)
        {
          /* TODO */
        }
      break;
    case UI_OVERLAY_ACTION_CUTTING:
      /* nothing to do, wait for drag end */
      break;
    default:
      /* TODO */
      break;
    }

  if (ar_prv->action != UI_OVERLAY_ACTION_NONE)
    auto_scroll (
      self, (int) (ar_prv->start_x + offset_x),
      (int) (ar_prv->start_y + offset_y));

  /* update last offsets */
  ar_prv->last_offset_x = offset_x;
  ar_prv->last_offset_y = offset_y;

  arranger_widget_refresh_cursor (self);
}
static void
drag_end (GtkGestureDrag *gesture,
               gdouble         offset_x,
               gdouble         offset_y,
               gpointer        user_data)
{
  ArrangerWidget * self =
    (ArrangerWidget *) user_data;
  GET_PRIVATE;

  if (ACTION_IS (SELECTING) ||
      ACTION_IS (DELETE_SELECTING))
    {
      EVENTS_PUSH (
        ET_SELECTING_IN_ARRANGER, self);
    }

  GET_ARRANGER_ALIASES (self);

#define ON_DRAG_END(sc) \
  if (sc##_arranger) \
    sc##_arranger_widget_on_drag_end ( \
      sc##_arranger)

  FORALL_ARRANGERS (ON_DRAG_END);

#undef ON_DRAG_END

  /* if clicked on nothing */
  if (ar_prv->action ==
           UI_OVERLAY_ACTION_STARTING_SELECTION)
    {
      /* deselect all */
      arranger_widget_select_all (self, 0);
    }

  /* reset start coordinates and offsets */
  ar_prv->start_x = 0;
  ar_prv->start_y = 0;
  ar_prv->last_offset_x = 0;
  ar_prv->last_offset_y = 0;
  ar_prv->start_object = NULL;

  ar_prv->shift_held = 0;
  ar_prv->ctrl_held = 0;

  /* reset action */
  ar_prv->action = UI_OVERLAY_ACTION_NONE;

  /* queue redraw to hide the selection */
  gtk_widget_queue_draw (GTK_WIDGET (ar_prv->bg));

  arranger_widget_refresh_cursor (self);
}
/**
 * Returns the ArrangerObject of the given type
 * at (x,y).
 */
ArrangerObject *
arranger_widget_get_hit_arranger_object (
  ArrangerWidget *   self,
  ArrangerObjectType type,
  const double       x,
  const double       y)
{
  ArrangerObjectWidget * obj_w =
    Z_ARRANGER_OBJECT_WIDGET (
      ui_get_hit_child (
        GTK_CONTAINER (self), x, y,
        arranger_object_get_widget_type_for_type (
          type)));
  if (obj_w)
    {
      ARRANGER_OBJECT_WIDGET_GET_PRIVATE (obj_w);
      return ao_prv->arranger_object;
    }
  else
    {
      return NULL;
    }
}

int
arranger_widget_pos_to_px (
  ArrangerWidget * self,
  Position * pos,
  int        use_padding)
{
  GET_ARRANGER_ALIASES (self);

  if (timeline_arranger)
    return ui_pos_to_px_timeline (
             pos, use_padding);
  else if (midi_arranger ||
           midi_modifier_arranger ||
           audio_arranger ||
           automation_arranger ||
           chord_arranger)
    return ui_pos_to_px_editor (
             pos, use_padding);

  return -1;
}

/**
 * Returns the ArrangerSelections for this
 * ArrangerWidget.
 */
ArrangerSelections *
arranger_widget_get_selections (
  ArrangerWidget * self)
{
  GET_ARRANGER_ALIASES (self);

  if (timeline_arranger)
    return
      (ArrangerSelections *) TL_SELECTIONS;
  else if (midi_arranger || midi_modifier_arranger)
    return
      (ArrangerSelections *) MA_SELECTIONS;
  else if (automation_arranger)
    return
      (ArrangerSelections *) AUTOMATION_SELECTIONS;
  else if (chord_arranger)
    return
      (ArrangerSelections *) CHORD_SELECTIONS;
  else if (audio_arranger)
    /* FIXME */
    return
      (ArrangerSelections *) TL_SELECTIONS;

  g_return_val_if_reached (NULL);
}

/**
 * Sets transient object and actual object
 * visibility for every ArrangerObject in the
 * ArrangerWidget based on the current action.
 */
void
arranger_widget_update_visibility (
  ArrangerWidget * self)
{
  GList *children, *iter;
  ArrangerObjectWidget * aow;

#define UPDATE_VISIBILITY(x) \
  children = \
    gtk_container_get_children ( \
      GTK_CONTAINER (x)); \
  aow = NULL; \
  for (iter = children; \
       iter != NULL; \
       iter = g_list_next (iter)) \
    { \
      if (!Z_IS_ARRANGER_OBJECT_WIDGET (iter->data)) \
        continue; \
 \
      aow = \
        Z_ARRANGER_OBJECT_WIDGET (iter->data); \
      ARRANGER_OBJECT_WIDGET_GET_PRIVATE (aow); \
      g_warn_if_fail (ao_prv->arranger_object); \
      arranger_object_set_widget_visibility_and_state ( \
        ao_prv->arranger_object, 1); \
    } \
  g_list_free (children);

  UPDATE_VISIBILITY (self);

  /* if midi arranger, do the same for midi modifier
   * arranger, and vice versa */
  if (Z_IS_MIDI_ARRANGER_WIDGET (self))
    {
      UPDATE_VISIBILITY (MW_MIDI_MODIFIER_ARRANGER);
    }
  else if (Z_IS_MIDI_MODIFIER_ARRANGER_WIDGET (self))
    {
      UPDATE_VISIBILITY (MW_MIDI_ARRANGER);
    }

#undef UPDATE_VISIBILITY
}

/**
 * Causes a redraw of the background.
 */
void
arranger_widget_redraw_bg (
  ArrangerWidget * self)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);
  ARRANGER_BG_WIDGET_GET_PRIVATE (ar_prv->bg);
  ab_prv->redraw = 1;
  gtk_widget_queue_draw (
    GTK_WIDGET (ar_prv->bg));
}

/**
 * Gets the corresponding scrolled window.
 */
GtkScrolledWindow *
arranger_widget_get_scrolled_window (
  ArrangerWidget * self)
{
  GET_ARRANGER_ALIASES (self);

  if (timeline_arranger)
    return MW_TIMELINE_PANEL->timeline_scroll;
  else if (midi_arranger)
    return MW_MIDI_EDITOR_SPACE->arranger_scroll;
  else if (midi_modifier_arranger)
    return MW_MIDI_EDITOR_SPACE->
      modifier_arranger_scroll;
  else if (audio_arranger)
    return MW_AUDIO_EDITOR_SPACE->arranger_scroll;
  else if (chord_arranger)
    return MW_CHORD_EDITOR_SPACE->arranger_scroll;
  else if (automation_arranger)
    return MW_AUTOMATION_EDITOR_SPACE->
      arranger_scroll;

  return NULL;
}

static gboolean
arranger_tick_cb (
  GtkWidget *     widget,
  GdkFrameClock * frame_clock,
  gpointer        user_data)
{
  ARRANGER_WIDGET_GET_PRIVATE (widget);

  gint64 frame_time =
    gdk_frame_clock_get_frame_time (frame_clock);

  if (gtk_widget_get_visible (widget) &&
      (frame_time - ar_prv->last_frame_time) >
        15000)
    {
      gtk_widget_queue_allocate (widget);
    }
  ar_prv->last_frame_time = frame_time;

  return G_SOURCE_CONTINUE;
}

static gboolean
on_scroll (
  GtkWidget *widget,
  GdkEventScroll  *event,
  ArrangerWidget * self)
{
  arranger_widget_redraw_bg (self);

  GET_ARRANGER_ALIASES (self);
  if (!(event->state & GDK_CONTROL_MASK))
    return FALSE;

  double x = event->x,
         /*y = event->y,*/
         adj_val,
         diff;
  Position cursor_pos;
   GtkScrolledWindow * scroll =
    arranger_widget_get_scrolled_window (self);
  GtkAdjustment * adj;
  int new_x;
  RulerWidget * ruler = NULL;
  RulerWidgetPrivate * rw_prv;

  if (timeline_arranger)
    ruler = Z_RULER_WIDGET (MW_RULER);
  else if (midi_modifier_arranger ||
           midi_arranger ||
           audio_arranger ||
           chord_arranger ||
           automation_arranger)
    ruler = Z_RULER_WIDGET (EDITOR_RULER);

  rw_prv = ruler_widget_get_private (ruler);

  /* get current adjustment so we can get the
   * difference from the cursor */
  adj = gtk_scrolled_window_get_hadjustment (
    scroll);
  adj_val = gtk_adjustment_get_value (adj);

  /* get positions of cursor */
  arranger_widget_px_to_pos (
    self, x, &cursor_pos, 1);

  /* get px diff so we can calculate the new
   * adjustment later */
  diff = x - adj_val;

  /* scroll down, zoom out */
  if (event->delta_y > 0)
    {
      ruler_widget_set_zoom_level (
        ruler,
        rw_prv->zoom_level / 1.3);
    }
  else /* scroll up, zoom in */
    {
      ruler_widget_set_zoom_level (
        ruler,
        rw_prv->zoom_level * 1.3);
    }

  new_x = arranger_widget_pos_to_px (
    self, &cursor_pos, 1);

  /* refresh relevant widgets */
  if (timeline_arranger)
    timeline_minimap_widget_refresh (
      MW_TIMELINE_MINIMAP);

  /* get updated adjustment and set its value
   at the same offset as before */
  adj = gtk_scrolled_window_get_hadjustment (
    scroll);
  gtk_adjustment_set_value (adj,
                            new_x - diff);

  return TRUE;
}

/**
 * Motion callback.
 */
static gboolean
on_motion (
  GtkEventControllerMotion * event,
  gdouble                    x,
  gdouble                    y,
  ArrangerWidget * self)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);
  GET_ARRANGER_ALIASES (self);

  ar_prv->hover_x = x;
  ar_prv->hover_y = y;

  GdkModifierType state;
  int has_state =
    gtk_get_current_event_state (&state);
  if (has_state)
    {
      ar_prv->alt_held =
        state & GDK_MOD1_MASK;
      ar_prv->ctrl_held =
        state & GDK_CONTROL_MASK;
      ar_prv->shift_held =
        state & GDK_SHIFT_MASK;
    }

  arranger_widget_refresh_cursor (self);

  if (timeline_arranger)
    {
      timeline_arranger_widget_set_cut_lines_visible (
        timeline_arranger);
    }
  else if (automation_arranger)
    {
    }
  else if (chord_arranger)
    {
    }
  else if (audio_arranger)
    {
    }
  else if (midi_modifier_arranger)
    {
    }
  else if (midi_arranger)
    {
    }

  return FALSE;
}

static gboolean
on_focus_out (GtkWidget *widget,
               GdkEvent  *event,
               ArrangerWidget * self)
{
  g_message ("arranger focus out");
  ARRANGER_WIDGET_GET_PRIVATE (self);

  ar_prv->alt_held = 0;
  ar_prv->ctrl_held = 0;
  ar_prv->shift_held = 0;

  return FALSE;
}

static gboolean
on_grab_broken (GtkWidget *widget,
               GdkEvent  *event,
               gpointer   user_data)
{
  /*GdkEventGrabBroken * ev =*/
    /*(GdkEventGrabBroken *) event;*/
  g_warning ("arranger grab broken");
  return FALSE;
}

void
arranger_widget_setup (
  ArrangerWidget *   self,
  SnapGrid *         snap_grid)
{
  GET_PRIVATE;

  GET_ARRANGER_ALIASES (self);

  ar_prv->snap_grid = snap_grid;

  /* create and set background widgets, setup */
  if (midi_arranger)
    {
      ar_prv->bg = (ArrangerBgWidget *) (
        midi_arranger_bg_widget_new (
          Z_RULER_WIDGET (EDITOR_RULER),
          self));
      midi_arranger_widget_setup (
        midi_arranger);
    }
  else if (timeline_arranger)
    {
      ar_prv->bg = (ArrangerBgWidget *) (
        timeline_bg_widget_new (
          Z_RULER_WIDGET (MW_RULER),
          self));
      timeline_arranger_widget_setup (
        timeline_arranger);
    }
  else if (midi_modifier_arranger)
    {
      ar_prv->bg = (ArrangerBgWidget *) (
        midi_modifier_arranger_bg_widget_new (
          Z_RULER_WIDGET (EDITOR_RULER),
          self));
      midi_modifier_arranger_widget_setup (
        midi_modifier_arranger);
    }
  else if (audio_arranger)
    {
      ar_prv->bg = (ArrangerBgWidget *) (
        audio_arranger_bg_widget_new (
          Z_RULER_WIDGET (EDITOR_RULER),
          self));
      audio_arranger_widget_setup (
        audio_arranger);
    }
  else if (chord_arranger)
    {
      ar_prv->bg = (ArrangerBgWidget *) (
        chord_arranger_bg_widget_new (
          Z_RULER_WIDGET (EDITOR_RULER),
          self));
      chord_arranger_widget_setup (
        chord_arranger);
    }
  else if (automation_arranger)
    {
      ar_prv->bg = (ArrangerBgWidget *) (
        automation_arranger_bg_widget_new (
          Z_RULER_WIDGET (EDITOR_RULER),
          self));
      automation_arranger_widget_setup (
        automation_arranger);
    }
  gtk_container_add (
    GTK_CONTAINER (self),
    GTK_WIDGET (ar_prv->bg));
  gtk_widget_add_events (
    GTK_WIDGET (ar_prv->bg),
    GDK_ALL_EVENTS_MASK);


  /* add the playhead */
  ar_prv->playhead =
    arranger_playhead_widget_new ();
  gtk_overlay_add_overlay (
    GTK_OVERLAY (self),
    GTK_WIDGET (ar_prv->playhead));

  /* make the arranger able to notify */
  gtk_widget_add_events (GTK_WIDGET (self),
                         GDK_ALL_EVENTS_MASK);
  gtk_widget_set_can_focus (GTK_WIDGET (self),
                           1);
  gtk_widget_set_focus_on_click (GTK_WIDGET (self),
                                 1);

  /* connect signals */
  g_signal_connect (
    G_OBJECT (self), "get-child-position",
    G_CALLBACK (arranger_get_child_position), self);
  g_signal_connect (
    G_OBJECT(ar_prv->drag), "drag-begin",
    G_CALLBACK (drag_begin),  self);
  g_signal_connect (
    G_OBJECT(ar_prv->drag), "drag-update",
    G_CALLBACK (drag_update),  self);
  g_signal_connect (
    G_OBJECT(ar_prv->drag), "drag-end",
    G_CALLBACK (drag_end),  self);
  g_signal_connect (
    G_OBJECT (ar_prv->drag), "cancel",
    G_CALLBACK (drag_cancel), self);
  g_signal_connect (
    G_OBJECT (ar_prv->multipress), "pressed",
    G_CALLBACK (multipress_pressed), self);
  g_signal_connect (
    G_OBJECT (ar_prv->right_mouse_mp), "pressed",
    G_CALLBACK (on_right_click), self);
  g_signal_connect (
    G_OBJECT (self), "scroll-event",
    G_CALLBACK (on_scroll), self);
  g_signal_connect (
    G_OBJECT (self), "key-press-event",
    G_CALLBACK (on_key_action), self);
  g_signal_connect (
    G_OBJECT (self), "key-release-event",
    G_CALLBACK (on_key_release_action), self);
  g_signal_connect (
    G_OBJECT (ar_prv->motion_controller), "motion",
    G_CALLBACK (on_motion), self);
  g_signal_connect (
    G_OBJECT (self), "focus-out-event",
    G_CALLBACK (on_focus_out), self);
  g_signal_connect (
    G_OBJECT (self), "grab-broken-event",
    G_CALLBACK (on_grab_broken), self);

  gtk_widget_add_tick_callback (
    GTK_WIDGET (self), arranger_tick_cb,
    NULL, NULL);
}

/**
 * Wrapper for ui_px_to_pos depending on the arranger
 * type.
 */
void
arranger_widget_px_to_pos (
  ArrangerWidget * self,
  double           px,
  Position *       pos,
  int              has_padding)
{
  GET_ARRANGER_ALIASES (self);

  if (timeline_arranger)
    ui_px_to_pos_timeline (
      px, pos, has_padding);
  else if (midi_arranger ||
           midi_modifier_arranger ||
           chord_arranger ||
           audio_arranger ||
           automation_arranger)
    ui_px_to_pos_editor (
      px, pos, has_padding);
}

void
arranger_widget_refresh_cursor (
  ArrangerWidget * self)
{
  ARRANGER_WIDGET_GET_PRIVATE (self);
  GET_ARRANGER_ALIASES (self);

  if (!gtk_widget_get_realized (
        GTK_WIDGET (self)))
    return;

  ArrangerCursor ac = ARRANGER_CURSOR_SELECT;

#define GET_CURSOR(sc) \
  if (sc##_arranger) \
    ac = \
      sc##_arranger_widget_get_cursor ( \
        sc##_arranger, ar_prv->action, \
        P_TOOL)

  FORALL_ARRANGERS (GET_CURSOR);

#undef GET_CURSOR

  arranger_widget_set_cursor (
    self, ac);
}

/**
 * Readd children.
 */
int
arranger_widget_refresh (
  ArrangerWidget * self)
{
  arranger_widget_set_cursor (
    self, ARRANGER_CURSOR_SELECT);
  ARRANGER_WIDGET_GET_PRIVATE (self);

  GET_ARRANGER_ALIASES (self);

  if (midi_arranger)
    {
      midi_arranger_widget_set_size (
        midi_arranger);
      midi_arranger_widget_refresh_children (
        midi_arranger);
    }
  else if (timeline_arranger)
    {
      timeline_arranger_widget_set_size (
        timeline_arranger);
      timeline_arranger_widget_refresh_children (
        timeline_arranger);
    }
  else if (midi_modifier_arranger)
    {
      RULER_WIDGET_GET_PRIVATE (EDITOR_RULER);
      gtk_widget_set_size_request (
        GTK_WIDGET (self),
        (int) rw_prv->total_px,
        -1);
      midi_modifier_arranger_widget_refresh_children (
        midi_modifier_arranger);
    }
  else if (audio_arranger)
    {
      RULER_WIDGET_GET_PRIVATE (EDITOR_RULER);
      gtk_widget_set_size_request (
        GTK_WIDGET (self),
        (int) rw_prv->total_px,
        -1);
      audio_arranger_widget_refresh_children (
        audio_arranger);
    }
  else if (chord_arranger)
    {
      RULER_WIDGET_GET_PRIVATE (EDITOR_RULER);
      gtk_widget_set_size_request (
        GTK_WIDGET (self),
        (int) rw_prv->total_px,
        -1);
      chord_arranger_widget_refresh_children (
        chord_arranger);
    }
  else if (automation_arranger)
    {
      RULER_WIDGET_GET_PRIVATE (EDITOR_RULER);
      gtk_widget_set_size_request (
        GTK_WIDGET (self),
        (int) rw_prv->total_px,
        -1);
      automation_arranger_widget_refresh_children (
        automation_arranger);
    }

	if (ar_prv->bg)
	{
		arranger_bg_widget_refresh (ar_prv->bg);
		arranger_widget_refresh_cursor (self);
	}

	return FALSE;
}

static void
arranger_widget_class_init (
  ArrangerWidgetClass * _klass)
{
}

static void
arranger_widget_init (
  ArrangerWidget *self)
{
  GET_PRIVATE;

  ar_prv->drag =
    GTK_GESTURE_DRAG (
      gtk_gesture_drag_new (GTK_WIDGET (self)));
  gtk_event_controller_set_propagation_phase (
    GTK_EVENT_CONTROLLER (ar_prv->drag),
    GTK_PHASE_CAPTURE);
  ar_prv->multipress =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (
        GTK_WIDGET (self)));
  gtk_event_controller_set_propagation_phase (
    GTK_EVENT_CONTROLLER (ar_prv->multipress),
    GTK_PHASE_CAPTURE);
  ar_prv->right_mouse_mp =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (
        GTK_WIDGET (self)));
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (
      ar_prv->right_mouse_mp),
      GDK_BUTTON_SECONDARY);
  ar_prv->motion_controller =
    GTK_EVENT_CONTROLLER_MOTION (
      gtk_event_controller_motion_new (
        GTK_WIDGET (self)));
}
