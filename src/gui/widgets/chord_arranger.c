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
 * The chord containing regions and other
 * objects.
 */

#include "actions/arranger_selections.h"
#include "actions/undoable_action.h"
#include "actions/undo_manager.h"
#include "audio/automation_track.h"
#include "audio/automation_tracklist.h"
#include "audio/audio_track.h"
#include "audio/audio_bus_track.h"
#include "audio/channel.h"
#include "audio/chord_object.h"
#include "audio/chord_region.h"
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
#include "gui/widgets/arranger.h"
#include "gui/widgets/automation_point.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/chord_object.h"
#include "gui/widgets/chord_editor_space.h"
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
#include "gui/widgets/midi_note.h"
#include "gui/widgets/region.h"
#include "gui/widgets/scale_object.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/chord_arranger.h"
#include "gui/widgets/chord_arranger_bg.h"
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
 * Create a ChordObject at the given Position in the
 * given Track.
 *
 * @param pos The pre-snapped position.
 */
void
chord_arranger_widget_create_chord (
  ArrangerWidget * self,
  const Position *      pos,
  int                   chord_index,
  Region *              region)
{
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

  /* create a new chord */
  ChordObject * chord =
    chord_object_new (
      CLIP_EDITOR->region, chord_index, 1);
  ArrangerObject * chord_obj =
    (ArrangerObject *) chord;

  /* add it to chord region */
  chord_region_add_chord_object (
    region, chord);

  /*arranger_object_gen_widget (chord_obj);*/

  /* set visibility */
  /*arranger_object_set_widget_visibility_and_state (*/
    /*chord_obj, 1);*/

  arranger_object_set_position (
    chord_obj, &local_pos,
    ARRANGER_OBJECT_POSITION_TYPE_START,
    F_NO_CACHED, F_NO_VALIDATE);

  EVENTS_PUSH (ET_ARRANGER_OBJECT_CREATED, chord);
  arranger_object_select (
    chord_obj, F_SELECT, F_NO_APPEND);
}

/**
 * Returns the chord index at y.
 */
int
chord_arranger_widget_get_chord_at_y (
  double y)
{
  double adj_y = y - 1;
  double adj_px_per_key =
    MW_CHORD_EDITOR_SPACE->px_per_key + 1;
  return (int) (adj_y / adj_px_per_key);
}


/*static int*/
/*on_motion (*/
  /*GtkWidget *      widget,*/
  /*GdkEventMotion * event,*/
  /*ArrangerWidget * self)*/
/*{*/
  /*if (event->type == GDK_LEAVE_NOTIFY)*/
    /*self->hovered_index = -1;*/
  /*else*/
    /*self->hovered_index =*/
      /*chord_arranger_widget_get_chord_at_y (*/
        /*event->y);*/
  /*[>g_message ("hovered index: %d",<]*/
             /*[>self->hovered_index);<]*/

  /*ARRANGER_WIDGET_GET_PRIVATE (self);*/
  /*gtk_widget_queue_draw (*/
    /*GTK_WIDGET (self->bg));*/

  /*return FALSE;*/
/*}*/


/**
 * Returns the ticks objects were moved by since
 * the start of the drag.
 *
 * FIXME not really needed, can use
 * chord_selections_get_start_pos and the
 * arranger's earliest_obj_start_pos.
 */
/*static long*/
/*get_moved_diff (*/
  /*ArrangerWidget * self)*/
/*{*/
/*#define GET_DIFF(sc,pos_name) \*/
  /*if (CHORD_SELECTIONS->num_##sc##s) \*/
    /*{ \*/
      /*return \*/
        /*position_to_ticks ( \*/
          /*&sc##_get_main_trans_##sc ( \*/
            /*CHORD_SELECTIONS->sc##s[0])->pos_name) - \*/
        /*position_to_ticks ( \*/
          /*&sc##_get_main_##sc ( \*/
            /*CHORD_SELECTIONS->sc##s[0])->pos_name); \*/
    /*}*/

  /*GET_DIFF (chord_object, pos);*/

  /*g_return_val_if_reached (0);*/
/*}*/

/**
 * Sets the default cursor in all selected regions
 * and intializes start positions.
 */
/*void*/
/*chord_arranger_widget_on_drag_end (*/
  /*ArrangerWidget * self)*/
/*{*/
  /*ARRANGER_WIDGET_GET_PRIVATE (self);*/

/*}*/

/*static void*/
/*add_children_from_chord_track (*/
  /*ArrangerWidget * self,*/
  /*ChordTrack *          ct)*/
/*{*/
  /*int i, j, k;*/
  /*Region * r;*/
  /*ChordObject * c;*/
  /*for (i = 0; i < ct->num_chord_regions; i++)*/
    /*{*/
      /*r = ct->chord_regions[i];*/

      /*for (j = 0; j < r->num_chord_objects; j++)*/
        /*{*/
          /*c = r->chord_objects[j];*/

          /*for (k = 0 ;*/
               /*k <= AOI_COUNTERPART_MAIN_TRANSIENT;*/
               /*k++)*/
            /*{*/
              /*if (k == AOI_COUNTERPART_MAIN)*/
                /*c =*/
                  /*chord_object_get_main (c);*/
              /*else if (*/
                /*k == AOI_COUNTERPART_MAIN_TRANSIENT)*/
                /*c =*/
                  /*chord_object_get_main_trans (c);*/

              /*g_return_if_fail (c);*/
              /*ArrangerObject * obj =*/
                /*(ArrangerObject *) c;*/

              /*if (!obj->widget)*/
                /*arranger_object_gen_widget (obj);*/

              /*arranger_widget_add_overlay_child (*/
                /*(ArrangerWidget *) self, obj);*/
            /*}*/
        /*}*/
    /*}*/
/*}*/

/**
 * Readd children.
 */

/**
 * Scroll to the given position.
 */
/*void*/
/*chord_arranger_widget_scroll_to (*/
  /*ArrangerWidget * self,*/
  /*Position *               pos)*/
/*{*/
  /*[> TODO <]*/

/*}*/

/*static gboolean*/
/*on_focus (*/
  /*GtkWidget * widget,*/
  /*gpointer    user_data)*/
/*{*/
  /*[>g_message ("chord focused");<]*/
  /*MAIN_WINDOW->last_focused = widget;*/

  /*return FALSE;*/
/*}*/

/*static void*/
/*chord_arranger_widget_class_init (*/
  /*ArrangerWidgetClass * klass)*/
/*{*/
/*}*/

/*static void*/
/*chord_arranger_widget_init (*/
  /*ArrangerWidget *self )*/
/*{*/
  /*g_signal_connect (*/
    /*self, "grab-focus",*/
    /*G_CALLBACK (on_focus), self);*/
/*}*/
