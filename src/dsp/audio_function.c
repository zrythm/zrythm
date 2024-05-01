// SPDX-FileCopyrightText: © 2020-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <inttypes.h>

#include "dsp/audio_function.h"
#include "dsp/audio_region.h"
#include "dsp/engine.h"
#include "gui/backend/arranger_selections.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/main_window.h"
#include "plugins/plugin_gtk.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/debug.h"
#include "utils/dsp.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

#include <rubberband/rubberband-c.h>

typedef enum
{
  Z_AUDIO_AUDIO_FUNCTION_ERROR_INVALID_POSITIONS,
  Z_AUDIO_AUDIO_FUNCTION_ERROR_FAILED,
} ZAudioAudioFunctionError;

#define Z_AUDIO_AUDIO_FUNCTION_ERROR z_audio_audio_function_error_quark ()
GQuark
z_audio_audio_function_error_quark (void);
G_DEFINE_QUARK (
  z - audio - audio - function - error - quark,
  z_audio_audio_function_error)

char *
audio_function_get_action_target_for_type (AudioFunctionType type)
{
  const char * type_str = audio_function_type_to_string (type);
  char *       type_str_lower = g_strdup (type_str);
  string_to_lower (type_str, type_str_lower);
  char * substituted = string_replace (type_str_lower, " ", "-");
  g_free (type_str_lower);

  return substituted;
}

/**
 * Returns a detailed action name to be used for
 * actionable widgets or menus.
 *
 * @param base_action Base action to use.
 */
char *
audio_function_get_detailed_action_for_type (
  AudioFunctionType type,
  const char *      base_action)
{
  char * target = audio_function_get_action_target_for_type (type);
  char * ret = g_strdup_printf ("%s::%s", base_action, target);
  g_free (target);

  return ret;
}

const char *
audio_function_get_icon_name_for_type (AudioFunctionType type)
{
  switch (type)
    {
    case AUDIO_FUNCTION_INVERT:
      return "edit-select-invert";
    case AUDIO_FUNCTION_REVERSE:
      return "path-reverse";
    case AUDIO_FUNCTION_PITCH_SHIFT:
      return "path-reverse";
    case AUDIO_FUNCTION_NORMALIZE_PEAK:
      return "kt-set-max-upload-speed";
    case AUDIO_FUNCTION_LINEAR_FADE_IN:
      return "arena-fade-in";
    case AUDIO_FUNCTION_LINEAR_FADE_OUT:
      return "arena-fade-out";
    case AUDIO_FUNCTION_NUDGE_LEFT:
      return "arrow-left";
    case AUDIO_FUNCTION_NUDGE_RIGHT:
      return "arrow-right";
    case AUDIO_FUNCTION_NORMALIZE_RMS:
    case AUDIO_FUNCTION_NORMALIZE_LUFS:
    default:
      return "modulator";
      break;
    }

  g_return_val_if_reached (NULL);
}

#if 0
/**
 * @param frames Interleaved frames.
 * @param num_frames Number of frames per channel.
 * @param channels Number of channels.
 *
 * @return Non-zero if fail
 */
static int
apply_plugin (
  const char * uri,
  float *      frames,
  size_t       num_frames,
  channels_t   channels,
  GError **    error)
{
  PluginDescriptor * descr =
    plugin_manager_find_plugin_from_uri (PLUGIN_MANAGER, uri);
  g_return_val_if_fail (descr, -1);
  PluginSetting * setting = plugin_setting_new_default (descr);
  g_return_val_if_fail (setting, -1);
  setting->force_generic_ui = true;
  GError * err = NULL;
  Plugin * pl =
    plugin_new_from_setting (setting, 0, Z_PLUGIN_SLOT_INSERT, 0, &err);
  if (!IS_PLUGIN_AND_NONNULL (pl))
    {
      PROPAGATE_PREFIXED_ERROR (error, err, "%s", _ ("Failed to create plugin"));
      return -1;
    }
  pl->is_function = true;
  int ret = plugin_instantiate (pl, NULL, &err);
  if (ret != 0)
    {
      PROPAGATE_PREFIXED_ERROR (
        error, err, "%s", _ ("Failed to instantiate plugin"));
      return -1;
    }
  ret = plugin_activate (pl, true);
  g_return_val_if_fail (ret == 0, -1);
  plugin_setting_free (setting);

  /* create window */
  pl->window = GTK_WINDOW (gtk_dialog_new ());
  gtk_window_set_title (pl->window, descr->name);
  gtk_window_set_icon_name (pl->window, "zrythm");
  /*gtk_window_set_role (*/
  /*pl->window, "plugin_ui");*/
  gtk_window_set_modal (pl->window, true);
  /*gtk_window_set_attached_to (*/
  /*pl->window, GTK_WIDGET (MAIN_WINDOW));*/

  /* add vbox for stacking elements */
  pl->vbox = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  GtkWidget * container = gtk_dialog_get_content_area (GTK_DIALOG (pl->window));
  gtk_box_append (GTK_BOX (container), GTK_WIDGET (pl->vbox));

#  if 0
  /* add menu bar */
  plugin_gtk_build_menu (
    pl, GTK_WIDGET (pl->window),
    GTK_WIDGET (pl->vbox));
#  endif

  /* Create/show alignment to contain UI (whether
   * custom or generic) */
  pl->ev_box = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));
  gtk_box_append (pl->vbox, GTK_WIDGET (pl->ev_box));
  gtk_widget_set_vexpand (GTK_WIDGET (pl->ev_box), true);

  /* open */
  plugin_gtk_open_generic_ui (pl, F_NO_PUBLISH_EVENTS);

  ret = z_gtk_dialog_run (GTK_DIALOG (pl->window), false);

  Port * l_in = NULL;
  Port * r_in = NULL;
  Port * l_out = NULL;
  Port * r_out = NULL;
  for (int i = 0; i < pl->num_out_ports; i++)
    {
      Port * port = pl->out_ports[i];
      if (port->id.type == Z_PORT_TYPE_AUDIO)
        {
          if (l_out)
            {
              r_out = port;
              break;
            }
          else
            {
              l_out = port;
            }
        }
    }
  int num_plugin_channels = r_out ? 2 : 1;
  for (int i = 0; i < pl->num_in_ports; i++)
    {
      Port * port = pl->in_ports[i];
      if (port->id.type == Z_PORT_TYPE_AUDIO)
        {
          if (l_in)
            {
              r_in = port;
              break;
            }
          else
            {
              l_in = port;
              if (num_plugin_channels == 1)
                break;
            }
        }
    }
  if (num_plugin_channels == 1)
    {
      r_in = l_in;
      r_out = l_out;
    }

  uint32_t step = (uint32_t) MIN (AUDIO_ENGINE->block_length, num_frames);
  size_t   i = 0; /* frames processed */
  plugin_update_latency (pl);
  nframes_t latency = pl->latency;
  while (i < num_frames)
    {
      for (size_t j = 0; j < step; j++)
        {
          g_return_val_if_fail (l_in, -1);
          g_return_val_if_fail (r_in, -1);
          l_in->buf[j] = frames[(i + j) * channels];
          r_in->buf[j] = frames[(i + j) * channels + 1];
        }
      lv2_ui_read_and_apply_events (pl->lv2, step);
      lilv_instance_run (pl->lv2->instance, step);
      for (size_t j = 0; j < step; j++)
        {
          signed_frame_t actual_j =
            (signed_frame_t) (i + j) - (signed_frame_t) latency;
          if (actual_j < 0)
            continue;
#  if 0
          g_message (
            "%ld %f",
            actual_j,
            fabsf (l_out->buf[j]));
#  endif
          g_return_val_if_fail (l_out, -1);
          g_return_val_if_fail (r_out, -1);
          frames[actual_j * (signed_frame_t) channels] = l_out->buf[j];
          frames[actual_j * (signed_frame_t) channels + 1] = r_out->buf[j];
        }
      if (i > latency)
        {
          plugin_update_latency (pl);
          /*g_message ("end latency %d", pl->latency);*/
        }
      i += step;
      step = (uint32_t) MIN (step, num_frames - i);
    }

  /* handle latency */
  i = 0;
  step = (uint32_t) MIN (AUDIO_ENGINE->block_length, latency);
  while (i < latency)
    {
      for (size_t j = 0; j < step; j++)
        {
          g_return_val_if_fail (l_in, -1);
          g_return_val_if_fail (r_in, -1);
          l_in->buf[j] = 0.f;
          r_in->buf[j] = 0.f;
        }
      lv2_ui_read_and_apply_events (pl->lv2, step);
      lilv_instance_run (pl->lv2->instance, step);
      for (size_t j = 0; j < step; j++)
        {
          signed_frame_t actual_j =
            (signed_frame_t) (i + j + num_frames) - (signed_frame_t) latency;
          g_return_val_if_fail (actual_j >= 0, -1);
#  if 0
          g_message (
            "%ld %f",
            actual_j,
            fabsf (l_out->buf[j]));
#  endif
          g_return_val_if_fail (l_out, -1);
          g_return_val_if_fail (r_out, -1);
          frames[actual_j * (signed_frame_t) channels] = l_out->buf[j];
          frames[actual_j * (signed_frame_t) channels + 1] = r_out->buf[j];
        }
      i += step;
      step = (uint32_t) MIN (step, latency - i);
    }
  plugin_update_latency (pl);
  g_message ("end latency %d", pl->latency);

  plugin_gtk_close_ui (pl);
  plugin_free (pl);

  return 0;
}
#endif

bool
audio_function_apply (
  ArrangerSelections * sel,
  AudioFunctionType    type,
  AudioFunctionOpts    opts,
  const char *         uri,
  GError **            error)
{
  g_message ("applying %s...", audio_function_type_to_string (type));

  AudioSelections * audio_sel = (AudioSelections *) sel;

  ZRegion * r = region_find (&audio_sel->region_id);
  g_return_val_if_fail (r, false);
  Track * tr = arranger_object_get_track ((ArrangerObject *) r);
  g_return_val_if_fail (tr, false);
  AudioClip * orig_clip = audio_region_get_clip (r);
  g_return_val_if_fail (orig_clip, false);

  Position init_pos;
  position_init (&init_pos);
  if (
    position_is_before (&audio_sel->sel_start, &r->base.pos)
    || position_is_after (&audio_sel->sel_end, &r->base.end_pos))
    {
      position_print (&audio_sel->sel_start);
      position_print (&audio_sel->sel_end);
      g_set_error_literal (
        error, Z_AUDIO_AUDIO_FUNCTION_ERROR,
        Z_AUDIO_AUDIO_FUNCTION_ERROR_INVALID_POSITIONS,
        _ ("Invalid positions - skipping function"));
      return false;
    }

  /* adjust the positions */
  Position start, end;
  position_set_to_pos (&start, &audio_sel->sel_start);
  position_set_to_pos (&end, &audio_sel->sel_end);
  position_add_frames (&start, -r->base.pos.frames);
  position_add_frames (&end, -r->base.pos.frames);

  /* create a copy of the frames to be replaced */
  unsigned_frame_t num_frames = (unsigned_frame_t) (end.frames - start.frames);

  bool use_interleaved = true;

  /* interleaved frames */
  channels_t channels = orig_clip->channels;
  float *    src_frames = object_new_n (num_frames * channels, float);
  float *    dest_frames = object_new_n (num_frames * channels, float);
  dsp_copy (
    &dest_frames[0], &orig_clip->frames[start.frames * (long) channels],
    num_frames * channels);
  dsp_copy (&src_frames[0], &dest_frames[0], num_frames * channels);

  /* uninterleaved frames */
  float * ch_src_frames[channels];
  float * ch_dest_frames[channels];
  for (size_t j = 0; j < channels; j++)
    {
      ch_src_frames[j] = object_new_n (num_frames, float);
      ch_dest_frames[j] = object_new_n (num_frames, float);
      for (size_t i = 0; i < num_frames; i++)
        {
          ch_src_frames[j][i] = src_frames[i * channels + j];
        }
      dsp_copy (&ch_dest_frames[j][0], &ch_src_frames[j][0], num_frames);
    }

  unsigned_frame_t nudge_frames = (unsigned_frame_t)
    position_get_frames_from_ticks (ARRANGER_SELECTIONS_DEFAULT_NUDGE_TICKS, 0.0);
  unsigned_frame_t nudge_frames_all_channels = channels * nudge_frames;
  unsigned_frame_t num_frames_excl_nudge;
  g_debug (
    "num frames %" PRIu64
    ", "
    "nudge_frames %" PRIu64,
    num_frames, nudge_frames);
  z_return_val_if_fail_cmp (nudge_frames, >, 0, false);

  switch (type)
    {
    case AUDIO_FUNCTION_INVERT:
      dsp_mul_k2 (&dest_frames[0], -1.f, num_frames * channels);
      break;
    case AUDIO_FUNCTION_NORMALIZE_PEAK:
      /* note: this normalizes by taking all channels into
       * account */
      dsp_normalize (&dest_frames[0], &dest_frames[0], num_frames * channels);
      break;
    case AUDIO_FUNCTION_NORMALIZE_RMS:
      /* TODO rms-normalize */
      break;
    case AUDIO_FUNCTION_NORMALIZE_LUFS:
      /* TODO lufs-normalize */
      break;
    case AUDIO_FUNCTION_LINEAR_FADE_IN:
      dsp_linear_fade_in_from (
        &dest_frames[0], 0, num_frames * channels, num_frames * channels, 0.f);
      break;
    case AUDIO_FUNCTION_LINEAR_FADE_OUT:
      dsp_linear_fade_out_to (
        &dest_frames[0], 0, num_frames * channels, num_frames * channels, 0.f);
      break;
    case AUDIO_FUNCTION_NUDGE_LEFT:
      g_return_val_if_fail (num_frames > nudge_frames, false);
      num_frames_excl_nudge = num_frames - (size_t) nudge_frames;
      dsp_copy (
        &dest_frames[0], &src_frames[nudge_frames_all_channels],
        channels * num_frames_excl_nudge);
      dsp_fill (
        &dest_frames[channels * num_frames_excl_nudge], 0.f,
        nudge_frames_all_channels);
      break;
    case AUDIO_FUNCTION_NUDGE_RIGHT:
      g_return_val_if_fail (num_frames > nudge_frames, false);
      num_frames_excl_nudge = num_frames - (size_t) nudge_frames;
      dsp_copy (
        &dest_frames[nudge_frames], &src_frames[0],
        channels * num_frames_excl_nudge);
      dsp_fill (&dest_frames[0], 0.f, nudge_frames_all_channels);
      break;
    case AUDIO_FUNCTION_REVERSE:
      use_interleaved = false;
      for (size_t j = 0; j < channels; j++)
        {
          dsp_reverse2 (&ch_dest_frames[j][0], &ch_src_frames[j][0], num_frames);
        }
      break;
    case AUDIO_FUNCTION_PITCH_SHIFT:
      {
        z_return_val_if_fail_cmp (channels, >=, 2, false);
        use_interleaved = false;
        RubberBandState   rubberband_state;
        RubberBandOptions rubberband_opts =
          RubberBandOptionProcessOffline
        /* use finer engine if rubberband v3 */
#if RUBBERBAND_API_MAJOR_VERSION > 2 \
  || (RUBBERBAND_API_MAJOR_VERSION == 2 && RUBBERBAND_API_MINOR_VERSION >= 7)
          | RubberBandOptionEngineFiner
#endif
          | RubberBandOptionPitchHighQuality | RubberBandOptionFormantPreserved
          | RubberBandOptionThreadingAlways | RubberBandOptionChannelsApart;
        rubberband_state = rubberband_new (
          AUDIO_ENGINE->sample_rate, channels, rubberband_opts, 1.0,
          opts.amount);
        const size_t max_process_size = 8192;
        rubberband_set_debug_level (rubberband_state, 2);
        rubberband_set_max_process_size (rubberband_state, max_process_size);
        rubberband_set_expected_input_duration (rubberband_state, num_frames);
        rubberband_study (
          rubberband_state, (const float * const *) ch_src_frames, num_frames,
          true);
        size_t samples_fed = 0;
        size_t frames_read = 0;
        while (frames_read < num_frames)
          {
            unsigned int samples_required =
              MIN (num_frames - samples_fed, max_process_size);
            /*rubberband_get_samples_required (*/
            /*rubberband_state));*/
            float * tmp_in_arrays[2] = {
              &ch_src_frames[0][samples_fed], &ch_src_frames[1][samples_fed]
            };
            samples_fed += samples_required;
            g_message (
              "samples required: %u (total fed %zu), latency: %u",
              samples_required, samples_fed,
              rubberband_get_latency (rubberband_state));
            if (samples_required > 0)
              {
                rubberband_process (
                  rubberband_state, (const float * const *) tmp_in_arrays,
                  samples_required, samples_fed == num_frames);
              }
            for (;;)
              {
                int avail = rubberband_available (rubberband_state);
                if (avail == 0)
                  {
                    g_message ("avail == 0");
                    break;
                  }
                else if (avail == -1)
                  {
                    g_message ("avail == -1");
                    /* FIXME for some reason rubberband
                     * skips the last few samples when the
                     * pitch ratio is < 1.0
                     * this workaround just keeps the copied
                     * original samples at the end and should
                     * be fixed eventually */
                    frames_read = num_frames;
                    break;
#if 0
                    g_set_error_literal (
                      error, Z_AUDIO_AUDIO_FUNCTION_ERROR,
                      Z_AUDIO_AUDIO_FUNCTION_ERROR_FAILED,
                      "rubberband: finished prematurely");
                    return false;
#endif
                  }
                float * tmp_out_arrays[2] = {
                  &ch_dest_frames[0][frames_read],
                  &ch_dest_frames[1][frames_read]
                };
                size_t retrieved_out_samples = rubberband_retrieve (
                  rubberband_state, tmp_out_arrays, (unsigned int) avail);
                if ((int) retrieved_out_samples != avail)
                  {
                    g_set_error (
                      error, Z_AUDIO_AUDIO_FUNCTION_ERROR,
                      Z_AUDIO_AUDIO_FUNCTION_ERROR_FAILED,
                      "rubberband: retrieved out samples (%zu) != available samples (%d)",
                      retrieved_out_samples, avail);
                    return false;
                  }
                frames_read += retrieved_out_samples;
                g_message (
                  "retrieved out samples %zu, frames read %zu",
                  retrieved_out_samples, frames_read);
              }
          }
        if (frames_read != num_frames)
          {
            g_set_error (
              error, Z_AUDIO_AUDIO_FUNCTION_ERROR,
              Z_AUDIO_AUDIO_FUNCTION_ERROR_FAILED,
              "rubberband: expected %zu frames but read %zu", num_frames,
              frames_read);
            return false;
          }
      }
      break;
    case AUDIO_FUNCTION_COPY_L_TO_R:
      use_interleaved = false;
      if (channels == 2)
        {
          dsp_copy (ch_dest_frames[1], ch_src_frames[0], num_frames);
        }
      else
        {
          g_set_error_literal (
            error, Z_AUDIO_AUDIO_FUNCTION_ERROR,
            Z_AUDIO_AUDIO_FUNCTION_ERROR_FAILED,
            _ ("This function can only be used on audio clips with 2 channels"));
          return false;
        }
      break;
    case AUDIO_FUNCTION_EXT_PROGRAM:
      {
        AudioClip * tmp_clip = audio_clip_new_from_float_array (
          src_frames, num_frames, channels, BIT_DEPTH_32, "tmp-clip");
        GError * err = NULL;
        tmp_clip = audio_clip_edit_in_ext_program (tmp_clip, &err);
        if (!tmp_clip)
          {
            /* FIXME this should be handled async */
            PROPAGATE_PREFIXED_ERROR (
              error, err, "%s",
              _ ("Failed to get audio clip from external program"));
            return false;
          }
        dsp_copy (
          &dest_frames[0], &tmp_clip->frames[0],
          MIN (num_frames, (size_t) tmp_clip->num_frames) * channels);
        if ((size_t) tmp_clip->num_frames < num_frames)
          {
            dsp_fill (
              &dest_frames[0], 0.f,
              (num_frames - (size_t) tmp_clip->num_frames) * channels);
          }
      }
      break;
    case AUDIO_FUNCTION_CUSTOM_PLUGIN:
      {
#if 0
        g_return_val_if_fail (uri, false);
        GError * err = NULL;
        int ret = apply_plugin (uri, dest_frames, num_frames, channels, &err);
        if (ret != 0)
          {
            PROPAGATE_PREFIXED_ERROR (
              error, err, "%s", _ ("Failed to apply plugin"));
            return false;
          }
#endif
      }
      break;
    case AUDIO_FUNCTION_INVALID:
      /* do nothing */
      break;
    default:
      g_warning ("not implemented");
      break;
    }

#if 0
  char * tmp =
    g_strdup_printf (
      "%s - %s - %s",
      tr->name, r->name,
      audio_function_type_to_string (type));

  /* remove dots from name */
  char * name = string_replace (tmp, ".", "_");
  g_free (tmp);
#endif

  /* convert to interleaved */
  if (!use_interleaved)
    {
      for (size_t j = 0; j < channels; j++)
        {
          for (size_t i = 0; i < num_frames; i++)
            {
              dest_frames[i * channels + j] = ch_dest_frames[j][i];
            }
        }
    }

  AudioClip * clip = audio_clip_new_from_float_array (
    &dest_frames[0], num_frames, channels, BIT_DEPTH_32, orig_clip->name);
  audio_pool_add_clip (AUDIO_POOL, clip);
  g_message ("writing %s to pool (id %d)", clip->name, clip->pool_id);
  GError * err = NULL;
  bool     success = audio_clip_write_to_pool (clip, false, F_NOT_BACKUP, &err);
  if (!success)
    {
      PROPAGATE_PREFIXED_ERROR (
        error, err, "%s", "Failed to write audio clip to pool");
      return false;
    }

  audio_sel->pool_id = clip->pool_id;

  if (type != AUDIO_FUNCTION_INVALID)
    {
      /* replace the frames in the region */
      success = audio_region_replace_frames (
        r, dest_frames, (size_t) start.frames, num_frames, F_NO_DUPLICATE_CLIP,
        &err);
      if (!success)
        {
          PROPAGATE_PREFIXED_ERROR (
            error, err, "%s", "Failed to replace region frames");
          return false;
        }
    }

  if (
    !ZRYTHM_TESTING && type != AUDIO_FUNCTION_INVALID
    && type != AUDIO_FUNCTION_CUSTOM_PLUGIN)
    {
      /* set last action */
      g_settings_set_int (S_UI, "audio-function", type);
      if (type == AUDIO_FUNCTION_PITCH_SHIFT)
        {
          g_settings_set_double (
            S_UI, "audio-function-pitch-shift-ratio", opts.amount);
        }
    }

  /* free allocated memory */
  free (src_frames);
  free (dest_frames);
  for (size_t j = 0; j < channels; j++)
    {
      free (ch_src_frames[j]);
      free (ch_dest_frames[j]);
    }

  EVENTS_PUSH (ET_EDITOR_FUNCTION_APPLIED, NULL);

  return true;
}
