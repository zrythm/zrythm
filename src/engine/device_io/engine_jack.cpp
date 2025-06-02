// SPDX-FileCopyrightText: © 2018-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "gui/backend/backend/settings_manager.h"
#include "gui/backend/backend/zrythm.h"
#include "utils/gtest_wrapper.h"

#ifdef HAVE_JACK

#  include <cmath>

#  include "engine/device_io/engine.h"
#  include "engine/device_io/engine_jack.h"
#  include "engine/device_io/ext_port.h"
#  include "engine/session/router.h"
#  include "engine/session/transport.h"
#  include "gui/backend/backend/project.h"
#  include "gui/backend/backend/settings/settings.h"
#  include "gui/backend/ui.h"
#  include "gui/dsp/plugin.h"
#  include "gui/dsp/port.h"
#  include "structure/tracks/channel.h"
#  include "structure/tracks/tempo_track.h"
#  include "structure/tracks/tracklist.h"
#  include "utils/midi.h"
#  include "utils/mpmc_queue.h"
#  include "utils/object_pool.h"
#  include "utils/string.h"

#  if !defined _WIN32 && defined __GLIBC__
#    include <dlfcn.h>
#  endif

#  include <jack/thread.h>

namespace zrythm::engine::device_io
{

/**
 * @brief
 *
 */
void
JackDriver::rescan_ports ()
{
  /* get all input ports */
  const char ** ports =
    jack_get_ports (client_, nullptr, nullptr, JackPortIsPhysical);

  int i = 0;
  /*jack_port_t * jport;*/
  while (ports[i] != nullptr)
    {
      /*jport =*/
      /*jack_port_by_name (*/
      /*self->client,*/
      /*ports[i]);*/

      /*add_port_if_not_exists (*/
      /*self->physical_ins,*/
      /*&self->num_physical_ins,*/
      /*jport);*/

      i++;
    }

  /* TODO clear unconnected remembered ports */
}

void
JackDriver::handle_sample_rate_change (uint32_t samplerate)
{
  engine_.sample_rate_ = samplerate;

  if (P_TEMPO_TRACK)
    {
      int beats_per_bar = P_TEMPO_TRACK->get_beats_per_bar ();
      engine_.update_frames_per_tick (
        beats_per_bar, P_TEMPO_TRACK->get_current_bpm (), engine_.sample_rate_,
        true, true, false);
    }

  z_info ("JACK: Sample rate changed to {}", samplerate);
}

void
JackDriver::process_change_request ()
{
  /* process immediately if gtk thread */
  if (ZRYTHM_IS_QT_THREAD)
    {
      engine_.process_events ();
    }
  /* otherwise if activated wait for gtk thread to process all events */
  else if (engine_.activated_)
    {
      auto cur_time = SteadyClock::now ();
      while (
        cur_time > engine_.last_events_process_started_
        || cur_time > engine_.last_events_processed_)
        {
          z_debug ("-------- waiting for change");
          std::this_thread::sleep_for (std::chrono::milliseconds (1000));
        }
    }
}

int
JackDriver::sample_rate_cb (uint32_t nframes, void * data)
{
  auto * self = static_cast<JackDriver *> (data);
  self->engine_.push_event (
    AudioEngine::AudioEngineEventType::AUDIO_ENGINE_EVENT_SAMPLE_RATE_CHANGE,
    nullptr, nframes, 0.f);

  self->process_change_request ();

  return 0;
}

void
JackDriver::handle_buf_size_change (uint32_t frames)
{
  engine_.realloc_port_buffers (frames);
#  if defined(HAVE_JACK_PORT_TYPE_GET_BUFFER_SIZE)
  engine_.midi_buf_size_ =
    jack_port_type_get_buffer_size (client_, JACK_DEFAULT_MIDI_TYPE);
#  endif
  z_info (
    "JACK: Block length changed to {}, "
    "midi buf size to {}",
    engine_.block_length_, engine_.midi_buf_size_);
  handled_jack_buffer_size_change_.store (true);
}

/** Jack buffer size callback. */
int
JackDriver::buffer_size_cb (uint32_t nframes, void * data)
{
  auto * self = static_cast<JackDriver *> (data);
  z_info ("JACK buffer size changed: {}", nframes);
  self->handled_jack_buffer_size_change_.store (false);
  self->engine_.push_event (
    AudioEngine::AudioEngineEventType::AUDIO_ENGINE_EVENT_BUFFER_SIZE_CHANGE,
    nullptr, nframes, 0.f);

  /* process immediately if gtk thread */
  if (ZRYTHM_IS_QT_THREAD)
    {
      self->engine_.process_events ();
    }
  /* otherwise if activated wait for gtk thread to process all events */
  else if (self->engine_.activated_ && self->engine_.run_.load ())
    {
      while (!self->handled_jack_buffer_size_change_.load ())
        {
          z_info (
            "-- waiting for engine to handle JACK buffer size change on GUI thread... (engine_process_events)");
          std::this_thread::sleep_for (std::chrono::milliseconds (1000000));
        }
    }

  return 0;
}

void
JackDriver::handle_position_change ()
{
  if (transport_type_ == TransportType::TimebaseMaster)
    {
      jack_transport_locate (client_, (jack_nframes_t) PLAYHEAD.frames_);
    }
}

void
JackDriver::handle_start ()
{
  if (transport_type_ == TransportType::TimebaseMaster)
    {
      jack_transport_start (client_);
    }
}

void
JackDriver::handle_stop ()
{
  if (transport_type_ == TransportType::TimebaseMaster)
    {
      jack_transport_stop (client_);
    }
}

void
JackDriver::prepare_process_audio ()
{
  /* if client, get transport position info */
  if (transport_type_ == TransportType::TransportClient)
    {
      jack_position_t        pos;
      jack_transport_state_t state = jack_transport_query (client_, &pos);

      if (state == JackTransportRolling)
        {
          TRANSPORT->play_state_ =
            zrythm::engine::session::Transport::PlayState::Rolling;
        }
      else if (state == JackTransportStopped)
        {
          TRANSPORT->play_state_ =
            zrythm::engine::session::Transport::PlayState::Paused;
        }

      /* get position from timebase master */
      zrythm::dsp::Position tmp{
        static_cast<signed_frame_t> (pos.frame), AUDIO_ENGINE->ticks_per_frame_
      };
      TRANSPORT->set_playhead_pos_rt_safe (tmp);

      /* BBT and BPM changes */
      if (pos.valid & JackPositionBBT)
        {
          P_TEMPO_TRACK->set_beats_per_bar ((int) pos.beats_per_bar);
          P_TEMPO_TRACK->set_bpm (
            (float) pos.beats_per_minute, (float) pos.beats_per_minute, true,
            true);
          P_TEMPO_TRACK->set_beat_unit ((int) pos.beat_type);
        }
    }

  /* clear output */
}

int
JackDriver::process_cb (uint32_t nframes, void * data)
{
  auto * self = (JackDriver *) data;
  return self->engine_.process (nframes);
}

int
JackDriver::xrun_cb (void * data)
{
  auto * self = static_cast<JackDriver *> (data);
  auto   cur_time = SteadyClock::now ();
  if ((cur_time - self->engine_.last_xrun_notification_).count () > 6000000)
    {
      /* TODO make a notification message queue */
#  if 0
      z_info (
        _("XRUN occurred - check your JACK "
        "configuration"));
#  endif
      self->engine_.last_xrun_notification_ = cur_time;
    }

  return 0;
}

void
JackDriver::timebase_cb (
  jack_transport_state_t state,
  jack_nframes_t         nframes,
  jack_position_t *      pos,
  int                    new_pos,
  void *                 arg)
{
  auto * self = (JackDriver *) arg;
  if (!self->engine_.run_.load ())
    return;

  const auto &transport = *self->engine_.project_->transport_;
  auto        playhead = transport.playhead_pos_->get_position ();
  const auto &tempo_track = *P_TEMPO_TRACK;

  /* Mandatory fields */
  pos->valid = JackPositionBBT;
  pos->frame = (jack_nframes_t) playhead.frames_;

  /* BBT */
  pos->bar = playhead.get_bars (true, transport.ticks_per_bar_);
  pos->beat = playhead.get_beats (
    true, tempo_track.get_beats_per_bar (), transport.ticks_per_beat_);
  pos->tick =
    playhead.get_sixteenths (
      true, tempo_track.get_beats_per_bar (), transport.sixteenths_per_beat_,
      self->engine_.frames_per_tick_)
      * dsp::Position::TICKS_PER_SIXTEENTH_NOTE
    + (int) floor (playhead.ticks_);
  dsp::Position bar_start;
  bar_start.set_to_bar (
    playhead.get_bars (true, transport.ticks_per_bar_),
    transport.ticks_per_bar_, self->engine_.frames_per_tick_);
  pos->bar_start_tick = playhead.ticks_ - bar_start.ticks_;
  pos->beats_per_bar = (float) tempo_track.get_beats_per_bar ();
  pos->beat_type = (float) tempo_track.get_beat_unit ();
  pos->ticks_per_beat = transport.ticks_per_beat_;
  pos->beats_per_minute = (double) tempo_track.get_current_bpm ();
}

void
JackDriver::shutdown_cb (void * arg)
{
  z_warning ("Jack shutting down...");

  if (ZRYTHM_HAVE_UI) //&& MAIN_WINDOW)
    {
      // auto msg = QObject::tr ("JACK has shut down");
      // ui_show_error_message (QObject::tr ("JACK Error"), msg);
    }
}

void
JackDriver::freewheel_cb (int starting, void * arg)
{
  if (starting != 0)
    {
      z_info ("JACK: starting freewheel");
    }
  else
    {
      z_info ("JACK: stopping freewheel");
    }
}

void
JackDriver::
  port_registration_cb (jack_port_id_t port_id, int registered, void * arg)
{
  auto * self = (JackDriver *) arg;

  jack_port_t * jport = jack_port_by_id (self->client_, port_id);
  z_info (
    "JACK: port '%s' %sregistered", jack_port_name (jport),
    registered ? "" : "un");
}

void
JackDriver::
  port_connect_cb (jack_port_id_t a, jack_port_id_t b, int connect, void * arg)
{
  auto * self = (JackDriver *) arg;

  jack_port_t * jport_a = jack_port_by_id (self->client_, a);
  jack_port_t * jport_b = jack_port_by_id (self->client_, b);
  z_info (
    "JACK: port '%s' %sconnected %s '%s'", jack_port_name (jport_a),
    connect ? "" : "dis", connect ? "to" : "from", jack_port_name (jport_b));

  /* if port was disconnected from one of Zrythm's tracked hardware ports, set
   * it as deactivated so it can be force-activated next scan */
  if (connect == 0)
    {
      ExtPort tmp (jport_a);
      auto    id = tmp.get_id ();
      auto    ext_port = HW_IN_PROCESSOR->find_ext_port (id);
      if (ext_port != nullptr)
        {
          z_debug ("setting '{}' to pending reconnect", id);
          ext_port->pending_reconnect_ = true;
        }
    }
}

/**
 * Sets up the MIDI engine to use jack.
 */
bool
JackDriver::setup_midi ()
{
  z_info ("{}: Setting up JACK MIDI", __func__);

  /* TODO: case 1 - no jack client (using another
   * backend)
   *
   * create a client and attach midi. */

  /* case 2 - jack client exists, just attach to
   * it */
  engine_.midi_buf_size_ = 4096;
#  if defined(HAVE_JACK_PORT_TYPE_GET_BUFFER_SIZE)
  engine_.midi_buf_size_ =
    jack_port_type_get_buffer_size (client_, JACK_DEFAULT_MIDI_TYPE);
#  endif

  return true;
}

void
JackDriver::set_transport_type (TransportType type)
{
  if (!ZRYTHM_TESTING && !ZRYTHM_BENCHMARKING)
    {
      gui::SettingsManager::get_instance ()->set_jackTransportType (
        ENUM_VALUE_TO_INT (type));
    }

  /* release timebase master if held */
  if (
    transport_type_ == TransportType::TimebaseMaster
    && type != TransportType::TimebaseMaster)
    {
      z_info ("releasing JACK timebase");
      jack_release_timebase (client_);
    }
  /* acquire timebase master */
  else if (
    transport_type_ != TransportType::TimebaseMaster
    && type == TransportType::TimebaseMaster)
    {
      z_info ("becoming JACK timebase master");
      jack_set_timebase_callback (client_, 0, timebase_cb, this);
    }

  z_info ("set JACK transport type to {}", ENUM_NAME (type));
  transport_type_ = type;
}

#  if 0
/**
 * Tests if JACK is working properly.
 *
 * Returns 0 if ok, non-null if has errors.
 *
 * If win is not null, it displays error messages
 * to it.
 */
int
engine_jack_test (GtkWindow * win)
{
  const char *   client_name = PROGRAM_NAME;
  const char *   server_name = NULL;
  jack_options_t options = JackNoStartServer;
  jack_status_t  status;

  /* open a client connection to the JACK server */
  jack_client_t * client =
    jack_client_open (client_name, options, &status, server_name);

  if (client)
    {
      jack_client_close (client);
      return 0;
    }
  else
    {
      auto msg = engine_jack_get_error_message (status);
      if (win)
        {
          ui_show_message_full (GTK_WIDGET (win), QObject::tr ("JACK Error"), msg);
        }
      else
        {
          z_warning ("JACK Error: {}", msg);
        }
      return 1;
    }

  return 0;
}
#  endif

bool
JackDriver::setup_audio ()
{
  z_info ("Setting up JACK...");

  const char *   client_name = PROGRAM_NAME;
  const char *   server_name = NULL;
  jack_options_t options = JackNoStartServer;
  if (ZRYTHM_TESTING || ZRYTHM_BENCHMARKING)
    {
      server_name = "zrythm-pipewire-0";
      options = options | JackServerName;
    }
  jack_status_t status;

  /* open a client connection to the JACK server */
  client_ = jack_client_open (client_name, options, &status, server_name);

  if (client_ == nullptr)
    {
// TODO
#  if 0
      auto msg = engine_jack_get_error_message (status);
      ui_show_error_message (QObject::tr ("JACK Error"), msg.c_str ());
#  endif

      return false;
    }

  /* set jack callbacks */
  int ret;
  jack_set_process_callback (client_, process_cb, this);
  handled_jack_buffer_size_change_.store (true);
  jack_set_buffer_size_callback (client_, buffer_size_cb, this);
  jack_set_sample_rate_callback (client_, sample_rate_cb, this);
  jack_set_xrun_callback (client_, xrun_cb, this);
  jack_set_freewheel_callback (client_, freewheel_cb, this);
  ret =
    jack_set_port_registration_callback (client_, port_registration_cb, this);
  z_return_val_if_fail (ret == 0, -1);
  ret = jack_set_port_connect_callback (client_, port_connect_cb, this);
  z_return_val_if_fail (ret == 0, -1);
  jack_on_shutdown (client_, shutdown_cb, this);
  /*jack_set_latency_callback(client, &jack_latency_cb, arg);*/
#  ifdef JALV_JACK_SESSION
  /*jack_set_session_callback(client, &jack_session_cb, arg);*/
#  endif

  /* Set audio engine properties */
  engine_.sample_rate_ = jack_get_sample_rate (client_);
  engine_.block_length_ = jack_get_buffer_size (client_);
  z_info (
    "jack sample rate {}, block length {}", engine_.sample_rate_,
    engine_.block_length_);

  set_transport_type (
    ZRYTHM_TESTING || ZRYTHM_BENCHMARKING
      ? TransportType::TransportClient
      : ENUM_INT_TO_VALUE (
          TransportType, gui::SettingsManager::jackTransportType ()));

  z_info ("JACK set up");
  return true;
}

void
JackDriver::tear_down_audio ()
{
  jack_client_close (client_);
  client_ = nullptr;

  /* init semaphore */
  // zix_sem_init (&self->port_operation_lock, 1);
}

void
JackDriver::reconnect_monitor (bool left)
{
  if (ZRYTHM_TESTING || ZRYTHM_BENCHMARKING)
    return;

  auto devices =
    left
      ? gui::SettingsManager::get_instance ()->get_monitorLeftOutputDeviceList ()
      : gui::SettingsManager::get_instance ()->get_monitorRightOutputDeviceList ();

  auto  monitor_out_ports = engine_.get_monitor_out_ports ();
  auto &port = left ? monitor_out_ports.first : monitor_out_ports.second;

  auto jport =
    dynamic_cast<JackPortBackend *> (port.backend_.get ())->get_jack_port ();

  /* disconnect port */
  int ret = jack_port_disconnect (client_, jport);
  if (ret != 0)
    {
      auto msg = utils::jack::get_error_message ((jack_status_t) ret);
      throw ZrythmException (format_qstr (
        QObject::tr ("JACK: Failed to disconnect monitor out: {}"), msg));
    }

  int num_connected = 0;
  for (const auto &device : devices)
    {
      ExtPort * ext_port = engine_.hw_out_processor_->find_ext_port (
        utils::Utf8String::from_qstring (device));
      if (ext_port != nullptr)
        {
          /*z_return_val_if_reached (-1);*/
          ret = jack_connect (
            client_, jack_port_name (jport), ext_port->full_name_.c_str ());
          if (ret != 0)
            {
              auto msg = utils::jack::get_error_message ((jack_status_t) ret);
              z_warning ("cannot connect monitor out: {}", msg);
            }
          else
            {
              num_connected++;
            }
        }
    }

  /* if nothing connected, attempt to connect to first port found */
  if (num_connected == 0)
    {
      const char ** ports = jack_get_ports (
        client_, nullptr, JACK_DEFAULT_AUDIO_TYPE,
        JackPortIsPhysical | JackPortIsInput);
      if (ports == nullptr || ports[0] == nullptr || ports[1] == nullptr)
        {
          throw ZrythmException (
            QObject::tr ("JACK: No physical playback ports found"));
        }

      ret = jack_connect (
        client_, jack_port_name (jport), left ? ports[0] : ports[1]);
      if (ret != 0)
        {
          auto msg = utils::jack::get_error_message ((jack_status_t) ret);
          throw ZrythmException (format_qstr (
            QObject::tr ("JACK: Failed to connect monitor output [{}]: {}"),
            left ? QObject::tr ("left") : QObject::tr ("right"), msg));
        }
      else
        {
          num_connected++;
        }

      jack_free (ports);
    }

  z_return_if_fail (num_connected > 0);
}

bool
JackDriver::activate_audio (bool activate)
{
  if (activate)
    {
      assert (client_);

      /* Tell the JACK server that we are ready to roll.  Our process() callback
       * will start running now. */
      if (jack_activate (client_) != 0)
        {
          z_warning ("cannot activate client");
          return false;
        }
      z_info ("Jack activated");

      /* Connect the ports.  You can't do this before the client is
       * activated, because we can't make connections to clients
       * that aren't running.  Note the confusing (but necessary)
       * orientation of the driver backend ports:
       * playback ports are "input" to the backend, and capture ports are
       * "output" from it.
       */

      auto has_jack_backend = [] (const auto &port) {
        z_return_val_if_fail (port.backend_, false);
        z_return_val_if_fail (port.backend_->is_exposed (), false);
        z_return_val_if_fail (
          dynamic_cast<JackPortBackend *> (port.backend_.get ()) != nullptr,
          false);
        return true;
      };
      auto monitor_out_ports = engine_.get_monitor_out_ports ();
      z_return_val_if_fail (
        has_jack_backend (monitor_out_ports.first)
          && has_jack_backend (monitor_out_ports.second),
        false);

      z_info ("connecting to system out ports...");

      try
        {
          reconnect_monitor (true);
        }
      catch (const ZrythmException &e)
        {
#  if 0
          HANDLE_ERROR (
            err, "%s", QObject::tr ("Failed to connect to left monitor output port"));
#  endif
          return false;
        }
      try
        {
          reconnect_monitor (false);
        }
      catch (const ZrythmException &e)
        {
#  if 0
          HANDLE_ERROR (
            err, "%s", QObject::tr ("Failed to connect to right monitor output port"));
#  endif
          return false;
        }
    }
  else
    {
      int ret = jack_deactivate (client_);
      if (ret != 0)
        {
          auto msg = utils::jack::get_error_message ((jack_status_t) ret);
          z_error ("Failed deactivating JACK client: {}", msg.c_str ());
        }
    }

  return true;
}

bool
JackDriver::is_pipewire () const
{
#  if defined(_WIN32) || defined(__APPLE__)
  return false;
#  else
  const char * libname = "libjack.so.0";
  void *       lib_handle = dlopen (libname, RTLD_LAZY);
  z_return_val_if_fail (lib_handle, false);

  const char * func_name = "jack_get_version_string";

  const char * (*jack_get_version_string) ();
  *(void **) (&jack_get_version_string) = dlsym (lib_handle, func_name);
  if (jack_get_version_string == nullptr)
    {
      z_info ("{} () not found in {}", func_name, libname);
      return false;
    }

  z_info ("{} () found in {}", func_name, libname);
  const char * ver = (*jack_get_version_string) ();
  z_info ("ver {}", ver);
  return utils::Utf8String::from_utf8_encoded_string (ver).contains_substr (
    u8"PipeWire");

#  endif
}

bool
JackDriver::sanity_check_should_return_early (nframes_t total_frames_to_process)
{
  if (
    engine_.run_.load ()
    && engine_.block_length_ != jack_get_buffer_size (client_)) [[unlikely]]
    {
      engine_.clear_output_buffers (total_frames_to_process);
      z_warning (
        "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! JACK buffer size changed from {} to {} without notifying us (likely pipewire bug #1591). Attempting workaround...",
        engine_.block_length_, jack_get_buffer_size (client_));
      buffer_size_cb (jack_get_buffer_size (client_), this);
      return true;
    }
  return false;
}

std::vector<ExtPort>
JackDriver::get_ext_audio_ports (dsp::PortFlow flow, bool hw) const
{
  std::vector<ExtPort> ports;
  get_ext_ports (dsp::PortType::Audio, flow, hw, ports);
  return ports;
}

std::vector<ExtPort>
JackDriver::get_ext_midi_ports (dsp::PortFlow flow, bool hw) const
{
  std::vector<ExtPort> ports;
  get_ext_ports (dsp::PortType::Event, flow, hw, ports);
  return ports;
}

void
JackDriver::get_ext_ports (
  dsp::PortType         type,
  dsp::PortFlow         flow,
  bool                  hw,
  std::vector<ExtPort> &ports) const
{
  unsigned long flags = 0;
  if (hw)
    flags |= JackPortIsPhysical;
  if (flow == dsp::PortFlow::Input)
    flags |= JackPortIsInput;
  else if (flow == dsp::PortFlow::Output)
    flags |= JackPortIsOutput;
  const char * jtype = JackPortBackend::get_jack_type (type);
  if (jtype == nullptr)
    return;

  if (client_ == nullptr)
    {
      z_error (
        "JACK client is NULL. make sure to call engine_pre_setup() before calling this");
      return;
    }

  const char ** jports = jack_get_ports (client_, nullptr, jtype, flags);

  if (jports == nullptr)
    return;

  for (size_t i = 0; jports[i] != nullptr; ++i)
    {
      jack_port_t * jport = jack_port_by_name (client_, jports[i]);

      ports.emplace_back (jport);
      if (type == dsp::PortType::Event)
        {
          ports.back ().is_midi_ = true;
        }
    }

  jack_free (jports);
}
} // namespace zrythm::engine::device_io

#endif // HAVE_JACK
