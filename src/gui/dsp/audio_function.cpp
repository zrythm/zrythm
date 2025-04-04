// SPDX-FileCopyrightText: © 2020-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings_manager.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/dsp/audio_function.h"
#include "gui/dsp/audio_region.h"
#include "gui/dsp/engine.h"

#include "utils/debug.h"
#include "utils/dsp.h"
#include "utils/exceptions.h"
#include "utils/gtest_wrapper.h"
#include "utils/rt_thread_id.h"
#include "utils/string.h"
#include <rubberband/rubberband-c.h>

using namespace zrythm;

std::string
audio_function_get_action_target_for_type (AudioFunctionType type)
{
  auto        type_str = AudioFunctionType_to_string (type);
  std::string type_str_lower (type_str);
  utils::string::to_lower_ascii (type_str_lower);
  auto substituted = utils::string::replace (type_str_lower, " ", "-");

  return substituted;
}

/**
 * Returns a detailed action name to be used for
 * actionable widgets or menus.
 *
 * @param base_action Base action to use.
 */
std::string
audio_function_get_detailed_action_for_type (
  AudioFunctionType  type,
  const std::string &base_action)
{
  auto target = audio_function_get_action_target_for_type (type);
  return fmt::format ("{}::{}", base_action, target);
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
  zrythm::gui::old_dsp::plugins::PluginDescriptor * descr =
    plugin_manager_find_plugin_from_uri (zrythm::gui::old_dsp::plugins::PluginManager::get_active_instance (), uri);
  z_return_val_if_fail (descr, -1);
  PluginSetting * setting = plugin_setting_new_default (descr);
  z_return_val_if_fail (setting, -1);
  setting->force_generic_ui = true;
  GError * err = NULL;
  zrythm::gui::old_dsp::plugins::Plugin * pl =
    plugin_new_from_setting (setting, 0, zrythm::dsp::PluginSlotType::Insert, 0, &err);
  if (!IS_PLUGIN_AND_NONNULL (pl))
    {
      PROPAGATE_PREFIXED_ERROR (error, err, "%s", QObject::tr ("Failed to create plugin"));
      return -1;
    }
  pl->is_function = true;
  int ret = plugin_instantiate (pl, nullptr, &err);
  if (ret != 0)
    {
      PROPAGATE_PREFIXED_ERROR (
        error, err, "%s", QObject::tr ("Failed to instantiate plugin"));
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
  zrythm::gui::old_dsp::plugins::plugin_gtk_build_menu (
    pl, GTK_WIDGET (pl->window),
    GTK_WIDGET (pl->vbox));
#  endif

  /* Create/show alignment to contain UI (whether
   * custom or generic) */
  pl->ev_box = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));
  gtk_box_append (pl->vbox, GTK_WIDGET (pl->ev_box));
  gtk_widget_set_vexpand (GTK_WIDGET (pl->ev_box), true);

  /* open */
  zrythm::gui::old_dsp::plugins::plugin_gtk_open_generic_ui (pl, false);

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

  zrythm::gui::old_dsp::plugins::plugin_gtk_close_ui (pl);
  plugin_free (pl);

  return 0;
}
#endif

void
audio_function_apply (
  ArrangerObject::Uuid       region_id,
  const dsp::Position       &sel_start,
  const dsp::Position       &sel_end,
  AudioFunctionType          type,
  AudioFunctionOpts          opts,
  std::optional<std::string> uri)
{
  using Position = AudioRegion::Position;
  z_debug ("applying {}...", AudioFunctionType_to_string (type));

  auto * r =
    std::get<AudioRegion *> (*PROJECT->find_arranger_object_by_id (region_id));
  z_return_if_fail (r);
  auto tr = std::get<AudioTrack *> (r->get_track ());
  z_return_if_fail (tr);
  auto * orig_clip = r->get_clip ();
  z_return_if_fail (orig_clip);

  Position init_pos;
  if (sel_start < *r->pos_ || sel_end > *r->end_pos_)
    {
      throw ZrythmException (
        QObject::tr ("Invalid positions - skipping function"));
    }

  /* adjust the positions */
  Position start = sel_start;
  Position end = sel_end;
  start.add_frames (-r->pos_->frames_, AUDIO_ENGINE->ticks_per_frame_);
  end.add_frames (-r->pos_->frames_, AUDIO_ENGINE->ticks_per_frame_);

  /* create a copy of the frames to be replaced */
  auto num_frames = (unsigned_frame_t) (end.frames_ - start.frames_);

  auto                      channels = orig_clip->get_num_channels ();
  utils::audio::AudioBuffer src_frames{
    channels, static_cast<int> (num_frames)
  };
  utils::audio::AudioBuffer dest_frames{
    channels, static_cast<int> (num_frames)
  };
  for (int j = 0; j < channels; ++j)
    {
      src_frames.copyFrom (
        j, 0, orig_clip->get_samples (), j, start.frames_, num_frames);
      dest_frames.copyFrom (
        j, 0, orig_clip->get_samples (), j, start.frames_, num_frames);
    }

  auto nudge_frames = (unsigned_frame_t) Position::get_frames_from_ticks (
    ArrangerObject::DEFAULT_NUDGE_TICKS, 0.0);
  unsigned_frame_t num_frames_excl_nudge;
  z_debug ("num frames {}, nudge_frames {}", num_frames, nudge_frames);
  z_return_if_fail_cmp (nudge_frames, >, 0);

  switch (type)
    {
    case AudioFunctionType::Invert:
      dest_frames.invert_phase ();
      break;
    case AudioFunctionType::NormalizePeak:
      dest_frames.normalize_peak ();
      break;
    case AudioFunctionType::NormalizeRMS:
      /* TODO rms-normalize */
      break;
    case AudioFunctionType::NormalizeLUFS:
      /* TODO lufs-normalize */
      break;
    case AudioFunctionType::LinearFadeIn:
      dest_frames.applyGainRamp (0, num_frames, 0.f, 1.f);
      break;
    case AudioFunctionType::LinearFadeOut:
      dest_frames.applyGainRamp (0, num_frames, 1.f, 0.f);
      break;
    case AudioFunctionType::NudgeLeft:
      z_return_if_fail (num_frames > nudge_frames);
      num_frames_excl_nudge = num_frames - (size_t) nudge_frames;

      for (int ch = 0; ch < channels; ch++)
        {
          // Copy shifted audio
          dest_frames.copyFrom (
            ch, 0, src_frames, ch, nudge_frames, num_frames_excl_nudge);
          // Clear end
          utils::float_ranges::fill (
            dest_frames.getWritePointer (ch, num_frames_excl_nudge), 0.f,
            nudge_frames);
        }
      break;
    case AudioFunctionType::NudgeRight:
      z_return_if_fail (num_frames > nudge_frames);
      num_frames_excl_nudge = num_frames - (size_t) nudge_frames;
      for (int ch = 0; ch < channels; ch++)
        {
          // Copy shifted audio
          dest_frames.copyFrom (
            ch, nudge_frames, src_frames, ch, 0, num_frames_excl_nudge);
          // Clear beginning
          utils::float_ranges::fill (
            dest_frames.getWritePointer (ch), 0.f, nudge_frames);
        }
      break;
    case AudioFunctionType::Reverse:
      dest_frames.reverse (0, num_frames);
      break;
    case AudioFunctionType::PitchShift:
      {
        z_return_if_fail_cmp (channels, >=, 2);
        RubberBandState   rubberband_state{};
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
          rubberband_state, src_frames.getArrayOfReadPointers (), num_frames,
          true);
        size_t samples_fed = 0;
        size_t frames_read = 0;
        while (frames_read < num_frames)
          {
            unsigned int samples_required =
              std::min (num_frames - samples_fed, max_process_size);
            /*rubberband_get_samples_required (*/
            /*rubberband_state));*/
            std::array<const float * const, 2> tmp_in_arrays = {
              &src_frames.getReadPointer (0)[samples_fed],
              &src_frames.getReadPointer (1)[samples_fed]
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
                  &dest_frames.getWritePointer (0)[frames_read],
                  &dest_frames.getWritePointer (1)[frames_read]
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
                z_debug (
                  "retrieved out samples {}, frames read {}",
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
      if (channels == 2)
        {
          utils::float_ranges::copy (
            dest_frames.getWritePointer (1), src_frames.getReadPointer (0),
            num_frames);
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
          src_frames, AudioClip::BitDepth::BIT_DEPTH_32,
          AUDIO_ENGINE->sample_rate_, P_TEMPO_TRACK->get_current_bpm (),
          "tmp-clip");
        auto tmp_clip = tmp_clip_before.edit_in_ext_program ();
        for (int i = 0; i < channels; ++i)
          {
            utils::float_ranges::copy (
              dest_frames.getWritePointer (i),
              tmp_clip->get_samples ().getReadPointer (i),
              std::min (
                static_cast<size_t> (num_frames),
                static_cast<size_t> (tmp_clip->get_num_frames ())));
          }

        if ((size_t) tmp_clip->get_num_frames () < num_frames)
          {
            for (int i = 0; i < channels; ++i)
              {
                utils::float_ranges::fill (
                  dest_frames.getWritePointer (i), 0.f,
                  num_frames - (size_t) tmp_clip->get_num_frames ());
              }
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
              error, err, "%s", QObject::tr ("Failed to apply plugin"));
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

// FIXME: where is the clip used??
#if 0
  int  id = AUDIO_POOL->add_clip (std::make_shared<AudioClip> (
    dest_frames, AudioClip::BitDepth::BIT_DEPTH_32, AUDIO_ENGINE->sample_rate_,
    P_TEMPO_TRACK->get_current_bpm (), orig_clip->get_name ()));
  auto clip = AUDIO_POOL->get_clip (id);
  z_debug (
    "writing {} to pool (id {})", clip->get_name (), clip->get_pool_id ());
  AUDIO_POOL->write_clip (*clip, false, false);
#endif

  // FIXME: needed?
  // audio_sel.pool_id_ = clip->get_pool_id ();

  if (type != AudioFunctionType::Invalid)
    {
      /* replace the frames in the region */
      r->replace_frames (dest_frames, start.frames_);
    }

  if (
    !ZRYTHM_TESTING && !ZRYTHM_BENCHMARKING && type != AudioFunctionType::Invalid
    && type != AudioFunctionType::CustomPlugin)
    {
      /* set last action */
      gui::SettingsManager::get_instance ()->set_lastAudioFunction (
        ENUM_VALUE_TO_INT (type));
      if (type == AudioFunctionType::PitchShift)
        {
          gui::SettingsManager::get_instance ()
            ->set_lastAudioFunctionPitchShiftRatio (opts.amount_);
        }
    }

  // EVENTS_PUSH (EventType::ET_EDITOR_FUNCTION_APPLIED, nullptr);
}
