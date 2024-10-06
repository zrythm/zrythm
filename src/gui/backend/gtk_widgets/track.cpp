// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/automation_track.h"
#include "common/dsp/instrument_track.h"
#include "common/dsp/region.h"
#include "common/dsp/track.h"
#include "common/dsp/tracklist.h"
#include "common/utils/flags.h"
#include "common/utils/gtk.h"
#include "common/utils/resources.h"
#include "common/utils/rt_thread_id.h"
#include "common/utils/ui.h"
#include "gui/backend/backend/event.h"
#include "gui/backend/backend/event_manager.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/tracklist_selections.h"
#include "gui/backend/backend/wrapped_object_with_change_signal.h"
#include "gui/backend/gtk_widgets/arranger.h"
#include "gui/backend/gtk_widgets/automation_mode.h"
#include "gui/backend/gtk_widgets/bot_bar.h"
#include "gui/backend/gtk_widgets/bot_dock_edge.h"
#include "gui/backend/gtk_widgets/center_dock.h"
#include "gui/backend/gtk_widgets/color_area.h"
#include "gui/backend/gtk_widgets/custom_button.h"
#include "gui/backend/gtk_widgets/dialogs/object_color_chooser_dialog.h"
#include "gui/backend/gtk_widgets/dialogs/track_icon_chooser_dialog.h"
#include "gui/backend/gtk_widgets/editable_label.h"
#include "gui/backend/gtk_widgets/fader_buttons.h"
#include "gui/backend/gtk_widgets/gtk_wrapper.h"
#include "gui/backend/gtk_widgets/left_dock_edge.h"
#include "gui/backend/gtk_widgets/main_notebook.h"
#include "gui/backend/gtk_widgets/main_window.h"
#include "gui/backend/gtk_widgets/meter.h"
#include "gui/backend/gtk_widgets/midi_activity_bar.h"
#include "gui/backend/gtk_widgets/mixer.h"
#include "gui/backend/gtk_widgets/popovers/automatable_selector_popover.h"
#include "gui/backend/gtk_widgets/timeline_arranger.h"
#include "gui/backend/gtk_widgets/timeline_bg.h"
#include "gui/backend/gtk_widgets/timeline_panel.h"
#include "gui/backend/gtk_widgets/track.h"
#include "gui/backend/gtk_widgets/track_canvas.h"
#include "gui/backend/gtk_widgets/tracklist.h"
#include "gui/backend/gtk_widgets/zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (TrackWidget, track_widget, GTK_TYPE_WIDGET)

/**
 * Width of each meter: total 8 for MIDI, total
 * 16 for audio.
 */
// static constexpr int METER_WIDTH = 8;

/** Pixels from the bottom edge to start resizing
 * at. */
static constexpr int RESIZE_PX = 12;

AutomationTrack *
track_widget_get_at_at_y (TrackWidget * self, double y)
{
  auto track = dynamic_cast<AutomatableTrack *> (self->track);
  if (!track || !track->automation_visible_)
    return nullptr;

  auto &atl = track->get_automation_tracklist ();

  for (auto &at : atl.ats_)
    {
      if (!at->created_ || !at->visible_)
        continue;

      if (y >= at->y_ && y < at->y_ + at->height_)
        return at.get ();
    }

  return NULL;
}

const char *
track_widget_highlight_to_str (TrackWidgetHighlight highlight)
{
  switch (highlight)
    {
    case TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_NONE:
      return "Highlight none";
    case TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_TOP:
      return "Highlight top";
    case TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_INSIDE:
      return "Highlight inside";
    case TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_BOTTOM:
      return "Highlight bottom";
    default:
      z_return_val_if_reached ("unknown highlight val");
    }
}

CustomButtonWidget *
track_widget_get_hovered_button (TrackWidget * self, int x, int y)
{
  auto is_button_hovered = [x, y] (const auto &cb) {
    return (
      x >= cb->x && x <= cb->x + (cb->width ? cb->width : TRACK_BUTTON_SIZE)
      && y >= cb->y && y <= cb->y + TRACK_BUTTON_SIZE);
  };

  // CustomButtonWidget * cb = nullptr;
  for (auto &cb_ref : self->top_buttons)
    {
      if (is_button_hovered (cb_ref))
        return cb_ref.get ();
    }
  if (TRACK_BOT_BUTTONS_SHOULD_BE_VISIBLE (self->track->main_height_))
    {
      for (auto &cb_ref : self->bot_buttons)
        {
          if (is_button_hovered (cb_ref))
            return cb_ref.get ();
        }
    }

  if (auto track = dynamic_cast<LanedTrack *> (self->track))
    {
      if (track->lanes_visible_)
        {
          auto laned_track_variant =
            convert_to_variant<LanedTrackPtrVariant> (track);
          auto ret = std::visit (
            [&] (auto &laned_track) {
              for (auto &lane : laned_track->lanes_)
                {
                  for (auto &lane_button : lane->buttons_)
                    {
                      if (is_button_hovered (lane_button))
                        return lane_button.get ();
                    }
                }
              return (CustomButtonWidget *) nullptr;
            },
            laned_track_variant);

          if (ret)
            return ret;
        }
    }

  if (auto track = dynamic_cast<AutomatableTrack *> (self->track))
    {
      auto &atl = track->get_automation_tracklist ();
      if (track->automation_visible_)
        {
          for (auto &at : atl.ats_)
            {
              /* skip invisible automation tracks */
              if (!at->visible_)
                continue;

              for (auto &button : at->top_left_buttons_)
                {
                  if (is_button_hovered (button))
                    return button.get ();
                }
              for (auto &button : at->top_right_buttons_)
                {
                  if (is_button_hovered (button))
                    return button.get ();
                }
              if (TRACK_BOT_BUTTONS_SHOULD_BE_VISIBLE (at->height_))
                {
                  for (auto &button : at->bot_left_buttons_)
                    {
                      if (is_button_hovered (button))
                        return button.get ();
                    }
                  for (auto &button : at->bot_right_buttons_)
                    {
                      if (is_button_hovered (button))
                        return button.get ();
                    }
                }
            }
        }
    }
  return nullptr;
}

AutomationModeWidget *
track_widget_get_hovered_am_widget (TrackWidget * self, int x, int y)
{
  if (auto track = dynamic_cast<AutomatableTrack *> (self->track))
    {
      if (track->automation_visible_)
        {
          auto &atl = track->get_automation_tracklist ();
          for (auto &at : atl.ats_)
            {
              /* skip invisible automation tracks */
              if (!at->visible_)
                continue;

              auto &am = at->am_widget_;
              if (!am)
                continue;
              if (
                x >= am->x_ && x <= am->x_ + am->width_ && y >= am->y_
                && y <= am->y_ + TRACK_BUTTON_SIZE)
                {
                  return am.get ();
                }
            }
        }
    }
  return NULL;
}

static AutomationTrack *
get_at_to_resize (TrackWidget * self, int y)
{
  auto track = dynamic_cast<AutomatableTrack *> (self->track);
  if (!track || !track->automation_visible_)
    return NULL;

  auto &atl = track->get_automation_tracklist ();
  int   total_height = (int) track->main_height_;
  if (auto laned_track = dynamic_cast<LanedTrack *> (track))
    {
      if (laned_track->lanes_visible_)
        {
          auto laned_track_variant =
            convert_to_variant<LanedTrackPtrVariant> (track);
          std::visit (
            [&] (auto &laned_t) {
              for (auto &lane : laned_t->lanes_)
                {
                  total_height += (int) lane->height_;
                }
            },
            laned_track_variant);
        }
    }

  for (auto &at : atl.ats_)
    {
      if (at->created_ && at->visible_)
        total_height += (int) at->height_;

      int val = total_height - y;
      if (val >= 0 && val < RESIZE_PX)
        {
          return at.get ();
        }
    }

  return NULL;
}

static TrackLane *
get_lane_to_resize (TrackWidget * self, int y)
{
  auto track = dynamic_cast<LanedTrack *> (self->track);

  if (!track || !track->lanes_visible_)
    return nullptr;

  int  total_height = (int) track->main_height_;
  auto laned_track_variant = convert_to_variant<LanedTrackPtrVariant> (track);
  return std::visit (
    [&] (auto &laned_track) {
      for (auto &lane : laned_track->lanes_)
        {
          total_height += (int) lane->height_;

          int val = total_height - y;
          if (val >= 0 && val < RESIZE_PX)
            {
              return static_cast<TrackLane *> (lane.get ());
            }
        }
      return static_cast<TrackLane *> (nullptr);
    },
    laned_track_variant);
}

static void
set_tooltip_from_button (TrackWidget * self, CustomButtonWidget * cb)
{
#define SET_TOOLTIP(txt) \
  gtk_widget_set_has_tooltip (GTK_WIDGET (self), true); \
  self->tooltip_text = txt

  Track * track = self->track;

  if (!cb)
    {
      gtk_widget_set_has_tooltip (GTK_WIDGET (self), false);
    }
  else if (TRACK_CB_ICON_IS (SOLO))
    {
      SET_TOOLTIP (_ ("Solo"));
    }
  else if (TRACK_CB_ICON_IS (SHOW_UI))
    {
      auto instrument_track = dynamic_cast<InstrumentTrack *> (track);
      if (instrument_track->is_plugin_visible ())
        {
          SET_TOOLTIP (_ ("Hide instrument UI"));
        }
      else
        {
          SET_TOOLTIP (_ ("Show instrument UI"));
        }
    }
  else if (TRACK_CB_ICON_IS (MUTE))
    {
      if (track->get_muted ())
        {
          SET_TOOLTIP (_ ("Unmute"));
        }
      else
        {
          SET_TOOLTIP (_ ("Mute"));
        }
    }
  else if (TRACK_CB_ICON_IS (LISTEN))
    {
      if (track->get_muted ())
        {
          SET_TOOLTIP (_ ("Unlisten"));
        }
      else
        {
          SET_TOOLTIP (_ ("Listen"));
        }
    }
  else if (TRACK_CB_ICON_IS (MONITOR_AUDIO))
    {
      SET_TOOLTIP (_ ("Monitor"));
    }
  else if (TRACK_CB_ICON_IS (MONO_COMPAT))
    {
      SET_TOOLTIP (_ ("Mono compatibility"));
    }
  else if (TRACK_CB_ICON_IS (SWAP_PHASE))
    {
      SET_TOOLTIP (_ ("Swap phase"));
    }
  else if (TRACK_CB_ICON_IS (RECORD))
    {
      auto recordable_track = dynamic_cast<RecordableTrack *> (track);
      if (recordable_track->get_recording ())
        {
          SET_TOOLTIP (_ ("Disarm"));
        }
      else
        {
          SET_TOOLTIP (_ ("Arm for recording"));
        }
    }
  else if (TRACK_CB_ICON_IS (SHOW_TRACK_LANES))
    {
      auto laned_track = dynamic_cast<LanedTrack *> (track);
      if (laned_track->lanes_visible_)
        {
          SET_TOOLTIP (_ ("Hide lanes"));
        }
      else
        {
          SET_TOOLTIP (_ ("Show lanes"));
        }
    }
  else if (TRACK_CB_ICON_IS (SHOW_AUTOMATION_LANES))
    {
      if (cb->owner_type == CustomButtonWidget::Owner::AT)
        {
          SET_TOOLTIP (_ ("Change automatable"));
        }
      else if (cb->owner_type == CustomButtonWidget::Owner::TRACK)
        {
          auto automatable_track = dynamic_cast<AutomatableTrack *> (track);
          if (automatable_track->automation_visible_)
            {
              SET_TOOLTIP (_ ("Hide automation"));
            }
          else
            {
              SET_TOOLTIP (_ ("Show automation"));
            }
        }
    }
  else if (TRACK_CB_ICON_IS (PLUS))
    {
      SET_TOOLTIP (_ ("Add"));
    }
  else if (TRACK_CB_ICON_IS (MINUS))
    {
      SET_TOOLTIP (_ ("Remove"));
    }
  else if (TRACK_CB_ICON_IS (FREEZE))
    {
      SET_TOOLTIP (_ ("Freeze/unfreeze"));
    }
  else if (TRACK_CB_ICON_IS (LOCK))
    {
      SET_TOOLTIP (_ ("Lock/unlock"));
    }
  else if (TRACK_CB_ICON_IS (FOLD_OPEN))
    {
      SET_TOOLTIP (_ ("Fold"));
    }
  else if (TRACK_CB_ICON_IS (FOLD))
    {
      SET_TOOLTIP (_ ("Unfold"));
    }
  else
    {
      /* tooltip missing */
      z_warn_if_reached ();
    }

#undef SET_TOOLTIP
}

static void
on_leave (GtkEventControllerMotion * motion_controller, TrackWidget * self)
{
  ui_set_pointer_cursor (GTK_WIDGET (self));
  if (!self->resizing)
    {
      self->bg_hovered = false;
      self->color_area_hovered = false;
      self->resize = 0;
      self->button_pressed = 0;
    }
  self->icon_hovered = false;
}

static void
on_enter (
  GtkEventControllerMotion * motion_controller,
  gdouble                    x,
  gdouble                    y,
  TrackWidget *              self)
{
  self->bg_hovered = true;
  self->color_area_hovered = false;
  self->resize = 0;
}

static void
on_motion (
  GtkEventControllerMotion * motion_controller,
  gdouble                    x,
  gdouble                    y,
  TrackWidget *              self)
{
  /*int height =*/
  /*gtk_widget_get_height (widget);*/

  /* show resize cursor or not */
  if (self->bg_hovered)
    {
      self->color_area_hovered = x < TRACK_COLOR_AREA_WIDTH;
      CustomButtonWidget * cb =
        track_widget_get_hovered_button (self, (int) x, (int) y);
      AutomationModeWidget * am =
        track_widget_get_hovered_am_widget (self, (int) x, (int) y);
      int               val = (int) self->track->main_height_ - (int) y;
      int               resizing_track = val >= 0 && val < RESIZE_PX;
      AutomationTrack * resizing_at = get_at_to_resize (self, (int) y);
      TrackLane *       resizing_lane = get_lane_to_resize (self, (int) y);
      if (self->resizing)
        {
          self->resize = 1;
        }
      else if (!cb && !am && resizing_track)
        {
          self->resize = 1;
          self->resize_target_type =
            TrackWidgetResizeTarget::TRACK_WIDGET_RESIZE_TARGET_TRACK;
          self->resize_target = self->track;
          /*z_info ("RESIZING TRACK");*/
        }
      else if (!cb && !am && resizing_at)
        {
          self->resize = 1;
          self->resize_target_type =
            TrackWidgetResizeTarget::TRACK_WIDGET_RESIZE_TARGET_AT;
          self->resize_target = resizing_at;
          /*z_info (*/
          /*"RESIZING AT %s",*/
          /*resizing_at->automatable->label);*/
        }
      else if (!cb && !am && resizing_lane)
        {
          self->resize = 1;
          self->resize_target_type =
            TrackWidgetResizeTarget::TRACK_WIDGET_RESIZE_TARGET_LANE;
          self->resize_target = resizing_lane;
          /*z_info (*/
          /*"RESIZING LANE %d",*/
          /*resizing_lane->pos);*/
        }
      else
        {
          self->resize = 0;
        }

      if (self->resize == 1)
        {
          ui_set_cursor_from_name (GTK_WIDGET (self), "s-resize");
        }
      else
        {
          ui_set_pointer_cursor (GTK_WIDGET (self));
        }
    } /* endif hovered */

  /* set tooltips */
  CustomButtonWidget * hovered_btn =
    track_widget_get_hovered_button (self, (int) x, (int) y);
  set_tooltip_from_button (self, hovered_btn);

  /* set whether icon is covered */
  if (x >= 1 && y >= 1 && x < 18 && y < 18)
    {
      self->icon_hovered = true;
    }
  else
    {
      self->icon_hovered = false;
    }

  self->bg_hovered = true;

  self->last_hovered_btn =
    track_widget_get_hovered_button (self, (int) x, (int) y);
  self->last_x = x;
  self->last_y = y;
}

static bool
on_query_tooltip (
  GtkWidget *   widget,
  gint          x,
  gint          y,
  gboolean      keyboard_mode,
  GtkTooltip *  tooltip,
  TrackWidget * self)
{
  /* TODO set tooltip rect */
  gtk_tooltip_set_text (tooltip, self->tooltip_text);

  return true;
}

static void
get_visible_rect (TrackWidget * self, GdkRectangle * rect)
{
  rect->x = 0;
  rect->y = 0;
  rect->height = gtk_widget_get_height (GTK_WIDGET (self));
  rect->width = gtk_widget_get_width (GTK_WIDGET (self));
}

/**
 * Causes a redraw of the meters only.
 */
void
track_widget_redraw_meters (TrackWidget * self)
{
  GdkRectangle rect;
  get_visible_rect (self, &rect);

  if (gtk_widget_get_visible (GTK_WIDGET (self->meter_l)))
    {
      gtk_widget_queue_draw (GTK_WIDGET (self->meter_l));
    }
  if (gtk_widget_get_visible (GTK_WIDGET (self->meter_r)))
    {
      gtk_widget_queue_draw (GTK_WIDGET (self->meter_r));
    }
}

/**
 * Returns if cursor is in the range select "half".
 *
 * Used by timeline to determine if it will select
 * objects or range.
 */
bool
track_widget_is_cursor_in_range_select_half (TrackWidget * self, double y)
{
  graphene_point_t wpt;
  graphene_point_t tmp_pt = Z_GRAPHENE_POINT_INIT (0.f, (float) y);
  bool             success = gtk_widget_compute_point (
    GTK_WIDGET (MW_TIMELINE), GTK_WIDGET (self), &tmp_pt, &wpt);
  z_return_val_if_fail (success, false);

  /* if bot 1/3rd */
  if (
    wpt.y >= ((self->track->main_height_ * 2) / 3)
    && wpt.y <= self->track->main_height_)
    {
      return true;
    }
  /* else if top 2/3rds */
  else
    {
      return false;
    }
}

static TrackLane *
get_lane_at_y (TrackWidget * self, double y)
{
  auto track = dynamic_cast<LanedTrack *> (self->track);

  if (!track->lanes_visible_)
    return NULL;

  auto laned_track_variant = convert_to_variant<LanedTrackPtrVariant> (track);
  return std::visit (
    [&] (auto &&laned_t) {
      double height_before = laned_t->main_height_;
      for (auto &lane : laned_t->lanes_)
        {
          double next_height = height_before + lane->height_;
          if (y > height_before && y <= next_height)
            {
              z_debug ("found lane {} at y {:f}", lane->name_, y);
              return static_cast<TrackLane *> (lane.get ());
            }
          height_before = next_height;
        }

      z_debug ("no lane found at y {:f}", y);

      return static_cast<TrackLane *> (nullptr);
    },
    laned_track_variant);
}

#if 0
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
#endif

#if 0
static void
on_lane_rename (
  GtkMenuItem * menu_item,
  TrackLane *   lane)
{
  StringEntryDialogWidget * dialog =
    string_entry_dialog_widget_new (
      _("Marker name"), lane,
      (GenericStringGetter)
      track_lane_get_name,
      (GenericStringSetter)
      track_lane_rename_with_action);
  gtk_window_present (GTK_WINDOW (dialog));
}
#endif

static void
show_context_menu (TrackWidget * self, double x, double y)
{
  GMenu *     menu = g_menu_new ();
  GMenuItem * menuitem;

  Track *     track = self->track;
  TrackLane * lane = get_lane_at_y (self, y);

  AutomationTrack * at = track_widget_get_at_at_y (self, y);

  int num_selected = TRACKLIST_SELECTIONS->get_num_tracks ();

  if (num_selected > 0)
    {
      char * str;

      GMenu * edit_submenu = track->generate_edit_context_menu (num_selected);
      GMenuItem * edit_submenu_item =
        g_menu_item_new_section (nullptr, G_MENU_MODEL (edit_submenu));
      g_menu_item_set_attribute (
        edit_submenu_item, "display-hint", "s", "horizontal-buttons");
      g_menu_append_item (menu, edit_submenu_item);

      GMenu * select_submenu = g_menu_new ();

      str = g_strdup ("app.append-track-objects-to-selection");
      menuitem = z_gtk_create_menu_item (
        _ ("Append Track Objects to Selection"), nullptr, str);
      g_menu_item_set_action_and_target_value (
        menuitem, str, g_variant_new_int32 (track->pos_));
      g_free (str);
      g_menu_append_item (select_submenu, menuitem);

      if (lane)
        {
          str = g_strdup_printf (
            "app.append-lane-objects-to-selection((%d,%d))", track->pos_,
            lane->pos_);
          menuitem = z_gtk_create_menu_item (
            _ ("Append Lane Objects to Selection"), nullptr, str);
          g_free (str);
          g_menu_append_item (select_submenu, menuitem);
        }

      if (at)
        {
          str = g_strdup_printf (
            "app.append-lane-automation-regions-to-selection((%d,%d))",
            track->pos_, at->index_);
          menuitem = z_gtk_create_menu_item (
            _ ("Append Lane Automation Regions to Selection"), nullptr, str);
          g_free (str);
          g_menu_append_item (select_submenu, menuitem);
        }

      g_menu_append_section (menu, nullptr, G_MENU_MODEL (select_submenu));
    }

  if (track->out_signal_type_ == PortType::Audio)
    {
      GMenu * bounce_submenu = g_menu_new ();

      menuitem = z_gtk_create_menu_item (
        _ ("Quick bounce"), "file-music-line",
        "app.quick-bounce-selected-tracks");
      g_menu_append_item (bounce_submenu, menuitem);

      menuitem = z_gtk_create_menu_item (
        _ ("Bounce..."), "document-export", "app.bounce-selected-tracks");
      g_menu_append_item (bounce_submenu, menuitem);

      g_menu_append_section (menu, nullptr, G_MENU_MODEL (bounce_submenu));
    }

  /* add solo/mute/listen */
  if (track->has_channel () || track->is_folder ())
    {
      auto    channel_track = dynamic_cast<ChannelTrack *> (track);
      GMenu * channel_submenu = channel_track->generate_channel_context_menu ();
      GMenuItem * channel_submenu_item =
        g_menu_item_new_section (nullptr, G_MENU_MODEL (channel_submenu));
      g_menu_append_item (menu, channel_submenu_item);
    } /* endif track has channel */

  /* add enable/disable */
  if (TRACKLIST_SELECTIONS->contains_enabled_track (F_ENABLED))
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

#if 0
  menuitem =
    z_gtk_create_menu_item (
      _("Rename lane..."), "text-field",
      F_NO_TOGGLE, nullptr);
  g_signal_connect (
    G_OBJECT (menuitem), "activate",
    G_CALLBACK (on_lane_rename), lane);
  APPEND (menuitem);
#endif

  /* add midi channel selectors */
  if (track->has_piano_roll ())
    {
      GMenu * piano_roll_section = g_menu_new ();

      GMenu * track_midi_ch_submenu = g_menu_new ();

      menuitem = z_gtk_create_menu_item (
        _ ("Passthrough input"), nullptr, "app.toggle-track-passthrough-input");
      g_menu_append_item (track_midi_ch_submenu, menuitem);

      /* add each MIDI ch */
      for (int i = 1; i <= 16; i++)
        {
          char lbl[600];
          char action[600];
          sprintf (lbl, _ ("MIDI Channel %d"), i);
          sprintf (
            action, "app.track-set-midi-channel::%d,%d,%d", track->pos_, -1, i);
          menuitem = z_gtk_create_menu_item (lbl, nullptr, action);
          g_menu_append_item (track_midi_ch_submenu, menuitem);
        }

      g_menu_append_submenu (
        piano_roll_section, _ ("Track MIDI Channel"),
        G_MENU_MODEL (track_midi_ch_submenu));

      if (lane)
        {
          GMenu * lane_midi_ch_submenu = g_menu_new ();

          for (int i = 0; i <= 16; i++)
            {
              char lbl[600];
              if (i == 0)
                strcpy (lbl, _ ("Inherit"));
              else
                sprintf (lbl, _ ("MIDI Channel %d"), i);

              char action[600];
              sprintf (
                action, "app.track-set-midi-channel::%d,%d,%d", track->pos_,
                lane->pos_, i);
              menuitem = z_gtk_create_menu_item (lbl, nullptr, action);
              g_menu_append_item (lane_midi_ch_submenu, menuitem);
            }

          g_menu_append_submenu (
            piano_roll_section, _ ("Lane MIDI Channel"),
            G_MENU_MODEL (lane_midi_ch_submenu));
        }

      g_menu_append_section (menu, nullptr, G_MENU_MODEL (piano_roll_section));
    }

  if (
    num_selected > 0 && !TRACKLIST_SELECTIONS->contains_non_automatable_track ())
    {
      GMenu * automation_section = g_menu_new ();

      menuitem = z_gtk_create_menu_item (
        _ ("Show Used Automation Lanes"), TRACK_ICON_NAME_SHOW_AUTOMATION_LANES,
        "app.show-used-automation-lanes-on-selected-tracks");
      g_menu_append_item (automation_section, menuitem);
      menuitem = z_gtk_create_menu_item (
        _ ("Hide Unused Automation Lanes"),
        TRACK_ICON_NAME_SHOW_AUTOMATION_LANES,
        "app.hide-unused-automation-lanes-on-selected-tracks");
      g_menu_append_item (automation_section, menuitem);

      g_menu_append_section (menu, nullptr, G_MENU_MODEL (automation_section));
    }

  z_gtk_show_context_menu_from_g_menu (self->popover_menu, x, y, menu);
  if (self->fader_buttons_for_popover)
    {
      gtk_popover_menu_add_child (
        self->popover_menu, GTK_WIDGET (self->fader_buttons_for_popover),
        "fader-buttons");
    }
}

static void
on_right_click_pressed (
  GtkGestureClick * gesture,
  gint              n_press,
  gdouble           x,
  gdouble           y,
  TrackWidget *     self)
{
  GdkModifierType state = gtk_event_controller_get_current_event_state (
    GTK_EVENT_CONTROLLER (gesture));

  Track * track = self->track;
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

/**
 * Shows a popover for editing the track name.
 */
static void
show_edit_name_popover (TrackWidget * self, TrackLane * lane)
{
  if (lane)
    {
      std::visit (
        [&] (const auto &l) {
          editable_label_widget_show_popover_for_widget (
            GTK_WIDGET (self), self->track_name_popover, l,
            bind_member_function (*l, &TrackLane::get_name),
            bind_member_function (
              *l, &base_type<decltype (l)>::rename_with_action));
        },
        convert_to_variant<TrackLanePtrVariant> (lane));
    }
  else
    {
      editable_label_widget_show_popover_for_widget (
        GTK_WIDGET (self), self->track_name_popover, self->track,
        bind_member_function (*self->track, &Track::get_name),
        bind_member_function (*self->track, &Track::set_name_with_action));
    }
}

static void
click_pressed (
  GtkGestureClick * gesture,
  gint              n_press,
  gdouble           x,
  gdouble           y,
  TrackWidget *     self)
{
  Track * track = self->track;
  self->was_armed =
    track->can_record ()
    && dynamic_cast<RecordableTrack *> (track)->get_recording ();

  gtk_widget_grab_focus (GTK_WIDGET (self));

  CustomButtonWidget * cb =
    track_widget_get_hovered_button (self, (int) x, (int) y);
  AutomationModeWidget * am =
    track_widget_get_hovered_am_widget (self, (int) x, (int) y);
  if (cb || am)
    {
      self->button_pressed = 1;
      self->clicked_button = cb;
      self->clicked_am = am;
    }
  else
    {
      PROJECT->last_selection_ = Project::SelectionType::Tracklist;
      EVENTS_PUSH (EventType::ET_PROJECT_SELECTION_TYPE_CHANGED, nullptr);
    }

  if (self->icon_hovered)
    {
      TrackIconChooserDialogWidget * icon_chooser =
        track_icon_chooser_dialog_widget_new (self->track);
      track_icon_chooser_dialog_widget_run (icon_chooser);
    }
  else if (self->color_area_hovered)
    {
      object_color_chooser_dialog_widget_run (
        GTK_WINDOW (MAIN_WINDOW), self->track, nullptr, nullptr);
    }
}

static void
click_released (
  GtkGestureClick * gesture,
  gint              n_press,
  gdouble           x,
  gdouble           y,
  TrackWidget *     self)
{
  Track * track = self->track;

  GdkModifierType state = gtk_event_controller_get_current_event_state (
    GTK_EVENT_CONTROLLER (gesture));
  bool ctrl = state & GDK_CONTROL_MASK;
  bool shift = state & GDK_SHIFT_MASK;
  TRACKLIST_SELECTIONS->handle_click (*track, ctrl, shift, self->dragged);

  if (self->clicked_button)
    {
      CustomButtonWidget * cb = self->clicked_button;

      /* if track not selected, select it */
      if (!track->is_selected ())
        {
          track->select (true, true, true);
        }

      if ((Track *) cb->owner == track)
        {
          auto ch_track = dynamic_cast<ChannelTrack *> (track);
          if (TRACK_CB_ICON_IS (MONO_COMPAT))
            {
              ch_track->channel_->set_mono_compat_enabled (
                !ch_track->channel_->get_mono_compat_enabled (), true);
            }
          else if (TRACK_CB_ICON_IS (SWAP_PHASE))
            {
              ch_track->channel_->set_swap_phase (
                !ch_track->channel_->get_swap_phase (), true);
            }
          else if (TRACK_CB_ICON_IS (RECORD))
            {
              track_widget_on_record_toggled (self);
            }
          else if (TRACK_CB_ICON_IS (SOLO))
            {
              ch_track->set_soloed (
                !ch_track->get_soloed (), F_TRIGGER_UNDO, F_AUTO_SELECT, true);
            }
          else if (TRACK_CB_ICON_IS (MUTE))
            {
              ch_track->set_muted (
                !ch_track->get_muted (), F_TRIGGER_UNDO, F_AUTO_SELECT, true);
            }
          else if (TRACK_CB_ICON_IS (LISTEN))
            {
              ch_track->set_listened (
                !ch_track->get_listened (), F_TRIGGER_UNDO, F_AUTO_SELECT, true);
            }
          else if (TRACK_CB_ICON_IS (MONITOR_AUDIO))
            {
              auto ptrack = dynamic_cast<ProcessableTrack *> (track);
              ptrack->set_monitor_audio (
                !ptrack->get_monitor_audio (), F_AUTO_SELECT, true);
            }
          else if (TRACK_CB_ICON_IS (SHOW_TRACK_LANES))
            {
              track_widget_on_show_lanes_toggled (self);
            }
          else if (TRACK_CB_ICON_IS (SHOW_UI))
            {
              auto ins_track = dynamic_cast<InstrumentTrack *> (track);
              ins_track->toggle_plugin_visible ();
            }
          else if (TRACK_CB_ICON_IS (SHOW_AUTOMATION_LANES))
            {
              track_widget_on_show_automation_toggled (self);
            }
          else if (TRACK_CB_ICON_IS (FOLD_OPEN) || TRACK_CB_ICON_IS (FOLD))
            {
              auto foldable_tr = dynamic_cast<FoldableTrack *> (track);
              foldable_tr->set_folded (
                !foldable_tr->folded_, F_TRIGGER_UNDO, F_AUTO_SELECT, true);
            }
          else if (TRACK_CB_ICON_IS (FREEZE))
            {
#if 0
              track_freeze (
                track, !track->frozen);
#endif
            }
        }
      else if (cb->owner_type == CustomButtonWidget::Owner::LANE)
        {
          TrackLane * lane = (TrackLane *) cb->owner;

          if (TRACK_CB_ICON_IS (SOLO))
            {
              std::visit (
                [&] (auto &&l) {
                  l->set_soloed (!l->get_soloed (), F_TRIGGER_UNDO, true);
                },
                convert_to_variant<TrackLanePtrVariant> (lane));
            }
          else if (TRACK_CB_ICON_IS (MUTE))
            {
              std::visit (
                [&] (auto &&l) {
                  l->set_muted (!l->get_muted (), F_TRIGGER_UNDO, true);
                },
                convert_to_variant<TrackLanePtrVariant> (lane));
            }
        }
      else if (cb->owner_type == CustomButtonWidget::Owner::AT)
        {
          auto * at = (AutomationTrack *) cb->owner;
          z_return_if_fail (at);
          AutomationTracklist * atl = at->get_automation_tracklist ();
          z_return_if_fail (atl);

          if (TRACK_CB_ICON_IS (PLUS))
            {
              AutomationTrack * new_at = atl->get_first_invisible_at ();

              /* if any invisible at exists, show it */
              if (new_at)
                {
                  if (!new_at->created_)
                    new_at->created_ = true;
                  atl->set_at_visible (*new_at, true);

                  /* move it after the clicked automation track */
                  atl->set_at_index (*new_at, at->index_ + 1, true);

                  EVENTS_PUSH (EventType::ET_AUTOMATION_TRACK_ADDED, new_at);
                }
            }
          else if (TRACK_CB_ICON_IS (MINUS))
            {
              /* don't allow deleting if no other visible automation tracks */
              if (atl->visible_ats_.size () > 1)
                {
                  atl->set_at_visible (*at, false);
                  EVENTS_PUSH (EventType::ET_AUTOMATION_TRACK_REMOVED, at);
                }
            }
          else if (TRACK_CB_ICON_IS (SHOW_AUTOMATION_LANES))
            {
              AutomatableSelectorPopoverWidget * popover =
                automatable_selector_popover_widget_new (at);
              GdkRectangle rect = {
                .x = (int) cb->x + (int) cb->width / 2,
                .y = (int) cb->y,
                .width = 0,
                .height = cb->size,
              };
              gtk_widget_set_parent (GTK_WIDGET (popover), GTK_WIDGET (self));
              gtk_popover_set_pointing_to (GTK_POPOVER (popover), &rect);
              gtk_popover_popup (GTK_POPOVER (popover));
              z_debug ("popping up automatable selector popover");
            }
        }
    }
  else if (self->clicked_am)
    {
      AutomationModeWidget * am = self->clicked_am;
      auto *                 at = (AutomationTrack *) am->owner_;
      z_return_if_fail (at);
      AutomationTracklist * atl = at->get_automation_tracklist ();
      z_return_if_fail (atl);
      if (
        at->automation_mode_ == AutomationMode::Record
        && am->hit_mode_ == AutomationMode::Record)
        {
          at->swap_record_mode ();
          am->init ();
        }
      at->set_automation_mode (am->hit_mode_, true);
    }
  else if (n_press == 2)
    {
      /* if pressed closed to the name, attempt rename,
       * otherwise just show the track info in the inspector */
      if (x < 60 && y < 17)
        {
          TrackLane * lane = get_lane_at_y (self, y);

          /* show popup to edit the name */
          show_edit_name_popover (self, lane);
        }
      else
        {
          left_dock_edge_widget_refresh_with_page (
            MW_LEFT_DOCK_EDGE, LeftDockEdgeTab::LEFT_DOCK_EDGE_TAB_TRACK);
        }
    }

  z_debug ("npress {}", n_press);

  self->button_pressed = 0;
  self->clicked_button = NULL;
  self->clicked_am = NULL;
}

static void
on_drag_begin (
  GtkGestureDrag * gesture,
  gdouble          start_x,
  gdouble          start_y,
  TrackWidget *    self)
{
  self->selected_in_dnd = 0;
  self->dragged = 0;

  if (self->resize)
    {
      /* start resizing */
      self->resizing = 1;
    }
  else if (self->button_pressed)
    {
      gtk_event_controller_reset (GTK_EVENT_CONTROLLER (gesture));
      /* if one of the buttons is pressed, ignore */
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

          if (TRACKLIST_SELECTIONS->contains_track (*track))
            {
              selected = 1;
            }

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

  self->start_x = start_x;
  self->start_y = start_y;
}

static void
on_drag_update (
  GtkGestureDrag * gesture,
  gdouble          offset_x,
  gdouble          offset_y,
  TrackWidget *    self)
{
  z_info ("drag_update");

  self->dragged = 1;

  Track * track = self->track;

  GdkModifierType state = gtk_event_controller_get_current_event_state (
    GTK_EVENT_CONTROLLER (gesture));
  bool ctrl = state & GDK_CONTROL_MASK;
  bool shift = state & GDK_SHIFT_MASK;
  TRACKLIST_SELECTIONS->handle_click (*track, ctrl, shift, self->dragged);

  if (self->resizing)
    {
      /* resize */
      int diff = (int) (offset_y - self->last_offset_y);

      switch (self->resize_target_type)
        {
        case TrackWidgetResizeTarget::TRACK_WIDGET_RESIZE_TARGET_TRACK:
          track->main_height_ =
            MAX (TRACK_MIN_HEIGHT, track->main_height_ + diff);
          break;
        case TrackWidgetResizeTarget::TRACK_WIDGET_RESIZE_TARGET_AT:
          {
            auto * at = (AutomationTrack *) self->resize_target;
            at->height_ = MAX (TRACK_MIN_HEIGHT, at->height_ + diff);
          }
          break;
        case TrackWidgetResizeTarget::TRACK_WIDGET_RESIZE_TARGET_LANE:
          {
            auto * lane = (TrackLane *) self->resize_target;
            lane->height_ = MAX (TRACK_MIN_HEIGHT, lane->height_ + diff);
            z_debug ("lane {} height changed", lane->pos_);
          }
          break;
        }
      /* FIXME should be event */
      track_widget_update_size (self);
    }
  else
    {
      int drag_threshold =
        zrythm_app->default_settings_->property_gtk_dnd_drag_threshold ()
          .get_value ();
      if (fabs (offset_x) > drag_threshold || fabs (offset_y) > drag_threshold)
        {
          std::visit (
            [&] (auto &&_track) {
              /* start dnd */
              WrappedObjectWithChangeSignal * wrapped_track =
                wrapped_object_with_change_signal_new (
                  _track, WrappedObjectType::WRAPPED_OBJECT_TYPE_TRACK);
              GdkContentProvider * content_providers[] = {
                gdk_content_provider_new_typed (
                  WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE, wrapped_track),
              };
              GdkContentProvider * provider = gdk_content_provider_new_union (
                content_providers, G_N_ELEMENTS (content_providers));

              GtkNative * native = gtk_widget_get_native (GTK_WIDGET (self));
              z_return_if_fail (native);
              GdkSurface * surface = gtk_native_get_surface (native);

              GdkDevice * device =
                gtk_gesture_get_device (GTK_GESTURE (gesture));

              /* begin drag */
              gdk_drag_begin (
                surface, device, provider, GDK_ACTION_MOVE | GDK_ACTION_COPY,
                offset_x, offset_y);
            },
            convert_to_variant<TrackPtrVariant> (self->track));
        }
    }

  self->last_offset_y = offset_y;
}

static void
on_drag_end (
  GtkGestureDrag * gesture,
  gdouble          offset_x,
  gdouble          offset_y,
  TrackWidget *    self)
{
  self->resizing = 0;
  self->last_offset_y = 0;
}

#if 0
static void
on_dnd_drag_begin (
  GtkWidget *      widget,
  GdkDragContext * context,
  TrackWidget *    self)
{
  gtk_drag_set_icon_name (
    context, "track-inspector", 0, 0);
}
#endif

/**
 * Returns the highlight location based on y
 * relative to @ref self.
 */
TrackWidgetHighlight
track_widget_get_highlight_location (TrackWidget * self, int y)
{
  Track * track = self->track;
  int     h = gtk_widget_get_height (GTK_WIDGET (self));
  if (track->is_foldable () && y < h - 12 && y > 12)
    {
      return TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_INSIDE;
    }
  else if (y < h / 2)
    {
      return TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_TOP;
    }
  else
    {
      return TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_BOTTOM;
    }
}

/**
 * Highlights/unhighlights the Tracks
 * appropriately.
 *
 * @param highlight 1 to highlight top or bottom,
 *   0 to unhighlight all.
 */
void
track_widget_do_highlight (TrackWidget * self, gint x, gint y, const int highlight)
{
  if (highlight)
    {
      TrackWidgetHighlight location =
        track_widget_get_highlight_location (self, y);
      self->highlight_loc = location;
      if (location == TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_INSIDE)
        {
          /* unhighlight top */
          gtk_widget_set_size_request (
            GTK_WIDGET (self->highlight_top_box), -1, -1);

          /* unhighlight bot */
          gtk_widget_set_size_request (
            GTK_WIDGET (self->highlight_bot_box), -1, -1);
        }
      else if (location == TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_TOP)
        {
          /* highlight top */
          gtk_widget_set_size_request (
            GTK_WIDGET (self->highlight_top_box), -1, 2);

          /* unhighlight bot */
          gtk_widget_set_size_request (
            GTK_WIDGET (self->highlight_bot_box), -1, -1);
        }
      else
        {
          /* highlight bot */
          gtk_widget_set_size_request (
            GTK_WIDGET (self->highlight_bot_box), -1, 2);

          /* unhighlight top */
          gtk_widget_set_size_request (
            GTK_WIDGET (self->highlight_top_box), -1, -1);
        }
    }
  else
    {
      /* unhighlight all */
      gtk_widget_set_size_request (GTK_WIDGET (self->highlight_top_box), -1, -1);
      gtk_widget_set_size_request (GTK_WIDGET (self->highlight_bot_box), -1, -1);
      self->highlight_loc = TrackWidgetHighlight::TRACK_WIDGET_HIGHLIGHT_NONE;
    }
}

/**
 * Converts Y from the arranger coordinates to
 * the track coordinates.
 */
int
track_widget_get_local_y (
  TrackWidget *    self,
  ArrangerWidget * arranger,
  int              arranger_y)
{
  graphene_point_t local;
  graphene_point_t tmp_pt = Z_GRAPHENE_POINT_INIT (0.f, (float) arranger_y);
  bool             success = gtk_widget_compute_point (
    arranger->is_pinned
      ? GTK_WIDGET (arranger)
      : GTK_WIDGET (MW_TRACKLIST->unpinned_box),
    GTK_WIDGET (self), &tmp_pt, &local);
  z_return_val_if_fail (success, -1);

  return (int) local.y;
}

/**
 * Callback when lanes button is toggled.
 */
void
track_widget_on_show_lanes_toggled (TrackWidget * self)
{
  auto track = dynamic_cast<LanedTrack *> (self->track);
  if (!track)
    return;

  /* set visibility flag */
  track->set_lanes_visible (!track->lanes_visible_);
}

void
track_widget_on_show_automation_toggled (TrackWidget * self)
{
  auto track = dynamic_cast<AutomatableTrack *> (self->track);
  if (!track)
    return;

  z_debug ("toggled on {}", track->name_);

  /* set visibility flag */
  track->set_automation_visible (!track->automation_visible_);

  if (track->is_tempo () && track->automation_visible_)
    {
      ui_show_warning_for_tempo_track_experimental_feature ();
    }
}

void
track_widget_on_record_toggled (TrackWidget * self)
{
  auto track = dynamic_cast<RecordableTrack *> (self->track);
  z_return_if_fail (track);

  /* toggle record flag */
  track->set_recording (!self->was_armed, true);
  track->record_set_automatically_ = false;
  z_debug ("{} recording: {}", track->name_, track->get_recording ());

  EVENTS_PUSH (EventType::ET_TRACK_STATE_CHANGED, track);
}

/**
 * Re-fills TrackWidget.group_colors_box.
 */
void
track_widget_recreate_group_colors (TrackWidget * self)
{
  auto track = self->track;

  std::vector<FoldableTrack *> array;
  track->add_folder_parents (array, false);

  /* remove existing group colors */
  z_gtk_widget_destroy_all_children (GTK_WIDGET (self->group_colors_box));

  for (auto &parent_track : array)
    {
      ColorAreaWidget * ca = Z_COLOR_AREA_WIDGET (
        g_object_new (COLOR_AREA_WIDGET_TYPE, "visible", true, nullptr));
      color_area_widget_setup_generic (ca, &parent_track->color_);
      gtk_widget_set_size_request (GTK_WIDGET (ca), 8, -1);
      gtk_box_append (GTK_BOX (self->group_colors_box), GTK_WIDGET (ca));
    }
}

/**
 * Add a button.
 *
 * @param top 1 for top, 0 for bottom.
 */
static auto &
add_button (TrackWidget * self, bool top, const char * icon_name)
{
  auto &vec = top ? self->top_buttons : self->bot_buttons;
  vec.emplace_back (
    std::make_unique<CustomButtonWidget> (icon_name, TRACK_BUTTON_SIZE));
  auto &cb = vec.back ();
  cb->owner_type = CustomButtonWidget::Owner::TRACK;
  cb->owner = self->track;
  return cb;
}

static auto &
add_solo_button (TrackWidget * self, int top)
{
  auto &cb = add_button (self, top, TRACK_ICON_NAME_SOLO);
  cb->toggled_color = UI_COLORS->solo_checked;
  cb->held_color = UI_COLORS->solo_active;

  return cb;
}

static auto &
add_record_button (TrackWidget * self, int top)
{
  auto &cb = add_button (self, top, TRACK_ICON_NAME_RECORD);
  cb->toggled_color = Color (UI_COLOR_RECORD_CHECKED);
  cb->held_color = Color (UI_COLOR_RECORD_ACTIVE);

  return cb;
}

/**
 * Updates the full track size and redraws the
 * track.
 */
void
track_widget_update_size (TrackWidget * self)
{
  z_return_if_fail (self->track);
  int height = (int) self->track->get_full_visible_height ();
  z_return_if_fail (height > 1);
  int w;
  gtk_widget_get_size_request ((GtkWidget *) self, &w, nullptr);
  gtk_widget_set_size_request (GTK_WIDGET (self), w, height);
}

void
track_widget_update_icons (TrackWidget * self)
{
  auto get_new_fold_button = [] (FoldableTrack * track) {
    auto new_icon_name =
      track->folded_ ? TRACK_ICON_NAME_FOLD : TRACK_ICON_NAME_FOLD_OPEN;
    auto cb =
      std::make_unique<CustomButtonWidget> (new_icon_name, TRACK_BUTTON_SIZE);
    cb->owner_type = CustomButtonWidget::Owner::TRACK;
    cb->owner = track;
    return cb;
  };

  for (auto &cb : self->bot_buttons)
    {
      if (TRACK_CB_ICON_IS (FOLD_OPEN) || TRACK_CB_ICON_IS (FOLD))
        {
          auto foldable_track = dynamic_cast<FoldableTrack *> (self->track);
          cb = get_new_fold_button (foldable_track);
        }
    }
}

static gboolean
track_tick_cb (GtkWidget * widget, GdkFrameClock * frame_clock, TrackWidget * self)
{
  if (!gtk_widget_get_mapped (GTK_WIDGET (self->canvas)))
    {
      return G_SOURCE_CONTINUE;
    }

  track_widget_redraw_meters (self);
  gtk_widget_queue_draw (GTK_WIDGET (self->canvas));

  return G_SOURCE_CONTINUE;
}

#if 0
static gboolean
on_dnd_drop (
  GtkDropTarget * drop_target,
  const GValue *  value,
  double          x,
  double          y,
  gpointer        data)
{
  TrackWidget * self = Z_TRACK_WIDGET (data);
  Track *       track = self->track;
  z_return_val_if_fail (IS_TRACK_AND_NONNULL (track), false);

  GdkDragAction action =
    z_gtk_drop_target_get_selected_action (drop_target);

  FileDescriptor *    file = NULL;
  PluginDescriptor * pd = NULL;
  Plugin *           pl = NULL;
  if (G_VALUE_HOLDS (
        value, WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE))
    {
      WrappedObjectWithChangeSignal * wrapped_obj =
        g_value_get_object (value);
      if (wrapped_obj->type == WrappedObjectType::WRAPPED_OBJECT_TYPE_SUPPORTED_FILE)
        {
          file = (FileDescriptor *) wrapped_obj->obj;
        }
      else if (wrapped_obj->type == WrappedObjectType::WRAPPED_OBJECT_TYPE_PLUGIN)
        {
          pl = (Plugin *) wrapped_obj->obj;
        }
      else if (
        wrapped_obj->type == WrappedObjectType::WRAPPED_OBJECT_TYPE_PLUGIN_DESCR)
        {
          pd = (PluginDescriptor *) wrapped_obj->obj;
        }
    }

  if (
    G_VALUE_HOLDS (value, GDK_TYPE_FILE_LIST)
    || G_VALUE_HOLDS (value, G_TYPE_FILE) || file)
    {
      char ** uris = NULL;
      if (G_VALUE_HOLDS (value, G_TYPE_FILE))
        {
          GFile *       gfile = g_value_get_object (value);
          StrvBuilder * uris_builder = strv_builder_new ();
          char *        uri = g_file_get_uri (gfile);
          strv_builder_add (uris_builder, uri);
          uris = strv_builder_end (uris_builder);
        }
      else if (G_VALUE_HOLDS (value, GDK_TYPE_FILE_LIST))
        {
          StrvBuilder * uris_builder = strv_builder_new ();
          GSList *      l;
          for (l = g_value_get_boxed (value); l; l = l->next)
            {
              char * uri = g_file_get_uri (l->data);
              strv_builder_add (uris_builder, uri);
              g_free (uri);
            }
          uris = strv_builder_end (uris_builder);
        }

      /* TODO import files on track */
      (void) uris;

      return false;
    }
  else if (pd)
    {
      PluginSetting * setting =
        plugin_setting_new_default (pd);

      /* TODO add the plugin to the track */
      switch (track->type)
        {
        case Track::Type::MIDI:
          if (plugin_descriptor_is_instrument (pd))
            {
              ui_show_error_message (
                MAIN_WINDOW, false,
                _("Operation not implemented"));
            }

          break;
        default:
          break;
        }

      plugin_setting_free (setting);

      return false;
    }
  else if (pl)
    {
      /* NOTE this is a cloned pointer, don't use
       * it */
      z_warn_if_fail (pl);

      GError * err = NULL;
      bool     ret = false;
      if (action == GDK_ACTION_COPY)
        {
          /* TODO */
          return false;
        }
      else if (action == GDK_ACTION_MOVE)
        {
          /* TODO */
          return false;
        }
      else
        z_return_val_if_reached (true);

      if (!ret)
        {
          HANDLE_ERROR (
            err, "%s", _ ("Failed to move or copy plugin"));
        }

      return true;
    }

  /* drag was not accepted */
  return false;
}

static GdkDragAction
on_dnd_motion (
  GtkDropTarget * drop_target,
  gdouble         x,
  gdouble         y,
  gpointer        user_data)
{
  /* request value */
  const GValue * value =
    gtk_drop_target_get_value (drop_target);
  if (G_VALUE_HOLDS (
        value, WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE))
    {
      WrappedObjectWithChangeSignal * wrapped_obj =
        g_value_get_object (value);
      switch (wrapped_obj->type)
        {
        case WrappedObjectType::WRAPPED_OBJECT_TYPE_SUPPORTED_FILE:
        case WrappedObjectType::WRAPPED_OBJECT_TYPE_PLUGIN:
        case WrappedObjectType::WRAPPED_OBJECT_TYPE_PLUGIN_DESCR:
          return GDK_ACTION_MOVE;
        default:
          return 0;
        }
    }

  return 0;
}

static void
setup_dnd (TrackWidget * self)
{
  GtkDropTarget * drop_target = gtk_drop_target_new (
    G_TYPE_INVALID, GDK_ACTION_COPY | GDK_ACTION_MOVE);
  GType types[] = {
    GDK_TYPE_FILE_LIST, G_TYPE_FILE,
    WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE
  };
  gtk_drop_target_set_gtypes (
    drop_target, types, G_N_ELEMENTS (types));
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (drop_target));

  /* connect signal */
  g_signal_connect (
    drop_target, "drop", G_CALLBACK (on_dnd_drop), self);
  g_signal_connect (
    drop_target, "motion", G_CALLBACK (on_dnd_motion), self);
}
#endif

static void
track_widget_key_released (
  GtkEventControllerKey * key_controller,
  guint                   keyval,
  guint                   keycode,
  GdkModifierType         state,
  TrackWidget *           self)
{
  if (z_gtk_keyval_is_menu (keyval))
    {
      show_context_menu (self, 0, 0);
    }
}

TrackWidget *
track_widget_new (Track * track)
{
  z_return_val_if_fail (track, nullptr);
  z_debug ("creating new track widget for {}", track->name_);

  auto * self =
    static_cast<TrackWidget *> (g_object_new (TRACK_WIDGET_TYPE, nullptr));

  self->track = track;
  auto foldable_track = dynamic_cast<FoldableTrack *> (track);

  switch (track->type_)
    {
    case Track::Type::Instrument:
      add_record_button (self, 1);
      add_solo_button (self, 1);
      add_button (self, true, TRACK_ICON_NAME_MUTE);
      add_button (self, true, TRACK_ICON_NAME_LISTEN);
      add_button (self, true, TRACK_ICON_NAME_SHOW_UI);
      add_button (self, false, TRACK_ICON_NAME_SWAP_PHASE);
#if 0
      add_button (self, false, TRACK_ICON_NAME_LOCK);
      add_button (self, false, TRACK_ICON_NAME_FREEZE);
#endif
      add_button (self, false, TRACK_ICON_NAME_SHOW_TRACK_LANES);
      add_button (self, false, TRACK_ICON_NAME_SHOW_AUTOMATION_LANES);
      break;
    case Track::Type::Midi:
      add_record_button (self, 1);
      add_solo_button (self, 1);
      add_button (self, 1, TRACK_ICON_NAME_MUTE);
#if 0
      add_button (self, 0, TRACK_ICON_NAME_LOCK);
#endif
      add_button (self, 0, TRACK_ICON_NAME_SHOW_TRACK_LANES);
      add_button (self, 0, TRACK_ICON_NAME_SHOW_AUTOMATION_LANES);
      break;
    case Track::Type::Master:
      add_button (self, true, TRACK_ICON_NAME_MONO_COMPAT);
      add_solo_button (self, 1);
      add_button (self, true, TRACK_ICON_NAME_MUTE);
      add_button (self, false, TRACK_ICON_NAME_SHOW_AUTOMATION_LANES);
      break;
    case Track::Type::Chord:
      add_record_button (self, 1);
      add_solo_button (self, 1);
      add_button (self, true, TRACK_ICON_NAME_MUTE);
      break;
    case Track::Type::Marker:
      break;
    case Track::Type::Tempo:
      add_button (self, true, TRACK_ICON_NAME_SHOW_AUTOMATION_LANES);
      break;
    case Track::Type::Modulator:
      add_button (self, true, TRACK_ICON_NAME_SHOW_AUTOMATION_LANES);
      break;
    case Track::Type::AudioBus:
      add_solo_button (self, 1);
      add_button (self, 1, TRACK_ICON_NAME_MUTE);
      add_button (self, true, TRACK_ICON_NAME_LISTEN);
      add_button (self, true, TRACK_ICON_NAME_SWAP_PHASE);
      add_button (self, 0, TRACK_ICON_NAME_SHOW_AUTOMATION_LANES);
      break;
    case Track::Type::MidiBus:
      add_solo_button (self, 1);
      add_button (self, 1, TRACK_ICON_NAME_MUTE);
      add_button (self, 0, TRACK_ICON_NAME_SHOW_AUTOMATION_LANES);
      break;
    case Track::Type::AudioGroup:
      add_button (self, 1, TRACK_ICON_NAME_MONO_COMPAT);
      add_solo_button (self, 1);
      add_button (self, 1, TRACK_ICON_NAME_MUTE);
      add_button (self, true, TRACK_ICON_NAME_LISTEN);
      add_button (self, true, TRACK_ICON_NAME_SWAP_PHASE);
      add_button (self, 0, TRACK_ICON_NAME_SHOW_AUTOMATION_LANES);
      add_button (
        self, false,
        foldable_track->folded_
          ? TRACK_ICON_NAME_FOLD
          : TRACK_ICON_NAME_FOLD_OPEN);
      break;
    case Track::Type::MidiGroup:
      add_solo_button (self, 1);
      add_button (self, 1, TRACK_ICON_NAME_MUTE);
      add_button (self, 0, TRACK_ICON_NAME_SHOW_AUTOMATION_LANES);
      add_button (
        self, false,
        foldable_track->folded_
          ? TRACK_ICON_NAME_FOLD
          : TRACK_ICON_NAME_FOLD_OPEN);
      break;
    case Track::Type::Audio:
      add_record_button (self, 1);
      add_solo_button (self, 1);
      add_button (self, 1, TRACK_ICON_NAME_MUTE);
      add_button (self, true, TRACK_ICON_NAME_LISTEN);
      add_button (self, true, TRACK_ICON_NAME_SWAP_PHASE);
      add_button (self, 0, TRACK_ICON_NAME_MONITOR_AUDIO);
#if 0
      add_button (self, 0, TRACK_ICON_NAME_LOCK);
#endif
      add_button (self, 0, TRACK_ICON_NAME_SHOW_TRACK_LANES);
      add_button (self, 0, TRACK_ICON_NAME_SHOW_AUTOMATION_LANES);
      break;
    case Track::Type::Folder:
      add_solo_button (self, 1);
      add_button (self, true, TRACK_ICON_NAME_MUTE);
      add_button (self, true, TRACK_ICON_NAME_LISTEN);
      add_button (
        self, false,
        foldable_track->folded_
          ? TRACK_ICON_NAME_FOLD
          : TRACK_ICON_NAME_FOLD_OPEN);
      break;
    }

  if (track->has_channel ())
    {
      auto ch_track = dynamic_cast<ChannelTrack *> (track);
      self->fader_buttons_for_popover =
        g_object_ref (fader_buttons_widget_new (ch_track));

      switch (track->out_signal_type_)
        {
        case PortType::Event:
          meter_widget_setup (
            self->meter_l, ch_track->channel_->midi_out_.get (), true);
          gtk_widget_set_margin_start (GTK_WIDGET (self->meter_l), 2);
          gtk_widget_set_margin_end (GTK_WIDGET (self->meter_l), 2);
          gtk_widget_set_visible (GTK_WIDGET (self->meter_r), 0);
          break;
        case PortType::Audio:
          meter_widget_setup (
            self->meter_l, &ch_track->channel_->stereo_out_->get_l (), true);
          meter_widget_setup (
            self->meter_r, &ch_track->channel_->stereo_out_->get_r (), true);
          break;
        default:
          break;
        }
    }
  else
    {
      gtk_widget_set_visible (GTK_WIDGET (self->meter_l), 0);
      gtk_widget_set_visible (GTK_WIDGET (self->meter_r), 0);
    }

  track_widget_recreate_group_colors (self);

  track_widget_update_size (self);

  gtk_widget_add_tick_callback (
    GTK_WIDGET (self), (GtkTickCallback) track_tick_cb, self, nullptr);

  track_canvas_widget_setup (self->canvas, self);

  /*setup_dnd (self);*/

  gtk_accessible_update_property (
    GTK_ACCESSIBLE (self), GTK_ACCESSIBLE_PROPERTY_LABEL, track->name_.c_str (),
    -1);

  return self;
}

static void
on_destroy (TrackWidget * self)
{
  Track * track = self->track;
  if (IS_TRACK_AND_NONNULL (track))
    {
      z_debug ("destroying '{}' widget", track->name_);

      if (track->widget_ == self)
        track->widget_ = nullptr;
    }
}

static void
dispose (TrackWidget * self)
{
  gtk_widget_unparent (GTK_WIDGET (self->popover_menu));
  gtk_widget_unparent (GTK_WIDGET (self->track_name_popover));
  gtk_widget_unparent (GTK_WIDGET (self->highlight_top_box));
  gtk_widget_unparent (GTK_WIDGET (self->main_box));
  gtk_widget_unparent (GTK_WIDGET (self->highlight_bot_box));

  G_OBJECT_CLASS (track_widget_parent_class)->dispose (G_OBJECT (self));
}

static void
track_widget_finalize (TrackWidget * self)
{
  std::destroy_at (&self->top_buttons);
  std::destroy_at (&self->bot_buttons);
  std::destroy_at (&self->last_midi_out_trigger_time);

  G_OBJECT_CLASS (track_widget_parent_class)->finalize (G_OBJECT (self));
}

static void
track_widget_init (TrackWidget * self)
{
  std::construct_at (&self->top_buttons);
  std::construct_at (&self->bot_buttons);
  std::construct_at (&self->last_midi_out_trigger_time);

  g_type_ensure (METER_WIDGET_TYPE);
  g_type_ensure (TRACK_CANVAS_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_accessible_update_property (
    GTK_ACCESSIBLE (self), GTK_ACCESSIBLE_PROPERTY_LABEL, "Track", -1);

  gtk_widget_set_focusable (GTK_WIDGET (self), true);

  gtk_orientable_set_orientation (
    GTK_ORIENTABLE (gtk_widget_get_layout_manager (GTK_WIDGET (self))),
    GTK_ORIENTATION_VERTICAL);

  self->popover_menu =
    GTK_POPOVER_MENU (gtk_popover_menu_new_from_model (nullptr));
  gtk_widget_set_parent (GTK_WIDGET (self->popover_menu), GTK_WIDGET (self));

  self->track_name_popover = GTK_POPOVER (gtk_popover_new ());
  gtk_widget_set_parent (
    GTK_WIDGET (self->track_name_popover), GTK_WIDGET (self));

  gtk_widget_set_vexpand_set (GTK_WIDGET (self), true);

  GtkEventControllerKey * key_controller =
    GTK_EVENT_CONTROLLER_KEY (gtk_event_controller_key_new ());
  g_signal_connect (
    G_OBJECT (key_controller), "key-released",
    G_CALLBACK (track_widget_key_released), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self), GTK_EVENT_CONTROLLER (key_controller));

  self->drag = GTK_GESTURE_DRAG (gtk_gesture_drag_new ());
  gtk_widget_add_controller (
    GTK_WIDGET (self->canvas), GTK_EVENT_CONTROLLER (self->drag));

  self->click = GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  gtk_widget_add_controller (
    GTK_WIDGET (self->canvas), GTK_EVENT_CONTROLLER (self->click));
  self->right_click = GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (self->right_click), GDK_BUTTON_SECONDARY);
  gtk_widget_add_controller (
    GTK_WIDGET (self->canvas), GTK_EVENT_CONTROLLER (self->right_click));

  GtkEventControllerMotion * motion_controller =
    GTK_EVENT_CONTROLLER_MOTION (gtk_event_controller_motion_new ());
  g_signal_connect (
    G_OBJECT (motion_controller), "enter", G_CALLBACK (on_enter), self);
  g_signal_connect (
    G_OBJECT (motion_controller), "leave", G_CALLBACK (on_leave), self);
  g_signal_connect (
    G_OBJECT (motion_controller), "motion", G_CALLBACK (on_motion), self);
  gtk_widget_add_controller (
    GTK_WIDGET (self->canvas), GTK_EVENT_CONTROLLER (motion_controller));

  g_signal_connect (
    G_OBJECT (self->click), "pressed", G_CALLBACK (click_pressed), self);
  g_signal_connect (
    G_OBJECT (self->click), "released", G_CALLBACK (click_released), self);
  g_signal_connect (
    G_OBJECT (self->right_click), "pressed",
    G_CALLBACK (on_right_click_pressed), self);
  g_signal_connect (
    G_OBJECT (self->drag), "drag-begin", G_CALLBACK (on_drag_begin), self);
  g_signal_connect (
    G_OBJECT (self->drag), "drag-update", G_CALLBACK (on_drag_update), self);
  g_signal_connect (
    G_OBJECT (self->drag), "drag-end", G_CALLBACK (on_drag_end), self);
  g_signal_connect (
    G_OBJECT (self), "destroy", G_CALLBACK (on_destroy), nullptr);
  g_signal_connect (
    G_OBJECT (self), "query-tooltip", G_CALLBACK (on_query_tooltip), self);

  self->redraw = 1;
}

static void
track_widget_class_init (TrackWidgetClass * _klass)
{
  GtkWidgetClass * wklass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (wklass, "track.ui");
  gtk_widget_class_set_accessible_role (wklass, GTK_ACCESSIBLE_ROLE_GROUP);
  gtk_widget_class_set_css_name (wklass, "track");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child (wklass, TrackWidget, x)

  BIND_CHILD (canvas);
  BIND_CHILD (main_box);
  BIND_CHILD (group_colors_box);
  BIND_CHILD (meter_l);
  BIND_CHILD (meter_r);
  BIND_CHILD (highlight_top_box);
  BIND_CHILD (highlight_bot_box);

#undef BIND_CHILD

  gtk_widget_class_set_layout_manager_type (wklass, GTK_TYPE_BOX_LAYOUT);

  GObjectClass * oklass = G_OBJECT_CLASS (_klass);
  oklass->dispose = (GObjectFinalizeFunc) dispose;
  oklass->finalize = (GObjectFinalizeFunc) track_widget_finalize;
}
