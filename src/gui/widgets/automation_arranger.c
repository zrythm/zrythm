// SPDX-FileCopyrightText: © 2018-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * The automation containing regions and other
 * objects.
 */

#include "actions/arranger_selections.h"
#include "actions/undo_manager.h"
#include "actions/undoable_action.h"
#include "audio/audio_bus_track.h"
#include "audio/audio_track.h"
#include "audio/automation_region.h"
#include "audio/automation_track.h"
#include "audio/automation_tracklist.h"
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
#include "gui/backend/automation_selections.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
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
#include "gui/widgets/midi_note.h"
#include "gui/widgets/midi_region.h"
#include "gui/widgets/pinned_tracklist.h"
#include "gui/widgets/region.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/scale_object.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/arrays.h"
#include "utils/cairo.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/objects.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

/**
 * Create an AutomationPointat the given Position
 * in the given Track's AutomationTrack.
 *
 * @param pos The pre-snapped position.
 */
void
automation_arranger_widget_create_ap (
  ArrangerWidget * self,
  const Position * pos,
  const double     start_y,
  ZRegion *        region,
  bool             autofilling)
{
  AutomationTrack * at = region_get_automation_track (region);
  g_return_if_fail (at);
  Port * port = port_find_from_identifier (&at->port_id);
  g_return_if_fail (port);

  if (!autofilling)
    {
      self->action = UI_OVERLAY_ACTION_CREATING_MOVING;
    }

  ArrangerObject * region_obj = (ArrangerObject *) region;

  /* get local pos */
  Position local_pos;
  position_from_ticks (
    &local_pos, pos->ticks - region_obj->pos.ticks);

  int height =
    gtk_widget_get_allocated_height (GTK_WIDGET (self));
  /* do height - because it's uside down */
  float normalized_val = (float) ((height - start_y) / height);
  g_message ("normalized val is %f", (double) normalized_val);

  /* clamp the value because the cursor might be
   * outside the widget */
  normalized_val = CLAMP (normalized_val, 0.f, 1.f);

  float value = control_port_normalized_val_to_real (
    port, normalized_val);

  /* create a new ap */
  AutomationPoint * ap = automation_point_new_float (
    value, normalized_val, &local_pos);
  ArrangerObject * ap_obj = (ArrangerObject *) ap;

  /* set it as start object */
  self->start_object = ap_obj;

  /* add it to automation track */
  automation_region_add_ap (region, ap, F_PUBLISH_EVENTS);

  /* set position to all counterparts */
  arranger_object_set_position (
    ap_obj, &local_pos, ARRANGER_OBJECT_POSITION_TYPE_START,
    F_NO_VALIDATE);

  EVENTS_PUSH (ET_ARRANGER_OBJECT_CREATED, ap);
  arranger_object_select (
    ap_obj, F_SELECT, F_NO_APPEND, F_NO_PUBLISH_EVENTS);
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
  diff = -diff;
  diff = diff / 120.0;
  for (int i = 0;
       i < AUTOMATION_SELECTIONS->num_automation_points; i++)
    {
      AutomationPoint * ap =
        AUTOMATION_SELECTIONS->automation_points[i];
      double new_curve_val =
        CLAMP (ap->curve_opts.curviness + diff, -1.0, 1.0);
      automation_point_set_curviness (ap, new_curve_val);
    }

  EVENTS_PUSH (
    ET_ARRANGER_SELECTIONS_CHANGED, AUTOMATION_SELECTIONS);
}

/** Used for passing a curve algorithm to some
 * actions. */
typedef struct CurveAlgorithmInfo
{
  AutomationPoint * ap;
  CurveAlgorithm    algo;
} CurveAlgorithmInfo;

/**
 * Generate a context menu at x, y.
 *
 * @param menu A menu to append entries to (optional).
 *
 * @return The given updated menu or a new menu.
 */
GMenu *
automation_arranger_widget_gen_context_menu (
  ArrangerWidget * self,
  GMenu *          menu,
  double           x,
  double           y)
{
  if (!menu)
    {
      menu = g_menu_new ();
    }
  GMenuItem * menuitem;

  ArrangerObject * obj =
    arranger_widget_get_hit_arranger_object (
      (ArrangerWidget *) self,
      ARRANGER_OBJECT_TYPE_AUTOMATION_POINT, x, y);
  AutomationPoint * ap = (AutomationPoint *) obj;

  if (ap)
    {
      GMenu * edit_submenu = g_menu_new ();

      /* create cut, copy, duplicate, delete */
      menuitem = CREATE_CUT_MENU_ITEM ("app.cut");
      g_menu_append_item (edit_submenu, menuitem);
      menuitem = CREATE_COPY_MENU_ITEM ("app.copy");
      g_menu_append_item (edit_submenu, menuitem);
      menuitem = CREATE_DUPLICATE_MENU_ITEM ("app.duplicate");
      g_menu_append_item (edit_submenu, menuitem);
      menuitem = CREATE_DELETE_MENU_ITEM ("app.delete");
      g_menu_append_item (edit_submenu, menuitem);

      char str[100];
      sprintf (str, "app.arranger-object-view-info::%p", obj);
      menuitem =
        z_gtk_create_menu_item (_ ("View info"), NULL, str);
      g_menu_append_item (edit_submenu, menuitem);

      g_menu_append_section (
        menu, _ ("Edit"), G_MENU_MODEL (edit_submenu));

      /* add curve algorithm selection */
      GMenu * curve_algorithm_submenu = g_menu_new ();
      for (int i = 0; i < NUM_CURVE_ALGORITHMS; i++)
        {
          char name[100];
          curve_algorithm_get_localized_name (
            (CurveAlgorithm) i, name);
          char tmp[200];
          /* TODO change action state so that
           * selected algorithm shows as selected */
          sprintf (tmp, "app.set-curve-algorithm");
          menuitem = z_gtk_create_menu_item (name, NULL, tmp);
          g_menu_item_set_action_and_target_value (
            menuitem, tmp, g_variant_new_int32 (i));
          g_menu_append_item (
            curve_algorithm_submenu, menuitem);
        }

      g_menu_append_section (
        menu, _ ("Curve algorithm"),
        G_MENU_MODEL (curve_algorithm_submenu));
    }

  return menu;
}

/**
 * Called when using the edit tool.
 *
 * @return Whether an automation point was moved.
 */
bool
automation_arranger_move_hit_aps (
  ArrangerWidget * self,
  double           x,
  double           y)
{
  int height =
    gtk_widget_get_allocated_height (GTK_WIDGET (self));

  /* get snapped x */
  Position pos;
  arranger_widget_px_to_pos (self, x, &pos, true);
  if (!self->shift_held && SNAP_GRID_ANY_SNAP (self->snap_grid))
    {
      position_snap (
        &self->earliest_obj_start_pos, &pos, NULL, NULL,
        self->snap_grid);
      x = arranger_widget_pos_to_px (self, &pos, true);
    }

  /* move any hit automation points */
  ArrangerObject * obj =
    arranger_widget_get_hit_arranger_object (
      self, ARRANGER_OBJECT_TYPE_AUTOMATION_POINT, x, -1);
  if (obj)
    {
      AutomationPoint * ap = (AutomationPoint *) obj;
      if (automation_point_is_point_hit (ap, x, -1))
        {
          arranger_object_select (
            obj, F_SELECT, F_APPEND, F_NO_PUBLISH_EVENTS);

          Port * port = automation_point_get_port (ap);
          g_return_val_if_fail (port, false);

          /* move it to the y value */
          /* do height - because it's uside down */
          float normalized_val =
            (float) ((height - y) / height);

          /* clamp the value because the cursor might
           * be outside the widget */
          normalized_val = CLAMP (normalized_val, 0.f, 1.f);
          float value = control_port_normalized_val_to_real (
            port, normalized_val);
          automation_point_set_fvalue (
            ap, value, F_NOT_NORMALIZED, F_PUBLISH_EVENTS);

          return true;
        }
    }

  return false;
}
