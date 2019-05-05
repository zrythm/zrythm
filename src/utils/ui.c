/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <math.h>

#include "audio/engine.h"
#include "audio/pan.h"
#include "gui/widgets/audio_clip_editor.h"
#include "gui/widgets/audio_ruler.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_ruler.h"
#include "gui/widgets/piano_roll.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_ruler.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/localization.h"
#include "utils/ui.h"

#include <glib/gi18n.h>

/**
 * Sets cursor from icon name.
 */
void
ui_set_cursor_from_icon_name (
  GtkWidget *  widget,
  const char * name,
  int          offset_x,
  int          offset_y)
{
  GdkWindow * win =
    gtk_widget_get_parent_window (widget);
  if (!GDK_IS_WINDOW (win))
    return;
  GdkPixbuf * pixbuf =
    gtk_icon_theme_load_icon (
      gtk_icon_theme_get_default (),
      name,
      18,
      0,
      NULL);
  if (!GDK_IS_PIXBUF (pixbuf))
    {
      g_warning ("no pixbuf for %s",
                 name);
      return;
    }
  GdkCursor * cursor =
    gdk_cursor_new_from_pixbuf (
      gdk_display_get_default (),
      pixbuf,
      offset_x,
      offset_y);
  gdk_window_set_cursor(win, cursor);
}

/**
 * Sets cursor from standard cursor name.
 */
void
ui_set_cursor_from_name (
  GtkWidget * widget, char * name)
{
  GdkWindow * win =
    gtk_widget_get_parent_window (widget);
  if (!GDK_IS_WINDOW (win))
    return;
  GdkCursor * cursor =
    gdk_cursor_new_from_name (
      gdk_display_get_default (),
      name);
  gdk_window_set_cursor(win, cursor);
}

/**
 * Shows error popup.
 */
void
ui_show_error_message_full (
  GtkWindow * parent_window,
  const char * message)
{
  GtkDialogFlags flags =
    GTK_DIALOG_DESTROY_WITH_PARENT;
  GtkWidget * dialog =
    gtk_message_dialog_new (parent_window,
      flags,
      GTK_MESSAGE_ERROR,
      GTK_BUTTONS_CLOSE,
      "%s",
      message);
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}

/**
 * Returns the matching hit child, or NULL.
 */
GtkWidget *
ui_get_hit_child (
  GtkContainer * parent,
  double         x, ///< x in parent space
  double         y, ///< y in parent space
  GType          type) ///< type to look for
{
  GList *children, *iter;

  /* go through each overlay child */
  children =
    gtk_container_get_children (parent);
  for (iter = children;
       iter != NULL;
       iter = g_list_next (iter))
    {
      GtkWidget * widget = GTK_WIDGET (iter->data);

      GtkAllocation allocation;
      gtk_widget_get_allocation (
        widget,
        &allocation);

      gint wx, wy;
      gtk_widget_translate_coordinates (
        GTK_WIDGET (parent),
        GTK_WIDGET (widget),
        x,
        y,
        &wx,
        &wy);

      /* if hit */
      if (wx >= 0 &&
          wx <= allocation.width &&
          wy >= 0 &&
          wy <= allocation.height)
        {
          /* if type matches */
          if (G_TYPE_CHECK_INSTANCE_TYPE (
                widget,
                type))
            {
              return widget;
            }
        }
    }
  return NULL;
}

/**
 * Returns if the child is hit or not by the coordinates in
 * parent.
 */
int
ui_is_child_hit (GtkContainer * parent,
                 GtkWidget *    child,
                 double         x, ///< x in parent space
                 double         y) ///< y in parent space
{
  GtkWidget * widget =
    ui_get_hit_child (parent,
                      x,
                      y,
                      G_OBJECT_TYPE (child));
  if (widget == child)
    return 1;
  else
    return 0;
}

static void
px_to_pos (
  double        px,
  Position *    pos,
  int           use_padding,
  RulerWidget * ruler)
{
  RULER_WIDGET_GET_PRIVATE (ruler);

  if (use_padding)
    {
      px -= SPACE_BEFORE_START_D;

      /* clamp at 0 */
      if (px < 0.0)
        px = 0.0;
    }

  pos->bars = px / rw_prv->px_per_bar + 1;
  px = fmod (px, rw_prv->px_per_bar);
  pos->beats = px / rw_prv->px_per_beat + 1;
  px = fmod (px, rw_prv->px_per_beat);
  pos->sixteenths = px / rw_prv->px_per_sixteenth + 1;
  px = fmod (px, rw_prv->px_per_sixteenth);
  pos->ticks = px / rw_prv->px_per_tick;
}

/**
 * Converts from pixels to position.
 *
 * Only works with positive numbers. Negatives will be
 * clamped at 0. If a negative is needed, pass the abs to
 * this function and then change the sign.
 */
void
ui_px_to_pos_timeline (
  double     px,
  Position * pos,
  int        has_padding) ///< whether the given px include padding
{
  if (!MAIN_WINDOW || !MW_RULER)
    return;

  px_to_pos (px, pos, has_padding,
             Z_RULER_WIDGET (MW_RULER));
}


/**
 * Converts from pixels to position.
 *
 * Only works with positive numbers. Negatives will be
 * clamped at 0. If a negative is needed, pass the abs to
 * this function and then change the sign.
 */
void
ui_px_to_pos_piano_roll (double               px,
           Position *        pos,
           int               has_padding) ///< whether the given px include padding
{
  if (!MAIN_WINDOW || !MIDI_RULER)
    return;

  px_to_pos (px, pos, has_padding,
             Z_RULER_WIDGET (MIDI_RULER));
}

/**
 * Converts from pixels to position.
 *
 * Only works with positive numbers. Negatives will be
 * clamped at 0. If a negative is needed, pass the abs to
 * this function and then change the sign.
 */
void
ui_px_to_pos_audio_clip_editor (
  double               px,
  Position *           pos,
  int                  has_padding) ///< whether the given px include padding
{
  if (!MAIN_WINDOW || !AUDIO_RULER)
    return;

  px_to_pos (px, pos, has_padding,
             Z_RULER_WIDGET (AUDIO_RULER));
}

static int
pos_to_px (
  Position *       pos,
  int              use_padding,
  RulerWidget *    ruler)
{
  RULER_WIDGET_GET_PRIVATE (ruler)

  int px =
    (pos->bars - 1) * rw_prv->px_per_bar +
    (pos->beats - 1) * rw_prv->px_per_beat +
    (pos->sixteenths - 1) * rw_prv->px_per_sixteenth +
    pos->ticks * rw_prv->px_per_tick;

  if (use_padding)
    px += SPACE_BEFORE_START;

  return px;
}

/**
 * Gets pixels from the position, based on the
 * timeline ruler.
 */
int
ui_pos_to_px_timeline (
  Position *       pos,
  int              use_padding)
{
  if (!MAIN_WINDOW || !MW_RULER)
    return 0;

  return pos_to_px (
    pos, use_padding, Z_RULER_WIDGET (MW_RULER));
}

/**
 * Gets pixels from the position, based on the
 * piano_roll ruler.
 */
int
ui_pos_to_px_piano_roll (
  Position *       pos,
  int              use_padding)
{
  if (!MAIN_WINDOW || !MIDI_RULER)
    return 0;

  return pos_to_px (
    pos, use_padding, Z_RULER_WIDGET (MIDI_RULER));
}

/**
 * Gets pixels from the position, based on the
 * piano_roll ruler.
 */
int
ui_pos_to_px_audio_clip_editor (
  Position *       pos,
  int              use_padding)
{
  if (!MAIN_WINDOW || !AUDIO_RULER)
    return 0;

  return pos_to_px (
    pos, use_padding, Z_RULER_WIDGET (AUDIO_RULER));
}

static long
px_to_frames (
  double        px,
  int           has_padding, ///< whether the given px contain padding
  RulerWidget * ruler)
{
  RULER_WIDGET_GET_PRIVATE (ruler)

  if (has_padding)
    {
      px -= SPACE_BEFORE_START;

      /* clamp at 0 */
      if (px < 0.0)
        px = 0.0;
    }

  return (AUDIO_ENGINE->frames_per_tick * px) /
    rw_prv->px_per_tick;
}

/**
 * Converts from pixels to frames, based on the
 * timeline.
 */
long
ui_px_to_frames_timeline (
  double   px,
  int   has_padding) ///< whether the given px contain padding
{
  if (!MAIN_WINDOW || !MW_RULER)
    return 0;

  return px_to_frames (px, has_padding,
                       Z_RULER_WIDGET (MW_RULER));
}

/**
 * Converts from pixels to frames, based on the
 * piano_roll.
 */
long
ui_px_to_frames_piano_roll (
  double   px,
  int   has_padding) ///< whether the given px contain padding
{
  if (!MAIN_WINDOW || !MIDI_RULER)
    return 0;

  return px_to_frames (px, has_padding,
                       Z_RULER_WIDGET (MIDI_RULER));
}

/**
 * Converts from pixels to frames, based on the
 * piano_roll.
 */
long
ui_px_to_frames_audio_clip_editor (
  double   px,
  int   has_padding) ///< whether the given px contain padding
{
  if (!MAIN_WINDOW || !AUDIO_RULER)
    return 0;

  return px_to_frames (px, has_padding,
                       Z_RULER_WIDGET (AUDIO_RULER));
}

/**
 * Hides the notification.
 *
 * Used ui_show_notification to be called after
 * a timeout.
 */
static int
hide_notification_async ()
{
  gtk_revealer_set_reveal_child (
    GTK_REVEALER (MAIN_WINDOW->revealer),
    0);

  return FALSE;
}

gboolean
ui_on_motion_set_status_bar_text_cb (
  GtkWidget * widget,
  GdkEvent *  event,
  char *      text)
{
  if (gdk_event_get_event_type (event) ==
        GDK_ENTER_NOTIFY)
    {
      bot_bar_change_status (text);
    }
  else if (gdk_event_get_event_type (event) ==
             GDK_LEAVE_NOTIFY)
    {
      bot_bar_change_status ("");
    }

  return FALSE;
}

/**
 * Shows a notification in the revealer.
 */
void
ui_show_notification (const char * msg)
{
  gtk_label_set_text (MAIN_WINDOW->notification_label,
                      msg);
  gtk_revealer_set_reveal_child (
    GTK_REVEALER (MAIN_WINDOW->revealer),
    1);
  g_timeout_add_seconds (
    3, (GSourceFunc) hide_notification_async, NULL);
}

/**
 * Show notification from non-GTK threads.
 *
 * This should be used internally. Use the
 * ui_show_notification_idle macro instead.
 */
int
ui_show_notification_idle_func (char * msg)
{
  ui_show_notification (msg);
  g_free (msg);

  return G_SOURCE_REMOVE;
}

/**
 * Returns the modifier type (state mask) from the
 * given gesture.
 */
void
ui_get_modifier_type_from_gesture (
  GtkGestureSingle * gesture,
  GdkModifierType *  state_mask) ///< return value
{
  GdkEventSequence *sequence =
    gtk_gesture_single_get_current_sequence (
      gesture);
  const GdkEvent * event =
    gtk_gesture_get_last_event (
      GTK_GESTURE (gesture), sequence);
  gdk_event_get_state (event, state_mask);
}

#define CREATE_SIMPLE_MODEL_BOILERPLATE \
  enum \
  { \
    VALUE_COL, \
    TEXT_COL \
  }; \
  GtkTreeIter iter; \
  GtkListStore *store; \
  gint i; \
 \
  store = gtk_list_store_new (2, \
                              G_TYPE_INT, \
                              G_TYPE_STRING); \
 \
  int num_elements = G_N_ELEMENTS (values); \
  for (i = 0; i < num_elements; i++) \
    { \
      gtk_list_store_append (store, &iter); \
      gtk_list_store_set (store, &iter, \
                          VALUE_COL, values[i], \
                          TEXT_COL, labels[i], \
                          -1); \
    } \
 \
  return GTK_TREE_MODEL (store);

/**
 * Creates and returns a language model for combo
 * boxes.
 */
static GtkTreeModel *
ui_create_language_model ()
{
  const int values[NUM_UI_LANGUAGES] = {
    UI_ENGLISH,
    UI_GERMAN,
    UI_FRENCH,
    UI_ITALIAN,
    UI_SPANISH,
    UI_JAPANESE,
    UI_PORTUGUESE,
    UI_RUSSIAN,
    UI_CHINESE,
  };
  const gchar *labels[NUM_UI_LANGUAGES] = {
    _("English [en]"),
    _("German [de]"),
    _("French [fr]"),
    _("Italian [it]"),
    _("Spanish [es]"),
    _("Japanese [ja]"),
    _("Portuguese [pt]"),
    _("Russian [ru]"),
    _("Chinese [zh]"),
  };

  CREATE_SIMPLE_MODEL_BOILERPLATE;
}

static GtkTreeModel *
ui_create_audio_backends_model (void)
{
  const int values[NUM_AUDIO_BACKENDS] = {
    AUDIO_BACKEND_DUMMY,
    AUDIO_BACKEND_JACK,
    AUDIO_BACKEND_ALSA,
    AUDIO_BACKEND_PORT_AUDIO,
  };
  const gchar *labels[NUM_AUDIO_BACKENDS] = {
    /* TRANSLATORS: Dummy audio backend */
    _("Dummy"),
    "Jack",
    "ALSA",
    "PortAudio",
  };

  CREATE_SIMPLE_MODEL_BOILERPLATE;
}
static GtkTreeModel *
ui_create_midi_backends_model (void)
{
  const int values[NUM_MIDI_BACKENDS] = {
    MIDI_BACKEND_DUMMY,
    MIDI_BACKEND_JACK,
  };
  const gchar *labels[NUM_AUDIO_BACKENDS] = {
    /* TRANSLATORS: Dummy audio backend */
    _("Dummy"),
    "Jack MIDI",
  };

  CREATE_SIMPLE_MODEL_BOILERPLATE;
}

static GtkTreeModel *
ui_create_pan_algo_model (void)
{

  const int values[3] = {
    PAN_ALGORITHM_LINEAR,
    PAN_ALGORITHM_SQUARE_ROOT,
    PAN_ALGORITHM_SINE_LAW,
  };
  const gchar *labels[3] = {
    /* TRANSLATORS: Pan algorithm */
    _("Linear"),
    _("Square Root"),
    _("Sine (Equal Power)"),
  };

  CREATE_SIMPLE_MODEL_BOILERPLATE;
}

static GtkTreeModel *
ui_create_pan_law_model (void)
{

  const int values[3] = {
    PAN_LAW_0DB,
    PAN_LAW_MINUS_3DB,
    PAN_LAW_MINUS_6DB,
  };
  const gchar *labels[3] = {
    /* TRANSLATORS: Pan algorithm */
    "0dB",
    "-3dB",
    "-6dB",
  };

  CREATE_SIMPLE_MODEL_BOILERPLATE;
}

/**
 * Sets up a combo box to have a selection of
 * languages.
 */
void
ui_setup_language_combo_box (
  GtkComboBox * language)
{
  z_gtk_configure_simple_combo_box (
    language, ui_create_language_model ());

  gtk_combo_box_set_active (
    GTK_COMBO_BOX (language),
    g_settings_get_enum (
      S_PREFERENCES,
      "language"));
}

/**
 * Sets up an audio backends combo box.
 */
void
ui_setup_audio_backends_combo_box (
  GtkComboBox * cb)
{
  z_gtk_configure_simple_combo_box (
    cb, ui_create_audio_backends_model ());

  gtk_combo_box_set_active (
    GTK_COMBO_BOX (cb),
    g_settings_get_enum (
      S_PREFERENCES,
      "audio-backend"));
}

/**
 * Sets up a MIDI backends combo box.
 */
void
ui_setup_midi_backends_combo_box (
  GtkComboBox * cb)
{
  z_gtk_configure_simple_combo_box (
    cb, ui_create_midi_backends_model ());

  gtk_combo_box_set_active (
    GTK_COMBO_BOX (cb),
    g_settings_get_enum (
      S_PREFERENCES,
      "midi-backend"));
}

/**
 * Sets up a pan algorithm combo box.
 */
void
ui_setup_pan_algo_combo_box (
  GtkComboBox * cb)
{
  z_gtk_configure_simple_combo_box (
    cb, ui_create_pan_algo_model ());

  gtk_combo_box_set_active (
    GTK_COMBO_BOX (cb),
    g_settings_get_enum (
      S_PREFERENCES,
      "pan-algo"));
}

/**
 * Sets up a pan law combo box.
 */
void
ui_setup_pan_law_combo_box (
  GtkComboBox * cb)
{
  z_gtk_configure_simple_combo_box (
    cb, ui_create_pan_law_model ());

  gtk_combo_box_set_active (
    GTK_COMBO_BOX (cb),
    g_settings_get_enum (
      S_PREFERENCES,
      "pan-law"));
}
