// SPDX-FileCopyrightText: © 2019-2021, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/tracks/fader.h"
#include "utils/icloneable.h"

#define CONTROL_ROOM (AUDIO_ENGINE->control_room_)

namespace zrythm
{
namespace structure::tracks
{
class Fader;
}
namespace engine::device_io
{
class AudioEngine;
}
}

namespace zrythm::engine::session
{

/**
 * The control room allows to specify how Listen will work on each Channel and
 * to set overall volume after the Master Channel so you can change the volume
 * without touching the Master Fader.
 */
class ControlRoom final
{
public:
  using Fader = structure::tracks::Fader;
  using AudioEngine = engine::device_io::AudioEngine;

  ControlRoom (
    dsp::PortRegistry               &port_registry,
    dsp::ProcessorParameterRegistry &param_registry,
    AudioEngine *                    engine);

  /**
   * Inits the control room from a project.
   */
  void init_loaded (
    dsp::PortRegistry               &port_registry,
    dsp::ProcessorParameterRegistry &param_registry,
    AudioEngine *                    engine);

  /**
   * Sets dim_output to on/off and notifies interested parties.
   */
  void set_dim_output (bool dim_output) { dim_output_ = dim_output; }

  friend void init_from (
    ControlRoom           &obj,
    const ControlRoom     &other,
    utils::ObjectCloneType clone_type);

private:
  static constexpr auto kMonitorFaderKey = "monitorFader"sv;
  friend void to_json (nlohmann::json &j, const ControlRoom &control_room)
  {
    j[kMonitorFaderKey] = control_room.listen_fader_;
  }
  friend void from_json (const nlohmann::json &j, ControlRoom &control_room)
  {
    j.at (kMonitorFaderKey).get_to (control_room.listen_fader_);
  }

  void init_common ();

public:
  /**
   * The volume to set muted channels to when
   * soloing/muting.
   */
  std::unique_ptr<Fader> mute_fader_;

  /**
   * The volume to set listened channels to when
   * Listen is enabled on a Channel.
   */
  std::unique_ptr<Fader> listen_fader_;

  /**
   * The volume to set other channels to when Listen
   * is enabled on a Channel, or the monitor when
   * dim is enabled.
   */
  std::unique_ptr<Fader> dim_fader_;

  /** Dim the output volume. */
  bool dim_output_ = false;

  /**
   * Monitor fader.
   *
   * The Master stereo out should connect to this.
   *
   * @note This needs to be serialized because some ports connect to it.
   */
  std::unique_ptr<Fader> monitor_fader_;

  /** Pointer to owner audio engine, if any. */
  AudioEngine * audio_engine_ = nullptr;

  OptionalRef<dsp::PortRegistry>               port_registry_;
  OptionalRef<dsp::ProcessorParameterRegistry> param_registry_;
};

}
