// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: © 2020 Ryan Gonzalez <rymg19 at gmail dot com>
// SPDX-FileCopyrightText: © 2024 Miró Allard <miro.allard@pm.me>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "common/dsp/engine.h"
#include "common/dsp/engine_rtaudio.h"
#include "common/dsp/pan.h"
#include "common/dsp/port.h"
#include "common/utils/color.h"
#include "common/utils/localization.h"
#include "common/utils/objects.h"
#include "common/utils/string.h"
#include "common/utils/ui.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings/settings.h"

UiCursor::UiCursor (std::string name, int offset_x, int offset_y)
    : name_ (std::move (name)), offset_x_ (offset_x), offset_y_ (offset_y)
{
#if 0
  if (!cursor || !GDK_IS_CURSOR (cursor))
    {
      throw ZrythmException ("cursor is invalid");
    }
  g_object_ref (cursor);
#endif
}

UiCursor::~UiCursor () = default;

#if 0
/**
 * Sets cursor from icon name.
 */
void
ui_set_cursor_from_icon_name (
  GtkWidget *        widget,
  const std::string &name,
  int                offset_x,
  int                offset_y)
{
  z_return_if_fail (offset_x >= 0 && offset_y >= 0);

  /* check the cache first */
  for (auto &cursor : UI_CACHES->cursors_)
    {
      if (
        name == cursor->name_ && cursor->offset_x_ == offset_x
        && cursor->offset_y_ == offset_y)
        {
          gtk_widget_set_cursor (widget, cursor->cursor_);
          return;
        }
    }

#  if 0
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
#  endif
  GdkTexture * texture =
    z_gdk_texture_new_from_icon_name (name.c_str (), 24, 24, 1);
  if (!texture || !GDK_IS_TEXTURE (texture))
    {
      z_warning ("no texture for {}", name);
      return;
    }
  int adjusted_offset_x = MIN (offset_x, gdk_texture_get_width (texture) - 1);
  int adjusted_offset_y = MIN (offset_y, gdk_texture_get_height (texture) - 1);
  GdkCursor * gdk_cursor = gdk_cursor_new_from_texture (
    texture, adjusted_offset_x, adjusted_offset_y, nullptr);
  g_object_unref (texture);
  z_return_if_fail (GDK_IS_CURSOR (gdk_cursor));

  /* add the cursor to the caches */
  UI_CACHES->cursors_.emplace_back (
    std::make_unique<UiCursor> (name, gdk_cursor, offset_x, offset_y));

  gtk_widget_set_cursor (widget, UI_CACHES->cursors_.back ()->cursor_);
}

/**
 * Sets cursor from standard cursor name.
 */
void
ui_set_cursor_from_name (GtkWidget * widget, const char * name)
{
  GdkCursor * cursor = gdk_cursor_new_from_name (name, nullptr);
  gtk_widget_set_cursor (widget, cursor);
}

void
ui_set_pointer_cursor (GtkWidget * widget)
{
  ui_set_cursor_from_icon_name (GTK_WIDGET (widget), "select-cursor", 3, 1);
}
#endif

void
ui_show_message_full (std::string_view title, std::string_view msg)
{
  /* log the message */
  z_info ("{}: {}", title, msg);

  /* if have UI, also show a message dialog */
  if (ZRYTHM_HAVE_UI)
    {
#if 0
      AdwAlertDialog * dialog =
        ADW_ALERT_DIALOG (adw_alert_dialog_new (title.data (), msg.data ()));
      adw_alert_dialog_add_responses (dialog, "ok", QObject::tr ("_OK"), nullptr);
      adw_alert_dialog_set_default_response (dialog, "ok");
      adw_alert_dialog_set_response_appearance (
        dialog, "ok", ADW_RESPONSE_SUGGESTED);
      adw_alert_dialog_set_close_response (dialog, "ok");
      adw_dialog_present (ADW_DIALOG (dialog), parent);
      return ADW_DIALOG (dialog);
#endif
    }
}

#if 0
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
      graphene_point_t tmp_pt = Z_GRAPHENE_POINT_INIT ((float) x, (float) y);
      bool             success = gtk_widget_compute_point (
        GTK_WIDGET (parent), GTK_WIDGET (child), &tmp_pt, &wpt);
      z_return_val_if_fail (success, nullptr);

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

static Position
px_to_pos (double px, bool use_padding, RulerWidget * ruler)
{
  if (use_padding)
    {
      px -= SPACE_BEFORE_START_D;

      /* clamp at 0 */
      if (px < 0.0)
        px = 0.0;
    }

  Position pos;
  pos.ticks_ = px / ruler->px_per_tick;
  pos.update_frames_from_ticks (0.0);
  return pos;
}
#endif

Position
ui_px_to_pos_timeline (double px, bool has_padding)
{
#if 0
  if (!MAIN_WINDOW || !MW_RULER)
    return {};

  return px_to_pos (px, has_padding, Z_RULER_WIDGET (MW_RULER));
#endif
  return {};
}

Position
ui_px_to_pos_editor (double px, bool has_padding)
{
#if 0
  if (!MAIN_WINDOW || !EDITOR_RULER)
    return {};

  return px_to_pos (px, has_padding, Z_RULER_WIDGET (EDITOR_RULER));
#endif
  return {};
}

#if 0
ATTR_NONNULL static inline int
pos_to_px (const Position pos, int use_padding, RulerWidget * ruler)
{
  int px = (int) (pos.ticks_ * ruler->px_per_tick);

  if (use_padding)
    px += SPACE_BEFORE_START;

  return px;
}
#endif

int
ui_pos_to_px_timeline (const Position pos, int use_padding)
{
#if 0
  if (!MAIN_WINDOW || !MW_RULER)
    return 0;

  return pos_to_px (pos, use_padding, (RulerWidget *) (MW_RULER));
#endif
  return 0;
}

/**
 * Gets pixels from the position, based on the
 * piano_roll ruler.
 */
int
ui_pos_to_px_editor (const Position pos, bool use_padding)
{
#if 0
  if (!MAIN_WINDOW || !EDITOR_RULER)
    return 0;

  return pos_to_px (pos, use_padding, Z_RULER_WIDGET (EDITOR_RULER));
#endif
  return 0;
}

#if 0
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
    signed_frame_t) (((double) AUDIO_ENGINE->frames_per_tick_ * px)
                     / ruler->px_per_tick);
}
#endif

signed_frame_t
ui_px_to_frames_timeline (double px, bool has_padding)
{
#if 0
  if (!MAIN_WINDOW || !MW_RULER)
    return 0;

  return px_to_frames (px, has_padding, Z_RULER_WIDGET (MW_RULER));
#endif
  return 0;
}

signed_frame_t
ui_px_to_frames_editor (double px, bool has_padding)
{
#if 0
  if (!MAIN_WINDOW || !EDITOR_RULER)
    return 0;

  return px_to_frames (px, has_padding, Z_RULER_WIDGET (EDITOR_RULER));
#endif
  return 0;
}

#if 0
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
  graphene_point_t tmp_pt = Z_GRAPHENE_POINT_INIT ((float) x, (float) y);
  bool             success = gtk_widget_compute_point (
    GTK_WIDGET (parent), GTK_WIDGET (child), &tmp_pt, &wpt);
  z_return_val_if_fail (success, -1);

  // z_info ("wpt.x wpt.y {} {}", wpt.x, wpt.y);

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
#endif

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
#if 0
  AdwToast * toast = adw_toast_new (msg);
  if (MAIN_WINDOW)
    {
      adw_toast_overlay_add_toast (MAIN_WINDOW->toast_overlay, toast);
    }
    /* TODO: add toast overlay in greeter too */
#  if 0
  gtk_label_set_text (MAIN_WINDOW->notification_label,
                      msg);
  gtk_revealer_set_reveal_child (
    GTK_REVEALER (MAIN_WINDOW->revealer),
    1);
  g_timeout_add_seconds (
    3, (GSourceFunc) hide_notification_async, nullptr);
#  endif
#endif
}

#if 0
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
#endif

#if 0
/**
 * Sets up a combo box to have a selection of
 * languages.
 */
void
ui_setup_language_combo_row (AdwComboRow * combo_row)
{
  GtkStringList * string_list = gtk_string_list_new (nullptr);
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
  AudioBackend backend = AudioBackend_from_string (str);
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
  StringArray labels{
    AudioBackend_to_string (AudioBackend::AUDIO_BACKEND_DUMMY),
#  ifdef HAVE_LIBSOUNDIO
    AudioBackend_to_string (AudioBackend::AUDIO_BACKEND_DUMMY_LIBSOUNDIO),
#  endif
#  ifdef HAVE_ALSA
  /* broken */
  /*AudioBackend_to_string(AudioBackend::AUDIO_BACKEND_ALSA),*/
#    ifdef HAVE_LIBSOUNDIO
    AudioBackend_to_string (AudioBackend::AUDIO_BACKEND_ALSA_LIBSOUNDIO),
#    endif
#    if HAVE_RTAUDIO
    AudioBackend_to_string (AudioBackend::AUDIO_BACKEND_ALSA_RTAUDIO),
#    endif
#  endif /* HAVE_ALSA */
#  if HAVE_JACK
    AudioBackend_to_string (AudioBackend::AUDIO_BACKEND_JACK),
#    ifdef HAVE_LIBSOUNDIO
    AudioBackend_to_string (AudioBackend::AUDIO_BACKEND_JACK_LIBSOUNDIO),
#    endif
#    if HAVE_RTAUDIO
  /* unnecessary */
  /*AudioBackend_to_string(AudioBackend::AUDIO_BACKEND_JACK_RTAUDIO),*/
#    endif
#  endif /* HAVE_JACK */
#  if HAVE_PULSEAUDIO
  /* use rtaudio version - this has known issues */
  /*AudioBackend_to_string(AudioBackend::AUDIO_BACKEND_PULSEAUDIO),*/
#    ifdef HAVE_LIBSOUNDIO
    AudioBackend_to_string (AudioBackend::AUDIO_BACKEND_PULSEAUDIO_LIBSOUNDIO),
#    endif
#    if HAVE_RTAUDIO
    AudioBackend_to_string (AudioBackend::AUDIO_BACKEND_PULSEAUDIO_RTAUDIO),
#    endif
#  endif /* HAVE_PULSEAUDIO */
#  ifdef __APPLE__
#    ifdef HAVE_LIBSOUNDIO
    AudioBackend_to_string (AudioBackend::AUDIO_BACKEND_COREAUDIO_LIBSOUNDIO),
#    endif
#    if HAVE_RTAUDIO
    AudioBackend_to_string (AudioBackend::AUDIO_BACKEND_COREAUDIO_RTAUDIO),
#    endif
#  endif /* __APPLE__ */
#  ifdef _WIN32
#    ifdef HAVE_LIBSOUNDIO
    AudioBackend_to_string (AudioBackend::AUDIO_BACKEND_WASAPI_LIBSOUNDIO),
#    endif
#    if HAVE_RTAUDIO
    AudioBackend_to_string (AudioBackend::AUDIO_BACKEND_WASAPI_RTAUDIO),
  /* doesn't work & licensing issues */
  /*AudioBackend_to_string(AudioBackend::AUDIO_BACKEND_ASIO_RTAUDIO),*/
#    endif
#  endif /* _WIN32 */
  };
  auto            clabels = labels.getNullTerminated ();
  GtkStringList * string_list = gtk_string_list_new (clabels);
  g_strfreev (clabels);
  AdwComboRow * combo_row = ADW_COMBO_ROW (adw_combo_row_new ());
  adw_combo_row_set_model (combo_row, G_LIST_MODEL (string_list));

  int selected = g_settings_get_enum (S_P_GENERAL_ENGINE, "audio-backend");
  for (size_t i = 0; i < labels.size (); ++i)
    {
      if (

        AudioBackend_to_string (ENUM_INT_TO_VALUE (AudioBackend, selected))
        == labels[i])
        {
          adw_combo_row_set_selected (combo_row, i);
          break;
        }
    }

  if (with_signal)
    {
      g_signal_connect (
        G_OBJECT (combo_row), "notify::selected-item",
        G_CALLBACK (on_audio_backend_selected_item_changed), nullptr);
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
  MidiBackend  backend = MidiBackend_from_string (str);
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
  StringArray labels = {
    MidiBackend_to_string (MidiBackend::MIDI_BACKEND_DUMMY),
#  ifdef HAVE_ALSA
  /* broken */
  /*MidiBackend_to_string(MidiBackend::MIDI_BACKEND_ALSA),*/
#    if HAVE_RTMIDI
    MidiBackend_to_string (MidiBackend::MIDI_BACKEND_ALSA_RTMIDI),
#    endif
#  endif
#  if HAVE_JACK
    MidiBackend_to_string (MidiBackend::MIDI_BACKEND_JACK),
#    if HAVE_RTMIDI
  /* unnecessary */
  /*MidiBackend_to_string(MidiBackend::MIDI_BACKEND_JACK_RTMIDI),*/
#    endif
#  endif
#  ifdef _WIN32
  /* has known issues - use rtmidi */
  /*MidiBackend_to_string(MidiBackend::MIDI_BACKEND_WINDOWS_MME),*/
#    if HAVE_RTMIDI
    MidiBackend_to_string (MidiBackend::MIDI_BACKEND_WINDOWS_MME_RTMIDI),
#    endif
#  endif /* _WIN32 */
#  ifdef __APPLE__
#    if HAVE_RTMIDI
    MidiBackend_to_string (MidiBackend::MIDI_BACKEND_COREMIDI_RTMIDI),
#    endif
#  endif /* __APPLE__ */
#  ifdef _WIN32
#    if HAVE_RTMIDI_6
#    endif
    MidiBackend_to_string (MidiBackend::MIDI_BACKEND_WINDOWS_UWP_RTMIDI),
#  endif
  };
  auto            clabels = labels.getNullTerminated ();
  GtkStringList * string_list = gtk_string_list_new (clabels);
  g_strfreev (clabels);
  AdwComboRow * combo_row = ADW_COMBO_ROW (adw_combo_row_new ());
  adw_combo_row_set_model (combo_row, G_LIST_MODEL (string_list));

  int selected = g_settings_get_enum (S_P_GENERAL_ENGINE, "midi-backend");
  for (size_t i = 0; i < labels.size (); ++i)
    {
      if (

        MidiBackend_to_string (ENUM_INT_TO_VALUE (MidiBackend, selected))
        == labels[i])
        {
          adw_combo_row_set_selected (combo_row, i);
          break;
        }
    }

  if (with_signal)
    {
      g_signal_connect (
        G_OBJECT (combo_row), "notify::selected-item",
        G_CALLBACK (on_midi_backend_selected_item_changed), nullptr);
    }

  return combo_row;
}
#endif

std::string
ui_get_locale_not_available_string (LocalizationLanguage lang)
{
  /* show warning */
#ifdef _WIN32
#  define _TEMPLATE \
    QObject::tr ( \
      "A locale for the language you have \
selected ({}) is not available. Please install one first \
and restart {}")
#else
#  define _TEMPLATE \
    QObject::tr ( \
      "A locale for the language you have selected is \
not available. Please enable one first using \
the steps below and try again.\n\
1. Uncomment any locale starting with the \
language code <b>{}</b> in <b>/etc/locale.gen</b> (needs \
root privileges)\n\
2. Run <b>locale-gen</b> as root\n\
3. Restart {}")
#endif

  const char * code = localization_get_string_code (lang);
  return format_str (_TEMPLATE.toStdString (), code, PROGRAM_NAME);

#undef _TEMPLATE
}

void
ui_show_warning_for_tempo_track_experimental_feature (void)
{
  static bool shown = false;
  if (!shown)
    {
// TODO
#if 0
      ui_show_message_literal (
        QObject::tr ("Experimental Feature"),
        QObject::tr (
          "BPM and time signature automation is an "
          "experimental feature. Using it may corrupt your "
          "project."));
#endif
      shown = true;
    }
}

#if 0
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
      z_info ("no selected item");
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

  GtkStringList * string_list = gtk_string_list_new (nullptr);
  char *          names[1024];
  int             num_names = 0;
  if (populate)
    {
      if (audio_backend_is_rtaudio (backend))
        {
#  if HAVE_RTAUDIO
          engine_rtaudio_get_device_names (
            AUDIO_ENGINE.get (), backend, 0, names, &num_names);
#  endif
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
  z_info ("current device from settings: {}", current_device);
  for (int i = 0; i < num_names; i++)
    {
      if (string_is_equal (names[i], current_device))
        {
          z_info ("settings {} as selected", i);
          found = true;
          adw_combo_row_set_selected (combo_row, (guint) i);
        }
      g_free (names[i]);
    }
  if (!found && num_names > 0)
    {
      z_warning ("rtaudio device not found, selecting first");
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
#endif

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
    case UiDragMode::UI_DRAG_MODE_CURSOR:
      return std::clamp<double> (cur_px / size, 0.0, 1.0);
    case UiDragMode::UI_DRAG_MODE_RELATIVE:
      return std::clamp<double> (cur_val + (cur_px - last_px) / size, 0.0, 1.0);
    case UiDragMode::UI_DRAG_MODE_RELATIVE_WITH_MULTIPLIER:
      return std::clamp<double> (
        cur_val + (multiplier * (cur_px - last_px)) / size, 0.0, 1.0);
    }

  z_return_val_if_reached (0.0);
}

UiDetail
ui_get_detail_level (void)
{
#if 0
  if (!UI_CACHES->detail_level_set_)
    {
      UI_CACHES->detail_level_ =
        (UiDetail) g_settings_get_enum (S_P_UI_GENERAL, "graphic-detail");
      UI_CACHES->detail_level_set_ = true;
    }

  return UI_CACHES->detail_level_;
#endif
  return UiDetail::High;
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

UiCaches::UiCaches ()
{
#define SET_COLOR(cname, caps) \
  colors_.cname = Color (std::string (UI_COLOR_##caps))

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
}