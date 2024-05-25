// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: © 2020 Ryan Gonzalez <rymg19 at gmail dot com>
// SPDX-FileCopyrightText: © 2024 Miró Allard <miro.allard@pm.me>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include <math.h>

#include "dsp/engine.h"
#include "dsp/engine_rtaudio.h"
#include "dsp/engine_sdl.h"
#include "dsp/pan.h"
#include "dsp/port.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/dialogs/bind_cc_dialog.h"
#include "gui/widgets/editor_ruler.h"
#include "gui/widgets/main_notebook.h"
#include "gui/widgets/main_window.h"
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

      UiCursor * cursor = &UI_CACHES->cursors[i];
      if (
        string_is_equal (name, cursor->name) && cursor->offset_x == offset_x
        && cursor->offset_y == offset_y)
        {
          gtk_widget_set_cursor (widget, cursor->cursor);
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
  GdkTexture * texture = z_gdk_texture_new_from_icon_name (name, 24, 24, 1);
  if (!texture || !GDK_IS_TEXTURE (texture))
    {
      g_warning ("no texture for %s", name);
      return;
    }
  int adjusted_offset_x = MIN (offset_x, gdk_texture_get_width (texture) - 1);
  int adjusted_offset_y = MIN (offset_y, gdk_texture_get_height (texture) - 1);
  GdkCursor * gdk_cursor = gdk_cursor_new_from_texture (
    texture, adjusted_offset_x, adjusted_offset_y, NULL);
  g_object_unref (texture);

  /* add the cursor to the caches */
  UiCursor * cursor = &UI_CACHES->cursors[UI_CACHES->num_cursors++];
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
ui_set_cursor_from_name (GtkWidget * widget, const char * name)
{
  GdkCursor * cursor = gdk_cursor_new_from_name (name, NULL);
  gtk_widget_set_cursor (widget, cursor);
}

void
ui_set_pointer_cursor (GtkWidget * widget)
{
  ui_set_cursor_from_icon_name (GTK_WIDGET (widget), "select-cursor", 3, 1);
}

AdwDialog *
ui_show_message_full (
  GtkWidget *  parent,
  const char * title,
  const char * format,
  ...)
{
  va_list args;
  va_start (args, format);

#define UI_MESSAGE_BUF_SZ 2000
  char * buf = object_new_n (UI_MESSAGE_BUF_SZ, char);
  int    printed = vsnprintf (buf, UI_MESSAGE_BUF_SZ, format, args);
  if (printed == -1)
    {
      g_warning ("vsnprintf failed");
      free (buf);
      va_end (args);
      return NULL;
    }
  g_return_val_if_fail (printed < UI_MESSAGE_BUF_SZ, NULL);
  buf = static_cast<char *> (
    g_realloc_n (buf, (size_t) (printed + 1), sizeof (char)));

  /* log the message anyway */
  g_message ("%s: %s", title, buf);

  /* if have UI, also show a message dialog */
  AdwDialog * win = NULL;
  if (ZRYTHM_HAVE_UI)
    {
      AdwAlertDialog * dialog =
        ADW_ALERT_DIALOG (adw_alert_dialog_new (title, buf));
      adw_alert_dialog_add_responses (dialog, "ok", _ ("_OK"), NULL);
      adw_alert_dialog_set_default_response (dialog, "ok");
      adw_alert_dialog_set_response_appearance (
        dialog, "ok", ADW_RESPONSE_SUGGESTED);
      adw_alert_dialog_set_close_response (dialog, "ok");
      adw_dialog_present (ADW_DIALOG (dialog), parent);
      win = ADW_DIALOG (dialog);
    }

  free (buf);
  va_end (args);

  return win;
}

/**
 * Returns the matching hit child, or NULL.
 *
 * @param x X in parent space.
 * @param y Y in parent space.
 * @param type Type to look for.
 */
GtkWidget *
ui_get_hit_child (GtkWidget * parent, double x, double y, GType type)
{
  for (
    GtkWidget * child = gtk_widget_get_first_child (parent); child != NULL;
    child = gtk_widget_get_next_sibling (child))
    {
      if (!gtk_widget_get_visible (child))
        continue;

      graphene_point_t wpt;
      graphene_point_t tmp_pt = GRAPHENE_POINT_INIT ((float) x, (float) y);
      bool             success = gtk_widget_compute_point (
        GTK_WIDGET (parent), GTK_WIDGET (child), &tmp_pt, &wpt);
      g_return_val_if_fail (success, NULL);

      /* if hit */
      int width = gtk_widget_get_width (GTK_WIDGET (child));
      int height = gtk_widget_get_height (GTK_WIDGET (child));
      if (wpt.x >= 0 && wpt.x <= width && wpt.y >= 0 && wpt.y <= height)
        {
          /* if type matches */
          if (G_TYPE_CHECK_INSTANCE_TYPE (child, type))
            {
              return child;
            }
        }
    }

  return NULL;
}

NONNULL static void
px_to_pos (double px, Position * pos, bool use_padding, RulerWidget * ruler)
{
  if (use_padding)
    {
      px -= SPACE_BEFORE_START_D;

      /* clamp at 0 */
      if (px < 0.0)
        px = 0.0;
    }

  pos->ticks = px / ruler->px_per_tick;
  position_update_frames_from_ticks (pos, 0.0);
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
ui_px_to_pos_timeline (double px, Position * pos, bool has_padding)
{
  if (!MAIN_WINDOW || !MW_RULER)
    return;

  px_to_pos (px, pos, has_padding, Z_RULER_WIDGET (MW_RULER));
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
ui_px_to_pos_editor (double px, Position * pos, bool has_padding)
{
  if (!MAIN_WINDOW || !EDITOR_RULER)
    return;

  px_to_pos (px, pos, has_padding, Z_RULER_WIDGET (EDITOR_RULER));
}

PURE NONNULL static inline int
pos_to_px (const Position * pos, int use_padding, RulerWidget * ruler)
{
  int px = (int) (pos->ticks * ruler->px_per_tick);

  if (use_padding)
    px += SPACE_BEFORE_START;

  return px;
}

/**
 * Converts position to px, optionally adding the
 * ruler padding.
 */
int
ui_pos_to_px_timeline (const Position * pos, int use_padding)
{
  if (!MAIN_WINDOW || !MW_RULER)
    return 0;

  return pos_to_px (pos, use_padding, (RulerWidget *) (MW_RULER));
}

/**
 * Gets pixels from the position, based on the
 * piano_roll ruler.
 */
int
ui_pos_to_px_editor (const Position * pos, bool use_padding)
{
  if (!MAIN_WINDOW || !EDITOR_RULER)
    return 0;

  return pos_to_px (pos, use_padding, Z_RULER_WIDGET (EDITOR_RULER));
}

/**
 * @param has_padding Whether the given px contains
 *   padding.
 */
static signed_frame_t
px_to_frames (double px, bool has_padding, RulerWidget * ruler)
{
  if (has_padding)
    {
      px -= SPACE_BEFORE_START;

      /* clamp at 0 */
      if (px < 0.0)
        px = 0.0;
    }

  return (
    signed_frame_t) (((double) AUDIO_ENGINE->frames_per_tick * px)
                     / ruler->px_per_tick);
}

signed_frame_t
ui_px_to_frames_timeline (double px, bool has_padding)
{
  if (!MAIN_WINDOW || !MW_RULER)
    return 0;

  return px_to_frames (px, has_padding, Z_RULER_WIDGET (MW_RULER));
}

signed_frame_t
ui_px_to_frames_editor (double px, bool has_padding)
{
  if (!MAIN_WINDOW || !EDITOR_RULER)
    return 0;

  return px_to_frames (px, has_padding, Z_RULER_WIDGET (EDITOR_RULER));
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
  if (
    (!check_x || (x >= -x_padding && x <= rect->width + x_padding))
    && (!check_y || (y >= -y_padding && y <= rect->height + y_padding)))
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
  GtkWidget *  parent,
  GtkWidget *  child,
  const int    check_x,
  const int    check_y,
  const double x,
  const double y,
  const double x_padding,
  const double y_padding)
{
  graphene_point_t wpt;
  graphene_point_t tmp_pt = GRAPHENE_POINT_INIT ((float) x, (float) y);
  bool             success = gtk_widget_compute_point (
    GTK_WIDGET (parent), GTK_WIDGET (child), &tmp_pt, &wpt);
  g_return_val_if_fail (success, -1);

  // g_message ("wpt.x wpt.y %d %d", wpt.x, wpt.y);

  /* if hit */
  int width = gtk_widget_get_width (GTK_WIDGET (child));
  int height = gtk_widget_get_height (GTK_WIDGET (child));
  if (
    (!check_x || (wpt.x >= -x_padding && wpt.x <= width + x_padding))
    && (!check_y || (wpt.y >= -y_padding && wpt.y <= height + y_padding)))
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
  if (MAIN_WINDOW)
    {
      adw_toast_overlay_add_toast (MAIN_WINDOW->toast_overlay, toast);
    }
    /* TODO: add toast overlay in greeter too */
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
ui_rgb_to_hex (double red, double green, double blue, char * buf)
{
  sprintf (
    buf, "#%hhx%hhx%hhx", (char) (red * 255.0), (char) (green * 255.0),
    (char) (blue * 255.0));
}

void
ui_gdk_rgba_to_hex (GdkRGBA * color, char * buf)
{
  ui_rgb_to_hex (color->red, color->green, color->blue, buf);
}

#define CREATE_SIMPLE_MODEL_BOILERPLATE \
  enum \
  { \
    VALUE_COL, \
    TEXT_COL, \
    ID_COL, \
  }; \
  GtkTreeIter    iter; \
  GtkListStore * store; \
  gint           i; \
\
  store = gtk_list_store_new (3, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING); \
\
  int num_elements = G_N_ELEMENTS (values); \
  for (i = 0; i < num_elements; i++) \
    { \
      gtk_list_store_append (store, &iter); \
      char id[40]; \
      sprintf (id, "%d", values[i]); \
      gtk_list_store_set ( \
        store, &iter, VALUE_COL, values[i], TEXT_COL, labels[i], ID_COL, id, \
        -1); \
    } \
\
  return GTK_TREE_MODEL (store);

/**
 * Sets up a combo box to have a selection of
 * languages.
 */
void
ui_setup_language_combo_row (AdwComboRow * combo_row)
{
  GtkStringList * string_list = gtk_string_list_new (NULL);
  const char **   lang_strings_w_codes =
    localization_get_language_strings_w_codes ();
  for (size_t i = 0; i < NUM_LL_LANGUAGES; i++)
    {
      gtk_string_list_append (string_list, lang_strings_w_codes[i]);
    }
  adw_combo_row_set_model (combo_row, G_LIST_MODEL (string_list));

  int active_lang = g_settings_get_enum (S_P_UI_GENERAL, "language");
  adw_combo_row_set_selected (combo_row, (unsigned int) active_lang);
}

static void
on_audio_backend_selected_item_changed (
  GObject *    gobject,
  GParamSpec * pspec,
  gpointer     user_data)
{
  AdwComboRow *     combo_row = ADW_COMBO_ROW (gobject);
  GtkStringObject * selected_item =
    GTK_STRING_OBJECT (adw_combo_row_get_selected_item (combo_row));
  const char * str = gtk_string_object_get_string (selected_item);
  AudioBackend backend = engine_audio_backend_from_string (str);
  g_settings_set_enum (
    S_P_GENERAL_ENGINE, "audio-backend", ENUM_VALUE_TO_INT (backend));
}

/**
 * Generates a combo row for selecting the audio backend.
 *
 * @param with_signal Add a signal to change the backend in
 *   GSettings.
 */
AdwComboRow *
ui_gen_audio_backends_combo_row (bool with_signal)
{
  const gchar * labels[] = {
    audio_backend_str[ENUM_VALUE_TO_INT (AudioBackend::AUDIO_BACKEND_DUMMY)],
#ifdef HAVE_LIBSOUNDIO
    audio_backend_str[ENUM_VALUE_TO_INT (
      AudioBackend::AUDIO_BACKEND_DUMMY_LIBSOUNDIO)],
#endif
#ifdef HAVE_ALSA
  /* broken */
  /*audio_backend_str[ENUM_VALUE_TO_INT(AudioBackend::AUDIO_BACKEND_ALSA],*/
#  ifdef HAVE_LIBSOUNDIO
    audio_backend_str[ENUM_VALUE_TO_INT (
      AudioBackend::AUDIO_BACKEND_ALSA_LIBSOUNDIO)],
#  endif
#  ifdef HAVE_RTAUDIO
    audio_backend_str[ENUM_VALUE_TO_INT (
      AudioBackend::AUDIO_BACKEND_ALSA_RTAUDIO)],
#  endif
#endif /* HAVE_ALSA */
#ifdef HAVE_JACK
    audio_backend_str[ENUM_VALUE_TO_INT (AudioBackend::AUDIO_BACKEND_JACK)],
#  ifdef HAVE_LIBSOUNDIO
    audio_backend_str[ENUM_VALUE_TO_INT (
      AudioBackend::AUDIO_BACKEND_JACK_LIBSOUNDIO)],
#  endif
#  ifdef HAVE_RTAUDIO
  /* unnecessary */
  /*audio_backend_str[ENUM_VALUE_TO_INT(AudioBackend::AUDIO_BACKEND_JACK_RTAUDIO],*/
#  endif
#endif /* HAVE_JACK */
#ifdef HAVE_PULSEAUDIO
  /* use rtaudio version - this has known issues */
  /*audio_backend_str[ENUM_VALUE_TO_INT(AudioBackend::AUDIO_BACKEND_PULSEAUDIO],*/
#  ifdef HAVE_LIBSOUNDIO
    audio_backend_str[ENUM_VALUE_TO_INT (
      AudioBackend::AUDIO_BACKEND_PULSEAUDIO_LIBSOUNDIO)],
#  endif
#  ifdef HAVE_RTAUDIO
    audio_backend_str[ENUM_VALUE_TO_INT (
      AudioBackend::AUDIO_BACKEND_PULSEAUDIO_RTAUDIO)],
#  endif
#endif /* HAVE_PULSEAUDIO */
#ifdef __APPLE__
#  ifdef HAVE_LIBSOUNDIO
    audio_backend_str[ENUM_VALUE_TO_INT (
      AudioBackend::AUDIO_BACKEND_COREAUDIO_LIBSOUNDIO)],
#  endif
#  ifdef HAVE_RTAUDIO
    audio_backend_str[ENUM_VALUE_TO_INT (
      AudioBackend::AUDIO_BACKEND_COREAUDIO_RTAUDIO)],
#  endif
#endif /* __APPLE__ */
#ifdef HAVE_SDL
  /* has issues */
  /*audio_backend_str[ENUM_VALUE_TO_INT(AudioBackend::AUDIO_BACKEND_SDL],*/
#endif
#ifdef _WOE32
#  ifdef HAVE_LIBSOUNDIO
    audio_backend_str[ENUM_VALUE_TO_INT (
      AudioBackend::AUDIO_BACKEND_WASAPI_LIBSOUNDIO)],
#  endif
#  ifdef HAVE_RTAUDIO
    audio_backend_str[ENUM_VALUE_TO_INT (
      AudioBackend::AUDIO_BACKEND_WASAPI_RTAUDIO)],
  /* doesn't work & licensing issues */
  /*audio_backend_str[ENUM_VALUE_TO_INT(AudioBackend::AUDIO_BACKEND_ASIO_RTAUDIO],*/
#  endif
#endif /* _WOE32 */
    NULL,
  };
  GtkStringList * string_list = gtk_string_list_new (labels);
  AdwComboRow *   combo_row = ADW_COMBO_ROW (adw_combo_row_new ());
  adw_combo_row_set_model (combo_row, G_LIST_MODEL (string_list));

  int selected = g_settings_get_enum (S_P_GENERAL_ENGINE, "audio-backend");
  for (size_t i = 0; i < G_N_ELEMENTS (labels); i++)
    {
      if (
        string_is_equal (
          engine_audio_backend_to_string (
            ENUM_INT_TO_VALUE (AudioBackend, selected)),
          labels[i]))
        {
          adw_combo_row_set_selected (combo_row, i);
          break;
        }
    }

  if (with_signal)
    {
      g_signal_connect (
        G_OBJECT (combo_row), "notify::selected-item",
        G_CALLBACK (on_audio_backend_selected_item_changed), NULL);
    }

  return combo_row;
}

static void
on_midi_backend_selected_item_changed (
  GObject *    gobject,
  GParamSpec * pspec,
  gpointer     user_data)
{
  AdwComboRow *     combo_row = ADW_COMBO_ROW (gobject);
  GtkStringObject * selected_item =
    GTK_STRING_OBJECT (adw_combo_row_get_selected_item (combo_row));
  const char * str = gtk_string_object_get_string (selected_item);
  MidiBackend  backend = engine_midi_backend_from_string (str);
  g_settings_set_enum (
    S_P_GENERAL_ENGINE, "midi-backend", ENUM_VALUE_TO_INT (backend));
}

/**
 * Generates a combo row for selecting the MIDI backend.
 *
 * @param with_signal Add a signal to change the backend in
 *   GSettings.
 */
AdwComboRow *
ui_gen_midi_backends_combo_row (bool with_signal)
{
  const gchar * labels[] = {
    midi_backend_str[ENUM_VALUE_TO_INT (MidiBackend::MIDI_BACKEND_DUMMY)],
#ifdef HAVE_ALSA
  /* broken */
  /*midi_backend_str[MidiBackend::MIDI_BACKEND_ALSA],*/
#  ifdef HAVE_RTMIDI
    midi_backend_str[ENUM_VALUE_TO_INT (MidiBackend::MIDI_BACKEND_ALSA_RTMIDI)],
#  endif
#endif
#ifdef HAVE_JACK
    midi_backend_str[ENUM_VALUE_TO_INT (MidiBackend::MIDI_BACKEND_JACK)],
#  ifdef HAVE_RTMIDI
  /* unnecessary */
  /*midi_backend_str[MidiBackend::MIDI_BACKEND_JACK_RTMIDI],*/
#  endif
#endif
#ifdef _WOE32
  /* has known issues - use rtmidi */
  /*midi_backend_str[MidiBackend::MIDI_BACKEND_WINDOWS_MME],*/
#  ifdef HAVE_RTMIDI
    midi_backend_str[ENUM_VALUE_TO_INT (
      MidiBackend::MIDI_BACKEND_WINDOWS_MME_RTMIDI)],
#  endif
#endif /* _WOE32 */
#ifdef __APPLE__
#  ifdef HAVE_RTMIDI
    midi_backend_str[ENUM_VALUE_TO_INT (
      MidiBackend::MIDI_BACKEND_COREMIDI_RTMIDI)],
#  endif
#endif /* __APPLE__ */
#ifdef _WOE32
#  ifdef HAVE_RTMIDI_6
#  endif
    midi_backend_str[ENUM_VALUE_TO_INT (
      MidiBackend::MIDI_BACKEND_WINDOWS_UWP_RTMIDI)],
#endif
    NULL,
  };
  GtkStringList * string_list = gtk_string_list_new (labels);
  AdwComboRow *   combo_row = ADW_COMBO_ROW (adw_combo_row_new ());
  adw_combo_row_set_model (combo_row, G_LIST_MODEL (string_list));

  int selected = g_settings_get_enum (S_P_GENERAL_ENGINE, "midi-backend");
  for (size_t i = 0; i < G_N_ELEMENTS (labels); i++)
    {
      if (
        string_is_equal (
          engine_midi_backend_to_string (
            ENUM_INT_TO_VALUE (MidiBackend, selected)),
          labels[i]))
        {
          adw_combo_row_set_selected (combo_row, i);
          break;
        }
    }

  if (with_signal)
    {
      g_signal_connect (
        G_OBJECT (combo_row), "notify::selected-item",
        G_CALLBACK (on_midi_backend_selected_item_changed), NULL);
    }

  return combo_row;
}

/**
 * Returns the "a locale for the language you have
 * selected..." text based on the given language.
 *
 * Must be free'd by caller.
 */
char *
ui_get_locale_not_available_string (LocalizationLanguage lang)
{
  /* show warning */
#ifdef _WOE32
#  define _TEMPLATE \
    _ ( \
      "A locale for the language you have \
selected (%s) is not available. Please install one first \
and restart %s")
#else
#  define _TEMPLATE \
    _ ( \
      "A locale for the language you have selected is \
not available. Please enable one first using \
the steps below and try again.\n\
1. Uncomment any locale starting with the \
language code <b>%s</b> in <b>/etc/locale.gen</b> (needs \
root privileges)\n\
2. Run <b>locale-gen</b> as root\n\
3. Restart %s")
#endif

  const char * code = localization_get_string_code (lang);
  char *       str = g_strdup_printf (_TEMPLATE, code, PROGRAM_NAME);

#undef _TEMPLATE

  return str;
}

void
ui_show_warning_for_tempo_track_experimental_feature (void)
{
  static bool shown = false;
  if (!shown)
    {
      ui_show_message_literal (
        _ ("Experimental Feature"),
        _ ("BPM and time signature automation is an "
           "experimental feature. Using it may corrupt your "
           "project."));
      shown = true;
    }
}

static void
on_audio_device_selection_changed (
  GObject *    gobject,
  GParamSpec * pspec,
  gpointer     user_data)
{
  const char *      settings_key = (const char *) user_data;
  AdwComboRow *     combo_row = ADW_COMBO_ROW (gobject);
  GtkStringObject * str_obj =
    GTK_STRING_OBJECT (adw_combo_row_get_selected_item (combo_row));
  if (!str_obj)
    {
      g_message ("no selected item");
      return;
    }
  const char * str = gtk_string_object_get_string (str_obj);
  g_settings_set_string (S_P_GENERAL_ENGINE, settings_key, str);
}

/**
 * Sets up a combo row for selecting the audio device name.
 *
 * @param with_signal Add a signal to change the backend in
 *   GSettings.
 */
void
ui_setup_audio_device_name_combo_row (
  AdwComboRow * combo_row,
  bool          populate,
  bool          with_signal)
{
  AudioBackend backend =
    (AudioBackend) g_settings_get_enum (S_P_GENERAL_ENGINE, "audio-backend");

  GtkStringList * string_list = gtk_string_list_new (NULL);
  char *          names[1024];
  int             num_names = 0;
  if (populate)
    {
      if (backend == AudioBackend::AUDIO_BACKEND_SDL)
        {
#ifdef HAVE_SDL
          engine_sdl_get_device_names (AUDIO_ENGINE, 0, names, &num_names);
#endif
        }
      else if (audio_backend_is_rtaudio (backend))
        {
#ifdef HAVE_RTAUDIO
          engine_rtaudio_get_device_names (
            AUDIO_ENGINE, backend, 0, names, &num_names);
#endif
        }
      else
        {
          g_object_unref (string_list);
          return;
        }
      for (int i = 0; i < num_names; i++)
        {
          gtk_string_list_append (string_list, names[i]);
        }
    }
  adw_combo_row_set_model (combo_row, G_LIST_MODEL (string_list));
  const char * settings_key = NULL;
  if (backend == AudioBackend::AUDIO_BACKEND_SDL)
    {
      settings_key = "sdl-audio-device-name";
    }
  else if (audio_backend_is_rtaudio (backend))
    {
      settings_key = "rtaudio-audio-device-name";
    }
  else
    {
      return;
    }
  char * current_device =
    g_settings_get_string (S_P_GENERAL_ENGINE, settings_key);
  bool found = false;
  g_message ("current device from settings: %s", current_device);
  for (int i = 0; i < num_names; i++)
    {
      if (string_is_equal (names[i], current_device))
        {
          g_message ("settings %d as selected", i);
          found = true;
          adw_combo_row_set_selected (combo_row, (guint) i);
        }
      g_free (names[i]);
    }
  if (!found && num_names > 0)
    {
      g_warning ("rtaudio device not found, selecting first");
      adw_combo_row_set_selected (combo_row, 0);
    }
  g_free (current_device);

  if (with_signal)
    {
      g_signal_connect (
        G_OBJECT (combo_row), "notify::selected",
        G_CALLBACK (on_audio_device_selection_changed), (gpointer) settings_key);
    }
}

/**
 * Sets up the VST paths entry.
 */
void
ui_setup_vst_paths_entry (GtkEntry * entry)
{
  char ** paths =
    g_settings_get_strv (S_P_PLUGINS_PATHS, "vst-search-paths-windows");
  g_return_if_fail (paths);

  int    path_idx = 0;
  char * path;
  char   delimited_paths[6000];
  delimited_paths[0] = '\0';
  while ((path = paths[path_idx++]) != NULL)
    {
      strcat (delimited_paths, path);
      strcat (delimited_paths, ";");
    }
  delimited_paths[strlen (delimited_paths) - 1] = '\0';

  gtk_editable_set_text (GTK_EDITABLE (entry), delimited_paths);
}

/**
 * Updates the the VST paths in the gsettings from
 * the text in the entry.
 */
void
ui_update_vst_paths_from_entry (GtkEntry * entry)
{
  const char * txt = gtk_editable_get_text (GTK_EDITABLE (entry));
  g_return_if_fail (txt);
  char ** paths = g_strsplit (txt, ";", 0);
  g_settings_set_strv (
    S_P_PLUGINS_PATHS, "vst-search-paths-windows", (const char * const *) paths);
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
ui_get_contrast_color (GdkRGBA * src, GdkRGBA * dest)
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
  GdkRGBA *       dest,
  const GdkRGBA * c1,
  const GdkRGBA * c2,
  const float     transition)
{
  dest->red = c1->red * transition + c2->red * (1.f - transition);
  dest->green = c1->green * transition + c2->green * (1.f - transition);
  dest->blue = c1->blue * transition + c2->blue * (1.f - transition);
  dest->alpha = c1->alpha * transition + c2->alpha * (1.f - transition);
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
  GdkRGBA *  color,
  const bool is_hovered,
  const bool is_selected,
  const bool is_transient,
  const bool is_muted)
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
  double     size,
  double     cur_val,
  double     start_px,
  double     cur_px,
  double     last_px,
  double     multiplier,
  UiDragMode mode)
{
  switch (mode)
    {
    case UI_DRAG_MODE_CURSOR:
      return CLAMP (cur_px / size, 0.0, 1.0);
    case UI_DRAG_MODE_RELATIVE:
      return CLAMP (cur_val + (cur_px - last_px) / size, 0.0, 1.0);
    case UI_DRAG_MODE_RELATIVE_WITH_MULTIPLIER:
      return CLAMP (
        cur_val + (multiplier * (cur_px - last_px)) / size, 0.0, 1.0);
    }

  g_return_val_if_reached (0.0);
}

UiDetail
ui_get_detail_level (void)
{
  if (!UI_CACHES->detail_level_set)
    {
      UI_CACHES->detail_level =
        (UiDetail) g_settings_get_enum (S_P_UI_GENERAL, "graphic-detail");
      UI_CACHES->detail_level_set = true;
    }

  return UI_CACHES->detail_level;
}

/**
 * Returns an appropriate string representation of the given
 * dB value.
 */
void
ui_get_db_value_as_string (float val, char * buf)
{
  if (val < -98.f)
    {
      strcpy (buf, "-∞");
    }
  else
    {
      if (val < -10.)
        {
          sprintf (buf, "%.0f", val);
        }
      else
        {
          sprintf (buf, "%.1f", val);
        }
    }
}

UiCaches *
ui_caches_new (void)
{
  UiCaches * self = object_new (UiCaches);

  UiColors * colors = &self->colors;

#define SET_COLOR(cname, caps) gdk_rgba_parse (&colors->cname, UI_COLOR_##caps)

  SET_COLOR (bright_green, BRIGHT_GREEN);
  SET_COLOR (darkish_green, DARKISH_GREEN);
  SET_COLOR (dark_orange, DARK_ORANGE);
  SET_COLOR (bright_orange, BRIGHT_ORANGE);
  SET_COLOR (matcha, MATCHA);
  SET_COLOR (prefader_send, PREFADER_SEND);
  SET_COLOR (postfader_send, POSTFADER_SEND);
  SET_COLOR (solo_active, SOLO_ACTIVE);
  SET_COLOR (solo_checked, SOLO_CHECKED);
  SET_COLOR (fader_fill_end, FADER_FILL_END);
  SET_COLOR (highlight_scale_bg, HIGHLIGHT_SCALE_BG);
  SET_COLOR (highlight_chord_bg, HIGHLIGHT_CHORD_BG);
  SET_COLOR (highlight_bass_bg, HIGHLIGHT_BASS_BG);
  SET_COLOR (highlight_both_bg, HIGHLIGHT_BOTH_BG);
  SET_COLOR (highlight_scale_fg, HIGHLIGHT_SCALE_FG);
  SET_COLOR (highlight_chord_fg, HIGHLIGHT_CHORD_FG);
  SET_COLOR (highlight_bass_fg, HIGHLIGHT_BASS_FG);
  SET_COLOR (highlight_both_fg, HIGHLIGHT_BOTH_FG);
  SET_COLOR (z_yellow, Z_YELLOW);
  SET_COLOR (z_purple, Z_PURPLE);
  SET_COLOR (dark_text, DARK_TEXT);
  SET_COLOR (bright_text, BRIGHT_TEXT);
  SET_COLOR (record_active, RECORD_ACTIVE);
  SET_COLOR (record_checked, RECORD_CHECKED);

#undef SET_COLOR

  return self;
}

void
ui_caches_free (UiCaches * self)
{
  for (int i = 0; i < self->num_cursors; i++)
    {
      UiCursor * cursor = &self->cursors[i];

      g_object_unref (cursor->cursor);
    }

  object_zero_and_free (self);
}
