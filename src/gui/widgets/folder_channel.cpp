// SPDX-FileCopyrightText: © 2021-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <sys/time.h>

#include "actions/tracklist_selections.h"
#include "actions/undo_manager.h"
#include "actions/undoable_action.h"
#include "dsp/master_track.h"
#include "dsp/meter.h"
#include "dsp/port_connections_manager.h"
#include "dsp/track.h"
#include "gui/backend/wrapped_object_with_change_signal.h"
#include "gui/widgets/balance_control.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/editable_label.h"
#include "gui/widgets/expander_box.h"
#include "gui/widgets/fader.h"
#include "gui/widgets/fader_buttons.h"
#include "gui/widgets/folder_channel.h"
#include "gui/widgets/gtk_flipper.h"
#include "gui/widgets/knob.h"
#include "gui/widgets/meter.h"
#include "gui/widgets/mixer.h"
#include "gui/widgets/plugin_strip_expander.h"
#include "gui/widgets/route_target_selector.h"
#include "project.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/math.h"
#include "utils/resources.h"
#include "utils/symap.h"
#include "utils/ui.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include <time.h>

G_DEFINE_TYPE (FolderChannelWidget, folder_channel_widget, GTK_TYPE_WIDGET)

static void
folder_channel_snapshot (GtkWidget * widget, GtkSnapshot * snapshot)
{
  FolderChannelWidget * self = Z_FOLDER_CHANNEL_WIDGET (widget);

  int width = gtk_widget_get_width (widget);
  int height = gtk_widget_get_height (widget);

  Track * track = self->track;
  if (track)
    {
      /* tint background */
      GdkRGBA tint_color = Z_GDK_RGBA_INIT (
        track->color.red, track->color.green, track->color.blue, 0.15f);
      gtk_snapshot_append_color (
        snapshot, &tint_color,
        &Z_GRAPHENE_RECT_INIT (0.f, 0.f, (float) width, (float) height));
    }

  GTK_WIDGET_CLASS (folder_channel_widget_parent_class)
    ->snapshot (widget, snapshot);
}

static gboolean
on_dnd_drop (
  GtkDropTarget *       drop_target,
  const GValue *        value,
  gdouble               x,
  gdouble               y,
  FolderChannelWidget * self)
{
  if (!G_VALUE_HOLDS (value, WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE))
    {
      g_message ("invalid DND type");
      return false;
    }

  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (g_value_get_object (value));
  Track * track = NULL;
  if (wrapped_obj->type == WrappedObjectType::WRAPPED_OBJECT_TYPE_TRACK)
    {
      track = (Track *) wrapped_obj->obj;
    }
  if (!track)
    {
      g_message ("dropped object not a track");
      return false;
    }

  Track * this_track = self->track;

  /* determine if moving or copying */
  GdkDragAction action = z_gtk_drop_target_get_selected_action (drop_target);

  int w = gtk_widget_get_width (GTK_WIDGET (self));

  /* determine position to move to */
  int pos;
  if (x < w / 2)
    {
      if (this_track->pos <= MW_MIXER->start_drag_track->pos)
        pos = this_track->pos;
      else
        {
          Track * prev =
            tracklist_get_prev_visible_track (TRACKLIST, this_track);
          pos = prev ? prev->pos : this_track->pos;
        }
    }
  else
    {
      if (this_track->pos >= MW_MIXER->start_drag_track->pos)
        pos = this_track->pos;
      else
        {
          Track * next =
            tracklist_get_next_visible_track (TRACKLIST, this_track);
          pos = next ? next->pos : this_track->pos;
        }
    }

  tracklist_selections_select_foldable_children (TRACKLIST_SELECTIONS);

  GError * err = NULL;
  bool     ret;
  if (action == GDK_ACTION_COPY)
    {
      ret = tracklist_selections_action_perform_copy (
        TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, pos, &err);
    }
  else
    {
      ret = tracklist_selections_action_perform_move (
        TRACKLIST_SELECTIONS, PORT_CONNECTIONS_MGR, pos, &err);
    }

  if (!ret)
    {
      HANDLE_ERROR (err, "%s", _ ("Failed to move or copy track(s)"));
    }

  return true;
}

static GdkContentProvider *
on_dnd_drag_prepare (
  GtkDragSource *       source,
  double                x,
  double                y,
  FolderChannelWidget * self)
{
  Track *                         track = self->track;
  WrappedObjectWithChangeSignal * wrapped_obj =
    wrapped_object_with_change_signal_new (
      track, WrappedObjectType::WRAPPED_OBJECT_TYPE_TRACK);
  GdkContentProvider * content_providers[] = {
    gdk_content_provider_new_typed (
      WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE, wrapped_obj),
  };

  return gdk_content_provider_new_union (
    content_providers, G_N_ELEMENTS (content_providers));
}

static void
on_dnd_drag_begin (GtkDragSource * source, GdkDrag * drag, gpointer user_data)
{
  FolderChannelWidget * self = Z_FOLDER_CHANNEL_WIDGET (user_data);

  Track * track = self->track;
  self->selected_in_dnd = 1;
  MW_MIXER->start_drag_track = track;

  if (self->n_press == 1)
    {
      int ctrl = 0, selected = 0;

      ctrl = self->ctrl_held_at_start;

      if (tracklist_selections_contains_track (TRACKLIST_SELECTIONS, track))
        selected = 1;

      /* no control & not selected */
      if (!ctrl && !selected)
        {
          tracklist_selections_select_single (
            TRACKLIST_SELECTIONS, track, F_PUBLISH_EVENTS);
        }
      else if (!ctrl && selected)
        {
        }
      else if (ctrl && !selected)
        tracklist_selections_add_track (TRACKLIST_SELECTIONS, track, 1);
    }
}

/**
 * Highlights/unhighlights the folder_channels
 * appropriately.
 */
static void
do_highlight (FolderChannelWidget * self, gint x, gint y)
{
  /* if we are closer to the start of selection than
   * the end */
  int w = gtk_widget_get_width (GTK_WIDGET (self));
  if (x < w / 2)
    {
      /* highlight left */
      /*gtk_drag_highlight (*/
      /*GTK_WIDGET (*/
      /*self->highlight_left_box));*/
      gtk_widget_set_size_request (GTK_WIDGET (self->highlight_left_box), 2, -1);

      /* unhighlight right */
      /*gtk_drag_unhighlight (*/
      /*GTK_WIDGET (*/
      /*self->highlight_right_box));*/
      gtk_widget_set_size_request (
        GTK_WIDGET (self->highlight_right_box), -1, -1);
    }
  else
    {
      /* highlight right */
      /*gtk_drag_highlight (*/
      /*GTK_WIDGET (*/
      /*self->highlight_right_box));*/
      gtk_widget_set_size_request (
        GTK_WIDGET (self->highlight_right_box), 2, -1);

      /* unhighlight left */
      /*gtk_drag_unhighlight (*/
      /*GTK_WIDGET (*/
      /*self->highlight_left_box));*/
      gtk_widget_set_size_request (
        GTK_WIDGET (self->highlight_left_box), -1, -1);
    }
}

static GdkDragAction
on_dnd_motion (
  GtkDropTarget * drop_target,
  gdouble         x,
  gdouble         y,
  gpointer        user_data)
{
  FolderChannelWidget * self = Z_FOLDER_CHANNEL_WIDGET (user_data);

  GdkModifierType state = gtk_event_controller_get_current_event_state (
    GTK_EVENT_CONTROLLER (drop_target));

  do_highlight (self, (int) x, (int) y);

  if (state & GDK_CONTROL_MASK)
    return GDK_ACTION_COPY;
  else
    return GDK_ACTION_MOVE;
}

static void
on_dnd_leave (GtkDropTarget * drop_target, FolderChannelWidget * self)
{
  g_debug ("folder channel dnd leave");

  /*do_highlight (self);*/
  /*gtk_drag_unhighlight (*/
  /*GTK_WIDGET (self->highlight_left_box));*/
  gtk_widget_set_size_request (GTK_WIDGET (self->highlight_left_box), -1, -1);
  /*gtk_drag_unhighlight (*/
  /*GTK_WIDGET (self->highlight_right_box));*/
  gtk_widget_set_size_request (GTK_WIDGET (self->highlight_right_box), -1, -1);
}

/**
 * Callback when somewhere in the folder_channel is
 * pressed.
 *
 * Only responsible for setting the tracklist
 * selection and should not do anything else.
 */
static void
on_whole_folder_channel_press (
  GtkGestureClick *     gesture,
  gint                  n_press,
  gdouble               x,
  gdouble               y,
  FolderChannelWidget * self)
{
  self->n_press = n_press;

  GdkModifierType state = gtk_event_controller_get_current_event_state (
    GTK_EVENT_CONTROLLER (gesture));
  self->ctrl_held_at_start = state & GDK_CONTROL_MASK;
}

static void
on_drag_begin (
  GtkGestureDrag *      gesture,
  gdouble               start_x,
  gdouble               start_y,
  FolderChannelWidget * self)
{
  self->selected_in_dnd = 0;
  self->dragged = 0;
}

static void
on_drag_update (
  GtkGestureDrag *      gesture,
  gdouble               offset_x,
  gdouble               offset_y,
  FolderChannelWidget * self)
{
  self->dragged = true;
}

static void
on_btn_release (
  GtkGestureClick * click_gesture,
  gint              n_press,
  gdouble           x,
  gdouble           y,
  gpointer          user_data)
{
  FolderChannelWidget * self = Z_FOLDER_CHANNEL_WIDGET (user_data);

  if (self->dragged || self->selected_in_dnd)
    return;

  GdkModifierType state = gtk_event_controller_get_current_event_state (
    GTK_EVENT_CONTROLLER (click_gesture));

  Track * track = self->track;
  if (self->n_press == 1)
    {
      PROJECT->last_selection =
        ZProjectSelectionType::Z_PROJECT_SELECTION_TYPE_TRACKLIST;

      bool ctrl = state & GDK_CONTROL_MASK;
      bool shift = state & GDK_SHIFT_MASK;
      tracklist_selections_handle_click (track, ctrl, shift, self->dragged);
    }
}

static void
refresh_color (FolderChannelWidget * self)
{
  Track * track = self->track;
  color_area_widget_setup_track (self->color_top, track);
  color_area_widget_set_color (self->color_top, &track->color);
  color_area_widget_set_color (self->color_left, &track->color);
}

static void
setup_folder_channel_icon (FolderChannelWidget * self)
{
  Track * track = self->track;
  gtk_image_set_from_icon_name (self->icon, track->icon_name);
  gtk_widget_set_sensitive (GTK_WIDGET (self->icon), track_is_enabled (track));
}

static void
refresh_name (FolderChannelWidget * self)
{
  Track * track = self->track;
  if (track_is_enabled (track))
    {
      gtk_label_set_markup (self->name_lbl, track->name);
    }
  else
    {
      char * markup =
        g_strdup_printf ("<span foreground=\"grey\">%s</span>", track->name);
      gtk_label_set_markup (self->name_lbl, markup);
    }
}

/**
 * Updates everything on the widget.
 *
 * It is redundant but keeps code organized. Should fix if it causes lags.
 */
void
folder_channel_widget_refresh (FolderChannelWidget * self)
{
  refresh_name (self);
  refresh_color (self);
  setup_folder_channel_icon (self);
  fader_buttons_widget_refresh (self->fader_buttons, self->track);

#define ICON_NAME_FOLD "fluentui-folder-regular"
#define ICON_NAME_FOLD_OPEN "fluentui-folder-open-regular"

  Track * track = self->track;
  gtk_button_set_icon_name (
    GTK_BUTTON (self->fold_toggle),
    track->folded ? ICON_NAME_FOLD : ICON_NAME_FOLD_OPEN);

#undef ICON_NAME_FOLD
#undef ICON_NAME_FOLD_OPEN

  g_signal_handler_block (self->fold_toggle, self->fold_toggled_handler_id);
  gtk_toggle_button_set_active (self->fold_toggle, !track->folded);
  g_signal_handler_unblock (self->fold_toggle, self->fold_toggled_handler_id);

  if (track_is_selected (track))
    {
      /* set selected or not */
      gtk_widget_set_state_flags (GTK_WIDGET (self), GTK_STATE_FLAG_SELECTED, 0);
    }
  else
    {
      gtk_widget_unset_state_flags (GTK_WIDGET (self), GTK_STATE_FLAG_SELECTED);
    }
}

static void
show_context_menu (FolderChannelWidget * self, double x, double y)
{
  GMenu * menu = channel_widget_generate_context_menu_for_track (self->track);

  z_gtk_show_context_menu_from_g_menu (self->popover_menu, x, y, menu);
}

static void
on_right_click (
  GtkGestureClick *     gesture,
  gint                  n_press,
  gdouble               x,
  gdouble               y,
  FolderChannelWidget * self)
{
  GdkModifierType state = gtk_event_controller_get_current_event_state (
    GTK_EVENT_CONTROLLER (gesture));

  Track * track = self->track;
  if (!track_is_selected (track))
    {
      if (state & GDK_SHIFT_MASK || state & GDK_CONTROL_MASK)
        {
          track_select (track, F_SELECT, 0, 1);
        }
      else
        {
          track_select (track, F_SELECT, 1, 1);
        }
    }
  if (n_press == 1)
    {
      show_context_menu (self, x, y);
    }
}

static void
on_fold_toggled (GtkToggleButton * toggle, FolderChannelWidget * self)
{
  bool folded = !gtk_toggle_button_get_active (toggle);

  track_set_folded (
    self->track, folded, F_TRIGGER_UNDO, F_AUTO_SELECT, F_PUBLISH_EVENTS);
}

static void
on_destroy (FolderChannelWidget * self)
{
  folder_channel_widget_tear_down (self);
}

static void
setup_dnd (FolderChannelWidget * self)
{
  GtkDragSource * drag_source = gtk_drag_source_new ();
  g_signal_connect (
    drag_source, "prepare", G_CALLBACK (on_dnd_drag_prepare), self);
  g_signal_connect (
    drag_source, "drag-begin", G_CALLBACK (on_dnd_drag_begin), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self->icon_and_name_event_box),
    GTK_EVENT_CONTROLLER (drag_source));

  /* set as drag dest for folder_channel (the
   * folder_channel will
   * be moved based on which half it was dropped in,
   * left or right) */
  GtkDropTarget * drop_target = gtk_drop_target_new (
    WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE, GDK_ACTION_MOVE | GDK_ACTION_COPY);
  gtk_drop_target_set_preload (drop_target, true);
  g_signal_connect (
    G_OBJECT (drop_target), "drop", G_CALLBACK (on_dnd_drop), self);
  g_signal_connect (
    G_OBJECT (drop_target), "motion", G_CALLBACK (on_dnd_motion), self);
  g_signal_connect (
    G_OBJECT (drop_target), "leave", G_CALLBACK (on_dnd_leave), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (drop_target));
}

FolderChannelWidget *
folder_channel_widget_new (Track * track)
{
  FolderChannelWidget * self = static_cast<FolderChannelWidget *> (
    g_object_new (FOLDER_CHANNEL_WIDGET_TYPE, NULL));
  self->track = track;

  setup_folder_channel_icon (self);

  self->fold_toggled_handler_id = g_signal_connect (
    self->fold_toggle, "toggled", G_CALLBACK (on_fold_toggled), self);
  g_signal_connect (self, "destroy", G_CALLBACK (on_destroy), NULL);

  setup_dnd (self);

  folder_channel_widget_refresh (self);

  g_object_ref (self);

  self->setup = true;

  return self;
}

/**
 * Prepare for finalization.
 */
void
folder_channel_widget_tear_down (FolderChannelWidget * self)
{
  if (self->setup)
    {
      g_object_unref (self);
      self->track->folder_ch_widget = NULL;
      self->setup = false;
    }
}

static void
dispose (FolderChannelWidget * self)
{
  gtk_widget_unparent (GTK_WIDGET (self->popover_menu));

  G_OBJECT_CLASS (folder_channel_widget_parent_class)->dispose (G_OBJECT (self));
}

static void
folder_channel_widget_class_init (FolderChannelWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "folder_channel.ui");
  gtk_widget_class_set_css_name (klass, "folder-channel");
  klass->snapshot = folder_channel_snapshot;

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child (klass, FolderChannelWidget, x)

  BIND_CHILD (color_left);
  BIND_CHILD (color_top);
  BIND_CHILD (grid);
  BIND_CHILD (icon_and_name_event_box);
  BIND_CHILD (name_lbl);
  BIND_CHILD (fader_buttons);
  BIND_CHILD (icon);
  BIND_CHILD (fold_toggle);
  BIND_CHILD (highlight_left_box);
  BIND_CHILD (highlight_right_box);

#undef BIND_CHILD

  gtk_widget_class_set_layout_manager_type (klass, GTK_TYPE_BOX_LAYOUT);

  GObjectClass * oklass = G_OBJECT_CLASS (_klass);
  oklass->dispose = (GObjectFinalizeFunc) dispose;
}

static void
folder_channel_widget_init (FolderChannelWidget * self)
{
  g_type_ensure (FADER_BUTTONS_WIDGET_TYPE);
  g_type_ensure (COLOR_AREA_WIDGET_TYPE);
  g_type_ensure (GTK_TYPE_FLIPPER);

  gtk_widget_init_template (GTK_WIDGET (self));

  self->popover_menu = GTK_POPOVER_MENU (gtk_popover_menu_new_from_model (NULL));
  gtk_widget_set_parent (GTK_WIDGET (self->popover_menu), GTK_WIDGET (self));

  gtk_widget_set_hexpand (GTK_WIDGET (self), 0);

  self->drag = GTK_GESTURE_DRAG (gtk_gesture_drag_new ());
  g_signal_connect (
    G_OBJECT (self->drag), "drag-begin", G_CALLBACK (on_drag_begin), self);
  g_signal_connect (
    G_OBJECT (self->drag), "drag-update", G_CALLBACK (on_drag_update), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (self->drag));

  self->mp = GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  g_signal_connect (
    G_OBJECT (self->mp), "pressed", G_CALLBACK (on_whole_folder_channel_press),
    self);
  g_signal_connect (
    G_OBJECT (self->mp), "released", G_CALLBACK (on_btn_release), self);
  gtk_widget_add_controller (GTK_WIDGET (self), GTK_EVENT_CONTROLLER (self->mp));

  self->right_mouse_mp = GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (self->right_mouse_mp), GDK_BUTTON_SECONDARY);
  g_signal_connect (
    G_OBJECT (self->right_mouse_mp), "pressed", G_CALLBACK (on_right_click),
    self);
  gtk_widget_add_controller (
    GTK_WIDGET (self->icon_and_name_event_box),
    GTK_EVENT_CONTROLLER (self->right_mouse_mp));
}
