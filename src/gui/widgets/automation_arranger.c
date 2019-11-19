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
 *
 * The automation containing regions and other
 * objects.
 */

#include "actions/arranger_selections.h"
#include "actions/undoable_action.h"
#include "actions/undo_manager.h"
#include "audio/automation_region.h"
#include "audio/automation_track.h"
#include "audio/automation_tracklist.h"
#include "audio/audio_track.h"
#include "audio/audio_bus_track.h"
#include "audio/channel.h"
#include "audio/chord_object.h"
#include "audio/chord_track.h"
#include "audio/instrument_track.h"
#include "audio/marker_track.h"
#include "audio/master_track.h"
#include "audio/midi_region.h"
#include "audio/mixer.h"
#include "audio/scale_object.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "audio/transport.h"
#include "gui/backend/automation_selections.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/automation_arranger.h"
#include "gui/widgets/automation_editor_space.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/chord_object.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/editor_ruler.h"
#include "gui/widgets/inspector.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/marker.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_arranger_bg.h"
#include "gui/widgets/midi_region.h"
#include "gui/widgets/pinned_tracklist.h"
#include "gui/widgets/midi_note.h"
#include "gui/widgets/region.h"
#include "gui/widgets/scale_object.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/automation_arranger.h"
#include "gui/widgets/automation_arranger_bg.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/arrays.h"
#include "utils/cairo.h"
#include "utils/flags.h"
#include "utils/objects.h"
#include "utils/ui.h"
#include "zrythm.h"

#include <gtk/gtk.h>

/**
 * Returns Y in pixels from the value based on the
 * allocation of the arranger.
 */
/*static int*/
/*get_automation_point_y (*/
  /*ArrangerWidget * self,*/
  /*AutomationPoint *          ap)*/
/*{*/
  /*[> ratio of current value in the range <]*/
  /*float ap_ratio = ap->normalized_val;*/

  /*int allocated_h =*/
    /*gtk_widget_get_allocated_height (*/
      /*GTK_WIDGET (self));*/
  /*int point =*/
    /*allocated_h -*/
    /*(int) (ap_ratio * (float) allocated_h);*/
  /*return point;*/
/*}*/

/**
 * To be called from get_child_position in parent widget.
 *
 * Used to allocate the overlay children.
 */
/*void*/
/*automation_arranger_widget_draw_ap (*/
  /*ArrangerWidget * self,*/
  /*GtkWidget *          widget,*/
  /*GdkRectangle *       allocation)*/
/*{*/
  /*if (Z_IS_AUTOMATION_POINT_WIDGET (widget))*/
    /*{*/
      /*AutomationPointWidget * ap_widget =*/
        /*Z_AUTOMATION_POINT_WIDGET (widget);*/
      /*AutomationPoint * ap =*/
        /*ap_widget->ap;*/
      /*ArrangerObject * ap_obj =*/
        /*(ArrangerObject *) ap;*/

      /* use transient or non transient region
       * depending on which is visible */
      /*Region * region = ap->region;*/
      /*region =*/
        /*region_get_visible_counterpart (region);*/
      /*ArrangerObject * region_obj =*/
        /*(ArrangerObject *) region;*/

      /*[> use absolute position <]*/
      /*long region_start_ticks =*/
        /*region_obj->pos.total_ticks;*/
      /*Position tmp;*/
      /*position_from_ticks (*/
        /*&tmp,*/
        /*region_start_ticks +*/
        /*ap_obj->pos.total_ticks);*/
      /*allocation->x =*/
        /*ui_pos_to_px_editor (*/
          /*&tmp, 1) -*/
          /*AP_WIDGET_POINT_SIZE / 2;*/
      /*g_warn_if_fail (allocation->x >= 0);*/

      /*AutomationPoint * next_ap =*/
        /*automation_region_get_next_ap (*/
          /*ap->region, ap);*/

      /*if (next_ap)*/
        /*{*/
          /*ArrangerObject * next_obj =*/
            /*(ArrangerObject *) next_ap;*/

          /* get relative position from the
           * start AP to the next ap. */
          /*position_from_ticks (*/
            /*&tmp,*/
            /*next_obj->pos.total_ticks -*/
              /*ap_obj->pos.total_ticks);*/

          /* width is the relative position in px
           * plus half an AP_WIDGET_POINT_SIZE for
           * each side */
          /*allocation->width =*/
            /*AP_WIDGET_POINT_SIZE +*/
            /*ui_pos_to_px_editor (&tmp, 0);*/

          /*int cur_y =*/
            /*get_automation_point_y (self, ap);*/
          /*int next_y =*/
            /*get_automation_point_y (self, next_ap);*/

          /*allocation->y =*/
            /*(cur_y > next_y ?*/
             /*next_y : cur_y) -*/
            /*AP_WIDGET_POINT_SIZE / 2;*/

          /* height is the relative relative diff in
           * px between the two points plus half an
           * AP_WIDGET_POINT_SIZE for each side */
          /*allocation->height =*/
            /*(cur_y > next_y ?*/
             /*cur_y - next_y :*/
             /*next_y - cur_y) + AP_WIDGET_POINT_SIZE;*/
        /*}*/
      /*else*/
        /*{*/
          /*allocation->y =*/
            /*(get_automation_point_y (self, ap)) -*/
            /*AP_WIDGET_POINT_SIZE / 2;*/

          /*allocation->width = AP_WIDGET_POINT_SIZE;*/
          /*allocation->height = AP_WIDGET_POINT_SIZE;*/
        /*}*/
    /*}*/
/*}*/

/**
 * Returns the appropriate cursor based on the
 * current hover_x and y.
 */
/*ArrangerCursor*/
/*automation_arranger_widget_get_cursor (*/
  /*ArrangerWidget * self,*/
  /*UiOverlayAction action,*/
  /*Tool            tool)*/
/*{*/
  /*ArrangerCursor ac = ARRANGER_CURSOR_SELECT;*/

  /*ARRANGER_WIDGET_GET_PRIVATE (self);*/

  /*int is_hit =*/
    /*arranger_widget_get_hit_arranger_object (*/
      /*(ArrangerWidget *) self,*/
      /*ARRANGER_OBJECT_TYPE_AUTOMATION_POINT,*/
      /*ar_prv->hover_x, ar_prv->hover_y) != NULL;*/

  /*switch (action)*/
    /*{*/
    /*case UI_OVERLAY_ACTION_NONE:*/
      /*switch (P_TOOL)*/
        /*{*/
        /*case TOOL_SELECT_NORMAL:*/
        /*{*/
          /*if (is_hit)*/
            /*{*/
              /*return ARRANGER_CURSOR_GRAB;*/
            /*}*/
          /*else*/
            /*{*/
              /*[> set cursor to normal <]*/
              /*return ARRANGER_CURSOR_SELECT;*/
            /*}*/
        /*}*/
          /*break;*/
        /*case TOOL_SELECT_STRETCH:*/
          /*break;*/
        /*case TOOL_EDIT:*/
          /*ac = ARRANGER_CURSOR_EDIT;*/
          /*break;*/
        /*case TOOL_CUT:*/
          /*ac = ARRANGER_CURSOR_CUT;*/
          /*break;*/
        /*case TOOL_ERASER:*/
          /*ac = ARRANGER_CURSOR_ERASER;*/
          /*break;*/
        /*case TOOL_RAMP:*/
          /*ac = ARRANGER_CURSOR_RAMP;*/
          /*break;*/
        /*case TOOL_AUDITION:*/
          /*ac = ARRANGER_CURSOR_AUDITION;*/
          /*break;*/
        /*}*/
      /*break;*/
    /*case UI_OVERLAY_ACTION_STARTING_DELETE_SELECTION:*/
    /*case UI_OVERLAY_ACTION_DELETE_SELECTING:*/
    /*case UI_OVERLAY_ACTION_ERASING:*/
      /*ac = ARRANGER_CURSOR_ERASER;*/
      /*break;*/
    /*case UI_OVERLAY_ACTION_STARTING_MOVING_COPY:*/
    /*case UI_OVERLAY_ACTION_MOVING_COPY:*/
      /*ac = ARRANGER_CURSOR_GRABBING_COPY;*/
      /*break;*/
    /*case UI_OVERLAY_ACTION_STARTING_MOVING:*/
    /*case UI_OVERLAY_ACTION_MOVING:*/
      /*ac = ARRANGER_CURSOR_GRABBING;*/
      /*break;*/
    /*case UI_OVERLAY_ACTION_STARTING_MOVING_LINK:*/
    /*case UI_OVERLAY_ACTION_MOVING_LINK:*/
      /*ac = ARRANGER_CURSOR_GRABBING_LINK;*/
      /*break;*/
    /*case UI_OVERLAY_ACTION_RESIZING_L:*/
      /*ac = ARRANGER_CURSOR_RESIZING_L;*/
      /*break;*/
    /*case UI_OVERLAY_ACTION_RESIZING_R:*/
      /*ac = ARRANGER_CURSOR_RESIZING_R;*/
      /*break;*/
    /*default:*/
      /*ac = ARRANGER_CURSOR_SELECT;*/
      /*break;*/
    /*}*/

  /*return ac;*/
/*}*/

/**
 * Create an AutomationPointat the given Position
 * in the given Track's AutomationTrack.
 *
 * @param pos The pre-snapped position.
 */
void
automation_arranger_widget_create_ap (
  ArrangerWidget * self,
  const Position *   pos,
  const double       start_y,
  Region *           region)
{
  AutomationTrack * at = CLIP_EDITOR->region->at;
  g_return_if_fail (at);

  self->action =
    UI_OVERLAY_ACTION_CREATING_MOVING;

  ArrangerObject * region_obj =
    (ArrangerObject *) region;

  /* get local pos */
  Position local_pos;
  position_from_ticks (
    &local_pos,
    pos->total_ticks -
    region_obj->pos.total_ticks);

  int height =
    gtk_widget_get_allocated_height (
      GTK_WIDGET (self));
  /* do height - because it's uside down */
	float normalized_val =
		(float) ((height - start_y) / height);
  float value =
    automatable_normalized_val_to_real (
      at->automatable, normalized_val);

  self->action =
    UI_OVERLAY_ACTION_CREATING_MOVING;

  /* create a new ap */
  AutomationPoint * ap =
    automation_point_new_float (
      value, normalized_val, &local_pos, F_MAIN);
  ArrangerObject * ap_obj =
    (ArrangerObject *) ap;

  /* set it as start object */
  self->start_object = ap_obj;

  /* add it to automation track */
  automation_region_add_ap (
    CLIP_EDITOR->region, ap);

  /* set visibility */
  /*arranger_object_gen_widget (*/
    /*ap_obj);*/
  /*arranger_object_set_widget_visibility_and_state (*/
    /*ap_obj, 1);*/

  /* set position to all counterparts */
  arranger_object_set_position (
    ap_obj, &local_pos,
    ARRANGER_OBJECT_POSITION_TYPE_START,
    F_NO_CACHED, F_NO_VALIDATE,
    AO_UPDATE_ALL);
  arranger_object_set_position (
    ap_obj, &local_pos,
    ARRANGER_OBJECT_POSITION_TYPE_START,
    F_CACHED, F_NO_VALIDATE,
    AO_UPDATE_ALL);

  EVENTS_PUSH (
    ET_ARRANGER_OBJECT_CREATED, ap);
  arranger_object_select (
    ap_obj, F_SELECT,
    F_NO_APPEND);
}


/**
 * Sets width to ruler width and height to
 * tracklist height.
 */
/*void*/
/*automation_arranger_widget_set_size (*/
  /*ArrangerWidget * self)*/
/*{*/
  // set the size
  /*RULER_WIDGET_GET_PRIVATE (EDITOR_RULER);*/
  /*gtk_widget_set_size_request (*/
    /*GTK_WIDGET (self),*/
    /*rw_prv->total_px,*/
    /*MW_AUTOMATION_EDITOR_SPACE->total_key_px);*/
/*}*/

/**
 * To be called once at init time.
 */
/*void*/
/*automation_arranger_widget_setup (*/
  /*ArrangerWidget * self)*/
/*{*/
  /*automation_arranger_widget_set_size (*/
    /*self);*/
/*}*/


/*void*/
/*automation_arranger_widget_move_items_y (*/
  /*ArrangerWidget * self,*/
  /*double                   offset_y)*/
/*{*/
  /*ARRANGER_WIDGET_GET_PRIVATE (self);*/

  /*if (AUTOMATION_SELECTIONS->num_automation_points)*/
    /*{*/
      /*AutomationPoint * ap;*/
      /*for (int i = 0;*/
           /*i < AUTOMATION_SELECTIONS->*/
             /*num_automation_points; i++)*/
        /*{*/
          /*ap =*/
            /*AUTOMATION_SELECTIONS->*/
              /*automation_points[i];*/

          /*ap =*/
            /*automation_point_get_main (ap);*/

          /*float fval =*/
            /*get_fvalue_at_y (*/
              /*self,*/
              /*self->start_y + offset_y);*/
          /*automation_point_set_fvalue (*/
            /*ap, fval, AO_UPDATE_NON_TRANS);*/
        /*}*/
      /*ArrangerObject * start_ap_obj =*/
        /*self->start_object;*/
      /*g_return_if_fail (start_ap_obj);*/
      /*arranger_object_widget_update_tooltip (*/
        /*Z_ARRANGER_OBJECT_WIDGET (*/
          /*start_ap_obj->widget), 1);*/
    /*}*/
/*}*/

/**
 * Returns the ticks objects were moved by since
 * the start of the drag.
 *
 * FIXME not really needed, can use
 * automation_selections_get_start_pos and the
 * arranger's earliest_obj_start_pos.
 */
/*static long*/
/*get_moved_diff (*/
  /*ArrangerWidget * self)*/
/*{*/
/*#define GET_DIFF(sc,pos_name) \*/
  /*if (AUTOMATION_SELECTIONS->num_##sc##s) \*/
    /*{ \*/
      /*return \*/
        /*position_to_ticks ( \*/
          /*&sc##_get_main_trans_##sc ( \*/
            /*AUTOMATION_SELECTIONS->sc##s[0])->pos_name) - \*/
        /*position_to_ticks ( \*/
          /*&sc##_get_main_##sc ( \*/
            /*AUTOMATION_SELECTIONS->sc##s[0])->pos_name); \*/
    /*}*/

  /*GET_DIFF (automation_point, pos);*/

  /*g_return_val_if_reached (0);*/
/*}*/

/**
 * Change curviness of selected curves.
 */
void
automation_arranger_widget_resize_curves (
  ArrangerWidget * self,
  double                     offset_y)
{
  double diff = offset_y - self->last_offset_y;
  diff = - diff;
  diff = diff / 120.0;
  for (int i = 0;
       i < AUTOMATION_SELECTIONS->num_automation_points; i++)
    {
      AutomationPoint * ap =
        AUTOMATION_SELECTIONS->automation_points[i];
      if (ap->curve_up)
        diff = - diff;
      double new_curve_val =
        CLAMP (
          ap->curviness + diff,
          AP_MIN_CURVINESS, 2.0);
      int new_curve_up = ap->curve_up;
      if (new_curve_val >= AP_MAX_CURVINESS)
        {
          new_curve_val -= AP_MAX_CURVINESS;
          new_curve_val =
            AP_MAX_CURVINESS - new_curve_val;
          new_curve_up = !ap->curve_up;
        }
      automation_point_set_curviness (
        ap, new_curve_val, new_curve_up);
    }

  EVENTS_PUSH (
    ET_ARRANGER_SELECTIONS_CHANGED,
    AUTOMATION_SELECTIONS);
}

/**
 * Sets the default cursor in all selected regions and
 * intializes start positions.
 */
/*void*/
/*automation_arranger_widget_on_drag_end (*/
  /*ArrangerWidget * self)*/
/*{*/
  /*ARRANGER_WIDGET_GET_PRIVATE (self);*/

  /*AutomationPoint * ap;*/
  /*for (int i = 0;*/
       /*i < AUTOMATION_SELECTIONS->*/
             /*num_automation_points; i++)*/
    /*{*/
      /*ap =*/
        /*AUTOMATION_SELECTIONS->automation_points[i];*/
      /*ArrangerObject * obj =*/
        /*(ArrangerObject *) ap;*/
      /*arranger_object_widget_update_tooltip (*/
        /*(ArrangerObjectWidget *) obj->widget, 0);*/
    /*}*/

  /*if (self->action ==*/
        /*UI_OVERLAY_ACTION_RESIZING_L)*/
    /*{*/
    /*}*/
  /*else if (self->action ==*/
        /*UI_OVERLAY_ACTION_RESIZING_R)*/
    /*{*/
    /*}*/
  /*else if (self->action ==*/
        /*UI_OVERLAY_ACTION_STARTING_MOVING)*/
    /*{*/
      /* if something was clicked with ctrl without
       * moving*/
      /*if (self->ctrl_held)*/
        /*{*/
          /*[>if (self->start_region &&<]*/
              /*[>self->start_region_was_selected)<]*/
            /*[>{<]*/
              /*[>[> deselect it <]<]*/
              /*[>[>ARRANGER_WIDGET_SELECT_REGION (<]<]*/
                /*[>[>self, self->start_region,<]<]*/
                /*[>[>F_NO_SELECT, F_APPEND);<]<]*/
            /*[>}<]*/
        /*}*/
      /*else if (self->n_press == 2)*/
        /*{*/
          /*[> double click on object <]*/
          /*[>g_message ("DOUBLE CLICK");<]*/
        /*}*/
    /*}*/
  /*else if (self->action ==*/
             /*UI_OVERLAY_ACTION_MOVING)*/
    /*{*/
      /*[>Position earliest_trans_pos;<]*/
      /*[>automation_selections_get_start_pos (<]*/
        /*[>AUTOMATION_SELECTIONS,<]*/
        /*[>&earliest_trans_pos, 1);<]*/
      /*[>UndoableAction * ua =<]*/
        /*[>(UndoableAction *)<]*/
        /*[>move_automation_selections_action_new (<]*/
          /*[>TL_SELECTIONS,<]*/
          /*[>position_to_ticks (<]*/
            /*[>&earliest_trans_pos) -<]*/
          /*[>position_to_ticks (<]*/
            /*[>&self->earliest_obj_start_pos),<]*/
          /*[>automation_selections_get_highest_track (<]*/
            /*[>TL_SELECTIONS, F_TRANSIENTS) -<]*/
          /*[>automation_selections_get_highest_track (<]*/
            /*[>TL_SELECTIONS, F_NO_TRANSIENTS));<]*/
      /*[>undo_manager_perform (<]*/
        /*[>UNDO_MANAGER, ua);<]*/
    /*}*/
  /*[> if copy/link-moved <]*/
  /*else if (self->action ==*/
             /*UI_OVERLAY_ACTION_MOVING_COPY ||*/
           /*self->action ==*/
             /*UI_OVERLAY_ACTION_MOVING_LINK)*/
    /*{*/
      /*[>Position earliest_trans_pos;<]*/
      /*[>automation_selections_get_start_pos (<]*/
        /*[>TL_SELECTIONS,<]*/
        /*[>&earliest_trans_pos, 1);<]*/
      /*[>UndoableAction * ua =<]*/
        /*[>(UndoableAction *)<]*/
        /*[>duplicate_automation_selections_action_new (<]*/
          /*[>TL_SELECTIONS,<]*/
          /*[>position_to_ticks (<]*/
            /*[>&earliest_trans_pos) -<]*/
          /*[>position_to_ticks (<]*/
            /*[>&self->earliest_obj_start_pos),<]*/
          /*[>automation_selections_get_highest_track (<]*/
            /*[>TL_SELECTIONS, F_TRANSIENTS) -<]*/
          /*[>automation_selections_get_highest_track (<]*/
            /*[>TL_SELECTIONS, F_NO_TRANSIENTS));<]*/
      /*[>automation_selections_reset_transient_poses (<]*/
        /*[>TL_SELECTIONS);<]*/
      /*[>automation_selections_clear (<]*/
        /*[>TL_SELECTIONS);<]*/
      /*[>undo_manager_perform (<]*/
        /*[>UNDO_MANAGER, ua);<]*/
    /*}*/
  /*else if (self->action ==*/
             /*UI_OVERLAY_ACTION_NONE ||*/
           /*self->action ==*/
             /*UI_OVERLAY_ACTION_STARTING_SELECTION)*/
    /*{*/
      /*arranger_selections_clear (*/
        /*(ArrangerSelections *)*/
        /*AUTOMATION_SELECTIONS);*/
    /*}*/
  /*[> if something was created <]*/
  /*[>else if (self->action ==<]*/
             /*[>UI_OVERLAY_ACTION_CREATING_MOVING ||<]*/
           /*[>self->action ==<]*/
             /*[>UI_OVERLAY_ACTION_CREATING_RESIZING_R)<]*/
    /*[>{<]*/
      /*[>automation_selections_set_to_transient_poses (<]*/
        /*[>TL_SELECTIONS);<]*/
      /*[>automation_selections_set_to_transient_values (<]*/
        /*[>TL_SELECTIONS);<]*/

      /*[>UndoableAction * ua =<]*/
        /*[>(UndoableAction *)<]*/
        /*[>create_automation_selections_action_new (<]*/
          /*[>TL_SELECTIONS);<]*/
      /*[>undo_manager_perform (<]*/
        /*[>UNDO_MANAGER, ua);<]*/
    /*[>}<]*/
  /*[> if didn't click on something <]*/
  /*else*/
    /*{*/
    /*}*/
/*}*/

/*static void*/
/*add_children_from_region (*/
  /*ArrangerWidget * self,*/
  /*Region *          region)*/
/*{*/
  /*int j,k;*/
  /*AutomationPoint * ap;*/
  /*for (j = region->num_aps - 1; j >= 0; j--)*/
    /*{*/
      /*ap = region->aps[j];*/

      /*for (k = 0; k < 2; k++)*/
        /*{*/
          /*if (k == 0)*/
            /*ap = automation_point_get_main (ap);*/
          /*else if (k == 1)*/
            /*ap =*/
              /*automation_point_get_main_trans (ap);*/

          /*ArrangerObject * obj =*/
            /*(ArrangerObject *) ap;*/

          /*if (!GTK_IS_WIDGET (obj->widget))*/
            /*{*/
              /*arranger_object_gen_widget (obj);*/
            /*}*/

          /*arranger_widget_add_overlay_child (*/
            /*(ArrangerWidget *) self, obj);*/
        /*}*/
    /*}*/
/*}*/

/**
 * Readd children.
 */
/*void*/
/*automation_arranger_widget_refresh_children (*/
  /*ArrangerWidget * self)*/
/*{*/
  /*ARRANGER_WIDGET_GET_PRIVATE (self);*/

  /*[> remove all children except bg && playhead <]*/
  /*GList *children, *iter;*/

  /*children =*/
    /*gtk_container_get_children (*/
      /*GTK_CONTAINER (self));*/
  /*for (iter = children;*/
       /*iter != NULL;*/
       /*iter = g_list_next (iter))*/
    /*{*/
      /*GtkWidget * widget = GTK_WIDGET (iter->data);*/
      /*if (widget != (GtkWidget *) self->bg &&*/
          /*widget != (GtkWidget *) self->playhead)*/
        /*{*/
          /*[>g_object_ref (widget);<]*/
          /*gtk_container_remove (*/
            /*GTK_CONTAINER (self),*/
            /*widget);*/
        /*}*/
    /*}*/
  /*g_list_free (children);*/

  /*[> add <]*/
  /*Region * region = CLIP_EDITOR->region;*/
  /*add_children_from_region (*/
    /*self, region);*/

  /*arranger_widget_update_visibility (*/
    /*(ArrangerWidget *) self);*/

  /*gtk_overlay_reorder_overlay (*/
    /*GTK_OVERLAY (self),*/
    /*(GtkWidget *) self->playhead, -1);*/
/*}*/

/**
 * Scroll to the given position.
 */
/*void*/
/*automation_arranger_widget_scroll_to (*/
  /*ArrangerWidget * self,*/
  /*Position *               pos)*/
/*{*/
  /*[> TODO <]*/

/*}*/

/*static gboolean*/
/*on_focus (GtkWidget       *widget,*/
          /*gpointer         user_data)*/
/*{*/
  /*MAIN_WINDOW->last_focused = widget;*/

  /*return FALSE;*/
/*}*/

/*static void*/
/*automation_arranger_widget_class_init (*/
  /*ArrangerWidgetClass * klass)*/
/*{*/
/*}*/

/*static void*/
/*automation_arranger_widget_init (*/
  /*ArrangerWidget *self )*/
/*{*/
  /*g_signal_connect (*/
    /*self, "grab-focus",*/
    /*G_CALLBACK (on_focus), self);*/
/*}*/
