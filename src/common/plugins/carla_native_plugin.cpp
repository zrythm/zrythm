// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include <algorithm>

#include "common/dsp/cv_port.h"
#include "common/utils/gtest_wrapper.h"
#include "common/utils/rt_thread_id.h"

#if HAVE_CARLA

#  include "common/dsp/engine.h"
#  include "common/dsp/midi_event.h"
#  include "common/dsp/tempo_track.h"
#  include "common/dsp/tracklist.h"
#  include "common/dsp/transport.h"
#  include "common/plugins/carla_discovery.h"
#  include "common/plugins/carla_native_plugin.h"
#  include "common/plugins/plugin.h"
#  include "common/plugins/plugin_manager.h"
#  include "common/utils/debug.h"
#  include "common/utils/directory_manager.h"
#  include "common/utils/dsp.h"
#  include "common/utils/gtk.h"
#  include "common/utils/io.h"
#  include "common/utils/math.h"
#  include "common/utils/string.h"
#  include "gui/backend/backend/event.h"
#  include "gui/backend/backend/event_manager.h"
#  include "gui/backend/backend/project.h"
#  include "gui/backend/backend/settings/g_settings_manager.h"
#  include "gui/backend/backend/zrythm.h"
#  include "gui/backend/gtk_widgets/main_window.h"
#  include "gui/backend/gtk_widgets/zrythm_app.h"

#  include <glib/gi18n.h>
#  include <gtk/gtk.h>

#  include "carla_wrapper.h"
#  include <fmt/format.h>
#  include <fmt/printf.h>

using namespace zrythm::plugins;

static GdkGLContext *
clear_gl_context (void)
{
  if (string_is_equal (z_gtk_get_gsk_renderer_type (), "GL"))
    {
      GdkGLContext * context = gdk_gl_context_get_current ();
      if (context)
        {
          /*z_warning("have GL context - clearing");*/
          g_object_ref (context);
          gdk_gl_context_clear_current ();
        }
      return context;
    }

  return NULL;
}

static void
return_gl_context (GdkGLContext * context)
{
  if (context)
    {
      /*z_debug ("returning context");*/
      gdk_gl_context_make_current (context);
      g_object_unref (context);
    }
}

bool
CarlaNativePlugin::idle_cb ()
{
  if (visible_ && MAIN_WINDOW)
    {
      GdkGLContext * context = clear_gl_context ();
      /*z_debug ("calling ui_idle()...");*/
      native_plugin_descriptor_->ui_idle (native_plugin_handle_);
      /*z_debug ("done calling ui_idle()");*/
      return_gl_context (context);

      return SourceFuncContinue;
    }
  else
    return SourceFuncRemove;
}

static uint32_t
host_get_buffer_size (NativeHostHandle handle)
{
  if (PROJECT && AUDIO_ENGINE && AUDIO_ENGINE->block_length_ > 0)
    return AUDIO_ENGINE->block_length_;
  return 512;
}

static double
host_get_sample_rate (NativeHostHandle handle)
{
  if (PROJECT && AUDIO_ENGINE && AUDIO_ENGINE->sample_rate_ > 0)
    return static_cast<double> (AUDIO_ENGINE->sample_rate_);
  return 44000.0;
}

static bool
host_is_offline (NativeHostHandle handle)
{
  return !PROJECT || !AUDIO_ENGINE || !AUDIO_ENGINE->run_.load ();
}

static const NativeTimeInfo *
host_get_time_info (NativeHostHandle handle)
{
  auto * plugin = static_cast<CarlaNativePlugin *> (handle);
  return &plugin->time_info_;
}

static bool
host_write_midi_event (NativeHostHandle handle, const NativeMidiEvent * event)
{
  auto * self = static_cast<CarlaNativePlugin *> (handle);
  auto * midi_out_port = self->get_midi_out_port ();
  if (!midi_out_port)
    return false;

  std::array<midi_byte_t, 4> buf;
  std::copy_n (
    event->data, std::min (event->size, static_cast<decltype (event->size)> (4)),
    buf.begin ());
  midi_out_port->midi_events_.active_events_.add_event_from_buf (
    event->time, buf.data (), event->size);
  return false; // (no idea what the return value means)
}

static void
host_ui_parameter_changed (NativeHostHandle handle, uint32_t index, float value)
{
  auto * self = static_cast<CarlaNativePlugin *> (handle);
  auto * port = self->get_port_from_param_id (index);
  if (!port)
    {
      z_warning ("No port found for param {}", index);
      return;
    }

  if (carla_get_current_plugin_count (self->host_handle_) == 2)
    {
      carla_set_parameter_value (self->host_handle_, 1, index, value);
    }

  port->set_control_value (value, false, false);
}

static void
host_ui_custom_data_changed (
  NativeHostHandle handle,
  const char *     key,
  const char *     value)
{
}

static void
host_ui_closed (NativeHostHandle handle)
{
  auto * self = static_cast<CarlaNativePlugin *> (handle);
  z_info ("{} UI closed", self->get_name ());
  self->idle_connection_.disconnect ();
}

static intptr_t
host_dispatcher (
  NativeHostHandle           handle,
  NativeHostDispatcherOpcode opcode,
  int32_t                    index,
  intptr_t                   value,
  void *                     ptr,
  float                      opt)
{
  switch (opcode)
    {
    case NATIVE_HOST_OPCODE_HOST_IDLE:
      /* some expensive computation is happening. this is used so that the GTK
       * ui does not block */
      /* note: disabled because some logic depends on this plugin being
       * activated */
#  if 0
      while (gtk_events_pending ())
        {
          gtk_main_iteration ();
        }
#  endif
      break;
    case NATIVE_HOST_OPCODE_GET_FILE_PATH:
      if (ptr && std::string_view (static_cast<char *> (ptr)) == "carla")
        {
          return reinterpret_cast<intptr_t> (PROJECT->dir_.c_str ());
        }
      break;
    case NATIVE_HOST_OPCODE_UI_RESIZE:
      /* TODO handle UI resize */
      z_debug ("UI resize");
      break;
    case NATIVE_HOST_OPCODE_UI_TOUCH_PARAMETER:
      z_debug ("UI touch");
      break;
    case NATIVE_HOST_OPCODE_UI_UNAVAILABLE:
      /* TODO handle UI close */
      z_debug ("UI unavailable");
      break;
    case NATIVE_HOST_OPCODE_INTERNAL_PLUGIN:
      z_debug ("internal plugin");
    default:
      break;
    }
  return 0;
}

static void
carla_engine_callback (
  void *               ptr,
  EngineCallbackOpcode action,
  uint                 plugin_id,
  int                  val1,
  int                  val2,
  int                  val3,
  float                valf,
  const char *         val_str)
{
  auto * self = static_cast<CarlaNativePlugin *> (ptr);

  switch (action)
    {
    case CarlaBackend::ENGINE_CALLBACK_PLUGIN_ADDED:
      z_debug ("Plugin added: {} - {}", plugin_id, val_str);
      break;
    case CarlaBackend::ENGINE_CALLBACK_PLUGIN_REMOVED:
      z_debug ("Plugin removed: {}", plugin_id);
      break;
    case CarlaBackend::ENGINE_CALLBACK_PLUGIN_RENAMED:
      break;
    case CarlaBackend::ENGINE_CALLBACK_PLUGIN_UNAVAILABLE:
      z_warning ("Plugin unavailable: {} - {}", plugin_id, val_str);
      break;
    case CarlaBackend::ENGINE_CALLBACK_PARAMETER_VALUE_CHANGED:
      /* if plugin was deactivated and we didn't explicitly tell it to
       * deactivate */
      if (
        val1 == CarlaBackend::PARAMETER_ACTIVE && val2 == 0 && val3 == 0
        && self->activated_ && !self->deactivating_ && !self->loading_state_)
        {
          /* send crash signal */
          EVENTS_PUSH (EventType::ET_PLUGIN_CRASHED, self);
        }
      break;
    case CarlaBackend::ENGINE_CALLBACK_PARAMETER_DEFAULT_CHANGED:
    case CarlaBackend::ENGINE_CALLBACK_PARAMETER_MAPPED_CONTROL_INDEX_CHANGED:
    case CarlaBackend::ENGINE_CALLBACK_PARAMETER_MIDI_CHANNEL_CHANGED:
      break;
    case CarlaBackend::ENGINE_CALLBACK_OPTION_CHANGED:
      z_debug ("Option changed: plugin {} - opt {}: {}", plugin_id, val1, val2);
      break;
    case CarlaBackend::ENGINE_CALLBACK_PROGRAM_CHANGED:
      z_debug ("Program changed: plugin {} - {}", plugin_id, val1);
      break;
    case CarlaBackend::ENGINE_CALLBACK_MIDI_PROGRAM_CHANGED:
      z_debug ("MIDI program changed: plugin {} - {}", plugin_id, val1);
      break;
    case CarlaBackend::ENGINE_CALLBACK_UI_STATE_CHANGED:
      switch (val1)
        {
        case 0:
        case -1:
          self->visible_ = false;
          break;
        case 1:
          self->visible_ = true;
          break;
        }
      EVENTS_PUSH (EventType::ET_PLUGIN_VISIBILITY_CHANGED, self);
      break;
    case CarlaBackend::ENGINE_CALLBACK_NOTE_ON:
    case CarlaBackend::ENGINE_CALLBACK_NOTE_OFF:
      break;
    case CarlaBackend::ENGINE_CALLBACK_UPDATE:
      z_debug ("Plugin {} needs update", plugin_id);
      break;
    case CarlaBackend::ENGINE_CALLBACK_RELOAD_INFO:
      z_debug ("Plugin {} reload info", plugin_id);
      break;
    case CarlaBackend::ENGINE_CALLBACK_RELOAD_PARAMETERS:
      z_debug ("Plugin {} reload parameters", plugin_id);
      break;
    case CarlaBackend::ENGINE_CALLBACK_RELOAD_PROGRAMS:
      z_debug ("Plugin {} reload programs", plugin_id);
      break;
    case CarlaBackend::ENGINE_CALLBACK_RELOAD_ALL:
      z_debug ("Plugin {} reload all", plugin_id);
      break;
    case CarlaBackend::ENGINE_CALLBACK_PATCHBAY_CLIENT_ADDED:
      z_debug (
        "Patchbay client added: {} plugin {} name {}", plugin_id, val2, val_str);
      break;
    case CarlaBackend::ENGINE_CALLBACK_PATCHBAY_CLIENT_REMOVED:
      z_debug ("Patchbay client removed: {}", plugin_id);
      break;
    case CarlaBackend::ENGINE_CALLBACK_PATCHBAY_CLIENT_RENAMED:
      z_debug ("Patchbay client renamed: {} - {}", plugin_id, val_str);
      break;
    case CarlaBackend::ENGINE_CALLBACK_PATCHBAY_CLIENT_DATA_CHANGED:
      z_debug (
        "Patchbay client data changed: {} - {} {}", plugin_id, val1, val2);
      break;
    case CarlaBackend::ENGINE_CALLBACK_PATCHBAY_PORT_ADDED:
      {
        z_trace (
          "PORT ADDED: client {} port {} group {} name {}", plugin_id, val1,
          val3, val_str);
        bool is_cv_variant =
          self->max_variant_cv_ins_ > 0 || self->max_variant_cv_outs_ > 0;
        auto port_id = static_cast<unsigned int> (val1);
        switch (plugin_id)
          {
          case 1:
            if (self->audio_input_port_id_ == 0)
              self->audio_input_port_id_ = port_id;
            break;
          case 2:
            if (self->audio_output_port_id_ == 0)
              self->audio_output_port_id_ = port_id;
            break;
          case 3:
            if (is_cv_variant && self->cv_input_port_id_ == 0)
              self->cv_input_port_id_ = port_id;
            else if (!is_cv_variant && self->midi_input_port_id_ == 0)
              self->midi_input_port_id_ = port_id;
            break;
          case 4:
            if (is_cv_variant && self->cv_output_port_id_ == 0)
              self->cv_output_port_id_ = port_id;
            else if (!is_cv_variant && self->midi_output_port_id_ == 0)
              self->midi_output_port_id_ = port_id;
            break;
          case 5:
            if (is_cv_variant && self->midi_input_port_id_ == 0)
              self->midi_input_port_id_ = port_id;
            break;
          case 6:
            if (is_cv_variant && self->midi_output_port_id_ == 0)
              self->midi_output_port_id_ = port_id;
            break;
          default:
            break;
          }

        /* if non-CV variant, there will be no CV clients */
        if (
          (is_cv_variant && plugin_id >= 7)
          || (!is_cv_variant && plugin_id >= 5))
          {
            auto port_hints = static_cast<unsigned int> (val2);
            self->patchbay_port_info_.push_back (CarlaPatchbayPortInfo{
              plugin_id, port_hints, port_id, val_str });
          }
      }
      break;
    case CarlaBackend::ENGINE_CALLBACK_PATCHBAY_PORT_REMOVED:
      z_debug ("Patchbay port removed: {} - {}", plugin_id, val1);
      break;
    case CarlaBackend::ENGINE_CALLBACK_PATCHBAY_PORT_CHANGED:
      z_debug ("Patchbay port changed: {} - {}", plugin_id, val1);
      break;
    case CarlaBackend::ENGINE_CALLBACK_PATCHBAY_CONNECTION_ADDED:
      z_debug ("Connection added {}", val_str);
      break;
    case CarlaBackend::ENGINE_CALLBACK_PATCHBAY_CONNECTION_REMOVED:
      z_debug ("Connection removed");
      break;
    case CarlaBackend::ENGINE_CALLBACK_ENGINE_STARTED:
      z_info ("Engine started");
      break;
    case CarlaBackend::ENGINE_CALLBACK_ENGINE_STOPPED:
      z_info ("Engine stopped");
      break;
    case CarlaBackend::ENGINE_CALLBACK_PROCESS_MODE_CHANGED:
      z_debug ("Process mode changed: {}", val1);
      break;
    case CarlaBackend::ENGINE_CALLBACK_TRANSPORT_MODE_CHANGED:
      z_debug ("Transport mode changed: {} - {}", val1, val_str);
      break;
    case CarlaBackend::ENGINE_CALLBACK_BUFFER_SIZE_CHANGED:
      z_info ("Buffer size changed: {}", val1);
      break;
    case CarlaBackend::ENGINE_CALLBACK_SAMPLE_RATE_CHANGED:
      z_info ("Sample rate changed: {:f}", static_cast<double> (valf));
      break;
    case CarlaBackend::ENGINE_CALLBACK_CANCELABLE_ACTION:
      z_debug (
        "Cancelable action: plugin {} - {} - {}", plugin_id, val1, val_str);
      break;
    case CarlaBackend::ENGINE_CALLBACK_PROJECT_LOAD_FINISHED:
      z_info ("Project load finished");
      break;
    case CarlaBackend::ENGINE_CALLBACK_NSM:
      break;
    /*!
     * Idle frontend.
     * This is used by the engine during long operations that might block the
     * frontend, giving it the possibility to idle while the operation is still
     * in place.
     */
    case CarlaBackend::ENGINE_CALLBACK_IDLE:
      /* TODO */
      break;
    case CarlaBackend::ENGINE_CALLBACK_INFO:
      z_info ("Engine info: {}", val_str);
      break;
    case CarlaBackend::ENGINE_CALLBACK_ERROR:
      z_warning ("Engine error: {}", val_str);
      break;
    case CarlaBackend::ENGINE_CALLBACK_QUIT:
      z_warning (
        "Engine quit: engine crashed or malfunctioned and will no longer work");
      break;
    /*!
     * A plugin requested for its inline display to be redrawn.
     * @a pluginId Plugin Id to redraw
     */
    case CarlaBackend::ENGINE_CALLBACK_INLINE_DISPLAY_REDRAW:
      /* TODO */
      break;
    /*!
     * A patchbay port group has been added.
     * @a pluginId Client Id
     * @a value1   Group Id (unique value within this client)
     * @a value2   Group hints
     * @a valueStr Group name
     * @see PatchbayPortGroupHints
     */
    case CarlaBackend::ENGINE_CALLBACK_PATCHBAY_PORT_GROUP_ADDED:
      /* TODO */
      break;
    /*!
     * A patchbay port group has been removed.
     * @a pluginId Client Id
     * @a value1   Group Id
     */
    case CarlaBackend::ENGINE_CALLBACK_PATCHBAY_PORT_GROUP_REMOVED:
      /* TODO */
      break;
    case CarlaBackend::ENGINE_CALLBACK_PATCHBAY_PORT_GROUP_CHANGED:
      z_debug (
        "Patchbay port group changed: client {} - group {} - hints {} - name {}",
        plugin_id, val1, val2, val_str);
      break;
    case CarlaBackend::ENGINE_CALLBACK_PARAMETER_MAPPED_RANGE_CHANGED:
      z_debug (
        "Parameter mapped range changed: {}:{} - {}", plugin_id, val1, val_str);
      break;
    case CarlaBackend::ENGINE_CALLBACK_PATCHBAY_CLIENT_POSITION_CHANGED:
      break;
    case CarlaBackend::ENGINE_CALLBACK_EMBED_UI_RESIZED:
      z_debug ("Embed UI resized: {} - {}x{}", plugin_id, val1, val2);
      break;
    default:
      z_warning ("unhandled callback");
      break;
    }
}

void
CarlaNativePlugin::set_selected_preset_from_index_impl (int idx)
{
  z_return_if_fail (host_handle_);

  /* if init preset */
  if (selected_bank_.bank_idx_ == 0 && idx == 0)
    {
      carla_reset_parameters (host_handle_, 0);
      z_info ("applied default preset");
    }
  else
    {
      const auto &pset = banks_[selected_bank_.bank_idx_].presets_[idx];
      carla_set_program (
        host_handle_, 0, static_cast<uint32_t> (pset.carla_program_));
      z_info ("applied preset '{}'", pset.name_);
    }
}

void
CarlaNativePlugin::cleanup_impl ()
{
#  if 0
          close();
#  endif
}

void
CarlaNativePlugin::populate_banks ()
{
  /* add default bank and preset */
  auto default_bank_uri = std::string (DEFAULT_BANK_URI);
  auto pl_def_bank =
    add_bank_if_not_exists (&default_bank_uri, _ ("Default bank"));
  {
    auto pl_def_preset = Preset ();
    pl_def_preset.uri_ = INIT_PRESET_URI;
    pl_def_preset.name_ = _ ("Init");
    pl_def_bank->add_preset (std::move (pl_def_preset));
  }

  std::string presets_gstr;

  uint32_t count = carla_get_program_count (host_handle_, 0);
  for (uint32_t i = 0; i < count; i++)
    {
      auto pl_preset = Preset ();
      pl_preset.carla_program_ = static_cast<int> (i);
      const char * program_name = carla_get_program_name (host_handle_, 0, i);
      if (strlen (program_name) == 0)
        {
          pl_preset.name_ = format_str (_ ("Preset {}"), i);
        }
      else
        {
          pl_preset.name_ = program_name;
        }
      pl_def_bank->add_preset (std::move (pl_preset));

      presets_gstr += fmt::format (
        "found preset {} ({})\n", pl_preset.name_, pl_preset.carla_program_);
    }

  z_info ("found {} presets", count);
  z_info ("{}", presets_gstr);
}

bool
CarlaNativePlugin::has_custom_ui (const zrythm::plugins::PluginDescriptor &descr)
{
#  if 0
  auto native_pl = _create (nullptr);

  /* instantiate the plugin to get its info */
  native_pl->native_plugin_descriptor =
    carla_get_native_rack_plugin ();
  native_pl->native_plugin_handle =
    native_pl->native_plugin_descriptor->instantiate (
      &native_pl->native_host_descriptor);
  native_pl->host_handle =
    carla_create_native_plugin_host_handle (
      native_pl->native_plugin_descriptor,
      native_pl->native_plugin_handle);
  auto type =
    z_carla_discovery_get_plugin_type_from_protocol (descr->protocol);
  carla_add_plugin (
    native_pl->host_handle,
    descr->arch == PluginArchitecture::ARCH_64_BIT ?
      CarlaBackend::BINARY_NATIVE : CarlaBackend::BINARY_WIN32,
    type, descr->path, descr->name,
    descr->uri, descr->unique_id, nullptr, 0);
  const CarlaPluginInfo * info =
    carla_get_plugin_info (
      native_pl->host_handle, 0);
  z_return_val_if_fail (info, false);
  bool has_custom_ui =
    info->hints & PLUGIN_HAS_CUSTOM_UI;

  carla_native_plugin_free (native_pl);

  return has_custom_ui;
#  endif
  z_return_val_if_reached (false);
}

void
CarlaNativePlugin::process_impl (const EngineProcessTimeInfo time_nfo)
{
  time_info_.playing = TRANSPORT->is_rolling ();
  time_info_.frame = static_cast<uint64_t> (time_nfo.g_start_frame_w_offset_);
  time_info_.bbt.bar = AUDIO_ENGINE->pos_nfo_current_.bar_;
  time_info_.bbt.beat = AUDIO_ENGINE->pos_nfo_current_.beat_; // within bar
  time_info_.bbt.tick =                                       // within beat
    AUDIO_ENGINE->pos_nfo_current_.tick_within_beat_;
  time_info_.bbt.barStartTick = AUDIO_ENGINE->pos_nfo_current_.tick_within_bar_;
  int beats_per_bar = P_TEMPO_TRACK->get_beats_per_bar ();
  int beat_unit = P_TEMPO_TRACK->get_beat_unit ();
  time_info_.bbt.beatsPerBar = static_cast<float> (beats_per_bar);
  time_info_.bbt.beatType = static_cast<float> (beat_unit);
  time_info_.bbt.ticksPerBeat = TRANSPORT->ticks_per_beat_;
  time_info_.bbt.beatsPerMinute = P_TEMPO_TRACK->get_current_bpm ();

  /* set actual audio in bufs */
  {
    size_t audio_ports = 0;
    for (
      size_t i = 0; i < audio_in_ports_.size () && i < max_variant_audio_ins_;
      i++)
      {
        auto &port = audio_in_ports_[i];
        inbufs_[audio_ports++] = &port->buf_[time_nfo.local_offset_];
      }
  }

  /* set actual cv in bufs */
  {
    size_t cv_ports = 0;
    for (size_t i = 0; i < cv_in_ports_.size () && i < max_variant_cv_ins_; i++)
      {
        auto port = cv_in_ports_[i];
        inbufs_[max_variant_audio_ins_ + cv_ports++] =
          &port->buf_[time_nfo.local_offset_];
      }
  }

  /* set actual audio out bufs (carla will write to these) */
  {
    size_t audio_ports = 0;
    for (auto &port : out_ports_)
      {
        if (port->id_.type_ == PortType::Audio)
          {
            outbufs_[audio_ports++] = &port->buf_[time_nfo.local_offset_];
          }
        if (audio_ports == max_variant_audio_outs_)
          break;
      }
  }

  /* set actual cv out bufs (carla will write to these) */
  {
    size_t cv_ports = 0;
    for (auto &port : out_ports_)
      {
        if (port->id_.type_ == PortType::CV)
          {
            outbufs_[max_variant_audio_outs_ + cv_ports++] =
              &port->buf_[time_nfo.local_offset_];
          }
        if (cv_ports == max_variant_cv_outs_)
          break;
      }
  }

  /* get main midi port */
  auto port = midi_in_port_;

  constexpr int                           MAX_EVENTS = 4000;
  std::array<NativeMidiEvent, MAX_EVENTS> events;
  int                                     num_events_written = 0;
  if (port)
    {
      for (auto &ev : port->midi_events_.active_events_)
        {
          if (
            ev.time_ < time_nfo.local_offset_
            || ev.time_ >= time_nfo.local_offset_ + time_nfo.nframes_)
            {
              /* skip events scheduled for another split within the processing
               * cycle
               */
#  if 0
          z_debug ("skip events scheduled for another split within the processing cycle: ev->time {}, local_offset {}, nframes {}", ev.time_, time_nfo.local_offset, time_nfo.nframes);
#  endif
              continue;
            }

#  if 0
      z_debug (
        "writing plugin input event {} "
        "at time {} - "
        "local offset {} nframes {}",
        num_events_written,
        ev.time_ - time_nfo.local_offset,
        time_nfo.local_offset, time_nfo.nframes);
      ev.print ();
#  endif

          /* event time is relative to the current zrythm full cycle (not
           * split). it needs to be made relative to the current split */
          events[num_events_written].time = ev.time_ - time_nfo.local_offset_;
          events[num_events_written].size = 3;
          events[num_events_written].data[0] = ev.raw_buffer_[0];
          events[num_events_written].data[1] = ev.raw_buffer_[1];
          events[num_events_written].data[2] = ev.raw_buffer_[2];
          ++num_events_written;

          if (num_events_written == MAX_EVENTS)
            {
              z_warning ("written {} events", MAX_EVENTS);
              break;
            }
        }
    }
  if (num_events_written > 0)
    {
#  if 0
      engine_process_time_info_print (time_nfo);
      z_debug (
        "Carla plugin {} has {} MIDI events",
        plugin_->setting->descr->name, num_events_written);
#  endif
    }

  native_plugin_descriptor_->process (
    native_plugin_handle_, const_cast<float **> (inbufs_.data ()),
    outbufs_.data (), time_nfo.nframes_, events.data (),
    static_cast<uint32_t> (num_events_written));

  /* update latency */
  latency_ = get_latency ();
}

static ZPluginCategory
carla_category_to_zrythm_category (CarlaBackend::PluginCategory category)
{
  switch (category)
    {
    case CarlaBackend::PLUGIN_CATEGORY_NONE:
      return ZPluginCategory::NONE;
    case CarlaBackend::PLUGIN_CATEGORY_SYNTH:
      return ZPluginCategory::INSTRUMENT;
    case CarlaBackend::PLUGIN_CATEGORY_DELAY:
      return ZPluginCategory::DELAY;
    case CarlaBackend::PLUGIN_CATEGORY_EQ:
      return ZPluginCategory::EQ;
    case CarlaBackend::PLUGIN_CATEGORY_FILTER:
      return ZPluginCategory::FILTER;
    case CarlaBackend::PLUGIN_CATEGORY_DISTORTION:
      return ZPluginCategory::DISTORTION;
    case CarlaBackend::PLUGIN_CATEGORY_DYNAMICS:
      return ZPluginCategory::DYNAMICS;
    case CarlaBackend::PLUGIN_CATEGORY_MODULATOR:
      return ZPluginCategory::MODULATOR;
    case CarlaBackend::PLUGIN_CATEGORY_UTILITY:
      return ZPluginCategory::UTILITY;
    case CarlaBackend::PLUGIN_CATEGORY_OTHER:
      return ZPluginCategory::NONE;
    }
  z_return_val_if_reached (ZPluginCategory::NONE);
}

static std::string
carla_category_to_zrythm_category_str (CarlaBackend::PluginCategory category)
{
  switch (category)
    {
    case CarlaBackend::PLUGIN_CATEGORY_SYNTH:
      return "Instrument";
    case CarlaBackend::PLUGIN_CATEGORY_DELAY:
      return "Delay";
    case CarlaBackend::PLUGIN_CATEGORY_EQ:
      return "Equalizer";
    case CarlaBackend::PLUGIN_CATEGORY_FILTER:
      return "Filter";
    case CarlaBackend::PLUGIN_CATEGORY_DISTORTION:
      return "Distortion";
    case CarlaBackend::PLUGIN_CATEGORY_DYNAMICS:
      return "Dynamics";
    case CarlaBackend::PLUGIN_CATEGORY_MODULATOR:
      return "Modulator";
    case CarlaBackend::PLUGIN_CATEGORY_UTILITY:
      return "Utility";
    case CarlaBackend::PLUGIN_CATEGORY_OTHER:
    case CarlaBackend::PLUGIN_CATEGORY_NONE:
    default:
      return "Plugin";
    }
  z_return_val_if_reached ("");
}

std::unique_ptr<PluginDescriptor>
CarlaNativePlugin::get_descriptor_from_cached (
  const CarlaCachedPluginInfo &info,
  PluginType                   type)
{
  auto descr = std::make_unique<PluginDescriptor> ();

  switch (type)
    {
#  if 0
    case CarlalBackend::PLUGIN_INTERNAL:
      descr->protocol = PluginProtocol::CARLA_INTERNAL;
      break;
    case CarlalBackend::PLUGIN_LADSPA:
      descr->protocol = PluginProtocol::LADSPA;
      break;
    case CarlalBackend::PLUGIN_DSSI:
      descr->protocol = PluginProtocol::DSSI;
      break;
#  endif
    case CarlaBackend::PLUGIN_LV2:
      descr->protocol_ = PluginProtocol::LV2;
      break;
    case CarlaBackend::PLUGIN_VST2:
      descr->protocol_ = PluginProtocol::VST;
      break;
#  if 0
    case CarlalBackend::PLUGIN_SF2:
      descr->protocol = PluginProtocol::SF2;
      break;
    case CarlalBackend::PLUGIN_SFZ:
      descr->protocol = PluginProtocol::SFZ;
      break;
    case CarlalBackend::PLUGIN_JACK:
      descr->protocol = PluginProtocol::JACK;
      break;
#  endif
    default:
      z_warn_if_reached ();
      break;
    }
  descr->name_ = info.name;
  descr->author_ = info.maker;
  descr->num_audio_ins_ = (int) info.audioIns;
  descr->num_audio_outs_ = (int) info.audioOuts;
  descr->num_midi_ins_ = (int) info.midiIns;
  descr->num_midi_outs_ = (int) info.midiOuts;
  descr->num_ctrl_ins_ = (int) info.parameterIns;
  descr->num_ctrl_outs_ = (int) info.parameterOuts;

  descr->category_ = carla_category_to_zrythm_category (info.category);
  descr->category_str_ = carla_category_to_zrythm_category_str (info.category);
  descr->min_bridge_mode_ = descr->get_min_bridge_mode ();
  descr->has_custom_ui_ = info.hints & CarlaBackend::PLUGIN_HAS_CUSTOM_UI;

  return descr;
}

CarlaNativePlugin::CarlaNativePlugin (
  const PluginSetting            &setting,
  unsigned int                    track_name_hash,
  zrythm::plugins::PluginSlotType slot_type,
  int                             slot)
    : Plugin (setting, track_name_hash, slot_type, slot)
{
  z_return_if_fail (setting.open_with_carla_);
}

void
CarlaNativePlugin::create_ports (bool loading)
{
  z_debug ("{}: loading: {}", __func__, loading);

  const auto &descr = setting_->descr_;
  if (!loading)
    {
      const CarlaPortCountInfo * audio_port_count_nfo =
        carla_get_audio_port_count_info (host_handle_, 0);
      /* TODO investigate:
       * sometimes port count info returns 1 audio in
       * but the descriptor thinks the plugin has 2 audio
       * ins
       * happens with WAVES Abbey Road Saturator Mono when
       * bridged with yabridge (VST2) */
      z_return_if_fail_cmp (
        (int) audio_port_count_nfo->ins, ==, descr->num_audio_ins_);
      int audio_ins_to_create =
        descr->num_audio_ins_ == 1 ? 2 : descr->num_audio_ins_;
      for (int i = 0; i < audio_ins_to_create; i++)
        {
          std::string port_name = fmt::format ("{} {}", _ ("Audio in"), i);
          auto port = std::make_unique<AudioPort> (port_name, PortFlow::Input);
          port->id_.sym_ = fmt::format ("audio_in_{}", i);
#  ifdef CARLA_HAVE_AUDIO_PORT_HINTS
          unsigned int audio_port_hints = carla_get_audio_port_hints (
            host_handle_, 0, false,
            (uint32_t) (descr->num_audio_ins_ == 1 ? 0 : i));
          z_debug ("audio port hints {}: {}", i, audio_port_hints);
          if (audio_port_hints & CarlaBackend::AUDIO_PORT_IS_SIDECHAIN)
            {
              z_debug ("{} is sidechain", port->id_.sym_.c_str ());
              port->id_.flags_ |= PortIdentifier::Flags::Sidechain;
            }
#  endif
          add_in_port (std::move (port));
        }

      /* set L/R for sidechain groups (assume stereo) */
      size_t num_default_sidechains_added = 0;
      for (auto &port : in_ports_)
        {
          if (
            port->id_.type_ == PortType::Audio
            && ENUM_BITSET_TEST (
              PortIdentifier::Flags, port->id_.flags_,
              PortIdentifier::Flags::Sidechain)
            && port->id_.port_group_.empty ()
            && !(ENUM_BITSET_TEST (
              PortIdentifier::Flags, port->id_.flags_,
              PortIdentifier::Flags::StereoL))
            && !(ENUM_BITSET_TEST (
              PortIdentifier::Flags, port->id_.flags_,
              PortIdentifier::Flags::StereoR)))
            {
              port->id_.port_group_ = ("[Zrythm] Sidechain Group");
              if (num_default_sidechains_added == 0)
                {
                  port->id_.flags_ |= PortIdentifier::Flags::StereoL;
                  num_default_sidechains_added++;
                }
              else if (num_default_sidechains_added == 1)
                {
                  port->id_.flags_ |= PortIdentifier::Flags::StereoR;
                  break;
                }
            }
        }

      size_t audio_outs_to_create =
        descr->num_audio_outs_ == 1 ? 2 : descr->num_audio_outs_;
      for (size_t i = 0; i < audio_outs_to_create; i++)
        {
          auto port = std::make_unique<AudioPort> (
            fmt::format ("{} {}", _ ("Audio out"), i), PortFlow::Output);
          port->id_.sym_ = fmt::format ("audio_out_{}", i);
          add_out_port (std::move (port));
        }
      for (int i = 0; i < descr->num_midi_ins_; i++)
        {
          auto port = std::make_unique<MidiPort> (
            fmt::format ("{} {}", _ ("MIDI in"), i), PortFlow::Input);
          port->id_.sym_ = fmt::format ("midi_in_{}", i);
          port->id_.flags2_ |= PortIdentifier::Flags2::SupportsMidi;
          add_in_port (std::move (port));
        }
      for (int i = 0; i < descr->num_midi_outs_; i++)
        {
          auto port = std::make_unique<MidiPort> (
            fmt::format ("{} {}", _ ("MIDI out"), i), PortFlow::Output);
          port->id_.sym_ = fmt::format ("midi_out_{}", i);
          port->id_.flags2_ |= PortIdentifier::Flags2::SupportsMidi;
          add_out_port (std::move (port));
        }
      for (int i = 0; i < descr->num_cv_ins_; i++)
        {
          auto port = std::make_unique<CVPort> (
            fmt::format ("{} {}", _ ("CV in"), i), PortFlow::Input);
          port->id_.sym_ = fmt::format ("cv_in_{}", i);
          add_in_port (std::move (port));
        }
      for (int i = 0; i < descr->num_cv_outs_; i++)
        {
          auto port = std::make_unique<CVPort> (
            fmt::format ("{} {}", _ ("CV out"), i), PortFlow::Output);
          port->id_.sym_ = fmt::format ("cv_out_{}", i);
          add_out_port (std::move (port));
        }
    }

  /* create controls */
  const CarlaPortCountInfo * param_counts =
    carla_get_parameter_count_info (host_handle_, 0);
#  if 0
  /* FIXME eventually remove this line. this is added
   * because carla discovery reports 0 params for
   * AU plugins, so we update the descriptor here */
  descr->num_ctrl_ins = (int) param_counts->ins;
  z_info (
    "params: %d ins and %d outs",
    descr->num_ctrl_ins, (int) param_counts->outs);
#  endif
  for (uint32_t i = 0; i < param_counts->ins; i++)
    {
      ControlPort * added_port = nullptr;
      if (loading)
        {
          auto port = get_port_from_param_id (i);
          if (!port)
            {
              const CarlaParameterInfo * param_info =
                carla_get_parameter_info (host_handle_, 0, i);
              z_warning (
                "port '%s' at param ID {} could not be retrieved [%s], will ignore",
                param_info->name, i, get_name ());
              continue;
            }
          added_port = port;
        }
      /* else if not loading (create new ports) */
      else
        {
          const CarlaParameterInfo * param_info =
            carla_get_parameter_info (host_handle_, 0, i);
          auto port = std::make_unique<ControlPort> (param_info->name);
          if (param_info->symbol && strlen (param_info->symbol) > 0)
            {
              port->id_.sym_ = param_info->symbol;
            }
          else
            {
              port->id_.sym_ = fmt::format ("param_{}", i);
            }
          port->id_.flags_ |= PortIdentifier::Flags::PluginControl;
          if (param_info->comment && strlen (param_info->comment) > 0)
            {
              port->id_.comment_ = param_info->comment;
            }
          if (param_info->unit && strlen (param_info->unit) > 0)
            {
              port->set_unit_from_str (param_info->unit);
            }
          if (param_info->groupName && strlen (param_info->groupName) > 0)
            {
              char * sub =
                string_get_substr_before_suffix (param_info->groupName, ":");
              z_return_if_fail (sub);
              port->id_.port_group_ = sub;
              g_free (sub);
            }
          port->carla_param_id_ = (int) i;

          const NativeParameter * native_param =
            native_plugin_descriptor_->get_parameter_info (
              native_plugin_handle_, i);
          z_return_if_fail (native_param);
          if (native_param->hints & NATIVE_PARAMETER_IS_LOGARITHMIC)
            {
              port->id_.flags_ |= PortIdentifier::Flags::Logarithmic;
            }
          if (native_param->hints & NATIVE_PARAMETER_IS_AUTOMABLE)
            {
              port->id_.flags_ |= PortIdentifier::Flags::Automatable;
            }
          if (!(native_param->hints & NATIVE_PARAMETER_IS_ENABLED))
            {
              port->id_.flags_ |= PortIdentifier::Flags::NotOnGui;
            }
          if (native_param->hints & NATIVE_PARAMETER_IS_BOOLEAN)
            {
              port->id_.flags_ |= PortIdentifier::Flags::Toggle;
            }
          else if (native_param->hints & NATIVE_PARAMETER_USES_SCALEPOINTS)
            {
              port->id_.flags2_ |= PortIdentifier::Flags2::Enumeration;
            }
          else if (native_param->hints & NATIVE_PARAMETER_IS_INTEGER)
            {
              port->id_.flags_ |= PortIdentifier::Flags::Integer;
            }

          /* get scale points */
          if (param_info->scalePointCount > 0)
            {
              port->scale_points_.reserve (param_info->scalePointCount);
            }
          for (uint32_t j = 0; j < param_info->scalePointCount; j++)
            {
              const CarlaScalePointInfo * scale_point_info =
                carla_get_parameter_scalepoint_info (host_handle_, 0, i, j);

              port->scale_points_.emplace_back (
                scale_point_info->value, scale_point_info->label);
            }
          std::sort (port->scale_points_.begin (), port->scale_points_.end ());

          added_port =
            static_cast<ControlPort *> (add_in_port (std::move (port)));

          for (uint32_t j = 0; j < param_info->scalePointCount; j++)
            {
              const CarlaScalePointInfo * sp_info =
                carla_get_parameter_scalepoint_info (host_handle_, 0, i, j);
              z_debug ("scale point: {}", sp_info->label);
            }

          const ParameterRanges * ranges =
            carla_get_parameter_ranges (host_handle_, 0, i);
          added_port->deff_ = ranges->def;
          added_port->minf_ = ranges->min;
          added_port->maxf_ = ranges->max;
#  if 0
          z_debug (
            "ranges: min {:f} max {:f} default {:f}",
            (double) port->minf_,
            (double) port->maxf_,
            (double) port->deff_;
#  endif
        }
      float cur_val = get_param_value (i);
      z_debug (
        "{}: {}={} {}", i, added_port->get_label (), cur_val,
        loading ? " (loading)" : "");
      added_port->set_control_value (cur_val, false, false);
    }

  ports_created_ = true;
}

void
CarlaNativePlugin::update_buffer_size_and_sample_rate ()
{
  const auto &engine = *AUDIO_ENGINE;
  z_debug (
    "setting carla buffer size and sample rate: "
    "{} {}",
    engine.block_length_, engine.sample_rate_);
  carla_set_engine_buffer_size_and_sample_rate (
    host_handle_, engine.block_length_, engine.sample_rate_);

  /* update processing buffers */
  const auto max_variant_ins = max_variant_audio_ins_ + max_variant_cv_ins_;
  zero_inbufs_.resize (max_variant_ins);
  inbufs_.resize (max_variant_ins);
  for (size_t i = 0; i < max_variant_ins; i++)
    {
      zero_inbufs_[i].resize (engine.block_length_);
      dsp_fill (zero_inbufs_[i].data (), 1e-20f, engine.block_length_);
      inbufs_[i] = zero_inbufs_[i].data ();
    }

  const auto max_variant_outs = max_variant_audio_outs_ + max_variant_cv_outs_;
  zero_outbufs_.resize (max_variant_outs);
  outbufs_.resize (max_variant_outs);
  for (size_t i = 0; i < max_variant_outs; i++)
    {
      zero_outbufs_[i].resize (engine.block_length_);
      dsp_fill (zero_outbufs_[i].data (), 1e-20f, engine.block_length_);
      outbufs_[i] = zero_outbufs_[i].data ();
    }

  if (native_plugin_descriptor_->dispatcher)
    {
      intptr_t ret = native_plugin_descriptor_->dispatcher (
        native_plugin_handle_, NATIVE_PLUGIN_OPCODE_BUFFER_SIZE_CHANGED, 0,
        engine.block_length_, nullptr, 0.f);
      z_return_if_fail (ret == 0);
      ret = native_plugin_descriptor_->dispatcher (
        native_plugin_handle_, NATIVE_PLUGIN_OPCODE_SAMPLE_RATE_CHANGED, 0, 0,
        nullptr, (float) engine.sample_rate_);
      z_return_if_fail (ret == 0);
    }
  else
    {
      z_warning ("native plugin descriptor has no dispatcher");
    }
}

bool
CarlaNativePlugin::add_internal_plugin_from_descr (
  const zrythm::plugins::PluginDescriptor &descr)
{
  /** Number of instances to instantiate (1 normally or 2 for mono plugins). */
  auto num_instances = descr.num_audio_ins_ == 1 ? 2 : 1;

  const auto type = zrythm::plugins::PluginDescriptor::
    get_carla_plugin_type_from_protocol (descr.protocol_);
  bool added = false;

  for (int i = 0; i < num_instances; i++)
    {
      switch (descr.protocol_)
        {
        case PluginProtocol::LV2:
        case PluginProtocol::AU:
          z_debug ("uri {}", descr.uri_);
          added = carla_add_plugin (
            host_handle_,
            descr.arch_ == PluginArchitecture::ARCH_64_BIT
              ? CarlaBackend::BINARY_NATIVE
              : CarlaBackend::BINARY_WIN32,
            type, nullptr, descr.name_.c_str (), descr.uri_.c_str (), 0,
            nullptr, CarlaBackend::PLUGIN_OPTIONS_NULL);
          break;
        case PluginProtocol::VST:
        case PluginProtocol::VST3:
          added = carla_add_plugin (
            host_handle_,
            descr.arch_ == PluginArchitecture::ARCH_64_BIT
              ? CarlaBackend::BINARY_NATIVE
              : CarlaBackend::BINARY_WIN32,
            type, descr.path_.string ().c_str (), descr.name_.c_str (),
            descr.name_.c_str (), descr.unique_id_, nullptr,
            CarlaBackend::PLUGIN_OPTIONS_NULL);
          break;
        case PluginProtocol::DSSI:
        case PluginProtocol::LADSPA:
          added = carla_add_plugin (
            host_handle_, CarlaBackend::BINARY_NATIVE, type,
            descr.path_.string ().c_str (), descr.name_.c_str (),
            descr.uri_.c_str (), 0, nullptr, CarlaBackend::PLUGIN_OPTIONS_NULL);
          break;
        case PluginProtocol::SFZ:
        case PluginProtocol::SF2:
          added = carla_add_plugin (
            host_handle_, CarlaBackend::BINARY_NATIVE, type,
            descr.path_.string ().c_str (), descr.name_.c_str (),
            descr.name_.c_str (), 0, nullptr, CarlaBackend::PLUGIN_OPTIONS_NULL);
          break;
        case PluginProtocol::JSFX:
          {
            /* the URI is a relative path - make it absolute */
            auto pl_path = PLUGIN_MANAGER->find_plugin_from_rel_path (
              descr.protocol_, descr.uri_);
            added = carla_add_plugin (
              host_handle_,
              descr.arch_ == PluginArchitecture::ARCH_64_BIT
                ? CarlaBackend::BINARY_NATIVE
                : CarlaBackend::BINARY_WIN32,
              type, pl_path.string ().c_str (), descr.name_.c_str (),
              descr.name_.c_str (), descr.unique_id_, nullptr,
              CarlaBackend::PLUGIN_OPTIONS_NULL);
          }
          break;
        case PluginProtocol::CLAP:
          added = carla_add_plugin (
            host_handle_,
            descr.arch_ == PluginArchitecture::ARCH_64_BIT
              ? CarlaBackend::BINARY_NATIVE
              : CarlaBackend::BINARY_WIN32,
            type, descr.path_.string ().c_str (), descr.name_.c_str (),
            descr.uri_.c_str (), 0, nullptr, CarlaBackend::PLUGIN_OPTIONS_NULL);
          break;
        default:
          z_return_val_if_reached (-1);
          break;
        }

      if (!added)
        return added;
    }

  return added;
}

void
CarlaNativePlugin::instantiate_impl (bool loading, bool use_state_file)
{
  z_debug (
    "loading: {}, use state file: {}, ports_created: {}", loading,
    use_state_file, ports_created_);

  native_host_descriptor_.handle = this;
  native_host_descriptor_.uiName = "Zrythm";

  native_host_descriptor_.uiParentId = 0;

  /* set resources dir */
  const char * carla_filename = carla_get_library_filename ();
  auto         dir =
    fs::path (carla_filename).parent_path ().parent_path ().parent_path ();
  auto res_dir = dir / "share" / "carla" / "resources";
  // FIXME leak
  native_host_descriptor_.resourceDir = g_strdup (res_dir.string ().c_str ());

  native_host_descriptor_.get_buffer_size = host_get_buffer_size;
  native_host_descriptor_.get_sample_rate = host_get_sample_rate;
  native_host_descriptor_.is_offline = host_is_offline;
  native_host_descriptor_.get_time_info = host_get_time_info;
  native_host_descriptor_.write_midi_event = host_write_midi_event;
  native_host_descriptor_.ui_parameter_changed = host_ui_parameter_changed;
  native_host_descriptor_.ui_custom_data_changed = host_ui_custom_data_changed;
  native_host_descriptor_.ui_closed = host_ui_closed;
  native_host_descriptor_.ui_open_file = nullptr;
  native_host_descriptor_.ui_save_file = nullptr;
  native_host_descriptor_.dispatcher = host_dispatcher;

  time_info_.bbt.valid = true;

  /* choose most appropriate patchbay variant */
  max_variant_midi_ins_ = 1;
  max_variant_midi_outs_ = 1;
  const auto &descr = setting_->descr_;
  if (
    descr->num_audio_ins_ <= 2 && descr->num_audio_outs_ <= 2
    && descr->num_cv_ins_ == 0 && descr->num_cv_outs_ == 0)
    {
      native_plugin_descriptor_ = carla_get_native_patchbay_plugin ();
      max_variant_audio_ins_ = 2;
      max_variant_audio_outs_ = 2;
      z_debug ("using standard patchbay variant");
    }
  else if (
    descr->num_audio_ins_ <= 16 && descr->num_audio_outs_ <= 16
    && descr->num_cv_ins_ == 0 && descr->num_cv_outs_ == 0)
    {
      native_plugin_descriptor_ = carla_get_native_patchbay16_plugin ();
      max_variant_audio_ins_ = 16;
      max_variant_audio_outs_ = 16;
      z_debug ("using patchbay 16 variant");
    }
  else if (
    descr->num_audio_ins_ <= 32 && descr->num_audio_outs_ <= 32
    && descr->num_cv_ins_ == 0 && descr->num_cv_outs_ == 0)
    {
      native_plugin_descriptor_ = carla_get_native_patchbay32_plugin ();
      max_variant_audio_ins_ = 32;
      max_variant_audio_outs_ = 32;
      z_debug ("using patchbay 32 variant");
    }
  else if (
    descr->num_audio_ins_ <= 64 && descr->num_audio_outs_ <= 64
    && descr->num_cv_ins_ == 0 && descr->num_cv_outs_ == 0)
    {
      native_plugin_descriptor_ = carla_get_native_patchbay64_plugin ();
      max_variant_audio_ins_ = 64;
      max_variant_audio_outs_ = 64;
      z_debug ("using patchbay 64 variant");
    }
  else if (
    descr->num_audio_ins_ <= 2 && descr->num_audio_outs_ <= 2
    && descr->num_cv_ins_ <= 5 && descr->num_cv_outs_ <= 5)
    {
      native_plugin_descriptor_ = carla_get_native_patchbay_cv_plugin ();
      max_variant_audio_ins_ = 2;
      max_variant_audio_outs_ = 2;
      max_variant_cv_ins_ = 5;
      max_variant_cv_outs_ = 5;
      z_debug ("using patchbay CV variant");
    }
#  ifdef CARLA_HAVE_CV8_PATCHBAY_VARIANT
  else if (
    descr->num_audio_ins_ <= 2 && descr->num_audio_outs_ <= 2
    && descr->num_cv_ins_ <= 8 && descr->num_cv_outs_ <= 8)
    {
      native_plugin_descriptor_ = carla_get_native_patchbay_cv8_plugin ();
      max_variant_audio_ins_ = 2;
      max_variant_audio_outs_ = 2;
      max_variant_cv_ins_ = 8;
      max_variant_cv_outs_ = 8;
      z_debug ("using patchbay CV8 variant");
    }
#  endif
#  ifdef CARLA_HAVE_CV32_PATCHBAY_VARIANT
  else if (
    descr->num_audio_ins_ <= 64 && descr->num_audio_outs_ <= 64
    && descr->num_cv_ins_ <= 32 && descr->num_cv_outs_ <= 32)
    {
      native_plugin_descriptor_ = carla_get_native_patchbay_cv32_plugin ();
      max_variant_audio_ins_ = 64;
      max_variant_audio_outs_ = 64;
      max_variant_cv_ins_ = 32;
      max_variant_cv_outs_ = 32;
      z_debug ("using patchbay CV32 variant");
    }
#  endif
  else
    {
      z_warning (
        "Plugin with {} audio ins, {} audio outs, {} CV ins, {} CV outs not fully supported. Using standard Carla Patchbay variant",
        descr->num_audio_ins_, descr->num_audio_outs_, descr->num_cv_ins_,
        descr->num_cv_outs_);
      native_plugin_descriptor_ = carla_get_native_patchbay_plugin ();
      max_variant_audio_ins_ = 2;
      max_variant_audio_outs_ = 2;
    }

  unsigned int max_variant_ins = max_variant_audio_ins_ + max_variant_cv_ins_;
  zero_inbufs_.resize (max_variant_ins);
  inbufs_.resize (max_variant_ins);
  for (auto &buf : zero_inbufs_)
    {
      buf.resize (AUDIO_ENGINE->block_length_);
    }
  unsigned int max_variant_outs = max_variant_audio_outs_ + max_variant_cv_outs_;
  zero_outbufs_.resize (max_variant_outs);
  outbufs_.resize (max_variant_ins);
  for (auto &buf : zero_outbufs_)
    {
      buf.resize (AUDIO_ENGINE->block_length_);
    }

  /* instantiate the plugin to get its info */
  native_plugin_handle_ =
    native_plugin_descriptor_->instantiate (&native_host_descriptor_);
  if (!native_plugin_handle_)
    {
      throw ZrythmException ("Failed to get native plugin handle");
    }
  host_handle_ = carla_create_native_plugin_host_handle (
    native_plugin_descriptor_, native_plugin_handle_);
  if (!host_handle_)
    {
      throw ZrythmException ("Failed to get host handle");
    }
  carla_plugin_id_ = 0;

  /* set binary paths */
  auto * dir_mgr = DirectoryManager::getInstance ();
  auto   zrythm_libdir =
    dir_mgr->get_dir (DirectoryManager::DirectoryType::SYSTEM_ZRYTHM_LIBDIR);
  auto carla_binaries_dir = fs::path (zrythm_libdir) / "carla";
  z_debug (
    "setting carla engine option [ENGINE_OPTION_PATH_BINARIES] to '{}'",
    carla_binaries_dir);
  carla_set_engine_option (
    host_handle_, CarlaBackend::ENGINE_OPTION_PATH_BINARIES, 0,
    carla_binaries_dir.string ().c_str ());

  /* set carla resource paths */
  carla_set_engine_option (
    host_handle_, CarlaBackend::ENGINE_OPTION_PATH_RESOURCES, 0,
    res_dir.string ().c_str ());

  /* set plugin paths */
  {
    auto paths =
      PLUGIN_MANAGER->get_paths_for_protocol_separated (get_protocol ());
    auto ptype = zrythm::plugins::PluginDescriptor::
      get_carla_plugin_type_from_protocol (get_protocol ());
    carla_set_engine_option (
      host_handle_, CarlaBackend::ENGINE_OPTION_PLUGIN_PATH, ptype,
      paths.c_str ());
  }

  /* set UI scale factor */
  carla_set_engine_option (
    host_handle_, CarlaBackend::ENGINE_OPTION_FRONTEND_UI_SCALE,
    static_cast<int> (ui_scale_factor_ * 1000.f), nullptr);

  /* set whether UI should stay on top */
  /* disable for now */
#  if 0
  if (!ZRYTHM_TESTING && !ZRYTHM_BENCHMARKING &&
      g_settings_get_boolean (
        S_P_PLUGINS_UIS, "stay-on-top"))
    {
      carla_set_engine_option (
        host_handle_,
        CarlaBackend::ENGINE_OPTION_UIS_ALWAYS_ON_TOP, true,
        nullptr);
    }
#  endif

  update_buffer_size_and_sample_rate ();

  z_debug ("using bridge mode {}", ENUM_NAME (setting_->bridge_mode_));

  /* set bridging on if needed */
  switch (setting_->bridge_mode_)
    {
    case zrythm::plugins::CarlaBridgeMode::Full:
      z_debug ("plugin must be bridged whole, using plugin bridge");
      carla_set_engine_option (
        host_handle_, CarlaBackend::ENGINE_OPTION_PREFER_PLUGIN_BRIDGES, true,
        nullptr);
      break;
    case zrythm::plugins::CarlaBridgeMode::UI:
      z_debug ("using UI bridge only");
      carla_set_engine_option (
        host_handle_, CarlaBackend::ENGINE_OPTION_PREFER_UI_BRIDGES, true,
        nullptr);
      break;
    default:
      break;
    }

  /* raise bridge timeout to 8 sec */
  if (
    setting_->bridge_mode_ == zrythm::plugins::CarlaBridgeMode::Full
    || setting_->bridge_mode_ == zrythm::plugins::CarlaBridgeMode::UI)
    {
      carla_set_engine_option (
        host_handle_, CarlaBackend::ENGINE_OPTION_UI_BRIDGES_TIMEOUT, 8000,
        nullptr);
    }

  intptr_t uses_embed_ret = native_plugin_descriptor_->dispatcher (
    native_plugin_handle_, NATIVE_PLUGIN_OPCODE_HOST_USES_EMBED, 0, 0, nullptr,
    0.f);
  if (uses_embed_ret != 0)
    {
      throw ZrythmException ("Failed to set USES_EMBED");
    }

  bool added = add_internal_plugin_from_descr (*descr);

  update_buffer_size_and_sample_rate ();

  if (!added)
    {
      throw ZrythmException (format_str (
        _ ("Error adding carla plugin: {}"),
        carla_get_last_error (host_handle_)));
    }

    /* enable various messages */
#  define ENABLE_OPTION(x) \
    carla_set_option (host_handle_, 0, CarlaBackend::PLUGIN_OPTION_##x, true)

  ENABLE_OPTION (SEND_CONTROL_CHANGES);
  ENABLE_OPTION (SEND_CHANNEL_PRESSURE);
  ENABLE_OPTION (SEND_NOTE_AFTERTOUCH);
  ENABLE_OPTION (SEND_PITCHBEND);
  ENABLE_OPTION (SEND_ALL_SOUND_OFF);
  ENABLE_OPTION (SEND_PROGRAM_CHANGES);

#  undef ENABLE_OPTION

  /* add engine callback */
  carla_set_engine_callback (host_handle_, carla_engine_callback, this);

  z_debug ("refreshing patchbay");
  carla_patchbay_refresh (host_handle_, false);
  z_debug ("refreshed patchbay");

  /* connect to patchbay ports */
  unsigned int num_audio_ins_connected = 0;
  unsigned int num_audio_outs_connected = 0;
  unsigned int num_cv_ins_connected = 0;
  unsigned int num_cv_outs_connected = 0;
  unsigned int num_midi_ins_connected = 0;
  unsigned int num_midi_outs_connected = 0;
  bool is_cv_variant = max_variant_cv_ins_ > 0 || max_variant_cv_outs_ > 0;
  for (const auto &nfo : patchbay_port_info_)
    {
      unsigned int plugin_id = nfo.plugin_id;
      unsigned int port_hints = nfo.port_hints;
      unsigned int port_id = nfo.port_id;
      const auto  &port_name = nfo.port_name;
      z_debug (
        "processing {}, plugin id {}, portid {}, port hints {}", port_name,
        plugin_id, port_id, port_hints);
      bool connected = false;
      if (port_hints & CarlaBackend::PATCHBAY_PORT_IS_INPUT)
        {
          if (
            port_hints & CarlaBackend::PATCHBAY_PORT_TYPE_AUDIO
            && num_audio_ins_connected < max_variant_audio_ins_)
            {
              z_debug (
                "connecting {}:{} to {}:{}", 1,
                audio_input_port_id_ + num_audio_ins_connected, plugin_id,
                port_id);
              connected = carla_patchbay_connect (
                host_handle_, false, 1,
                audio_input_port_id_ + num_audio_ins_connected++, plugin_id,
                port_id);
              if (!connected)
                {
                  z_error (
                    "Failed to connect: {}",
                    carla_get_last_error (host_handle_));
                }
            }
          else if (
            port_hints & CarlaBackend::PATCHBAY_PORT_TYPE_CV
            && num_cv_ins_connected < max_variant_cv_ins_)
            {
              z_debug (
                "connecting {}:{} to {}:{}", 3,
                cv_input_port_id_ + num_cv_ins_connected, plugin_id, port_id);
              connected = carla_patchbay_connect (
                host_handle_, false, 3,
                cv_input_port_id_ + num_cv_ins_connected++, plugin_id, port_id);
              if (!connected)
                {
                  z_error (
                    "Failed to connect: {}",
                    carla_get_last_error (host_handle_));
                }
            }
          else if (
            port_hints & CarlaBackend::PATCHBAY_PORT_TYPE_MIDI
            && num_midi_ins_connected < max_variant_midi_ins_)
            {
              z_debug (
                "connecting {}:{} to {}:{}", is_cv_variant ? 5 : 3,
                midi_input_port_id_ + num_midi_ins_connected, plugin_id,
                port_id);
              connected = carla_patchbay_connect (
                host_handle_, false, is_cv_variant ? 5 : 3,
                midi_input_port_id_ + num_midi_ins_connected++, plugin_id,
                port_id);
              if (!connected)
                {
                  z_error (
                    "Failed to connect: {}",
                    carla_get_last_error (host_handle_));
                }
            }
        }
      /* else if output */
      else
        {
          if (
            port_hints & CarlaBackend::PATCHBAY_PORT_TYPE_AUDIO
            && num_audio_outs_connected < max_variant_audio_outs_)
            {
              z_debug (
                "connecting {}:{} to {}:{}", plugin_id, port_id, 2,
                audio_output_port_id_ + num_audio_outs_connected);
              connected = carla_patchbay_connect (
                host_handle_, false, plugin_id, port_id, 2,
                audio_output_port_id_ + num_audio_outs_connected++);
              if (!connected)
                {
                  z_error (
                    "Failed to connect: {}",
                    carla_get_last_error (host_handle_));
                }
            }
          else if (
            port_hints & CarlaBackend::PATCHBAY_PORT_TYPE_CV
            && num_cv_outs_connected < max_variant_cv_outs_)
            {
              z_debug (
                "connecting {}:{} to {}:{}", plugin_id, port_id, 4,
                cv_output_port_id_ + num_cv_outs_connected);
              connected = carla_patchbay_connect (
                host_handle_, false, plugin_id, port_id, 4,
                cv_output_port_id_ + num_cv_outs_connected++);
              if (!connected)
                {
                  z_error (
                    "Failed to connect: {}",
                    carla_get_last_error (host_handle_));
                }
            }
          else if (
            port_hints & CarlaBackend::PATCHBAY_PORT_TYPE_MIDI
            && num_midi_outs_connected < max_variant_midi_outs_)
            {
              z_debug (
                "connecting {}:{} to {}:{}", plugin_id, port_id,
                is_cv_variant ? 6 : 4,
                midi_output_port_id_ + num_midi_outs_connected);
              connected = carla_patchbay_connect (
                host_handle_, false, plugin_id, port_id, is_cv_variant ? 6 : 4,
                midi_output_port_id_ + num_midi_outs_connected++);
              if (!connected)
                {
                  z_warning (
                    "Failed to connect: {}",
                    carla_get_last_error (host_handle_));
                }
            }
        }
    }

  if (use_state_file)
    {
      /* load the state */
      try
        {
          load_state (nullptr);
        }
      catch (const ZrythmException &e)
        {
          e.handle (_ ("Failed to load Carla state"));
        }
    }

  if (
    !native_plugin_handle_ || !native_plugin_descriptor_->activate
    || !native_plugin_descriptor_->ui_show)
    {
      instantiation_failed_ = true;
      throw ZrythmException (_ (
        "Failed to instantiate Carla plugin: handle/descriptor not initialized properly"));
    }

  /* create ports */
  if (!loading && !use_state_file && !ports_created_)
    {
      create_ports (false);
    }

  z_debug ("activating carla plugin...");
  native_plugin_descriptor_->activate (native_plugin_handle_);
  z_debug ("carla plugin activated");

  update_buffer_size_and_sample_rate ();

  /* load data into existing ports */
  if (loading || use_state_file)
    {
      create_ports (true);
    }
}

void
CarlaNativePlugin::open_custom_ui (bool show)
{
  z_return_if_fail (is_in_active_project ());

  z_debug ("show/hide '{} ({})' UI: {}", get_name (), fmt::ptr (this), show);

  if (
    (!idle_connection_.connected () && !show)
    || (idle_connection_.connected () && show))
    {
      z_info (
        "plugin already has visibility status %d, "
        "doing nothing",
        show);
      return;
    }

  switch (get_protocol ())
    {
    case PluginProtocol::VST:
    case PluginProtocol::VST3:
    case PluginProtocol::DSSI:
    case PluginProtocol::LV2:
    case PluginProtocol::AU:
    case PluginProtocol::CLAP:
    case PluginProtocol::JSFX:
      {
        if (show)
          {
            auto title = generate_window_title ();
            z_debug ("plugin window title '{}'", title);
            carla_set_custom_ui_title (host_handle_, 0, title.c_str ());

            /* set whether to keep window on top */
            if (
              ZRYTHM_HAVE_UI
              && g_settings_get_boolean (S_P_PLUGINS_UIS, "stay-on-top"))
              {
#  if HAVE_X11 && !defined(GDK_WINDOWING_WAYLAND)
                Window xid = z_gtk_window_get_x11_xid (GTK_WINDOW (MAIN_WINDOW));
                z_debug ("FRONTEND_WIN_ID: setting X11 parent to %lx", xid);
                char xid_str[400];
                sprintf (xid_str, "%lx", xid);
                carla_set_engine_option (
                  host_handle_, CarlaBackend::ENGINE_OPTION_FRONTEND_WIN_ID, 0,
                  xid_str);
#  else
                z_warning (
                  "stay-on-top unavailable on this "
                  "window manager");
#  endif
              }
          }

#  if defined(_WIN32)
        HWND hwnd = z_gtk_window_get_windows_hwnd (GTK_WINDOW (MAIN_WINDOW));
        z_debug (
          "FRONTEND_WIN_ID: setting Windows parent to HWND(0x{:p})",
          static_cast<const void *> (hwnd));
        char hwnd_str[400];
        sprintf (hwnd_str, "%" PRIxPTR, hwnd);
        carla_set_engine_option (
          host_handle_, CarlaBackend::ENGINE_OPTION_FRONTEND_WIN_ID, 0,
          hwnd_str);
#  endif

        {
          z_debug (
            "Attempting to %s UI for %s", show ? "show" : "hide", get_name ());
          GdkGLContext * context = clear_gl_context ();
          carla_show_custom_ui (host_handle_, 0, show);
          return_gl_context (context);
          z_debug ("Completed {} UI", show ? "show" : "hide");
          visible_ = show;
        }

        if (idle_connection_.connected ())
          {
            z_debug ("removing tick callback for {}", get_name ());
            idle_connection_.disconnect ();
          }

        if (show)
          {
            z_return_if_fail (MAIN_WINDOW);
            z_debug ("setting tick callback for {}", get_name ());
            idle_connection_ = Glib::signal_timeout ().connect (
              sigc::track_obj (
                sigc::mem_fun (*this, &CarlaNativePlugin::idle_cb), *this),
              /* 60 fps */
              1000 / 60, Glib::PRIORITY_DEFAULT);
          }

        if (ZRYTHM_HAVE_UI)
          {
            EVENTS_PUSH (EventType::ET_PLUGIN_WINDOW_VISIBILITY_CHANGED, this);
          }
      }
      break;
    default:
      break;
    }
}

void
CarlaNativePlugin::activate_impl (bool activate)
{
  z_debug ("setting plugin {} active {}", get_name (), activate);
  carla_set_active (host_handle_, 0, activate);
}

float
CarlaNativePlugin::get_param_value (const uint32_t id)
{
  return carla_get_current_parameter_value (host_handle_, 0, id);
}

void
CarlaNativePlugin::set_param_value (const uint32_t id, float val)
{
  if (instantiation_failed_)
    {
      return;
    }

  float cur_val = carla_get_current_parameter_value (host_handle_, 0, id);
  if (DEBUGGING && !math_floats_equal (cur_val, val))
    {
      z_debug ("setting param {} value to {:f}", id, (double) val);
    }
  carla_set_parameter_value (host_handle_, 0, id, val);
  if (carla_get_current_plugin_count (host_handle_) == 2)
    {
      carla_set_parameter_value (host_handle_, 1, id, val);
    }
}

MidiPort *
CarlaNativePlugin::get_midi_out_port ()
{
  for (auto &port : out_ports_)
    {
      if (
        port->id_.type_ == PortType::Event
        && ENUM_BITSET_TEST (
          PortIdentifier::Flags2, port->id_.flags2_,
          PortIdentifier::Flags2::SupportsMidi))
        return static_cast<MidiPort *> (port.get ());
    }

  z_return_val_if_reached (nullptr);
}

ControlPort *
CarlaNativePlugin::get_port_from_param_id (const uint32_t id)
{
  int j = 0;
  for (auto &port : in_ports_)
    {
      if (port->id_.type_ != PortType::Control)
        continue;

      auto ctrl_port = static_cast<ControlPort *> (port.get ());
      j = ctrl_port->carla_param_id_;
      if (static_cast<int> (id) == ctrl_port->carla_param_id_)
        return ctrl_port;
    }

  if (static_cast<int> (id) > j)
    {
      z_info (
        "index {} not found in input ports. this is likely an output param. ignoring",
        id);
      return nullptr;
    }

  z_return_val_if_reached (nullptr);
}

nframes_t
CarlaNativePlugin::get_latency () const
{
  return carla_get_plugin_latency (host_handle_, 0);
}

void
CarlaNativePlugin::save_state (bool is_backup, const std::string * abs_state_dir)
{
  if (!instantiated_)
    {
      z_debug ("plugin {} not instantiated, skipping saving state", get_name ());
      return;
    }

  auto dir_to_use =
    abs_state_dir ? *abs_state_dir : get_abs_state_dir (is_backup, true);

  io_mkdir (dir_to_use);

  auto state_file_abs_path = fs::path (dir_to_use) / CARLA_STATE_FILENAME;
  z_debug ("saving carla plugin state to {}", state_file_abs_path);

  if (!carla_save_plugin_state (
        host_handle_, 0, state_file_abs_path.string ().c_str ()))
    {
      throw ZrythmException (format_str (
        "Failed to save plugin state: {}", carla_get_last_error (host_handle_)));
    }

  z_warn_if_fail (!state_dir_.empty ());
}

void
CarlaNativePlugin::load_state (const std::string * abs_path)
{
  fs::path state_file;
  if (abs_path)
    {
      state_file = *abs_path;
    }
  else
    {
      if (state_dir_.empty ())
        {
          throw ZrythmException ("Plugin doesn't have a state directory");
        }
      auto state_dir_abs_path =
        get_abs_state_dir (PROJECT->loading_from_backup_, false);
      state_file = fs::path (state_dir_abs_path) / CARLA_STATE_FILENAME;
    }
  z_debug (
    "loading state from {} (given path: {})...", state_file.string (),
    abs_path ? *abs_path : "");

  if (!fs::exists (state_file))
    {
      throw ZrythmException (
        format_str ("State file {} doesn't exist", state_file.string ()));
    }

  loading_state_ = true;
  carla_load_plugin_state (host_handle_, 0, state_file.string ().c_str ());
  auto plugin_count = carla_get_current_plugin_count (host_handle_);
  if (plugin_count == 2)
    {
      carla_load_plugin_state (host_handle_, 1, state_file.string ().c_str ());
    }
  loading_state_ = false;
  if (visible_ && is_in_active_project ())
    {
      EVENTS_PUSH (EventType::ET_PLUGIN_VISIBILITY_CHANGED, this);
    }
  z_debug (
    "successfully loaded carla plugin state from {}", state_file.string ());
}

void
CarlaNativePlugin::init_after_cloning (
  const zrythm::plugins::CarlaNativePlugin &other)
{
  Plugin::copy_members_from (
    const_cast<Plugin &> (*dynamic_cast<const Plugin *> (&other)));
}

void
CarlaNativePlugin::close ()
{
  if (host_handle_)
    {
      z_debug ("setting carla engine about to close...");
      carla_set_engine_about_to_close (host_handle_);
    }

  idle_connection_.disconnect ();

  auto &descr = get_descriptor ();
  z_debug ("closing plugin {}...", descr.name_);
  if (native_plugin_descriptor_)
    {
      z_debug ("deactivating {}...", descr.name_);
      native_plugin_descriptor_->deactivate (native_plugin_handle_);
      z_debug ("deactivated {}", descr.name_);
      z_debug ("cleaning up {}...", descr.name_);
      native_plugin_descriptor_->cleanup (native_plugin_handle_);
      z_debug ("cleaned up {}", descr.name_);
      native_plugin_descriptor_ = nullptr;
    }
  if (host_handle_)
    {
      z_debug ("freeing host handle for {}...", descr.name_);
      carla_host_handle_free (host_handle_);
      host_handle_ = nullptr;
      z_debug ("free'd host handle for {}", descr.name_);
    }
  z_debug ("closed plugin {}", descr.name_);
}

CarlaNativePlugin::~CarlaNativePlugin ()
{
  close ();
}
#endif
