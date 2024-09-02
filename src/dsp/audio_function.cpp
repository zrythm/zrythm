// SPDX-FileCopyrightText: © 2020-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/audio_function.h"
#include "dsp/audio_region.h"
#include "dsp/engine.h"
#include "gui/backend/arranger_selections.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/main_window.h"
#include "project.h"
#include "settings/g_settings_manager.h"
#include "utils/debug.h"
#include "utils/dsp.h"
#include "utils/exceptions.h"
#include "utils/rt_thread_id.h"
#include "utils/string.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

#include "doctest_wrapper.h"
#include <rubberband/rubberband-c.h>

char *
audio_function_get_action_target_for_type (AudioFunctionType type)
{
  auto   type_str = AudioFunctionType_to_string (type);
  char * type_str_lower = g_strdup (type_str.c_str ());
  string_to_lower (type_str.c_str (), type_str_lower);
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
  auto   ret = fmt::format ("{}::{}", base_action, target);
  g_free (target);

  return g_strdup (ret.c_str ());
}

const char *
audio_function_get_icon_name_for_type (AudioFunctionType type)
{
  switch (type)
    {
    case AudioFunctionType::Invert:
      return "edit-select-invert";
    case AudioFunctionType::Reverse:
      return "path-reverse";
    case AudioFunctionType::PitchShift:
      return "path-reverse";
    case AudioFunctionType::NormalizePeak:
      return "kt-set-max-upload-speed";
    case AudioFunctionType::LinearFadeIn:
      return "arena-fade-in";
    case AudioFunctionType::LinearFadeOut:
      return "arena-fade-out";
    case AudioFunctionType::NudgeLeft:
      return "arrow-left";
    case AudioFunctionType::NudgeRight:
      return "arrow-right";
    case AudioFunctionType::NormalizeRMS:
    case AudioFunctionType::NormalizeLUFS:
    default:
      return "modulator";
      break;
    }

  z_return_val_if_reached (nullptr);
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
  z_return_val_if_fail (descr, -1);
  PluginSetting * setting = plugin_setting_new_default (descr);
  z_return_val_if_fail (setting, -1);
  setting->force_generic_ui = true;
  GError * err = NULL;
  Plugin * pl =
    plugin_new_from_setting (setting, 0, PluginSlotType::Insert, 0, &err);
  if (!IS_PLUGIN_AND_NONNULL (pl))
    {
      PROPAGATE_PREFIXED_ERROR (error, err, "%s", _ ("Failed to create plugin"));
      return -1;
    }
  pl->is_function = true;
  int ret = plugin_instantiate (pl, nullptr, &err);
  if (ret != 0)
    {
      PROPAGATE_PREFIXED_ERROR (
        error, err, "%s", _ ("Failed to instantiate plugin"));
      return -1;
    }
  ret = plugin_activate (pl, true);
  z_return_val_if_fail (ret == 0, -1);
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
  plugin_gtk_open_generic_ui (pl, false);

  ret = z_gtk_dialog_run (GTK_DIALOG (pl->window), false);

  Port * l_in = NULL;
  Port * r_in = NULL;
  Port * l_out = NULL;
  Port * r_out = NULL;
  for (int i = 0; i < pl->num_out_ports; i++)
    {
      Port * port = pl->out_ports[i];
      if (port->id_.type == PortType::Audio)
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
      if (port->id_.type == PortType::Audio)
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

  uint32_t step = (uint32_t) MIN (AUDIO_ENGINE->block_length_, num_frames);
  size_t   i = 0; /* frames processed */
  plugin_update_latency (pl);
  nframes_t latency = pl->latency;
  while (i < num_frames)
    {
      for (size_t j = 0; j < step; j++)
        {
          z_return_val_if_fail (l_in, -1);
          z_return_val_if_fail (r_in, -1);
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
          z_info (
            "{} {}",
            actual_j,
            fabsf (l_out->buf[j]));
#  endif
          z_return_val_if_fail (l_out, -1);
          z_return_val_if_fail (r_out, -1);
          frames[actual_j * (signed_frame_t) channels] = l_out->buf[j];
          frames[actual_j * (signed_frame_t) channels + 1] = r_out->buf[j];
        }
      if (i > latency)
        {
          plugin_update_latency (pl);
          /*z_info ("end latency {}", pl->latency);*/
        }
      i += step;
      step = (uint32_t) MIN (step, num_frames - i);
    }

  /* handle latency */
  i = 0;
  step = (uint32_t) MIN (AUDIO_ENGINE->block_length_, latency);
  while (i < latency)
    {
      for (size_t j = 0; j < step; j++)
        {
          z_return_val_if_fail (l_in, -1);
          z_return_val_if_fail (r_in, -1);
          l_in->buf[j] = 0.f;
          r_in->buf[j] = 0.f;
        }
      lv2_ui_read_and_apply_events (pl->lv2, step);
      lilv_instance_run (pl->lv2->instance, step);
      for (size_t j = 0; j < step; j++)
        {
          signed_frame_t actual_j =
            (signed_frame_t) (i + j + num_frames) - (signed_frame_t) latency;
          z_return_val_if_fail (actual_j >= 0, -1);
#  if 0
          z_info (
            "{} {}",
            actual_j,
            fabsf (l_out->buf[j]));
#  endif
          z_return_val_if_fail (l_out, -1);
          z_return_val_if_fail (r_out, -1);
          frames[actual_j * (signed_frame_t) channels] = l_out->buf[j];
          frames[actual_j * (signed_frame_t) channels + 1] = r_out->buf[j];
        }
      i += step;
      step = (uint32_t) MIN (step, latency - i);
    }
  plugin_update_latency (pl);
  z_info ("end latency {}", pl->latency);

  plugin_gtk_close_ui (pl);
  plugin_free (pl);

  return 0;
}
#endif

void
audio_function_apply (
  ArrangerSelections &sel,
  AudioFunctionType   type,
  AudioFunctionOpts   opts,
  const std::string * uri)
{
  z_debug ("applying {}...", AudioFunctionType_to_string (type));

  auto &audio_sel = (AudioSelections &) sel;

  auto r = AudioRegion::find (audio_sel.region_id_);
  z_return_if_fail (r);
  auto tr = r->get_track_as<AudioTrack> ();
  z_return_if_fail (tr);
  auto * orig_clip = r->get_clip ();
  z_return_if_fail (orig_clip);

  Position init_pos;
  if (audio_sel.sel_start_ < r->pos_ || audio_sel.sel_end_ > r->end_pos_)
    {
      audio_sel.sel_start_.print ();
      audio_sel.sel_end_.print ();
      throw ZrythmException (_ ("Invalid positions - skipping function"));
    }

  /* adjust the positions */
  Position start = audio_sel.sel_start_;
  Position end = audio_sel.sel_end_;
  start.add_frames (-r->pos_.frames_);
  end.add_frames (-r->pos_.frames_);

  /* create a copy of the frames to be replaced */
  auto num_frames = (unsigned_frame_t) (end.frames_ - start.frames_);

  bool use_interleaved = true;

  /* interleaved frames */
  channels_t         channels = orig_clip->channels_;
  std::vector<float> src_frames (num_frames * channels);
  std::vector<float> dest_frames (num_frames * channels);
  dsp_copy (
    &dest_frames[0],
    &orig_clip->frames_.getReadPointer (0)[start.frames_ * (long) channels],
    num_frames * channels);
  dsp_copy (&src_frames[0], &dest_frames[0], num_frames * channels);

  /* uninterleaved frames */
  juce::AudioBuffer<float> ch_src_frames (channels, num_frames);
  juce::AudioBuffer<float> ch_dest_frames (channels, num_frames);
  for (size_t j = 0; j < channels; j++)
    {
      for (size_t i = 0; i < num_frames; i++)
        {
          ch_src_frames.getWritePointer (j)[i] = src_frames[i * channels + j];
        }
      dsp_copy (
        &ch_dest_frames.getWritePointer (j)[0],
        &ch_src_frames.getReadPointer (j)[0], num_frames);
    }

  unsigned_frame_t nudge_frames = (unsigned_frame_t) Position::
    get_frames_from_ticks (ARRANGER_SELECTIONS_DEFAULT_NUDGE_TICKS, 0.0);
  unsigned_frame_t nudge_frames_all_channels = channels * nudge_frames;
  unsigned_frame_t num_frames_excl_nudge;
  z_debug ("num frames {}, nudge_frames {}", num_frames, nudge_frames);
  z_return_if_fail_cmp (nudge_frames, >, 0);

  switch (type)
    {
    case AudioFunctionType::Invert:
      dsp_mul_k2 (&dest_frames[0], -1.f, num_frames * channels);
      break;
    case AudioFunctionType::NormalizePeak:
      /* note: this normalizes by taking all channels into
       * account */
      dsp_normalize (&dest_frames[0], &dest_frames[0], num_frames * channels);
      break;
    case AudioFunctionType::NormalizeRMS:
      /* TODO rms-normalize */
      break;
    case AudioFunctionType::NormalizeLUFS:
      /* TODO lufs-normalize */
      break;
    case AudioFunctionType::LinearFadeIn:
      dsp_linear_fade_in_from (
        &dest_frames[0], 0, num_frames * channels, num_frames * channels, 0.f);
      break;
    case AudioFunctionType::LinearFadeOut:
      dsp_linear_fade_out_to (
        &dest_frames[0], 0, num_frames * channels, num_frames * channels, 0.f);
      break;
    case AudioFunctionType::NudgeLeft:
      z_return_if_fail (num_frames > nudge_frames);
      num_frames_excl_nudge = num_frames - (size_t) nudge_frames;
      dsp_copy (
        &dest_frames[0], &src_frames[nudge_frames_all_channels],
        channels * num_frames_excl_nudge);
      dsp_fill (
        &dest_frames[channels * num_frames_excl_nudge], 0.f,
        nudge_frames_all_channels);
      break;
    case AudioFunctionType::NudgeRight:
      z_return_if_fail (num_frames > nudge_frames);
      num_frames_excl_nudge = num_frames - (size_t) nudge_frames;
      dsp_copy (
        &dest_frames[nudge_frames], &src_frames[0],
        channels * num_frames_excl_nudge);
      dsp_fill (&dest_frames[0], 0.f, nudge_frames_all_channels);
      break;
    case AudioFunctionType::Reverse:
      use_interleaved = false;
      for (size_t j = 0; j < channels; j++)
        {
          dsp_reverse2 (
            &ch_dest_frames.getWritePointer (j)[0],
            &ch_src_frames.getReadPointer (j)[0], num_frames);
        }
      break;
    case AudioFunctionType::PitchShift:
      {
        z_return_if_fail_cmp (channels, >=, 2);
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
          AUDIO_ENGINE->sample_rate_, channels, rubberband_opts, 1.0,
          opts.amount_);
        const size_t max_process_size = 8192;
        rubberband_set_debug_level (rubberband_state, 2);
        rubberband_set_max_process_size (rubberband_state, max_process_size);
        rubberband_set_expected_input_duration (rubberband_state, num_frames);
        rubberband_study (
          rubberband_state, ch_src_frames.getArrayOfReadPointers (), num_frames,
          true);
        size_t samples_fed = 0;
        size_t frames_read = 0;
        while (frames_read < num_frames)
          {
            unsigned int samples_required =
              MIN (num_frames - samples_fed, max_process_size);
            /*rubberband_get_samples_required (*/
            /*rubberband_state));*/
            std::array<const float * const, 2> tmp_in_arrays = {
              &ch_src_frames.getReadPointer (0)[samples_fed],
              &ch_src_frames.getReadPointer (1)[samples_fed]
            };
            samples_fed += samples_required;
            z_info (
              "samples required: {} (total fed {}), latency: {}",
              samples_required, samples_fed,
              rubberband_get_latency (rubberband_state));
            if (samples_required > 0)
              {
                rubberband_process (
                  rubberband_state, tmp_in_arrays.data (), samples_required,
                  samples_fed == num_frames);
              }
            for (;;)
              {
                int avail = rubberband_available (rubberband_state);
                if (avail == 0)
                  {
                    z_info ("avail == 0");
                    break;
                  }
                else if (avail == -1)
                  {
                    z_info ("avail == -1");
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
                      error, Z_AUDIOAUDIO_FUNCTION_ERROR,
                      Z_AUDIOAUDIO_FUNCTION_ERROR_FAILED,
                      "rubberband: finished prematurely");
                    return false;
#endif
                  }
                std::array<float *, 2> tmp_out_arrays = {
                  &ch_dest_frames.getWritePointer (0)[frames_read],
                  &ch_dest_frames.getWritePointer (1)[frames_read]
                };
                size_t retrieved_out_samples = rubberband_retrieve (
                  rubberband_state, tmp_out_arrays.data (),
                  (unsigned int) avail);
                if ((int) retrieved_out_samples != avail)
                  {
                    throw ZrythmException (fmt::format (
                      "rubberband: retrieved out samples ({}) != available samples ({})",
                      retrieved_out_samples, avail));
                  }
                frames_read += retrieved_out_samples;
                z_info (
                  "retrieved out samples %zu, frames read %zu",
                  retrieved_out_samples, frames_read);
              }
          }
        if (frames_read != num_frames)
          {
            throw ZrythmException (fmt::format (
              "rubberband: expected {} frames but read {}", num_frames,
              frames_read));
          }
      }
      break;
    case AudioFunctionType::CopyLtoR:
      use_interleaved = false;
      if (channels == 2)
        {
          dsp_copy (
            ch_dest_frames.getWritePointer (1),
            ch_src_frames.getReadPointer (0), num_frames);
        }
      else
        {
          throw ZrythmException (fmt::format (
            "copy_lto_r: expected 2 channels but got {}", channels));
        }
      break;
    case AudioFunctionType::ExternalProgram:
      {
        AudioClip tmp_clip_before (
          src_frames.data (), num_frames, channels, BitDepth::BIT_DEPTH_32,
          "tmp-clip");
        auto tmp_clip = tmp_clip_before.edit_in_ext_program ();
        dsp_copy (
          &dest_frames[0], &tmp_clip->frames_.getReadPointer (0)[0],
          std::min (num_frames, (size_t) tmp_clip->num_frames_) * channels);
        if ((size_t) tmp_clip->num_frames_ < num_frames)
          {
            dsp_fill (
              &dest_frames[0], 0.f,
              (num_frames - (size_t) tmp_clip->num_frames_) * channels);
          }
      }
      break;
    case AudioFunctionType::CustomPlugin:
      {
#if 0
        z_return_val_if_fail (uri, false);
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
    case AudioFunctionType::Invalid:
      /* do nothing */
      break;
    default:
      z_warning ("not implemented");
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
              dest_frames[i * channels + j] = ch_dest_frames.getSample (j, i);
            }
        }
    }

  int  id = AUDIO_POOL->add_clip (std::make_unique<AudioClip> (
    &dest_frames[0], num_frames, channels, BitDepth::BIT_DEPTH_32,
    orig_clip->name_));
  auto clip = AUDIO_POOL->get_clip (id);
  z_debug ("writing {} to pool (id {})", clip->name_, clip->pool_id_);
  clip->write_to_pool (false, false);

  audio_sel.pool_id_ = clip->pool_id_;

  if (type != AudioFunctionType::Invalid)
    {
      /* replace the frames in the region */
      r->replace_frames (dest_frames.data (), start.frames_, num_frames, false);
    }

  if (
    !ZRYTHM_TESTING && !ZRYTHM_BENCHMARKING && type != AudioFunctionType::Invalid
    && type != AudioFunctionType::CustomPlugin)
    {
      /* set last action */
      g_settings_set_int (S_UI, "audio-function", ENUM_VALUE_TO_INT (type));
      if (type == AudioFunctionType::PitchShift)
        {
          g_settings_set_double (
            S_UI, "audio-function-pitch-shift-ratio", opts.amount_);
        }
    }

  EVENTS_PUSH (EventType::ET_EDITOR_FUNCTION_APPLIED, nullptr);
}
