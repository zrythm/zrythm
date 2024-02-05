// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: © 2024 Miró Allard <miro.allard@pm.me>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Bottomest widget holding a status bar.
 */

#include "zrythm-config.h"

#include "dsp/engine.h"
#include "dsp/engine_jack.h"
#include "dsp/master_track.h"
#include "dsp/metronome.h"
#include "dsp/transport.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/button_with_menu.h"
#include "gui/widgets/cpu.h"
#include "gui/widgets/dialogs/string_entry_dialog.h"
#include "gui/widgets/digital_meter.h"
#include "gui/widgets/gtk_flipper.h"
#include "gui/widgets/live_waveform.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/meter.h"
#include "gui/widgets/midi_activity_bar.h"
#include "gui/widgets/preroll_count_selector.h"
#include "gui/widgets/spectrum_analyzer.h"
#include "gui/widgets/transport_controls.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/gtk.h"
#include "utils/objects.h"
#include "utils/resources.h"
#include "utils/string.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

G_DEFINE_TYPE (BotBarWidget, bot_bar_widget, GTK_TYPE_WIDGET)

#define PLAYHEAD_CAPTION _ ("Playhead")
#define PLAYHEAD_JACK_MASTER_CAPTION _ ("Playhead [Jack Timebase Master]")
#define PLAYHEAD_JACK_CLIENT_CAPTION _ ("Playhead [Jack Client]")

#if 0
static void
on_playhead_display_type_changed (
  GtkCheckMenuItem * menuitem,
  BotBarWidget *     self)
{
  if (menuitem == self->bbt_display_check &&
        gtk_check_menu_item_get_active (menuitem))
    {
      g_settings_set_enum (
        S_UI, "transport-display",
        TRANSPORT_DISPLAY_BBT);
    }
  else if (menuitem == self->time_display_check &&
             gtk_check_menu_item_get_active (
               menuitem))
    {
      g_settings_set_enum (
        S_UI, "transport-display",
        TRANSPORT_DISPLAY_TIME);
    }

  gtk_widget_queue_draw (
    GTK_WIDGET (self->digital_transport));
}
#endif

#ifdef HAVE_JACK
static void
activate_jack_mode (
  GSimpleAction * action,
  GVariant *      _variant,
  gpointer        user_data)
{
  BotBarWidget * self = Z_BOT_BAR_WIDGET (user_data);

  g_return_if_fail (_variant);

  gsize        size;
  const char * variant = g_variant_get_string (_variant, &size);
  g_simple_action_set_state (action, _variant);
  if (string_is_equal (variant, "become-master"))
    {
      engine_jack_set_transport_type (
        AUDIO_ENGINE, AUDIO_ENGINE_JACK_TIMEBASE_MASTER);
      gtk_widget_set_visible (self->master_img, 1);
      gtk_widget_set_visible (self->client_img, 0);
      gtk_widget_set_tooltip_text (
        GTK_WIDGET (self->digital_transport), PLAYHEAD_JACK_MASTER_CAPTION);
    }
  else if (string_is_equal (variant, "sync"))
    {
      engine_jack_set_transport_type (
        AUDIO_ENGINE, AUDIO_ENGINE_JACK_TRANSPORT_CLIENT);
      gtk_widget_set_visible (self->master_img, 0);
      gtk_widget_set_visible (self->client_img, 1);
      gtk_widget_set_tooltip_text (
        GTK_WIDGET (self->digital_transport), PLAYHEAD_JACK_CLIENT_CAPTION);
    }
  else if (string_is_equal (variant, "unlink"))
    {
      engine_jack_set_transport_type (
        AUDIO_ENGINE, AUDIO_ENGINE_NO_JACK_TRANSPORT);
      gtk_widget_set_visible (self->master_img, 0);
      gtk_widget_set_visible (self->client_img, 0);
      gtk_widget_set_tooltip_text (
        GTK_WIDGET (self->digital_transport), PLAYHEAD_CAPTION);
    }

  g_message ("jack mode changed");
}
#endif

static void
on_bpm_right_click (
  GtkGestureClick * gesture,
  gint              n_press,
  gdouble           x,
  gdouble           y,
  BotBarWidget *    self)
{
  if (n_press != 1)
    return;

  GMenu *     menu = g_menu_new ();
  GMenuItem * menuitem;

  menuitem = z_gtk_create_menu_item (_ ("Input"), NULL, "app.input-bpm");
  g_menu_append_item (menu, menuitem);
  menuitem = z_gtk_create_menu_item (
    /* TRANSLATORS: tap tempo */
    _ ("Tap"), NULL, "app.tap-bpm");
  g_menu_append_item (menu, menuitem);

  digital_meter_show_context_menu (self->digital_bpm, menu);
}

static void
on_transport_playhead_right_click (
  GtkGestureClick * gesture,
  gint              n_press,
  gdouble           x,
  gdouble           y,
  BotBarWidget *    self)
{
  if (n_press != 1)
    return;

  GMenu * menu = g_menu_new ();

  GMenu * section = g_menu_new ();
  g_menu_append (section, _ ("BBT"), "bot-bar-transport.transport-display::bbt");
  g_menu_append (
    section, _ ("Time"), "bot-bar-transport.transport-display::time");
  g_menu_append_section (menu, _ ("Display"), G_MENU_MODEL (section));

  /* TODO fire event on change */

  /* jack transport related */
#ifdef HAVE_JACK
  if (AUDIO_ENGINE->audio_backend == AUDIO_BACKEND_JACK)
    {
      GMenu * jack_section = g_menu_new ();

      g_menu_append (
        jack_section, _ ("Become JACK Transport master"),
        "bot-bar-transport.jack-mode::become-master");
      g_menu_append (
        jack_section, _ ("Sync to JACK Transport"),
        "bot-bar-transport.jack-mode::sync");
      g_menu_append (
        jack_section, _ ("Unlink JACK Transport"),
        "bot-bar-transport.jack-mode::unlink");

      g_menu_append_section (menu, "JACK", G_MENU_MODEL (jack_section));
    }
#endif

  digital_meter_show_context_menu (self->digital_transport, menu);
}

/**
 * Setter for transport display meter.
 */
static void
set_playhead_pos (Transport * transport, Position * pos)
{
  /* only set playhead position if the user is currently
   * allowed to move the playhead */
  if (transport_can_user_move_playhead (transport))
    {
      transport_set_playhead_pos (transport, pos);
    }
}

void
bot_bar_widget_refresh (BotBarWidget * self)
{
  bot_bar_widget_update_status (self);

  if (self->digital_transport)
    {
      /* FIXME */
      return;
      /*gtk_overlay_set_child (*/
      /*self->playhead_overlay, NULL);*/
    }
  self->digital_transport = digital_meter_widget_new_for_position (
    TRANSPORT, NULL, transport_get_playhead_pos, set_playhead_pos, NULL,
    _ ("playhead"));
  self->digital_transport->is_transport = true;
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (self->digital_transport), PLAYHEAD_CAPTION);
  self->playhead_overlay = GTK_OVERLAY (gtk_overlay_new ());
  gtk_overlay_set_child (
    self->playhead_overlay, GTK_WIDGET (self->digital_transport));

  /* create action group for right click menu */
  GSimpleActionGroup * action_group = g_simple_action_group_new ();
  GAction *            display_action =
    g_settings_create_action (S_UI, "transport-display");
  g_action_map_add_action (G_ACTION_MAP (action_group), display_action);
#ifdef HAVE_JACK
  const char * jack_modes[] = {
    "'become-master'",
    "'sync'",
    "'unlink'",
  };
  GActionEntry actions[] = {
    {"jack-mode", activate_jack_mode, "s",
     jack_modes[AUDIO_ENGINE->transport_type]},
  };
  g_action_map_add_action_entries (
    G_ACTION_MAP (action_group), actions, G_N_ELEMENTS (actions), self);
#endif
  gtk_widget_insert_action_group (
    GTK_WIDGET (self->digital_transport), "bot-bar-transport",
    G_ACTION_GROUP (action_group));

#ifdef HAVE_JACK
  if (AUDIO_ENGINE->audio_backend == AUDIO_BACKEND_JACK)
    {
      GtkWidget * img = gtk_image_new_from_icon_name ("jack-transport-client");
      gtk_widget_set_halign (img, GTK_ALIGN_END);
      gtk_widget_set_valign (img, GTK_ALIGN_START);
      gtk_widget_set_visible (img, true);
      gtk_widget_set_tooltip_text (img, _ ("JACK Transport client"));
      gtk_overlay_add_overlay (self->playhead_overlay, img);
      self->client_img = img;
      img = gtk_image_new_from_icon_name ("jack-timebase-master");
      gtk_widget_set_halign (img, GTK_ALIGN_END);
      gtk_widget_set_valign (img, GTK_ALIGN_START);
      gtk_widget_set_visible (img, true);
      gtk_widget_set_tooltip_text (img, _ ("JACK Timebase master"));
      gtk_overlay_add_overlay (self->playhead_overlay, img);
      self->master_img = img;
      if (AUDIO_ENGINE->transport_type == AUDIO_ENGINE_JACK_TRANSPORT_CLIENT)
        {
          gtk_widget_set_visible (self->master_img, 0);
          gtk_widget_set_visible (self->client_img, 1);
          gtk_widget_set_tooltip_text (
            GTK_WIDGET (self->digital_transport), PLAYHEAD_JACK_CLIENT_CAPTION);
        }
      else if (AUDIO_ENGINE->transport_type == AUDIO_ENGINE_JACK_TIMEBASE_MASTER)
        {
          gtk_widget_set_visible (self->master_img, 1);
          gtk_widget_set_visible (self->client_img, 0);
          gtk_widget_set_tooltip_text (
            GTK_WIDGET (self->digital_transport), PLAYHEAD_JACK_MASTER_CAPTION);
        }
      else
        {
          gtk_widget_set_visible (self->master_img, false);
          gtk_widget_set_visible (self->client_img, false);
        }
    }
#endif

  GtkGestureClick * right_mp = GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  gtk_widget_add_controller (
    GTK_WIDGET (self->digital_transport), GTK_EVENT_CONTROLLER (right_mp));
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (right_mp), GDK_BUTTON_SECONDARY);
  g_signal_connect (
    G_OBJECT (right_mp), "pressed",
    G_CALLBACK (on_transport_playhead_right_click), self);

  gtk_box_append (self->playhead_box, GTK_WIDGET (self->playhead_overlay));
}

static void
set_buffer_size (void * object, const char * str)
{
  if (str)
    {
      unsigned long long int buf_sz = strtoull (str, NULL, 10);
      if (buf_sz == 0ULL || (buf_sz == ULLONG_MAX && errno == ERANGE))
        {
          ui_show_error_message (
            _ ("Invalid Value"), _ ("Please enter a valid number"));
          return;
        }
      else
        {
          engine_set_buffer_size (AUDIO_ENGINE, (uint32_t) buf_sz);
        }
    }
}

static const char *
get_buffer_size (void * object)
{
  static char * buffer_size = NULL;
  if (buffer_size != NULL)
    {
      g_free_and_null (buffer_size);
    }
  buffer_size = g_strdup_printf ("%u", AUDIO_ENGINE->block_length);
  return buffer_size;
}

static bool
activate_link (GtkWidget * label, const gchar * uri, gpointer data)
{
  g_debug ("link activated");
  if (string_is_equal (uri, "enable"))
    {
      g_debug ("enable pressed");
      goto feature_unavailable;
      engine_activate (AUDIO_ENGINE, true);
      return true;
    }
  else if (string_is_equal (uri, "disable"))
    {
      g_debug ("disable pressed");
      goto feature_unavailable;
      engine_activate (AUDIO_ENGINE, false);
      return true;
    }
  else if (string_is_equal (uri, "change"))
    {
      g_debug ("change buf size pressed");
      StringEntryDialogWidget * dialog = string_entry_dialog_widget_new (
        _ ("Buffer Size"), NULL, (GenericStringGetter) get_buffer_size,
        (GenericStringSetter) set_buffer_size);
      gtk_window_present (GTK_WINDOW (dialog));
      return true;
    }

  return false;

feature_unavailable:
  ui_show_error_message (
    _ ("Feature Not Available"),
    _ ("This feature is not available at the "
       "moment"));
  return false;
}

/**
 * Updates the content of the status bar.
 */
void
bot_bar_widget_update_status (BotBarWidget * self)
{
  char color_prefix[60];
  sprintf (color_prefix, "<span foreground=\"%s\">", self->hex_color);
  char color_suffix[40] = "</span>";
  char green_prefix[60];
  sprintf (green_prefix, "<span foreground=\"%s\">", self->green_hex);
  char red_prefix[60];
  sprintf (red_prefix, "<span foreground=\"%s\">", self->red_hex);

  /* TODO show xruns on status */

  const char * audio_pipewire_str = "";
  const char * midi_pipewire_str = "";
  if (AUDIO_ENGINE->audio_backend == AUDIO_BACKEND_JACK)
    {
#ifdef HAVE_JACK
      audio_pipewire_str = engine_jack_is_pipewire (AUDIO_ENGINE) ? " (pw)" : "";
#endif
    }
  if (AUDIO_ENGINE->midi_backend == MIDI_BACKEND_JACK)
    {
#ifdef HAVE_JACK
      midi_pipewire_str = engine_jack_is_pipewire (AUDIO_ENGINE) ? " (pw)" : "";
#endif
    }

#define BUF_SZ 1200
  char str[BUF_SZ];
  snprintf (
    str, BUF_SZ,
    "<span size=\"small\">"
    "%s: %s%s%s%s | "
    "%s: %s%s%s%s | "
    "%s: %s%s%s\n"
    "%s: %s%d frames%s "
#ifndef _WOE32
    "(<a href=\"change\">%s</a>) "
#endif
    "| "
    "%s: %s%d Hz%s | "
    "<a href=\"%s\">%s</a>"
    "</span>",
    _ ("Audio"), color_prefix,
    engine_audio_backend_to_string (AUDIO_ENGINE->audio_backend),
    audio_pipewire_str, color_suffix, "MIDI", color_prefix,
    engine_midi_backend_to_string (AUDIO_ENGINE->midi_backend),
    midi_pipewire_str, color_suffix, _ ("Status"),
    AUDIO_ENGINE->activated ? green_prefix : red_prefix,
    AUDIO_ENGINE->activated ? _ ("On") : _ ("Off"), color_suffix,
    /* TRANSLATORS: buffer size, please keep the
     * translation short */
    _ ("Buf sz"), color_prefix,
    AUDIO_ENGINE->activated ? AUDIO_ENGINE->block_length : 0, color_suffix,
#ifndef _WOE32
    /* TRANSLATORS: verb - change buffer size */
    _ ("change"),
#endif
    /* TRANSLATORS: sample rate */
    _ ("Rate"), color_prefix,
    AUDIO_ENGINE->activated ? AUDIO_ENGINE->sample_rate : 0, color_suffix,
    AUDIO_ENGINE->activated ? "disable" : "enable",
    AUDIO_ENGINE->activated ? _ ("Disable") : _ ("Enable"));
#undef BUF_SZ

  gtk_label_set_markup (self->engine_status_label, str);
}

static void
on_metronome_volume_changed (GtkRange * range, BotBarWidget * self)
{
  metronome_set_volume (METRONOME, (float) gtk_range_get_value (range));
}

static void
setup_metronome (BotBarWidget * self)
{
  /* setup metronome button */
  self->metronome_btn = z_gtk_toggle_button_new_with_icon ("metronome");
  gtk_widget_set_size_request (GTK_WIDGET (self->metronome_btn), 18, -1);
  gtk_actionable_set_action_name (
    GTK_ACTIONABLE (self->metronome_btn), "app.toggle-metronome");
  button_with_menu_widget_setup (
    self->metronome, GTK_BUTTON (self->metronome_btn), NULL, false, 38,
    _ ("Metronome"), _ ("Metronome options"));

  /* create popover for changing metronome options */
  GtkPopover * popover = GTK_POPOVER (gtk_popover_new ());
  GtkWidget *  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 2);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 4);
  gtk_popover_set_child (popover, grid);

  /* volume */
  GtkWidget * label = gtk_label_new (_ ("Volume"));
  gtk_grid_attach (GTK_GRID (grid), GTK_WIDGET (label), 0, 0, 1, 1);
  GtkScale * scale = GTK_SCALE (
    gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, 0.f, 2.f, 0.1f));
  gtk_widget_set_size_request (GTK_WIDGET (scale), 120, -1);
  g_signal_connect (
    scale, "value-changed", G_CALLBACK (on_metronome_volume_changed), self);
  gtk_range_set_value (GTK_RANGE (scale), METRONOME->volume);
  gtk_widget_set_visible (GTK_WIDGET (scale), true);
  gtk_grid_attach (GTK_GRID (grid), GTK_WIDGET (scale), 1, 0, 1, 1);

  /* countin */
  label = gtk_label_new (_ ("Count-in"));
  gtk_widget_set_visible (label, true);
  gtk_grid_attach (GTK_GRID (grid), GTK_WIDGET (label), 0, 1, 1, 1);
  PrerollCountSelectorWidget * preroll_count =
    preroll_count_selector_widget_new (PREROLL_COUNT_SELECTOR_METRONOME_COUNTIN);
  gtk_grid_attach (GTK_GRID (grid), GTK_WIDGET (preroll_count), 1, 1, 1, 1);

  gtk_menu_button_set_popover (self->metronome->menu_btn, GTK_WIDGET (popover));
}

/**
 * Sets up the bot bar.
 */
void
bot_bar_widget_setup (BotBarWidget * self)
{
  bot_bar_widget_refresh (self);

  GtkGestureClick * right_mp = GTK_GESTURE_CLICK (gtk_gesture_click_new ());
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (right_mp), GDK_BUTTON_SECONDARY);
  gtk_widget_add_controller (
    GTK_WIDGET (self->digital_bpm), GTK_EVENT_CONTROLLER (right_mp));
  g_signal_connect (
    G_OBJECT (right_mp), "pressed", G_CALLBACK (on_bpm_right_click), self);
}

static void
dispose (BotBarWidget * self)
{
  gtk_widget_unparent (GTK_WIDGET (self->center_box));

  G_OBJECT_CLASS (bot_bar_widget_parent_class)->dispose (G_OBJECT (self));
}

static void
bot_bar_widget_init (BotBarWidget * self)
{
  g_type_ensure (DIGITAL_METER_WIDGET_TYPE);
  g_type_ensure (TRANSPORT_CONTROLS_WIDGET_TYPE);
  g_type_ensure (CPU_WIDGET_TYPE);
  g_type_ensure (BUTTON_WITH_MENU_WIDGET_TYPE);
  g_type_ensure (PANEL_TYPE_TOGGLE_BUTTON);
  g_type_ensure (LIVE_WAVEFORM_WIDGET_TYPE);
  g_type_ensure (MIDI_ACTIVITY_BAR_WIDGET_TYPE);
  g_type_ensure (GTK_TYPE_FLIPPER);
  g_type_ensure (SPECTRUM_ANALYZER_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

  ui_gdk_rgba_to_hex (&UI_COLORS->bright_orange, self->hex_color);
  ui_gdk_rgba_to_hex (&UI_COLORS->bright_green, self->green_hex);
  ui_gdk_rgba_to_hex (&UI_COLORS->record_checked, self->red_hex);

  setup_metronome (self);

  /* setup midi visualizers */
  live_waveform_widget_setup_engine (self->live_waveform);
  spectrum_analyzer_widget_setup_engine (self->spectrum_analyzer);
  midi_activity_bar_widget_setup_engine (self->midi_activity);
  midi_activity_bar_widget_set_animation (
    self->midi_activity, MAB_ANIMATION_FLASH);

  MeterWidget * l = meter_widget_new (P_MASTER_TRACK->channel->stereo_out->l, 8);
  MeterWidget * r = meter_widget_new (P_MASTER_TRACK->channel->stereo_out->r, 8);
  gtk_box_append (self->meter_box, GTK_WIDGET (l));
  gtk_box_append (self->meter_box, GTK_WIDGET (r));

  /* setup digital meters */
  self->digital_bpm =
    digital_meter_widget_new (DIGITAL_METER_TYPE_BPM, NULL, NULL, _ ("bpm"));
  self->digital_timesig = digital_meter_widget_new (
    DIGITAL_METER_TYPE_TIMESIG, NULL, NULL, _ ("time sig."));
  gtk_box_append (self->digital_meters, GTK_WIDGET (self->digital_bpm));
  gtk_box_append (self->digital_meters, GTK_WIDGET (self->digital_timesig));

  g_signal_connect (
    self->engine_status_label, "activate-link", G_CALLBACK (activate_link),
    self);
}

static void
bot_bar_widget_class_init (BotBarWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "bot_bar.ui");

  gtk_widget_class_set_css_name (klass, "bot-bar");

#define BIND_CHILD(child) \
  gtk_widget_class_bind_template_child (klass, BotBarWidget, child)

  BIND_CHILD (center_box);
  BIND_CHILD (digital_meters);
  BIND_CHILD (metronome);
  BIND_CHILD (transport_controls);
  BIND_CHILD (live_waveform);
  BIND_CHILD (spectrum_analyzer);
  BIND_CHILD (midi_activity);
  BIND_CHILD (meter_box);
  BIND_CHILD (cpu_load);
  BIND_CHILD (engine_status_label);
  BIND_CHILD (playhead_box);
  BIND_CHILD (bot_dock_switcher);

#undef BIND_CHILD

  gtk_widget_class_set_layout_manager_type (klass, GTK_TYPE_BIN_LAYOUT);

  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->dispose = (GObjectFinalizeFunc) dispose;
}
