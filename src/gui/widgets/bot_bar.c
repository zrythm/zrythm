/*
 * Copyright (C) 2018-2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * Bottomest widget holding a status bar.
 */

#include "zrythm-config.h"

#include "audio/engine.h"
#include "audio/engine_jack.h"
#include "audio/metronome.h"
#include "audio/transport.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/button_with_menu.h"
#include "gui/widgets/cpu.h"
#include "gui/widgets/dialogs/string_entry_dialog.h"
#include "gui/widgets/digital_meter.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/top_bar.h"
#include "gui/widgets/transport_controls.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/gtk.h"
#include "utils/resources.h"
#include "utils/string.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

G_DEFINE_TYPE (BotBarWidget,
               bot_bar_widget,
               GTK_TYPE_BOX)

#define PLAYHEAD_CAPTION \
  _("Playhead")
#define PLAYHEAD_JACK_MASTER_CAPTION \
  _("Playhead [Jack Timebase Master]")
#define PLAYHEAD_JACK_CLIENT_CAPTION \
  _("Playhead [Jack Client]")

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

#ifdef HAVE_JACK
static void
on_jack_transport_type_changed (
  GtkCheckMenuItem * menuitem,
  BotBarWidget *     self)
{
  if (menuitem == self->timebase_master_check &&
        gtk_check_menu_item_get_active (menuitem))
    {
      engine_jack_set_transport_type (
        AUDIO_ENGINE,
        AUDIO_ENGINE_JACK_TIMEBASE_MASTER);
      gtk_widget_set_visible (
        self->master_img, 1);
      gtk_widget_set_visible (
        self->client_img, 0);
      gtk_widget_set_tooltip_text (
        GTK_WIDGET (self->digital_transport),
        PLAYHEAD_JACK_MASTER_CAPTION);
    }
  else if (menuitem ==
             self->transport_client_check &&
           gtk_check_menu_item_get_active (
             menuitem))
    {
      engine_jack_set_transport_type (
        AUDIO_ENGINE,
        AUDIO_ENGINE_JACK_TRANSPORT_CLIENT);
      gtk_widget_set_visible (
        self->master_img, 0);
      gtk_widget_set_visible (
        self->client_img, 1);
      gtk_widget_set_tooltip_text (
        GTK_WIDGET (self->digital_transport),
        PLAYHEAD_JACK_CLIENT_CAPTION);
    }
  else if (menuitem ==
             self->no_jack_transport_check &&
           gtk_check_menu_item_get_active (
              menuitem))
    {
      engine_jack_set_transport_type (
        AUDIO_ENGINE,
        AUDIO_ENGINE_NO_JACK_TRANSPORT);
      gtk_widget_set_visible (
        self->master_img, 0);
      gtk_widget_set_visible (
        self->client_img, 0);
      gtk_widget_set_tooltip_text (
        GTK_WIDGET (self->digital_transport),
        PLAYHEAD_CAPTION);
    }
}
#endif

static void
on_transport_playhead_right_click (
  GtkGestureMultiPress *gesture,
  gint                  n_press,
  gdouble               x,
  gdouble               y,
  BotBarWidget *      self)
{
  if (n_press != 1)
    return;

  GtkWidget *menu, *menuitem;

  menu = gtk_menu_new();

  /* switch display */
  GSList *group = NULL;
  menuitem =
    gtk_radio_menu_item_new_with_label (
      group, _("BBT display"));
  group =
    gtk_radio_menu_item_get_group (
      GTK_RADIO_MENU_ITEM (menuitem));
  gtk_check_menu_item_set_active (
    GTK_CHECK_MENU_ITEM (menuitem),
    g_settings_get_enum (
      S_UI, "transport-display") ==
      TRANSPORT_DISPLAY_BBT);
  g_signal_connect (
    G_OBJECT (menuitem), "toggled",
    G_CALLBACK (on_playhead_display_type_changed),
    self);
  self->bbt_display_check =
    GTK_CHECK_MENU_ITEM (menuitem);
  gtk_menu_shell_append (
    GTK_MENU_SHELL (menu), menuitem);

  menuitem =
    gtk_radio_menu_item_new_with_label (
      group, _("Time display"));
  gtk_check_menu_item_set_active (
    GTK_CHECK_MENU_ITEM (menuitem),
    g_settings_get_enum (
      S_UI, "transport-display") ==
      TRANSPORT_DISPLAY_TIME);
  g_signal_connect (
    G_OBJECT (menuitem), "toggled",
    G_CALLBACK (on_playhead_display_type_changed),
    self);
  self->time_display_check =
    GTK_CHECK_MENU_ITEM (menuitem);
  gtk_menu_shell_append (
    GTK_MENU_SHELL (menu), menuitem);

  menuitem =
    gtk_separator_menu_item_new ();
  gtk_widget_set_visible (menuitem, true);
  gtk_menu_shell_append (
    GTK_MENU_SHELL (menu), menuitem);

  /* jack transport related */
#ifdef HAVE_JACK
  GtkWidget * menuitem2;
  if (AUDIO_ENGINE->audio_backend ==
        AUDIO_BACKEND_JACK)
    {
      menuitem =
        gtk_radio_menu_item_new_with_mnemonic (
          NULL,
          _("Become JACK Transport master"));
      gtk_check_menu_item_set_active (
        GTK_CHECK_MENU_ITEM (menuitem),
        AUDIO_ENGINE->transport_type ==
          AUDIO_ENGINE_JACK_TIMEBASE_MASTER);
      g_signal_connect (
        G_OBJECT (menuitem), "toggled",
        G_CALLBACK (on_jack_transport_type_changed),
        self);
      /*gtk_actionable_set_action_name (*/
        /*GTK_ACTIONABLE (menuitem),*/
        /*"set-timebase-master");*/
      self->timebase_master_check =
        GTK_CHECK_MENU_ITEM (menuitem);
      gtk_menu_shell_append (
        GTK_MENU_SHELL (menu), menuitem);
      menuitem2 =
        gtk_radio_menu_item_new_with_mnemonic (
          NULL, _("Sync to JACK Transport"));
      gtk_radio_menu_item_join_group (
        GTK_RADIO_MENU_ITEM (menuitem2),
        GTK_RADIO_MENU_ITEM (menuitem));
      menuitem = menuitem2;
      gtk_check_menu_item_set_active (
        GTK_CHECK_MENU_ITEM (menuitem),
        AUDIO_ENGINE->transport_type ==
          AUDIO_ENGINE_JACK_TRANSPORT_CLIENT);
      self->transport_client_check =
        GTK_CHECK_MENU_ITEM (menuitem);
      g_signal_connect (
        G_OBJECT (menuitem), "toggled",
        G_CALLBACK (on_jack_transport_type_changed),
        self);
      /*gtk_actionable_set_action_name (*/
        /*GTK_ACTIONABLE (menuitem),*/
        /*"set-transport-client");*/
      gtk_menu_shell_append (
        GTK_MENU_SHELL (menu), menuitem);
      menuitem2 =
        gtk_radio_menu_item_new_with_mnemonic (
          NULL, _("Unlink JACK Transport"));
      gtk_radio_menu_item_join_group (
        GTK_RADIO_MENU_ITEM (menuitem2),
        GTK_RADIO_MENU_ITEM (menuitem));
      menuitem = menuitem2;
      gtk_check_menu_item_set_active (
        GTK_CHECK_MENU_ITEM (menuitem),
        AUDIO_ENGINE->transport_type ==
          AUDIO_ENGINE_NO_JACK_TRANSPORT);
      self->no_jack_transport_check =
        GTK_CHECK_MENU_ITEM (menuitem);
      g_signal_connect (
        G_OBJECT (menuitem), "toggled",
        G_CALLBACK (on_jack_transport_type_changed),
        self);
      /*gtk_actionable_set_action_name (*/
        /*GTK_ACTIONABLE (menuitem),*/
        /*"unlink-jack-transport");*/
      gtk_menu_shell_append (
        GTK_MENU_SHELL (menu), menuitem);
    }
#endif

  menuitem =
    gtk_separator_menu_item_new ();
  (void) menuitem;

  /* display format related */
  /*gtk_menu_shell_append (*/
    /*GTK_MENU_SHELL (menu), menuitem);*/
  /*menuitem =*/
    /*gtk_radio_menu_item_new_with_mnemonic (*/
      /*NULL,*/
      /*_("Show seconds"));*/

  /*g_signal_connect(menuitem, "activate",*/
                   /*(GCallback) view_popup_menu_onDoSomething, treeview);*/

  /*gtk_menu_shell_append (*/
    /*GTK_MENU_SHELL (menu), menuitem);*/

  gtk_widget_show_all (menu);

  gtk_menu_popup_at_pointer (
    GTK_MENU (menu), NULL);
}

void
bot_bar_widget_refresh (BotBarWidget * self)
{
  if (self->digital_transport)
    gtk_widget_destroy (
      GTK_WIDGET (self->digital_transport));
  self->digital_transport =
    digital_meter_widget_new_for_position (
      TRANSPORT, NULL,
      transport_get_playhead_pos,
      transport_set_playhead_pos, NULL,
      _("playhead"));
  self->digital_transport->is_transport = true;
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (self->digital_transport),
    PLAYHEAD_CAPTION);
  self->playhead_overlay =
    GTK_OVERLAY (gtk_overlay_new ());
  gtk_widget_set_visible (
    GTK_WIDGET (self->playhead_overlay), 1);
  gtk_container_add (
    GTK_CONTAINER (self->playhead_overlay),
    GTK_WIDGET (self->digital_transport));

#ifdef HAVE_JACK
  if (AUDIO_ENGINE->audio_backend ==
        AUDIO_BACKEND_JACK)
    {
      int size = 8;
      GdkPixbuf * pixbuf;
      GtkWidget * img;

      pixbuf =
        gtk_icon_theme_load_icon (
          gtk_icon_theme_get_default (),
          "jack_transport_client",
          size, 0, NULL);
      img =
        gtk_image_new_from_pixbuf (pixbuf);
      gtk_widget_set_halign (
        img, GTK_ALIGN_END);
      gtk_widget_set_valign (
        img, GTK_ALIGN_START);
      gtk_widget_set_visible (img, true);
      gtk_widget_set_tooltip_text (
        img,
        _("JACK Transport client"));
      gtk_overlay_add_overlay (
        self->playhead_overlay, img);
      self->client_img = img;
      pixbuf =
        gtk_icon_theme_load_icon (
          gtk_icon_theme_get_default (),
          "jack_timebase_master",
          size, 0, NULL);
      img =
        gtk_image_new_from_pixbuf (pixbuf);
      gtk_widget_set_halign (
        img, GTK_ALIGN_END);
      gtk_widget_set_valign (
        img, GTK_ALIGN_START);
      gtk_widget_set_margin_end (
        img, size + 2);
      gtk_widget_set_visible (img, true);
      gtk_widget_set_tooltip_text (
        img,
        _("JACK Timebase master"));
      gtk_overlay_add_overlay (
        self->playhead_overlay, img);
      self->master_img = img;
      if (AUDIO_ENGINE->transport_type ==
            AUDIO_ENGINE_JACK_TRANSPORT_CLIENT)
        {
          gtk_widget_set_visible (
            self->master_img, 0);
          gtk_widget_set_visible (
            self->client_img, 1);
          gtk_widget_set_tooltip_text (
            GTK_WIDGET (self->digital_transport),
            PLAYHEAD_JACK_CLIENT_CAPTION);
        }
      else if (AUDIO_ENGINE->transport_type ==
                 AUDIO_ENGINE_JACK_TIMEBASE_MASTER)
        {
          gtk_widget_set_visible (
            self->master_img, 1);
          gtk_widget_set_visible (
            self->client_img, 0);
          gtk_widget_set_tooltip_text (
            GTK_WIDGET (self->digital_transport),
            PLAYHEAD_JACK_MASTER_CAPTION);
        }
    }
#endif

  GtkGestureMultiPress * right_mp =
    GTK_GESTURE_MULTI_PRESS (
      gtk_gesture_multi_press_new (
        GTK_WIDGET (self->digital_transport)));
  gtk_gesture_single_set_button (
    GTK_GESTURE_SINGLE (
      right_mp),
      GDK_BUTTON_SECONDARY);
  g_signal_connect (
    G_OBJECT (right_mp), "pressed",
    G_CALLBACK (on_transport_playhead_right_click),
    self);

  gtk_container_add (
    GTK_CONTAINER (self->playhead_box),
    GTK_WIDGET (self->playhead_overlay));

  bot_bar_widget_update_status (self);
}

static bool
activate_link (
  GtkWidget   * label,
  const gchar * uri,
  gpointer      data)
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
      char * str =
        string_entry_dialog_widget_new_return_string (
          _("New buffer size"));
      if (str)
        {
          unsigned long long int buf_sz =
            strtoull (str, NULL, 10);
          g_free (str);
          if (buf_sz == 0ULL ||
              (buf_sz == ULLONG_MAX &&
               errno == ERANGE))
            {
              g_warning ("failed reading value");
            }
          else
            {
              engine_set_buffer_size (
                AUDIO_ENGINE, (uint32_t) buf_sz);
            }
        }
      return true;
    }

  return false;

feature_unavailable:
  ui_show_error_message (
    MAIN_WINDOW,
    _("This feature is not available at the "
    "moment"));
  return false;
}

static void
on_text_pushed (
  GtkStatusbar *statusbar,
  guint         context_id,
  gchar        *text,
  BotBarWidget * self)
{
  gtk_label_set_markup (self->label, text);
}

/**
 * Updates the content of the status bar.
 */
void
bot_bar_widget_update_status (
  BotBarWidget * self)
{
  gtk_statusbar_remove_all (
    MW_STATUS_BAR,
    MW_BOT_BAR->context_id);

  char color_prefix[60];
  sprintf (
    color_prefix,
    "<span foreground=\"%s\">",
    self->hex_color);
  char color_suffix[40] = "</span>";
  char green_prefix[60];
  sprintf (
    green_prefix,
    "<span foreground=\"%s\">",
    self->green_hex);
  char red_prefix[60];
  sprintf (
    red_prefix,
    "<span foreground=\"%s\">",
    self->red_hex);

  /* TODO show xruns on status */

  const char * pipewire_str = "";
  if (AUDIO_ENGINE->audio_backend ==
        AUDIO_BACKEND_JACK)
    {
#ifdef HAVE_JACK
      pipewire_str =
        engine_jack_is_pipewire (AUDIO_ENGINE) ?
          " (pw)" : "";
#endif
    }

  char str[860];
  sprintf (
    str,
    "<span size=\"small\">"
    "%s: %s%s%s%s | "
    "%s: %s%s%s%s | "
    "%s: %s%s%s\n"
    "%s: %s%d frames%s (<a href=\"change\">%s</a>) | "
    "%s: %s%d Hz%s | "
    "<a href=\"%s\">%s</a>"
    "</span>",
    _("Audio"),
    color_prefix,
    engine_audio_backend_to_string (
      AUDIO_ENGINE->audio_backend),
    pipewire_str,
    color_suffix,
    "MIDI",
    color_prefix,
    engine_midi_backend_to_string (
      AUDIO_ENGINE->midi_backend),
    pipewire_str,
    color_suffix,
    _("Status"),
    AUDIO_ENGINE->activated ?
      green_prefix : red_prefix,
    AUDIO_ENGINE->activated ? _("On") : _("Off"),
    color_suffix,
    /* TRANSLATORS: buffer size, please keep the
     * translation short */
    _("Buf sz"),
    color_prefix,
    AUDIO_ENGINE->activated ?
      AUDIO_ENGINE->block_length : 0,
    color_suffix,
    /* TRANSLATORS: verb - change buffer size */
    _("change"),
    /* TRANSLATORS: sample rate */
    _("Rate"),
    color_prefix,
    AUDIO_ENGINE->activated ?
      AUDIO_ENGINE->sample_rate : 0,
    color_suffix,
    AUDIO_ENGINE->activated ? "disable" : "enable",
    AUDIO_ENGINE->activated ?
      _("Disable") : _("Enable"));

  gtk_statusbar_push (
    MW_STATUS_BAR, MW_BOT_BAR->context_id, str);
}

static void
on_metronome_volume_changed (
  GtkRange *     range,
  BotBarWidget * self)
{
  metronome_set_volume (
    METRONOME, (float) gtk_range_get_value (range));
}

static void
setup_metronome (
  BotBarWidget * self)
{
  /* setup metronome button */
  self->metronome_btn =
    z_gtk_toggle_button_new_with_icon ("metronome");
  gtk_widget_set_size_request (
    GTK_WIDGET (self->metronome_btn), 18, -1);
  gtk_actionable_set_action_name (
    GTK_ACTIONABLE (self->metronome_btn),
    "win.toggle-metronome");
  button_with_menu_widget_setup (
    self->metronome,
    GTK_BUTTON (self->metronome_btn),
    NULL, NULL,
    false, 38, _("Metronome"), _("Volume"));

  /* create popover for changing volume */
  GtkPopover * popover =
    GTK_POPOVER (
      gtk_popover_new (
        GTK_WIDGET (self->metronome->menu_btn)));
  GtkScale * scale =
    GTK_SCALE (
      gtk_scale_new_with_range (
        GTK_ORIENTATION_HORIZONTAL, 0.f, 2.f, 0.1f));
  gtk_widget_set_size_request (
    GTK_WIDGET (scale), 120, -1);
  g_signal_connect (
    scale, "value-changed",
    G_CALLBACK (on_metronome_volume_changed),
    self);
  gtk_range_set_value (
    GTK_RANGE (scale), METRONOME->volume);
  gtk_widget_set_visible (GTK_WIDGET (scale), true);
  gtk_container_add (
    GTK_CONTAINER (popover), GTK_WIDGET (scale));

  gtk_menu_button_set_popover (
    self->metronome->menu_btn, GTK_WIDGET (popover));
}

/**
 * Sets up the bot bar.
 */
void
bot_bar_widget_setup (
  BotBarWidget * self)
{
  bot_bar_widget_refresh (self);
}

static void
bot_bar_widget_class_init (
  BotBarWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "bot_bar.ui");

  gtk_widget_class_set_css_name (
    klass, "bot-bar");

#define BIND_CHILD(child) \
  gtk_widget_class_bind_template_child ( \
    klass, BotBarWidget, child)

  BIND_CHILD (digital_meters);
  BIND_CHILD (metronome);
  BIND_CHILD (transport_controls);
  BIND_CHILD (cpu_load);
  BIND_CHILD (status_bar);
  BIND_CHILD (playhead_box);

#undef BIND_CHILD
}

static void
bot_bar_widget_init (BotBarWidget * self)
{
  g_type_ensure (DIGITAL_METER_WIDGET_TYPE);
  g_type_ensure (TRANSPORT_CONTROLS_WIDGET_TYPE);
  g_type_ensure (CPU_WIDGET_TYPE);
  g_type_ensure (BUTTON_WITH_MENU_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

  ui_gdk_rgba_to_hex (
    &UI_COLORS->bright_orange, self->hex_color);
  ui_gdk_rgba_to_hex (
    &UI_COLORS->bright_green, self->green_hex);
  ui_gdk_rgba_to_hex (
    &UI_COLORS->record_checked, self->red_hex);

  setup_metronome (self);

  /* setup digital meters */
  self->digital_bpm =
    digital_meter_widget_new (
      DIGITAL_METER_TYPE_BPM, NULL, NULL, _("bpm"));
  self->digital_timesig =
    digital_meter_widget_new (
      DIGITAL_METER_TYPE_TIMESIG, NULL,
      NULL, _("time sig."));
  gtk_container_add (
    GTK_CONTAINER (self->digital_meters),
    GTK_WIDGET (self->digital_bpm));
  gtk_container_add (
    GTK_CONTAINER (self->digital_meters),
    GTK_WIDGET (self->digital_timesig));
  gtk_widget_show_all (
    GTK_WIDGET (self->digital_meters));
  gtk_widget_show_all (GTK_WIDGET (self));

  cpu_widget_setup (
    self->cpu_load);

  self->context_id =
    gtk_statusbar_get_context_id (
      self->status_bar,
      "Main context");

  self->label =
    GTK_LABEL (gtk_label_new (""));
  gtk_label_set_markup (self->label, "");
  gtk_widget_set_visible (
    GTK_WIDGET (self->label), 1);
  GtkWidget * box =
    gtk_statusbar_get_message_area (
      self->status_bar);
  z_gtk_container_remove_all_children (
    GTK_CONTAINER (box));
  gtk_container_add (
    GTK_CONTAINER (box),
    GTK_WIDGET (self->label));

  g_signal_connect (
    self->status_bar, "text-pushed",
    G_CALLBACK (on_text_pushed),
    self);
  g_signal_connect (
    self->label, "activate-link",
    G_CALLBACK (activate_link), self);
}
