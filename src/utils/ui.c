/*
 * Copyright (C) 2018-2022 Alexandros Theodotou <alex at zrythm dot org>
 * Copyright (C) 2020 Ryan Gonzalez <rymg19 at gmail dot com>
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

#include "zrythm-config.h"

#include <math.h>

#include "audio/engine.h"
#include "audio/engine_rtaudio.h"
#include "audio/engine_sdl.h"
#include "audio/pan.h"
#include "audio/port.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/dialogs/bind_cc_dialog.h"
#include "gui/widgets/main_notebook.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/editor_ruler.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline_panel.h"
#include "gui/widgets/timeline_ruler.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/color.h"
#include "utils/gtk.h"
#include "utils/localization.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "utils/ui.h"
#include "zrythm_app.h"

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
  g_return_if_fail (offset_x >= 0 && offset_y >= 0);

  /* check the cache first */
  for (int i = 0; i < UI_CACHES->num_cursors; i++)
    {
      g_return_if_fail (i < UI_MAX_CURSORS);

      UiCursor * cursor =
        &UI_CACHES->cursors[i];
      if (string_is_equal (name, cursor->name)
          && cursor->offset_x == offset_x
          && cursor->offset_y == offset_y)
        {
          gtk_widget_set_cursor (
            widget, cursor->cursor);
          return;
        }
    }

#if 0
  GtkImage * image =
    GTK_IMAGE (
      gtk_image_new_from_icon_name (name));
  gtk_image_set_pixel_size (image, 18);
  gtk_widget_add_css_class (
    GTK_WIDGET (image), "lowres-icon-shadow");
  GtkSnapshot * snapshot =
    gtk_snapshot_new ();
  GdkPaintable * paintable =
    gtk_image_get_paintable (image);
  gdk_paintable_snapshot (
    paintable, snapshot,
    gdk_paintable_get_intrinsic_width (paintable),
    gdk_paintable_get_intrinsic_height (paintable));
  GskRenderNode * node =
    gtk_snapshot_to_node (snapshot);
  graphene_rect_t bounds;
  gsk_render_node_get_bounds (node, &bounds);
  cairo_surface_t * surface =
    cairo_image_surface_create (
      CAIRO_FORMAT_ARGB32,
      (int) bounds.size.width,
      (int) bounds.size.height);
  cairo_t * cr = cairo_create (surface);
  gsk_render_node_draw (node, cr);
  cairo_surface_write_to_png (
    surface, "/tmp/test.png");
#endif
  GdkTexture * texture =
    z_gdk_texture_new_from_icon_name (
      name, 18, 18, 1);
  if (!texture || !GDK_IS_TEXTURE (texture))
    {
      g_warning (
        "no texture for %s", name);
      return;
    }
  int adjusted_offset_x =
    MIN (
      offset_x,
      gdk_texture_get_width (texture) - 1);
  int adjusted_offset_y =
    MIN (
      offset_y,
      gdk_texture_get_height (texture) - 1);
  GdkCursor * gdk_cursor =
    gdk_cursor_new_from_texture (
      texture,
      adjusted_offset_x, adjusted_offset_y,
      NULL);
  g_object_unref (texture);

  /* add the cursor to the caches */
  UiCursor * cursor =
    &UI_CACHES->cursors[UI_CACHES->num_cursors++];
  strcpy (cursor->name, name);
  cursor->cursor = gdk_cursor;
  cursor->offset_x = offset_x;
  cursor->offset_y = offset_y;

  gtk_widget_set_cursor (widget, cursor->cursor);
}

/**
 * Sets cursor from standard cursor name.
 */
void
ui_set_cursor_from_name (
  GtkWidget * widget,
  const char * name)
{
  GdkCursor * cursor =
    gdk_cursor_new_from_name (name, NULL);
  gtk_widget_set_cursor (widget, cursor);
}

void
ui_set_pointer_cursor (
  GtkWidget * widget)
{
  ui_set_cursor_from_icon_name (
    GTK_WIDGET (widget), "edit-select", 3, 1);
}

/**
 * Shows a popup message of the given type with the
 * given message.
 */
void
ui_show_message_full (
  GtkWindow *    parent_window,
  GtkMessageType type,
  bool           block,
  const char *   format,
  ...)
{
  va_list args;
  va_start (args, format);

  static char buf[40000];
  vsprintf (buf, format, args);

  if (ZRYTHM_HAVE_UI)
    {
      GtkDialogFlags flags =
        parent_window ?
          GTK_DIALOG_DESTROY_WITH_PARENT : 0;
      flags |= GTK_DIALOG_MODAL;
      GtkWidget * dialog =
        gtk_message_dialog_new (
          parent_window, flags, type,
          GTK_BUTTONS_CLOSE, "%s", buf);
      gtk_window_set_title (
        GTK_WINDOW (dialog), PROGRAM_NAME);
      gtk_window_set_icon_name (
        GTK_WINDOW (dialog), "zrythm");
      if (parent_window)
        {
          gtk_window_set_transient_for (
            GTK_WINDOW (dialog), parent_window);
        }
      if (block)
        {
          z_gtk_dialog_run (
            GTK_DIALOG (dialog), true);
        }
      else
        {
          gtk_window_present (GTK_WINDOW (dialog));
          g_signal_connect (
            GTK_DIALOG (dialog), "response",
            G_CALLBACK (gtk_window_destroy), NULL);
        }
    }
  else
    {
      switch (type)
        {
        case GTK_MESSAGE_ERROR:
          g_warning ("%s", buf);
          break;
        case GTK_MESSAGE_INFO:
          g_message ("%s", buf);
          break;
        default:
          g_critical ("should not be reached");
          break;
        }
    }

  va_end (args);
}

/**
 * Returns the matching hit child, or NULL.
 *
 * @param x X in parent space.
 * @param y Y in parent space.
 * @param type Type to look for.
 */
GtkWidget *
ui_get_hit_child (
  GtkWidget * parent,
  double      x,
  double      y,
  GType       type)
{
  for (
    GtkWidget * child =
      gtk_widget_get_first_child (parent);
    child != NULL;
    child = gtk_widget_get_next_sibling (child))
    {
      if (!gtk_widget_get_visible (child))
        continue;

      GtkAllocation allocation;
      gtk_widget_get_allocation (
        child,
        &allocation);

      double wx, wy;
      gtk_widget_translate_coordinates (
        GTK_WIDGET (parent),
        GTK_WIDGET (child),
        (int) x, (int) y, &wx, &wy);

      /* if hit */
      if (wx >= 0 &&
          wx <= allocation.width &&
          wy >= 0 &&
          wy <= allocation.height)
        {
          /* if type matches */
          if (G_TYPE_CHECK_INSTANCE_TYPE (
                child, type))
            {
              return child;
            }
        }
    }

  return NULL;
}

NONNULL
static void
px_to_pos (
  double        px,
  Position *    pos,
  bool          use_padding,
  RulerWidget * ruler)
{
  if (use_padding)
    {
      px -= SPACE_BEFORE_START_D;

      /* clamp at 0 */
      if (px < 0.0)
        px = 0.0;
    }

  pos->schema_version = POSITION_SCHEMA_VERSION;
  pos->ticks = px / ruler->px_per_tick;
  position_update_frames_from_ticks (pos);
}

/**
 * Converts from pixels to position.
 *
 * Only works with positive numbers. Negatives will
 * be clamped at 0. If a negative is needed, pass
 * the abs to this function and then change the
 * sign.
 *
 * @param has_padding Whether @ref px contains
 *   padding.
 */
void
ui_px_to_pos_timeline (
  double     px,
  Position * pos,
  bool       has_padding)
{
  if (!MAIN_WINDOW || !MW_RULER)
    return;

  px_to_pos (
    px, pos, has_padding,
    Z_RULER_WIDGET (MW_RULER));
}


/**
 * Converts from pixels to position.
 *
 * Only works with positive numbers. Negatives will
 * be clamped at 0. If a negative is needed, pass
 * the abs to this function and then change the
 * sign.
 *
 * @param has_padding Whether @ref px contains
 *   padding.
 */
void
ui_px_to_pos_editor (
  double     px,
  Position * pos,
  bool       has_padding)
{
  if (!MAIN_WINDOW || !EDITOR_RULER)
    return;

  px_to_pos (
    px, pos, has_padding,
    Z_RULER_WIDGET (EDITOR_RULER));
}

PURE
NONNULL
static inline int
pos_to_px (
  Position *       pos,
  int              use_padding,
  RulerWidget *    ruler)
{
  int px =
    (int)
    (pos->ticks * ruler->px_per_tick);

  if (use_padding)
    px += SPACE_BEFORE_START;

  return px;
}

/**
 * Converts position to px, optionally adding the
 * ruler padding.
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
  bool             use_padding)
{
  if (!MAIN_WINDOW || !EDITOR_RULER)
    return 0;

  return
    pos_to_px (
      pos, use_padding,
      Z_RULER_WIDGET (EDITOR_RULER));
}

/**
 * @param has_padding Whether the given px contains
 *   padding.
 */
static signed_frame_t
px_to_frames (
  double        px,
  int           has_padding,
  RulerWidget * ruler)
{
  if (has_padding)
    {
      px -= SPACE_BEFORE_START;

      /* clamp at 0 */
      if (px < 0.0)
        px = 0.0;
    }

  return
    (signed_frame_t)
    (((double) AUDIO_ENGINE->frames_per_tick * px) /
    ruler->px_per_tick);
}

/**
 * Converts from pixels to frames.
 *
 * Returns the frames.
 *
 * @param has_padding Whether then given px contains
 *   padding.
 */
signed_frame_t
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
signed_frame_t
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
 * Returns if \ref rect is hit or not by the
 * given coordinate.
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
bool
ui_is_point_in_rect_hit (
  GdkRectangle * rect,
  const bool     check_x,
  const bool     check_y,
  double         x,
  double         y,
  double         x_padding,
  double         y_padding)
{
  /* make coordinates local to the rect */
  x -= rect->x;
  y -= rect->y;

  /* if hit */
  if ((!check_x ||
        (x >= - x_padding &&
         x <= rect->width + x_padding)) &&
      (!check_y ||
        (y >= - y_padding &&
         y <= rect->height + y_padding)))
    {
      return true;
    }
  return false;
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

  double wx, wy;
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

#if 0
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
#endif

/**
 * Shows a notification in the revealer.
 */
void
ui_show_notification (const char * msg)
{
  AdwToast * toast = adw_toast_new (msg);
  adw_toast_overlay_add_toast (
    MAIN_WINDOW->toast_overlay, toast);
#if 0
  gtk_label_set_text (MAIN_WINDOW->notification_label,
                      msg);
  gtk_revealer_set_reveal_child (
    GTK_REVEALER (MAIN_WINDOW->revealer),
    1);
  g_timeout_add_seconds (
    3, (GSourceFunc) hide_notification_async, NULL);
#endif
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
 * Converts RGB to hex string.
 */
void
ui_rgb_to_hex (
  double red,
  double green,
  double blue,
  char * buf)
{
  sprintf (
    buf, "#%hhx%hhx%hhx",
    (char) (red * 255.0),
    (char) (green * 255.0),
    (char) (blue * 255.0));
}

void
ui_gdk_rgba_to_hex (
  GdkRGBA * color,
  char *    buf)
{
  ui_rgb_to_hex (
    color->red, color->green, color->blue, buf);
}

#define CREATE_SIMPLE_MODEL_BOILERPLATE \
  enum \
  { \
    VALUE_COL, \
    TEXT_COL, \
    ID_COL, \
  }; \
  GtkTreeIter iter; \
  GtkListStore *store; \
  gint i; \
 \
  store = \
  gtk_list_store_new (3, \
         G_TYPE_INT, \
         G_TYPE_STRING, \
  G_TYPE_STRING); \
 \
  int num_elements = G_N_ELEMENTS (values); \
  for (i = 0; i < num_elements; i++) \
    { \
      gtk_list_store_append (store, &iter); \
      char id[40]; \
      sprintf (id, "%d", values[i]); \
      gtk_list_store_set (store, &iter, \
                          VALUE_COL, values[i], \
                          TEXT_COL, labels[i], \
                          ID_COL, id, \
                          -1); \
    } \
 \
  return GTK_TREE_MODEL (store);

#if 0
/**
 * Creates and returns a language model for combo
 * boxes.
 */
static GtkTreeModel *
ui_create_language_model ()
{
  int values[NUM_LL_LANGUAGES];
  const char * labels[NUM_LL_LANGUAGES];
  for (int i = 0; i < NUM_LL_LANGUAGES; i++)
    {
      values[i] = i;
      labels[i] =
        localization_get_string_w_code (i);
    }

  CREATE_SIMPLE_MODEL_BOILERPLATE;
}
#endif

static GtkTreeModel *
ui_create_audio_backends_model (void)
{
  const int values[] = {
    AUDIO_BACKEND_DUMMY,
#ifdef HAVE_LIBSOUNDIO
    AUDIO_BACKEND_DUMMY_LIBSOUNDIO,
#endif
#ifdef HAVE_ALSA
    AUDIO_BACKEND_ALSA,
  #ifdef HAVE_LIBSOUNDIO
    AUDIO_BACKEND_ALSA_LIBSOUNDIO,
  #endif
  #ifdef HAVE_RTAUDIO
    AUDIO_BACKEND_ALSA_RTAUDIO,
  #endif
#endif /* HAVE_ALSA */
#ifdef HAVE_JACK
    AUDIO_BACKEND_JACK,
  #ifdef HAVE_LIBSOUNDIO
    AUDIO_BACKEND_JACK_LIBSOUNDIO,
  #endif
  #ifdef HAVE_RTAUDIO
    AUDIO_BACKEND_JACK_RTAUDIO,
  #endif
#endif /* HAVE_JACK */
#ifdef HAVE_PULSEAUDIO
    AUDIO_BACKEND_PULSEAUDIO,
  #ifdef HAVE_LIBSOUNDIO
    AUDIO_BACKEND_PULSEAUDIO_LIBSOUNDIO,
  #endif
  #ifdef HAVE_RTAUDIO
    AUDIO_BACKEND_PULSEAUDIO_RTAUDIO,
  #endif
#endif /* HAVE_PULSEAUDIO */
#ifdef __APPLE__
  #ifdef HAVE_LIBSOUNDIO
    AUDIO_BACKEND_COREAUDIO_LIBSOUNDIO,
  #endif
  #ifdef HAVE_RTAUDIO
    AUDIO_BACKEND_COREAUDIO_RTAUDIO,
  #endif
#endif /* __APPLE__ */
#ifdef HAVE_SDL
    AUDIO_BACKEND_SDL,
#endif
#ifdef _WOE32
  #ifdef HAVE_LIBSOUNDIO
    AUDIO_BACKEND_WASAPI_LIBSOUNDIO,
  #endif
  #ifdef HAVE_RTAUDIO
    AUDIO_BACKEND_WASAPI_RTAUDIO,
    AUDIO_BACKEND_ASIO_RTAUDIO,
  #endif
#endif /* _WOE32 */
  };
  const gchar *labels[] = {
    _(audio_backend_str[AUDIO_BACKEND_DUMMY]),
#ifdef HAVE_LIBSOUNDIO
    _(audio_backend_str[AUDIO_BACKEND_DUMMY_LIBSOUNDIO]),
#endif
#ifdef HAVE_ALSA
    _(audio_backend_str[AUDIO_BACKEND_ALSA]),
  #ifdef HAVE_LIBSOUNDIO
    _(audio_backend_str[AUDIO_BACKEND_ALSA_LIBSOUNDIO]),
  #endif
  #ifdef HAVE_RTAUDIO
    _(audio_backend_str[AUDIO_BACKEND_ALSA_RTAUDIO]),
  #endif
#endif /* HAVE_ALSA */
#ifdef HAVE_JACK
    _(audio_backend_str[AUDIO_BACKEND_JACK]),
  #ifdef HAVE_LIBSOUNDIO
    _(audio_backend_str[AUDIO_BACKEND_JACK_LIBSOUNDIO]),
  #endif
  #ifdef HAVE_RTAUDIO
    _(audio_backend_str[AUDIO_BACKEND_JACK_RTAUDIO]),
  #endif
#endif /* HAVE_JACK */
#ifdef HAVE_PULSEAUDIO
    _(audio_backend_str[AUDIO_BACKEND_PULSEAUDIO]),
  #ifdef HAVE_LIBSOUNDIO
    _(audio_backend_str[AUDIO_BACKEND_PULSEAUDIO_LIBSOUNDIO]),
  #endif
  #ifdef HAVE_RTAUDIO
    _(audio_backend_str[AUDIO_BACKEND_PULSEAUDIO_RTAUDIO]),
  #endif
#endif /* HAVE_PULSEAUDIO */
#ifdef __APPLE__
  #ifdef HAVE_LIBSOUNDIO
    _(audio_backend_str[AUDIO_BACKEND_COREAUDIO_LIBSOUNDIO]),
  #endif
  #ifdef HAVE_RTAUDIO
    _(audio_backend_str[AUDIO_BACKEND_COREAUDIO_RTAUDIO]),
  #endif
#endif /* __APPLE__ */
#ifdef HAVE_SDL
    _(audio_backend_str[AUDIO_BACKEND_SDL]),
#endif
#ifdef _WOE32
  #ifdef HAVE_LIBSOUNDIO
    _(audio_backend_str[AUDIO_BACKEND_WASAPI_LIBSOUNDIO]),
  #endif
  #ifdef HAVE_RTAUDIO
    _(audio_backend_str[AUDIO_BACKEND_WASAPI_RTAUDIO]),
    _(audio_backend_str[AUDIO_BACKEND_ASIO_RTAUDIO]),
  #endif
#endif /* _WOE32 */
  };

  CREATE_SIMPLE_MODEL_BOILERPLATE;
}
static GtkTreeModel *
ui_create_midi_backends_model (void)
{
  const int values[] = {
    MIDI_BACKEND_DUMMY,
#ifdef HAVE_ALSA
    MIDI_BACKEND_ALSA,
  #ifdef HAVE_RTMIDI
    MIDI_BACKEND_ALSA_RTMIDI,
  #endif
#endif
#ifdef HAVE_JACK
    MIDI_BACKEND_JACK,
  #ifdef HAVE_RTMIDI
    MIDI_BACKEND_JACK_RTMIDI,
  #endif
#endif
#ifdef _WOE32
    MIDI_BACKEND_WINDOWS_MME,
  #ifdef HAVE_RTMIDI
    MIDI_BACKEND_WINDOWS_MME_RTMIDI,
  #endif
#endif
#ifdef __APPLE__
  #ifdef HAVE_RTMIDI
    MIDI_BACKEND_COREMIDI_RTMIDI,
  #endif
#endif
  };
  const gchar * labels[] = {
    _(midi_backend_str[MIDI_BACKEND_DUMMY]),
#ifdef HAVE_ALSA
    _(midi_backend_str[MIDI_BACKEND_ALSA]),
  #ifdef HAVE_RTMIDI
    _(midi_backend_str[MIDI_BACKEND_ALSA_RTMIDI]),
  #endif
#endif
#ifdef HAVE_JACK
    _(midi_backend_str[MIDI_BACKEND_JACK]),
  #ifdef HAVE_RTMIDI
    _(midi_backend_str[MIDI_BACKEND_JACK_RTMIDI]),
  #endif
#endif
#ifdef _WOE32
    _(midi_backend_str[MIDI_BACKEND_WINDOWS_MME]),
  #ifdef HAVE_RTMIDI
    _(midi_backend_str[MIDI_BACKEND_WINDOWS_MME_RTMIDI]),
  #endif
#endif
#ifdef __APPLE__
  #ifdef HAVE_RTMIDI
    _(midi_backend_str[MIDI_BACKEND_COREMIDI_RTMIDI]),
  #endif
#endif
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

static GtkTreeModel *
ui_create_buffer_size_model (void)
{
  const int values[NUM_AUDIO_ENGINE_BUFFER_SIZES] = {
    AUDIO_ENGINE_BUFFER_SIZE_16,
    AUDIO_ENGINE_BUFFER_SIZE_32,
    AUDIO_ENGINE_BUFFER_SIZE_64,
    AUDIO_ENGINE_BUFFER_SIZE_128,
    AUDIO_ENGINE_BUFFER_SIZE_256,
    AUDIO_ENGINE_BUFFER_SIZE_512,
    AUDIO_ENGINE_BUFFER_SIZE_1024,
    AUDIO_ENGINE_BUFFER_SIZE_2048,
    AUDIO_ENGINE_BUFFER_SIZE_4096,
  };
  const gchar *labels[NUM_AUDIO_ENGINE_BUFFER_SIZES] = {
    "16", "32", "64", "128", "256", "512",
    "1024", "2048", "4096",
  };

  CREATE_SIMPLE_MODEL_BOILERPLATE;
}

static GtkTreeModel *
ui_create_samplerate_model (void)
{
  const int values[NUM_AUDIO_ENGINE_SAMPLERATES] = {
    AUDIO_ENGINE_SAMPLERATE_22050,
    AUDIO_ENGINE_SAMPLERATE_32000,
    AUDIO_ENGINE_SAMPLERATE_44100,
    AUDIO_ENGINE_SAMPLERATE_48000,
    AUDIO_ENGINE_SAMPLERATE_88200,
    AUDIO_ENGINE_SAMPLERATE_96000,
    AUDIO_ENGINE_SAMPLERATE_192000,
  };
  const gchar *labels[NUM_AUDIO_ENGINE_BUFFER_SIZES] = {
    "22050", "32000", "44100", "48000",
    "88200", "96000", "192000",
  };

  CREATE_SIMPLE_MODEL_BOILERPLATE;
}

/**
 * Sets up a combo box to have a selection of
 * languages.
 */
void
ui_setup_language_dropdown (
  GtkDropDown * dropdown)
{
  GtkStringList * string_list =
    gtk_string_list_new (NULL);
  const char ** lang_strings_w_codes =
    localization_get_language_strings_w_codes ();
  for (size_t i = 0; i < NUM_LL_LANGUAGES; i++)
    {
      gtk_string_list_append (
        string_list, lang_strings_w_codes[i]);
    }
  gtk_drop_down_set_model (
    dropdown, G_LIST_MODEL (string_list));

  int active_lang =
    g_settings_get_enum (
      S_P_UI_GENERAL, "language");
  gtk_drop_down_set_selected (
    dropdown, (unsigned int) active_lang);
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

  char id[40];
  sprintf (id, "%d",
    g_settings_get_enum (
      S_P_GENERAL_ENGINE,
      "audio-backend"));
  gtk_combo_box_set_active_id (
    GTK_COMBO_BOX (cb), id);
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

  char id[40];
  sprintf (id, "%d",
    g_settings_get_enum (
      S_P_GENERAL_ENGINE,
      "midi-backend"));
  gtk_combo_box_set_active_id (
    GTK_COMBO_BOX (cb), id);
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
      S_P_DSP_PAN,
      "pan-algorithm"));
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
      S_P_DSP_PAN,
      "pan-law"));
}

/**
 * Returns the "a locale for the language you have
 * selected..." text based on the given language.
 *
 * Must be free'd by caller.
 */
char *
ui_get_locale_not_available_string (
  LocalizationLanguage lang)
{
  /* show warning */
#ifdef _WOE32
  char template =
    _("A locale for the language you have \
selected (%s) is not available. Please install one first \
and restart %s");
#else
  char * template =
    _("A locale for the language you have selected is \
not available. Please enable one first using \
the steps below and try again.\n\
1. Uncomment any locale starting with the \
language code <b>%s</b> in <b>/etc/locale.gen</b> (needs \
root privileges)\n\
2. Run <b>locale-gen</b> as root\n\
3. Restart %s");
#endif

  const char * code =
    localization_get_string_code (lang);
  char * str =
    g_strdup_printf (template, code, PROGRAM_NAME);

  return str;
}

/**
 * Sets up a pan law combo box.
 */
void
ui_setup_buffer_size_combo_box (
  GtkComboBox * cb)
{
  z_gtk_configure_simple_combo_box (
    cb, ui_create_buffer_size_model ());

  char id[40];
  sprintf (id, "%d",
    g_settings_get_enum (
      S_P_GENERAL_ENGINE,
      "buffer-size"));
  gtk_combo_box_set_active_id (
    GTK_COMBO_BOX (cb), id);
}

/**
 * Sets up a pan law combo box.
 */
void
ui_setup_samplerate_combo_box (
  GtkComboBox * cb)
{
  z_gtk_configure_simple_combo_box (
    cb, ui_create_samplerate_model ());

  char id[40];
  sprintf (id, "%d",
    g_settings_get_enum (
      S_P_GENERAL_ENGINE,
      "sample-rate"));
  gtk_combo_box_set_active_id (
    GTK_COMBO_BOX (cb), id);
}

/**
 * Sets up a pan law combo box.
 */
void
ui_setup_device_name_combo_box (
  GtkComboBoxText * cb)
{
  AudioBackend backend =
    (AudioBackend)
    g_settings_get_enum (
      S_P_GENERAL_ENGINE, "audio-backend");

  gtk_combo_box_text_remove_all (cb);

#define SETUP_DEVICES(type) \
  { \
    char * names[1024]; \
    int num_names; \
    engine_##type##_get_device_names ( \
      AUDIO_ENGINE, 0, names, &num_names); \
    for (int i = 0; i < num_names; i++) \
      { \
        gtk_combo_box_text_append ( \
          cb, NULL, names[i]); \
      } \
    char * current_device = \
      g_settings_get_string ( \
        S_P_GENERAL_ENGINE, \
        #type "-audio-device-name"); \
    for (int i = 0; i < num_names; i++) \
      { \
        if (string_is_equal ( \
              names[i], current_device)) \
          { \
            gtk_combo_box_set_active ( \
              GTK_COMBO_BOX (cb), i); \
          } \
        g_free (names[i]); \
      } \
    g_free (current_device); \
  }

  switch (backend)
    {
#ifdef HAVE_SDL
    case AUDIO_BACKEND_SDL:
      SETUP_DEVICES (sdl);
      break;
#endif
#ifdef HAVE_RTAUDIO
    case AUDIO_BACKEND_ALSA_RTAUDIO:
    case AUDIO_BACKEND_JACK_RTAUDIO:
    case AUDIO_BACKEND_PULSEAUDIO_RTAUDIO:
    case AUDIO_BACKEND_COREAUDIO_RTAUDIO:
    case AUDIO_BACKEND_WASAPI_RTAUDIO:
    case AUDIO_BACKEND_ASIO_RTAUDIO:
      SETUP_DEVICES (rtaudio);
      break;
#endif
    default:
      break;
    }
}

/**
 * Sets up the VST paths entry.
 */
void
ui_setup_vst_paths_entry (
  GtkEntry * entry)
{
  char ** paths =
    g_settings_get_strv (
      S_P_PLUGINS_PATHS,
      "vst-search-paths-windows");
  g_return_if_fail (paths);

  int path_idx = 0;
  char * path;
  char delimited_paths[6000];
  delimited_paths[0] = '\0';
  while ((path = paths[path_idx++]) != NULL)
    {
      strcat (delimited_paths, path);
      strcat (delimited_paths, ";");
    }
  delimited_paths[strlen (delimited_paths) - 1] =
    '\0';

  gtk_editable_set_text (
    GTK_EDITABLE (entry), delimited_paths);
}

/**
 * Updates the the VST paths in the gsettings from
 * the text in the entry.
 */
void
ui_update_vst_paths_from_entry (
  GtkEntry * entry)
{
  const char * txt =
    gtk_editable_get_text (GTK_EDITABLE (entry));
  g_return_if_fail (txt);
  char ** paths =
    g_strsplit (txt, ";", 0);
  g_settings_set_strv (
    S_P_PLUGINS_PATHS, "vst-search-paths-windows",
    (const char * const *) paths);
  g_free (paths);
}

/**
 * Returns the contrasting color (variation of
 * black or white) based on if the given color is
 * dark enough or not.
 *
 * @param src The source color.
 * @param dest The destination color to write to.
 */
void
ui_get_contrast_color (
  GdkRGBA * src,
  GdkRGBA * dest)
{
  /* if color is too bright use dark text,
   * otherwise use bright text */
  if (color_is_bright (src))
    *dest = UI_COLORS->dark_text;
  else
    *dest = UI_COLORS->bright_text;
}

/**
 * Returns the color in-between two colors.
 *
 * @param transition How far to transition (0.5 for
 *   half).
 */
void
ui_get_mid_color (
  GdkRGBA * dest,
  const GdkRGBA * c1,
  const GdkRGBA * c2,
  const float     transition)
{
  dest->red =
    c1->red * transition +
    c2->red * (1.f - transition);
  dest->green =
    c1->green * transition +
    c2->green * (1.f - transition);
  dest->blue =
    c1->blue * transition +
    c2->blue * (1.f - transition);
  dest->alpha =
    c1->alpha * transition +
    c2->alpha * (1.f - transition);
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
  const bool   is_hovered,
  const bool   is_selected,
  const bool   is_transient,
  const bool   is_muted)
{
  if (DEBUGGING)
    color->alpha = 0.f;
  else
    color->alpha = is_transient ? 0.7f : 1.f;
  if (is_muted)
    {
      color->red = 0.6f;
      color->green = 0.6f;
      color->blue = 0.6f;
    }
  if (is_selected)
    {
      color->red += is_muted ? 0.2f : 0.4f;
      color->green += 0.2f;
      color->blue += 0.2f;
      color->alpha = DEBUGGING ? 0.5f : 1.f;
    }
  else if (is_hovered)
    {
      if (color_is_very_bright (color))
        {
          color->red -= 0.1f;
          color->green -= 0.1f;
          color->blue -= 0.1f;
        }
      else
        {
          color->red += 0.1f;
          color->green += 0.1f;
          color->blue += 0.1f;
        }
    }
}

/**
 * Gets a draggable value as a normalized value
 * between 0 and 1.
 *
 * @param size Widget size (either width or height).
 * @param start_px Px at start of drag.
 * @param cur_px Current px.
 * @param last_px Px during last call.
 */
double
ui_get_normalized_draggable_value (
  double       size,
  double       cur_val,
  double       start_px,
  double       cur_px,
  double       last_px,
  double       multiplier,
  UiDragMode   mode)
{
  switch (mode)
    {
    case UI_DRAG_MODE_CURSOR:
      return CLAMP (cur_px / size, 0.0, 1.0);
    case UI_DRAG_MODE_RELATIVE:
      return
        CLAMP (
          cur_val + (cur_px - last_px) / size,
          0.0, 1.0);
    case UI_DRAG_MODE_RELATIVE_WITH_MULTIPLIER:
      return
        CLAMP (
          cur_val +
          (multiplier * (cur_px - last_px)) / size,
          0.0, 1.0);
    }

  g_return_val_if_reached (0.0);
}

UiDetail
ui_get_detail_level (void)
{
  if (!UI_CACHES->detail_level_set)
    {
      UI_CACHES->detail_level =
        (UiDetail)
        g_settings_get_enum (
          S_P_UI_GENERAL, "graphic-detail");
      UI_CACHES->detail_level_set = true;
    }

  return UI_CACHES->detail_level;
}

UiCaches *
ui_caches_new ()
{
  UiCaches * self = object_new (UiCaches);

  GtkWidget * widget =
    gtk_drawing_area_new ();
  widget = g_object_ref_sink (widget);
  GtkStyleContext * context =
    gtk_widget_get_style_context (widget);

  UiColors * colors = &self->colors;
  bool ret;

#define GET_COLOR_FROM_THEME(cname) \
  ret = \
    gtk_style_context_lookup_color ( \
      context, #cname, \
      &colors->cname); \
  g_return_val_if_fail (ret, NULL)

  GET_COLOR_FROM_THEME (bright_green);
  GET_COLOR_FROM_THEME (darkish_green);
  GET_COLOR_FROM_THEME (dark_orange);
  GET_COLOR_FROM_THEME (bright_orange);
  GET_COLOR_FROM_THEME (matcha);
  GET_COLOR_FROM_THEME (prefader_send);
  GET_COLOR_FROM_THEME (postfader_send);
  GET_COLOR_FROM_THEME (solo_active);
  GET_COLOR_FROM_THEME (solo_checked);
  GET_COLOR_FROM_THEME (fader_fill_start);
  GET_COLOR_FROM_THEME (fader_fill_end);
  GET_COLOR_FROM_THEME (highlight_scale_bg);
  GET_COLOR_FROM_THEME (highlight_chord_bg);
  GET_COLOR_FROM_THEME (highlight_bass_bg);
  GET_COLOR_FROM_THEME (highlight_both_bg);
  GET_COLOR_FROM_THEME (highlight_scale_fg);
  GET_COLOR_FROM_THEME (highlight_chord_fg);
  GET_COLOR_FROM_THEME (highlight_bass_fg);
  GET_COLOR_FROM_THEME (highlight_both_fg);
  GET_COLOR_FROM_THEME (z_yellow);
  GET_COLOR_FROM_THEME (z_purple);

#undef GET_COLOR_FROM_THEME

  g_object_unref (widget);

  gdk_rgba_parse (
    &colors->dark_text, UI_COLOR_DARK_TEXT);
  gdk_rgba_parse (
    &colors->bright_text, UI_COLOR_BRIGHT_TEXT);
  gdk_rgba_parse (
    &colors->record_active, UI_COLOR_RECORD_ACTIVE);
  gdk_rgba_parse (
    &colors->record_checked,
    UI_COLOR_RECORD_CHECKED);

  return self;
}

void
ui_caches_free (
  UiCaches * self)
{
  for (int i = 0; i < self->num_cursors; i++)
    {
      UiCursor * cursor = &self->cursors[i];

      g_object_unref (cursor->cursor);
    }

  object_zero_and_free (self);
}
