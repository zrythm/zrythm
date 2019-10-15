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

#include <math.h>

#include "audio/engine.h"
#include "audio/pan.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/editor_ruler.h"
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
  gdk_window_set_cursor (win, cursor);
}

/**
 * Sets cursor from standard cursor name.
 */
void
ui_set_cursor_from_name (
  GtkWidget * widget,
  const char * name)
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
 * Shows a popup message of the given type with the
 * given message.
 */
void
ui_show_message_full (
  GtkWindow *    parent_window,
  GtkMessageType type,
  const char *   message)
{
  GtkDialogFlags flags =
    GTK_DIALOG_DESTROY_WITH_PARENT;
  GtkWidget * dialog =
    gtk_message_dialog_new (parent_window,
      flags,
      type,
      GTK_BUTTONS_CLOSE,
      "%s",
      message);
  gtk_window_set_title (
    GTK_WINDOW (dialog), "Zrythm");
  gtk_window_set_icon_name (
    GTK_WINDOW (dialog), "zrythm");
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

      if (!gtk_widget_get_visible (widget))
        continue;

      GtkAllocation allocation;
      gtk_widget_get_allocation (
        widget,
        &allocation);

      gint wx, wy;
      gtk_widget_translate_coordinates (
        GTK_WIDGET (parent),
        GTK_WIDGET (widget),
        (int) x, (int) y, &wx, &wy);

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
              g_list_free (children);
              return widget;
            }
        }
    }

  g_list_free (children);
  return NULL;
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

  pos->bars =
    (int)
    (px / rw_prv->px_per_bar + 1.0);
  px = fmod (px, rw_prv->px_per_bar);
  pos->beats =
    (int)
    (px / rw_prv->px_per_beat + 1.0);
  px = fmod (px, rw_prv->px_per_beat);
  pos->sixteenths =
    (int)
    (px / rw_prv->px_per_sixteenth + 1.0);
  px = fmod (px, rw_prv->px_per_sixteenth);
  pos->ticks =
    (int)
    (px / rw_prv->px_per_tick);
  pos->total_ticks = position_to_ticks (pos);
  pos->frames = position_to_frames (pos);
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
ui_px_to_pos_editor (double               px,
           Position *        pos,
           int               has_padding) ///< whether the given px include padding
{
  if (!MAIN_WINDOW || !EDITOR_RULER)
    return;

  px_to_pos (px, pos, has_padding,
             (RulerWidget *) (EDITOR_RULER));
}

static int
pos_to_px (
  Position *       pos,
  int              use_padding,
  RulerWidget *    ruler)
{
  RULER_WIDGET_GET_PRIVATE (ruler)

  int px =
    (int)
    ((double) (pos->bars - 1) * rw_prv->px_per_bar +
    (double) (pos->beats - 1) * rw_prv->px_per_beat +
    (double) (pos->sixteenths - 1) *
      rw_prv->px_per_sixteenth +
    (double) pos->ticks * rw_prv->px_per_tick);

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
    pos, use_padding, (RulerWidget *) (MW_RULER));
}

/**
 * Gets pixels from the position, based on the
 * piano_roll ruler.
 */
int
ui_pos_to_px_editor (
  Position *       pos,
  int              use_padding)
{
  if (!MAIN_WINDOW || !EDITOR_RULER)
    return 0;

  return
    pos_to_px (
      pos, use_padding,
      Z_RULER_WIDGET (EDITOR_RULER));
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

  return
    (long)
    (((double) AUDIO_ENGINE->frames_per_tick * px) /
    rw_prv->px_per_tick);
}

/**
 * Converts from pixels to frames.
 *
 * Returns the frames.
 *
 * @param has_padding Whether then given px contains
 *   padding.
 */
long
ui_px_to_frames_timeline (
  double px,
  int    has_padding)
{
  if (!MAIN_WINDOW || !MW_RULER)
    return 0;

  return
    px_to_frames (
      px, has_padding,
      Z_RULER_WIDGET (MW_RULER));
}

/**
 * Converts from pixels to frames.
 *
 * Returns the frames.
 *
 * @param has_padding Whether then given px contains
 *   padding.
 */
long
ui_px_to_frames_editor (
  double px,
  int    has_padding)
{
  if (!MAIN_WINDOW || !EDITOR_RULER)
    return 0;

  return
    px_to_frames (
      px, has_padding,
      Z_RULER_WIDGET (EDITOR_RULER));
}

/**
 * Returns if the child is hit or not by the
 * coordinates in parent.
 *
 * @param check_x Check x-axis for match.
 * @param check_y Check y-axis for match.
 * @param x x in parent space.
 * @param y y in parent space.
 * @param x_padding Padding to add to the x
 *   of the object when checking if hit.
 *   The bigger the padding the more space the
 *   child will have to get hit.
 * @param y_padding Padding to add to the y
 *   of the object when checking if hit.
 *   The bigger the padding the more space the
 *   child will have to get hit.
 */
int
ui_is_child_hit (
  GtkWidget * parent,
  GtkWidget *    child,
  const int            check_x,
  const int            check_y,
  const double         x,
  const double         y,
  const double         x_padding,
  const double         y_padding)
{
  GtkAllocation allocation;
  gtk_widget_get_allocation (
    child,
    &allocation);

  gint wx, wy;
  gtk_widget_translate_coordinates (
    GTK_WIDGET (parent),
    child,
    (int) x, (int) y, &wx, &wy);

  //g_message ("wx wy %d %d", wx, wy);

  /* if hit */
  if ((!check_x ||
        (wx >= - x_padding &&
         wx <= allocation.width + x_padding)) &&
      (!check_y ||
        (wy >= - y_padding &&
         wy <= allocation.height + y_padding)))
    {
      return 1;
    }
  return 0;
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
    UI_NORWEGIAN,
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
    _("Norwegian [nb]"),
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
    AUDIO_BACKEND_ALSA,
    AUDIO_BACKEND_JACK,
    AUDIO_BACKEND_PORT_AUDIO,
  };
  const gchar *labels[NUM_AUDIO_BACKENDS] = {
    /* TRANSLATORS: Dummy audio backend */
    _("Dummy"),
    "ALSA",
    "Jack",
    "PortAudio",
  };

  CREATE_SIMPLE_MODEL_BOILERPLATE;
}
static GtkTreeModel *
ui_create_midi_backends_model (void)
{
  const int values[NUM_MIDI_BACKENDS] = {
    MIDI_BACKEND_DUMMY,
    MIDI_BACKEND_ALSA,
    MIDI_BACKEND_JACK,
  };
  const gchar *labels[NUM_AUDIO_BACKENDS] = {
    /* TRANSLATORS: Dummy audio backend */
    _("Dummy"),
    _("ALSA Sequencer"),
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

/**
 * Returns the contrasting color (variation of
 * black or white) based on if the given color is
 * dark enough or not.
 *
 * @param src The source color.
 * @param dest The desination color to write to.
 */
void
ui_get_contrast_color (
  GdkRGBA * src,
  GdkRGBA * dest)
{
  /* if color is too bright use dark text,
   * otherwise use bright text */
  if (ui_is_color_bright (src))
    *dest = UI_CACHES->dark_text;
  else
    *dest = UI_CACHES->bright_text;
}

/**
 * Returns if the color is bright or not.
 */
int
ui_is_color_bright (
  GdkRGBA * src)
{
  return src->red + src->green + src->blue >= 1.5;
}

/**
 * Returns if the color is very bright or not.
 */
int
ui_is_color_very_bright (
  GdkRGBA * src)
{
  return src->red + src->green + src->blue >= 2.0;
}

/**
 * Used in handlers to get the state mask.
 */
GdkModifierType
ui_get_state_mask (
  GtkGesture * gesture)
{
  GdkEventSequence * _sequence =
    gtk_gesture_single_get_current_sequence (
      GTK_GESTURE_SINGLE (gesture));
  const GdkEvent * _event =
    gtk_gesture_get_last_event (
      GTK_GESTURE (gesture), _sequence);
  GdkModifierType state_mask;
  gdk_event_get_state (_event, &state_mask);
  return state_mask;
}

/**
 * Gets the color the widget should be.
 *
 * @param color The original color.
 * @param is_selected Whether the widget is supposed
 *   to be selected or not.
 */
void
ui_get_arranger_object_color (
  GdkRGBA *    color,
  const int    is_hovered,
  const int    is_selected,
  const int    is_transient)
{
  if (DEBUGGING)
    color->alpha = 0.2;
  else
    color->alpha = is_transient ? 0.7 : 1.0;
  if (is_selected)
    {
      color->red += 0.4;
      color->green += 0.2;
      color->blue += 0.2;
      color->alpha = DEBUGGING ? 0.5 : 1.0;
    }
  else if (is_hovered)
    {
      if (ui_is_color_very_bright (color))
        {
          color->red -= 0.1;
          color->green -= 0.1;
          color->blue -= 0.1;
        }
      else
        {
          color->red += 0.1;
          color->green += 0.1;
          color->blue += 0.1;
        }
    }
}

UiCaches *
ui_caches_new ()
{
  UiCaches * self =
    calloc (1, sizeof (UiCaches));

  gdk_rgba_parse (&self->dark_text, "#323232");
  gdk_rgba_parse (&self->bright_text, "#CDCDCD");

  return self;
}
