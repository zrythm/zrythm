// SPDX-FileCopyrightText: © 2018-2019, 2022, 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "zrythm-config.h"

#include "engine/device_io/engine.h"

#ifdef HAVE_JACK

#  include <cstdlib>

#  define JACK_PORT_T(exp) (static_cast<jack_port_t *> (exp))

namespace zrythm::engine::device_io
{
class JackDriver : public AudioEngine::AudioDriver, public AudioEngine::MidiDriver
{
public:
  enum class TransportType : basic_enum_base_type_t
  {
    TimebaseMaster,
    TransportClient,
    NoJackTransport,
  };

  JackDriver (AudioEngine &engine) : engine_ (engine) { }

  utils::Utf8String get_driver_name () const override { return u8"JACK"; }

  // Audio driver implementation
  void set_buffer_size (uint32_t buf_size) override
  {
    jack_set_buffer_size (client_, buf_size);
    z_debug ("called jack_set_buffer_size");
  }
  bool buffer_size_change_handled () const override
  {
    return handled_jack_buffer_size_change_;
  }
  void handle_buf_size_change (uint32_t frames) override;
  void handle_sample_rate_change (uint32_t samplerate) override;
  bool setup_audio () override;
  bool activate_audio (bool activate) override;
  void tear_down_audio () override;
  void handle_start () override;
  void handle_stop () override;
  void prepare_process_audio () override;
  bool
  sanity_check_should_return_early (nframes_t total_frames_to_process) override;
  void                         handle_position_change () override;
  std::unique_ptr<PortBackend> create_audio_port_backend () const override
  {
    return std::make_unique<JackPortBackend> (*client_);
  }
  std::vector<ExtPort>
  get_ext_audio_ports (dsp::PortFlow flow, bool hw) const override;

  // MIDI driver implementation
  bool setup_midi () override;
  bool activate_midi (bool activate) override { /* noop */ return true; }
  void tear_down_midi () override { /* noop */ }
  std::unique_ptr<PortBackend> create_midi_port_backend () const override
  {
    return std::make_unique<JackPortBackend> (*client_);
  }
  std::vector<ExtPort>
  get_ext_midi_ports (dsp::PortFlow flow, bool hw) const override;

  jack_client_t * get_client () const { return client_; }

private:
  /**
   * Refreshes the list of external ports.
   */
  void rescan_ports ();

  /**
   * Disconnects and reconnects the monitor output
   * port to the selected devices.
   *
   * @throw ZrythmException on error.
   */
  void reconnect_monitor (bool left);

  /**
   * Updates the JACK Transport type.
   */
  void set_transport_type (TransportType type);

  /**
   * Fills the external out bufs.
   */
  void fill_out_bufs (const nframes_t nframes);

  void process_change_request ();

  /**
   * Jack sample rate callback.
   *
   * This is called in a non-RT thread by JACK in between its processing cycles,
   * and will block until this function returns so it is safe to change the
   * buffers here.
   */
  static int sample_rate_cb (uint32_t nframes, void * data);

  /** Jack buffer size callback. */
  static int buffer_size_cb (uint32_t nframes, void * data);
  /**
   * The process callback for this JACK application is called in a special
   * realtime thread once for each audio cycle.
   */
  static int process_cb (uint32_t nframes, void * data);
  /**
   * Client-supplied function that is called whenever an xrun has occurred.
   *
   * @see jack_get_xrun_delayed_usecs()
   *
   * @return zero on success, non-zero on error
   */
  static int xrun_cb (void * data);

  static void timebase_cb (
    jack_transport_state_t state,
    jack_nframes_t         nframes,
    jack_position_t *      pos,
    int                    new_pos,
    void *                 arg);
  /**
   * JACK calls this shutdown_callback if the server ever shuts down or decides
   * to disconnect the client.
   */
  static void shutdown_cb (void * arg);
  static void freewheel_cb (int starting, void * arg);
  static void
  port_registration_cb (jack_port_id_t port_id, int registered, void * arg);
  static void
  port_connect_cb (jack_port_id_t a, jack_port_id_t b, int connect, void * arg);

  /**
   * Returns if this is a pipewire session.
   */
  bool is_pipewire () const;

  void get_ext_ports (
    dsp::PortType         type,
    dsp::PortFlow         flow,
    bool                  hw,
    std::vector<ExtPort> &ports) const;

private:
  AudioEngine &engine_;

  /**
   * Whether transport master/client or no connection with jack transport.
   */
  TransportType transport_type_{};

  /**
   * Whether pending jack buffer change was handled (buffers reallocated).
   *
   * To be set to zero when a change starts and 1 when the change is fully
   * processed.
   */
  std::atomic_bool handled_jack_buffer_size_change_ = false;

  /** JACK client. */
  jack_client_t * client_ = nullptr;
};
}
#endif // HAVE_JACK
