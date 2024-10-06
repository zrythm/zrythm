// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/master_track.h"
#include "common/dsp/meter.h"
#include "common/dsp/track.h"
#include "common/dsp/tracklist.h"
#include "common/utils/flags.h"
#include "common/utils/gtk.h"
#include "common/utils/math.h"
#include "common/utils/resources.h"
#include "common/utils/rt_thread_id.h"
#include "gui/backend/backend/event.h"
#include "gui/backend/backend/event_manager.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings/g_settings_manager.h"
#include "gui/backend/backend/settings/settings.h"
#include "gui/backend/backend/wrapped_object_with_change_signal.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/gtk_widgets/balance_control.h"
#include "gui/backend/gtk_widgets/bot_dock_edge.h"
#include "gui/backend/gtk_widgets/center_dock.h"
#include "gui/backend/gtk_widgets/channel.h"
#include "gui/backend/gtk_widgets/channel_sends_expander.h"
#include "gui/backend/gtk_widgets/channel_slot.h"
#include "gui/backend/gtk_widgets/color_area.h"
#include "gui/backend/gtk_widgets/editable_label.h"
#include "gui/backend/gtk_widgets/expander_box.h"
#include "gui/backend/gtk_widgets/fader.h"
#include "gui/backend/gtk_widgets/fader_buttons.h"
#include "gui/backend/gtk_widgets/gtk_wrapper.h"
#include "gui/backend/gtk_widgets/meter.h"
#include "gui/backend/gtk_widgets/mixer.h"
#include "gui/backend/gtk_widgets/plugin_strip_expander.h"
#include "gui/backend/gtk_widgets/route_target_selector.h"
#include "gui/backend/gtk_widgets/track.h"
#include "gui/backend/gtk_widgets/zrythm_app.h"

#include <glib/gi18n.h>

#include <time.h>

G_DEFINE_TYPE (ChannelWidget, channel_widget, GTK_TYPE_WIDGET)

/**
 * Updates the meter reading
 */
static gboolean
channel_widget_tick_cb (
  GtkWidget *     widget,
  GdkFrameClock * frame_clock,
  gpointer        user_data);

static std::shared_ptr<Channel>
get_channel (ChannelWidget * self)
{
  auto ret = self->channel.lock ();
  z_return_val_if_fail (ret, nullptr);
  return ret;
}

static ChannelTrack *
get_track (ChannelWidget * self)
{
  return get_channel (self)->get_track ();
}

static void
channel_snapshot (GtkWidget * widget, GtkSnapshot * snapshot)
{
  ChannelWidget * self = Z_CHANNEL_WIDGET (widget);

  int width = gtk_widget_get_width (widget);
  int height = gtk_widget_get_height (widget);

  auto track = get_track (self);
  /* tint background */
  GdkRGBA         color = track->color_.to_gdk_rgba_with_alpha (0.15f);
  graphene_rect_t rect =
    Z_GRAPHENE_RECT_INIT (0.f, 0.f, (float) width, (float) height);
  gtk_snapshot_append_color (snapshot, &color, &rect);

  GTK_WIDGET_CLASS (channel_widget_parent_class)->snapshot (widget, snapshot);
}

/**
 * Tick function.
 *
 * usually, the way to do that is via g_idle_add() or g_main_context_invoke()
 * the other option is to use polling and have the GTK thread check if the
 * value changed every monitor refresh
 * that would be done via gtk_widget_set_tick_functions()
 * gtk_widget_set_tick_function()
 */
static gboolean
channel_widget_tick_cb (
  GtkWidget *     widget,
  GdkFrameClock * frame_clock,
  gpointer        user_data)
{
  ChannelWidget * self = Z_CHANNEL_WIDGET (widget);
  double          prev = self->meter_reading_val;
  auto            channel = get_channel (self);

  if (!gtk_widget_get_mapped (GTK_WIDGET (self)))
    {
      return G_SOURCE_CONTINUE;
    }

  auto track = channel->get_track ();

  if (track->is_selected ())
    {
      gtk_widget_add_css_class (GTK_WIDGET (self->name), "caption-heading");
      gtk_widget_remove_css_class (GTK_WIDGET (self->name), "caption");
    }
  else
    {
      gtk_widget_add_css_class (GTK_WIDGET (self->name), "caption");
      gtk_widget_remove_css_class (GTK_WIDGET (self->name), "caption-heading");
    }

  /* TODO */
  if (track->out_signal_type_ == PortType::Event)
    {
      gtk_label_set_text (self->meter_reading, "-∞");
      return G_SOURCE_CONTINUE;
    }

  float amp =
    std::max (self->meter_l->meter->prev_max_, self->meter_r->meter->prev_max_);
  double val = (double) math_amp_to_dbfs (amp);
  if (math_doubles_equal (val, prev))
    return G_SOURCE_CONTINUE;
  if (val < -100.)
    gtk_label_set_text (self->meter_reading, "-∞");
  else
    {
      char string[40];
      if (val < -9.99)
        {
          sprintf (string, "%.0f", val);
        }
      else
        {
          sprintf (string, "%.1f", val);
        }
      char formatted_str[80];
      if (val > 0)
        {
          sprintf (
            formatted_str, "<span foreground=\"#FF0A05\">%s</span>", string);
        }
      else
        {
          strcpy (formatted_str, string);
        }
      gtk_label_set_markup (self->meter_reading, formatted_str);
    }

  self->meter_reading_val = val;

  return G_SOURCE_CONTINUE;
}

static gboolean
on_dnd_drop (
  GtkDropTarget * drop_target,
  const GValue *  value,
  gdouble         x,
  gdouble         y,
  gpointer        user_data)
{
  if (!G_VALUE_HOLDS (value, WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE))
    {
      z_info ("invalid DND type");
      return false;
    }

  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (g_value_get_object (value));
  if (wrapped_obj->type != WrappedObjectType::WRAPPED_OBJECT_TYPE_TRACK)
    {
      z_info ("dropped object not a track");
      return false;
    }

  ChannelWidget * self = Z_CHANNEL_WIDGET (user_data);
  GtkWidget *     widget = GTK_WIDGET (self);

  auto owner_track = get_track (self);

  /* determine if moving or copying */
  GdkDragAction action = z_gtk_drop_target_get_selected_action (drop_target);

  const char * action_str = "???";
  if (action == GDK_ACTION_COPY)
    action_str = "COPY";
  else if (action == GDK_ACTION_MOVE)
    action_str = "MOVE";
  z_debug ("channel widget: dnd drop (action {})", action_str);

  int w = gtk_widget_get_width (widget);

  TrackWidgetHighlight location;
  if (owner_track->is_foldable () && x < w - 12 && x > 12)
    {
      location = TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_INSIDE;
    }
  else if (x < w / 2)
    {
      location = TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_TOP;
    }
  else
    {
      location = TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_BOTTOM;
    }
  TRACKLIST->handle_move_or_copy (*owner_track, location, action);

  return true;
}

/**
 * Highlights/unhighlights the channels
 * appropriately.
 */
static void
do_highlight (ChannelWidget * self, gint x, gint y)
{
  return;
  /* if we are closer to the start of selection than
   * the end */
  int w = gtk_widget_get_width (GTK_WIDGET (self));
  if (x < w / 2)
    {
      /* highlight left */
#if 0
      gtk_drag_highlight (
        GTK_WIDGET (
          self->highlight_left_box));
#endif
      gtk_widget_set_size_request (GTK_WIDGET (self->highlight_left_box), 2, -1);

      /* unhighlight right */
#if 0
      gtk_drag_unhighlight (
        GTK_WIDGET (
          self->highlight_right_box));
#endif
      gtk_widget_set_size_request (
        GTK_WIDGET (self->highlight_right_box), -1, -1);
    }
  else
    {
      /* highlight right */
#if 0
      gtk_drag_highlight (
        GTK_WIDGET (
          self->highlight_right_box));
#endif
      gtk_widget_set_size_request (
        GTK_WIDGET (self->highlight_right_box), 2, -1);

      /* unhighlight left */
#if 0
      gtk_drag_unhighlight (
        GTK_WIDGET (
          self->highlight_left_box));
#endif
      gtk_widget_set_size_request (
        GTK_WIDGET (self->highlight_left_box), -1, -1);
    }
}

static GdkDragAction
on_dnd_drag_motion (
  GtkDropTarget * drop_target,
  gdouble         x,
  gdouble         y,
  gpointer        user_data)
{
  ChannelWidget * self = Z_CHANNEL_WIDGET (user_data);

  do_highlight (self, (int) x, (int) y);

  return GDK_ACTION_MOVE;
}

static void
on_dnd_drag_leave (GtkDropTarget * drop_target, gpointer user_data)
{
  ChannelWidget * self = Z_CHANNEL_WIDGET (user_data);

  z_debug ("channel dnd drag leave");

  /*do_highlight (self);*/
  /*gtk_drag_unhighlight (*/
  /*GTK_WIDGET (self->highlight_left_box));*/
  gtk_widget_set_size_request (GTK_WIDGET (self->highlight_left_box), -1, -1);
  /*gtk_drag_unhighlight (*/
  /*GTK_WIDGET (self->highlight_right_box));*/
  gtk_widget_set_size_request (GTK_WIDGET (self->highlight_right_box), -1, -1);
}

/**
 * Callback when somewhere in the channel is
 * pressed.
 *
 * Only responsible for setting the tracklist
 * selection and should not do anything else.
 */
static void
on_whole_channel_press (
  GtkGestureClick * gesture,
  gint              n_press,
  gdouble           x,
  gdouble           y,
  ChannelWidget *   self)
{
  self->n_press = n_press;

  GdkModifierType state = gtk_event_controller_get_current_event_state (
    GTK_EVENT_CONTROLLER (gesture));
  self->ctrl_held_at_start = state & GDK_CONTROL_MASK;
}

void
channel_widget_redraw_fader (ChannelWidget * self)
{
  z_return_if_fail (self->fader);
  gtk_widget_queue_draw (GTK_WIDGET (self->fader));
}

static void
on_drag_begin (
  GtkGestureDrag * gesture,
  gdouble          start_x,
  gdouble          start_y,
  ChannelWidget *  self)
{
  self->selected_in_dnd = 0;
  self->dragged = 0;
}

static void
on_drag_update (
  GtkGestureDrag * gesture,
  gdouble          offset_x,
  gdouble          offset_y,
  ChannelWidget *  self)
{
  self->dragged = 1;
}

static void
on_btn_release (
  GtkGestureClick * gesture_click,
  gint              n_press,
  gdouble           x,
  gdouble           y,
  gpointer          user_data)
{
  ChannelWidget * self = Z_CHANNEL_WIDGET (user_data);

  if (self->dragged || self->selected_in_dnd)
    return;

  auto track = get_track (self);
  if (n_press == 1)
    {
      auto cur_time = SteadyClock::now ();
      if (cur_time - self->last_plugin_press > std::chrono::milliseconds (1))
        {
          PROJECT->last_selection_ = Project::SelectionType::Tracklist;
        }

      GdkModifierType state = gtk_event_controller_get_current_event_state (
        GTK_EVENT_CONTROLLER (gesture_click));

      bool ctrl = state & GDK_CONTROL_MASK;
      bool shift = state & GDK_SHIFT_MASK;
      TRACKLIST_SELECTIONS->handle_click (*track, ctrl, shift, self->dragged);
    }
}

static void
refresh_color (ChannelWidget * self)
{
  auto track = get_track (self);
  color_area_widget_set_color (self->color, track->color_);
}

#if 0
static void
setup_phase_panel (ChannelWidget * self)
{
  self->phase_knob =
    knob_widget_new_simple (
      channel_get_phase,
      channel_set_phase,
      self->channel,
      0, 180, 20, 0.0f);
  gtk_box_pack_end (self->phase_controls,
                       GTK_WIDGET (self->phase_knob),
                       0, 1, 0);
  char * str =
    g_strdup_printf (
      "%.1f",
      (double) self->channel->fader.phase);
  gtk_label_set_text (
    self->phase_reading, str);
  g_free (str);
}
#endif

static void
setup_meter (ChannelWidget * self)
{
  auto ch = get_channel (self);
  auto track = ch->get_track ();
  switch (track->out_signal_type_)
    {
    case PortType::Event:
      meter_widget_setup (self->meter_l, ch->midi_out_.get (), false);
      gtk_widget_set_margin_start (GTK_WIDGET (self->meter_l), 5);
      gtk_widget_set_margin_end (GTK_WIDGET (self->meter_l), 5);
      gtk_widget_set_visible (GTK_WIDGET (self->meter_r), 0);
      break;
    case PortType::Audio:
      meter_widget_setup (self->meter_l, &ch->stereo_out_->get_l (), false);
      meter_widget_setup (self->meter_r, &ch->stereo_out_->get_r (), false);
      break;
    default:
      break;
    }
}

/**
 * Updates the inserts.
 */
void
channel_widget_update_midi_fx_and_inserts (ChannelWidget * self)
{
  auto track = get_track (self);
  plugin_strip_expander_widget_refresh (self->inserts);
  if (track->in_signal_type_ == PortType::Event)
    {
      plugin_strip_expander_widget_refresh (self->midi_fx);
    }
}

void
channel_widget_refresh_instrument_ui_toggle (ChannelWidget * self)
{
  auto ch = get_channel (self);
  if (ch->instrument_)
    {
      g_signal_handler_block (
        self->instrument_ui_toggle, self->instrument_ui_toggled_id);
      gtk_toggle_button_set_active (
        self->instrument_ui_toggle, ch->instrument_->visible_);
      g_signal_handler_unblock (
        self->instrument_ui_toggle, self->instrument_ui_toggled_id);
    }
}

static void
on_instrument_ui_toggled (GtkToggleButton * toggle, gpointer user_data)
{
  ChannelWidget * self = Z_CHANNEL_WIDGET (user_data);
  auto            ch = get_channel (self);
  auto           &pl = ch->instrument_;
  pl->visible_ = gtk_toggle_button_get_active (toggle);
  EVENTS_PUSH (EventType::ET_PLUGIN_VISIBILITY_CHANGED, pl.get ());
}

static void
setup_instrument_ui_toggle (ChannelWidget * self)
{
  auto ch = get_channel (self);
  auto track = ch->get_track ();
  gtk_widget_set_visible (
    GTK_WIDGET (self->instrument_ui_toggle), track->is_instrument ());
  if (!track->is_instrument ())
    return;

  gtk_toggle_button_set_active (
    self->instrument_ui_toggle, ch->instrument_->visible_);
  self->instrument_ui_toggled_id = g_signal_connect (
    self->instrument_ui_toggle, "toggled",
    G_CALLBACK (on_instrument_ui_toggled), self);
}

static void
refresh_output (ChannelWidget * self)
{
  auto track = get_track (self);
  route_target_selector_widget_refresh (self->output, track);
}

static void
refresh_name (ChannelWidget * self)
{
  auto track = get_track (self);
  z_return_if_fail (!track->name_.empty ());
  if (track->is_enabled ())
    {
      gtk_label_set_text (GTK_LABEL (self->name->label), track->name_.c_str ());
    }
  else
    {
      auto markup =
        fmt::format ("<span foreground=\"grey\">{}</span>", track->name_);
      gtk_label_set_markup (GTK_LABEL (self->name->label), markup.c_str ());
    }
}

static void
setup_balance_control (ChannelWidget * self)
{
  auto ch = get_channel (self);
  self->balance_control = balance_control_widget_new (
    bind_member_function (*ch, &Channel::get_balance_control),
    bind_member_function (*ch, &Channel::set_balance_control), ch.get (),
    ch->fader_->balance_.get (), 12);
  gtk_box_append (self->balance_control_box, GTK_WIDGET (self->balance_control));
}

static void
setup_aux_buttons (ChannelWidget * self)
{
}

void
channel_widget_refresh_buttons (ChannelWidget * self)
{
  fader_buttons_widget_refresh (self->fader_buttons, get_track (self));
}

static void
update_reveal_status (ChannelWidget * self)
{
  expander_box_widget_set_reveal (
    Z_EXPANDER_BOX_WIDGET (self->inserts),
    g_settings_get_boolean (S_UI_MIXER, "inserts-expanded"));
  expander_box_widget_set_reveal (
    Z_EXPANDER_BOX_WIDGET (self->midi_fx),
    g_settings_get_boolean (S_UI_MIXER, "midi-fx-expanded"));
  expander_box_widget_set_reveal (
    Z_EXPANDER_BOX_WIDGET (self->sends),
    g_settings_get_boolean (S_UI_MIXER, "sends-expanded"));
}

/**
 * Updates everything on the widget.
 *
 * It is redundant but keeps code organized. Should fix if it causes lags.
 */
void
channel_widget_refresh (ChannelWidget * self)
{
  refresh_name (self);
  refresh_output (self);
  /*channel_widget_update_meter_reading (*/
  /*self, nullptr, nullptr);*/
  channel_widget_refresh_buttons (self);
  refresh_color (self);
  update_reveal_status (self);

  auto track = get_track (self);
  if (track->is_selected ())
    {
      /* set selected or not */
      gtk_widget_set_state_flags (GTK_WIDGET (self), GTK_STATE_FLAG_SELECTED, 0);
    }
  else
    {
      gtk_widget_unset_state_flags (GTK_WIDGET (self), GTK_STATE_FLAG_SELECTED);
    }
}

/**
 * Generates a context menu for the given track
 * to be used in the mixer.
 */
GMenu *
channel_widget_generate_context_menu_for_track (Track * track)
{
  GMenu *     menu = g_menu_new ();
  GMenuItem * menuitem;

  int num_selected = TRACKLIST_SELECTIONS->get_num_tracks ();

  if (num_selected > 0)
    {
      GMenu * track_submenu = g_menu_new ();

      GMenu * edit_submenu = track->generate_edit_context_menu (num_selected);
      GMenuItem * edit_submenu_item =
        g_menu_item_new_section (nullptr, G_MENU_MODEL (edit_submenu));
      g_menu_item_set_attribute (
        edit_submenu_item, "display-hint", "s", "horizontal-buttons");
      g_menu_append_item (menu, edit_submenu_item);

      g_menu_append_section (menu, _ ("Track"), G_MENU_MODEL (track_submenu));
    }

  /* add solo/mute/listen */
  if (track->has_channel () || track->is_folder ())
    {
      auto        ch_track = dynamic_cast<ChannelTrack *> (track);
      GMenu *     channel_submenu = ch_track->generate_channel_context_menu ();
      GMenuItem * channel_submenu_item =
        g_menu_item_new_section (_ ("Channel"), G_MENU_MODEL (channel_submenu));
      g_menu_append_item (menu, channel_submenu_item);
    }

  /* add enable/disable */
  if (TRACKLIST_SELECTIONS->contains_enabled_track (true))
    {
      menuitem = z_gtk_create_menu_item (
        _ ("Disable"), "offline", "app.disable-selected-tracks");
      g_menu_append_item (menu, menuitem);
    }
  else
    {
      menuitem = z_gtk_create_menu_item (
        _ ("Enable"), "online", "app.enable-selected-tracks");
      g_menu_append_item (menu, menuitem);
    }

  return menu;
}

static void
show_context_menu (ChannelWidget * self, double x, double y)
{
  auto    track = get_track (self);
  GMenu * menu = channel_widget_generate_context_menu_for_track (track);

  z_gtk_show_context_menu_from_g_menu (self->popover_menu, x, y, menu);
  if (self->fader_buttons_for_popover)
    {
      gtk_popover_menu_add_child (
        self->popover_menu, GTK_WIDGET (self->fader_buttons_for_popover),
        "fader-buttons");
    }
}

static void
on_right_click (
  GtkGestureClick * gesture,
  gint              n_press,
  gdouble           x,
  gdouble           y,
  ChannelWidget *   self)
{
  GdkModifierType state = gtk_event_controller_get_current_event_state (
    GTK_EVENT_CONTROLLER (gesture));

  auto track = get_track (self);
  if (!track->is_selected ())
    {
      if (state & GDK_SHIFT_MASK || state & GDK_CONTROL_MASK)
        {
          track->select (true, false, true);
        }
      else
        {
          track->select (true, true, true);
        }
    }
  if (n_press == 1)
    {
      show_context_menu (self, x, y);
    }
}

static void
on_destroy (ChannelWidget * self)
{
  channel_widget_tear_down (self);
}

static GdkContentProvider *
on_dnd_drag_prepare (
  GtkDragSource * source,
  double          x,
  double          y,
  ChannelWidget * self)
{
  auto track = get_track (self);
  return std::visit (
    [&] (auto &&derived_track) {
      WrappedObjectWithChangeSignal * wrapped_obj =
        wrapped_object_with_change_signal_new (
          derived_track, WrappedObjectType::WRAPPED_OBJECT_TYPE_TRACK);
      GdkContentProvider * content_providers[] = {
        gdk_content_provider_new_typed (
          WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE, wrapped_obj),
      };

      return gdk_content_provider_new_union (
        content_providers, G_N_ELEMENTS (content_providers));
    },
    convert_to_variant<TrackPtrVariant> (track));
}

static void
on_dnd_drag_begin (GtkDragSource * source, GdkDrag * drag, gpointer user_data)
{
  ChannelWidget * self = Z_CHANNEL_WIDGET (user_data);

  /* set the widget as the drag icon */
  GdkPaintable * paintable = gtk_widget_paintable_new (GTK_WIDGET (self));
  gtk_drag_source_set_icon (source, paintable, 0, 0);
  g_object_unref (paintable);

  auto track = get_track (self);
  self->selected_in_dnd = true;
  MW_MIXER->start_drag_track = track;

  if (self->n_press == 1)
    {
      int ctrl = 0, selected = 0;

      ctrl = self->ctrl_held_at_start;

      if (TRACKLIST_SELECTIONS->contains_track (*track))
        selected = 1;

      /* no control & not selected */
      if (!ctrl && !selected)
        {
          TRACKLIST_SELECTIONS->select_single (*track, true);
        }
      else if (!ctrl && selected)
        {
        }
      else if (ctrl && !selected)
        TRACKLIST_SELECTIONS->add_track (*track, true);
    }
}

static void
setup_dnd (ChannelWidget * self)
{
  /* set as drag source for track */
  GtkDragSource * drag_source = gtk_drag_source_new ();
  gtk_drag_source_set_actions (
    drag_source,
    static_cast<GdkDragAction> (
      static_cast<int> (GDK_ACTION_COPY) | static_cast<int> (GDK_ACTION_MOVE)));
  g_signal_connect (
    drag_source, "prepare", G_CALLBACK (on_dnd_drag_prepare), self);
  g_signal_connect (
    drag_source, "drag-begin", G_CALLBACK (on_dnd_drag_begin), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self->icon_and_name_event_box),
    GTK_EVENT_CONTROLLER (drag_source));

  /* set as drag dest for channel (the channel will
   * be moved based on which half it was dropped in,
   * left or right) */
  GtkDropTarget * drop_target = gtk_drop_target_new (
    WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE,
    static_cast<GdkDragAction> (
      static_cast<int> (GDK_ACTION_MOVE) | static_cast<int> (GDK_ACTION_COPY)));
  gtk_drop_target_set_preload (drop_target, true);
  g_signal_connect (drop_target, "drop", G_CALLBACK (on_dnd_drop), self);
  g_signal_connect (
    drop_target, "motion", G_CALLBACK (on_dnd_drag_motion), self);
  g_signal_connect (drop_target, "leave", G_CALLBACK (on_dnd_drag_leave), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (drop_target));
}

ChannelWidget *
channel_widget_new (const std::shared_ptr<Channel> &channel)
{
  auto * self =
    static_cast<ChannelWidget *> (g_object_new (CHANNEL_WIDGET_TYPE, nullptr));
  self->channel = channel;

  auto track = get_track (self);

  self->fader_buttons_for_popover =
    g_object_ref (fader_buttons_widget_new (track));

  plugin_strip_expander_widget_setup (
    self->inserts, PluginSlotType::Insert,
    PluginStripExpanderPosition::PSE_POSITION_CHANNEL, track);
  if (track->in_signal_type_ == PortType::Event)
    {
      plugin_strip_expander_widget_setup (
        self->midi_fx, PluginSlotType::MidiFx,
        PluginStripExpanderPosition::PSE_POSITION_CHANNEL, track);
    }
  else
    {
      gtk_widget_set_visible (GTK_WIDGET (self->midi_fx), false);
    }
  channel_sends_expander_widget_setup (
    self->sends, ChannelSendsExpanderPosition::CSE_POSITION_CHANNEL, track);
  setup_aux_buttons (self);
  fader_widget_setup (self->fader, channel->fader_.get (), -1);
  setup_meter (self);
  setup_balance_control (self);
  setup_instrument_ui_toggle (self);
  editable_label_widget_setup (
    self->name, track,
    bind_member_function (*(dynamic_cast<Track *> (track)), &Track::get_name),
    bind_member_function (
      *(dynamic_cast<Track *> (track)), &Track::set_name_with_action));
  route_target_selector_widget_refresh (self->output, track);
  color_area_widget_setup_track (self->color, track);

#if 0
  /*if (self->channel->track.type ==*/
        /*Track::Type::INSTRUMENT)*/
    /*{*/
      self->instrument_slot =
        channel_slot_widget_new (
          -1, self->channel, PluginSlotType::Instrument,
          true);
      gtk_widget_set_visible (
        GTK_WIDGET (self->instrument_slot), true);
      gtk_widget_set_hexpand (
        GTK_WIDGET (self->instrument_slot), true);
      gtk_container_add (
        GTK_CONTAINER (self->instrument_box),
        GTK_WIDGET (self->instrument_slot));
    /*}*/
#endif

  channel_widget_refresh (self);

  gtk_widget_add_tick_callback (
    GTK_WIDGET (self), (GtkTickCallback) channel_widget_tick_cb, self, nullptr);

  g_signal_connect (self, "destroy", G_CALLBACK (on_destroy), nullptr);

  self->setup = true;

  return self;
}

/**
 * Prepare for finalization.
 */
void
channel_widget_tear_down (ChannelWidget * self)
{
  z_debug ("tearing down channel widget {}...", fmt::ptr (self));

  if (self->setup)
    {
      if (auto ch = self->channel.lock ())
        {
          ch->widget_ = nullptr;
        }
      self->setup = false;
    }

  z_debug ("done");
}

static void
channel_widget_dispose (GObject * obj)
{
  ChannelWidget * self = Z_CHANNEL_WIDGET (obj);

  gtk_widget_unparent (GTK_WIDGET (self->box));
  gtk_widget_unparent (GTK_WIDGET (self->popover_menu));

  G_OBJECT_CLASS (channel_widget_parent_class)->dispose (obj);
}

static void
channel_widget_finalize (ChannelWidget * self)
{
  self->channel.~weak_ptr<Channel> ();
  self->last_plugin_press.~SteadyTimePoint ();
  self->last_midi_trigger_time.~SteadyTimePoint ();

  G_OBJECT_CLASS (channel_widget_parent_class)->finalize (G_OBJECT (self));
}

static void
channel_widget_class_init (ChannelWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "channel.ui");
  gtk_widget_class_set_css_name (klass, "channel");
  klass->snapshot = channel_snapshot;

  gtk_widget_class_set_layout_manager_type (klass, GTK_TYPE_BIN_LAYOUT);

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child (klass, ChannelWidget, x)

  BIND_CHILD (box);
  BIND_CHILD (color);
  BIND_CHILD (output);
  BIND_CHILD (grid);
  BIND_CHILD (icon_and_name_event_box);
  BIND_CHILD (name);
  BIND_CHILD (mid_box);
  BIND_CHILD (inserts);
  BIND_CHILD (midi_fx);
  BIND_CHILD (sends);
  BIND_CHILD (instrument_box);
  BIND_CHILD (fader_buttons);
  BIND_CHILD (fader);
  BIND_CHILD (meter_area);
  BIND_CHILD (meter_l);
  BIND_CHILD (meter_r);
  BIND_CHILD (meter_reading);
  BIND_CHILD (instrument_ui_toggle);
  BIND_CHILD (balance_control_box);
  BIND_CHILD (highlight_left_box);
  BIND_CHILD (highlight_right_box);
  BIND_CHILD (aux_buttons_box);

#undef BIND_CHILD

  GObjectClass * object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = channel_widget_dispose;
  object_class->finalize = (GObjectFinalizeFunc) channel_widget_finalize;
}

static void
channel_widget_init (ChannelWidget * self)
{
  new (&self->last_midi_trigger_time) SteadyTimePoint ();
  new (&self->last_plugin_press) SteadyTimePoint ();
  new (&self->channel) std::weak_ptr<Channel> ();

  g_type_ensure (ROUTE_TARGET_SELECTOR_WIDGET_TYPE);
  g_type_ensure (FADER_WIDGET_TYPE);
  g_type_ensure (FADER_BUTTONS_WIDGET_TYPE);
  g_type_ensure (METER_WIDGET_TYPE);
  g_type_ensure (COLOR_AREA_WIDGET_TYPE);
  g_type_ensure (PLUGIN_STRIP_EXPANDER_WIDGET_TYPE);
  g_type_ensure (CHANNEL_SENDS_EXPANDER_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

  self->popover_menu =
    GTK_POPOVER_MENU (gtk_popover_menu_new_from_model (nullptr));
  gtk_widget_set_parent (GTK_WIDGET (self->popover_menu), GTK_WIDGET (self));

  /* set font sizes */
  gtk_label_set_max_width_chars (self->name->label, 10);

  self->mp = GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  gtk_widget_add_controller (GTK_WIDGET (self), GTK_EVENT_CONTROLLER (self->mp));
  self->drag = GTK_GESTURE_DRAG (gtk_gesture_drag_new ());
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (self->drag));

  gtk_widget_set_hexpand (GTK_WIDGET (self), 0);

  self->right_mouse_mp = GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (self->right_mouse_mp), GDK_BUTTON_SECONDARY);
  gtk_widget_add_controller (
    GTK_WIDGET (self->icon_and_name_event_box),
    GTK_EVENT_CONTROLLER (self->right_mouse_mp));

  g_signal_connect (
    G_OBJECT (self->mp), "pressed", G_CALLBACK (on_whole_channel_press), self);
  g_signal_connect (
    G_OBJECT (self->mp), "released", G_CALLBACK (on_btn_release), self);
  g_signal_connect (
    G_OBJECT (self->drag), "drag-begin", G_CALLBACK (on_drag_begin), self);
  g_signal_connect (
    G_OBJECT (self->drag), "drag-update", G_CALLBACK (on_drag_update), self);
  /*g_signal_connect (*/
  /*G_OBJECT (self->drag), "drag-end",*/
  /*G_CALLBACK (on_drag_end), self);*/
  g_signal_connect (
    G_OBJECT (self->right_mouse_mp), "pressed", G_CALLBACK (on_right_click),
    self);

  setup_dnd (self);
}
