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
#include "audio/control_port.h"
#include "audio/curve.h"
#include "audio/instrument_track.h"
#include "audio/marker_track.h"
#include "audio/master_track.h"
#include "audio/midi_region.h"
#include "audio/scale_object.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "audio/transport.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
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
#include "zrythm_app.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

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
  ZRegion *           region)
{
  AutomationTrack * at =
    region_get_automation_track (region);
  g_return_if_fail (at);
  Port * port =
    automation_track_get_port (at);
  g_return_if_fail (port);

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
    control_port_normalized_val_to_real (
      port, normalized_val);

  /* create a new ap */
  AutomationPoint * ap =
    automation_point_new_float (
      value, normalized_val, &local_pos);
  ArrangerObject * ap_obj =
    (ArrangerObject *) ap;

  /* set it as start object */
  self->start_object = ap_obj;

  /* add it to automation track */
  automation_region_add_ap (
    region, ap, F_PUBLISH_EVENTS);

  /* set position to all counterparts */
  arranger_object_set_position (
    ap_obj, &local_pos,
    ARRANGER_OBJECT_POSITION_TYPE_START,
    F_NO_VALIDATE);

  EVENTS_PUSH (
    ET_ARRANGER_OBJECT_CREATED, ap);
  arranger_object_select (
    ap_obj, F_SELECT,
    F_NO_APPEND);
}

/**
 * Change curviness of selected curves.
 */
void
automation_arranger_widget_resize_curves (
  ArrangerWidget * self,
  double           offset_y)
{
  double diff = offset_y - self->last_offset_y;
  diff = - diff;
  diff = diff / 120.0;
  for (int i = 0;
       i < AUTOMATION_SELECTIONS->num_automation_points; i++)
    {
      AutomationPoint * ap =
        AUTOMATION_SELECTIONS->automation_points[i];
      double new_curve_val =
        CLAMP (
          ap->curve_opts.curviness + diff,
          - 1.0, 1.0);
      automation_point_set_curviness (
        ap, new_curve_val);
    }

  EVENTS_PUSH (
    ET_ARRANGER_SELECTIONS_CHANGED,
    AUTOMATION_SELECTIONS);
}

/** Used for passing a curve algorithm to some
 * actions. */
typedef struct CurveAlgorithmInfo
{
  AutomationPoint * ap;
  CurveAlgorithm    algo;
} CurveAlgorithmInfo;

static void
on_curve_algorithm_selected (
  GtkMenuItem *        menu_item,
  CurveAlgorithmInfo * info)
{
  g_return_if_fail (
    AUTOMATION_SELECTIONS->num_automation_points ==
      1 &&
    AUTOMATION_SELECTIONS->automation_points[0] ==
      info->ap);

  /* clone the selections before the change */
  ArrangerSelections * sel_before =
    arranger_selections_clone (
      (ArrangerSelections *) AUTOMATION_SELECTIONS);

  /* change */
  info->ap->curve_opts.algo = info->algo;

  /* create undoable action */
  UndoableAction * ua =
    arranger_selections_action_new_edit (
      sel_before,
      (ArrangerSelections *) AUTOMATION_SELECTIONS,
      ARRANGER_SELECTIONS_ACTION_EDIT_PRIMITIVE,
      true);
  undo_manager_perform (UNDO_MANAGER, ua);

  free (info);
  arranger_selections_free_full (sel_before);
}

/**
 * Show context menu at x, y.
 */
void
automation_arranger_widget_show_context_menu (
  ArrangerWidget * self,
  double           x,
  double           y)
{
  GtkWidget *menu, *menuitem;
  menu = gtk_menu_new();

  ArrangerObject * obj =
    arranger_widget_get_hit_arranger_object (
      (ArrangerWidget *) self,
      ARRANGER_OBJECT_TYPE_AUTOMATION_POINT, x, y);
  AutomationPoint * ap = (AutomationPoint *) obj;

  if (ap)
    {
      /* add curve algorithm selection */
      menuitem =
        gtk_menu_item_new_with_label (
          _("Curve algorithm"));
      GtkMenu * submenu =
        GTK_MENU (gtk_menu_new ());
      gtk_widget_set_visible (
        GTK_WIDGET (submenu), 1);
      GtkMenuItem * submenu_item;
      for (int i = 0; i < NUM_CURVE_ALGORITHMS; i++)
        {
          char name[100];
          curve_algorithm_get_localized_name (
            (CurveAlgorithm) i, name);
          submenu_item =
            GTK_MENU_ITEM (
              gtk_check_menu_item_new_with_label (
                name));
          gtk_check_menu_item_set_draw_as_radio (
            GTK_CHECK_MENU_ITEM (submenu_item), 1);
          if ((CurveAlgorithm) i ==
                ap->curve_opts.algo)
            {
              gtk_check_menu_item_set_active (
                GTK_CHECK_MENU_ITEM (submenu_item), 1);
            }

          CurveAlgorithmInfo * info =
            calloc (1, sizeof (CurveAlgorithmInfo));
          info->algo = (CurveAlgorithm) i;
          info->ap = ap;
          g_signal_connect (
            G_OBJECT (submenu_item), "activate",
            G_CALLBACK (on_curve_algorithm_selected),
            info);
          gtk_menu_shell_append (
            GTK_MENU_SHELL (submenu),
            GTK_WIDGET (submenu_item));
          gtk_widget_set_visible (
            GTK_WIDGET (submenu_item), 1);
        }

      gtk_menu_item_set_submenu (
        GTK_MENU_ITEM (menuitem),
        GTK_WIDGET (submenu));
      gtk_widget_set_visible (
        GTK_WIDGET (menuitem), 1);

      gtk_menu_shell_append (
        GTK_MENU_SHELL (menu),
        GTK_WIDGET (menuitem));
    }

  gtk_widget_show_all (GTK_WIDGET (menu));
  gtk_menu_popup_at_pointer (
    GTK_MENU (menu), NULL);
}
