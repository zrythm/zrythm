// SPDX-FileCopyrightText: © 2019-2021, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/tracks/fader.h"
#include "utils/icloneable.h"

#define CONTROL_ROOM (AUDIO_ENGINE->control_room_)

#define MONITOR_FADER (CONTROL_ROOM->monitor_fader_)

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
class ControlRoom : public QObject
{
  Q_OBJECT
  Q_PROPERTY (
    bool dimOutput READ dimOutput WRITE setDimOutput NOTIFY dimOutputChanged)
  Q_PROPERTY (
    zrythm::dsp::ProcessorParameter * muteVolume READ muteVolume CONSTANT)
  Q_PROPERTY (
    zrythm::dsp::ProcessorParameter * listenVolume READ muteVolume CONSTANT)
  Q_PROPERTY (
    zrythm::dsp::ProcessorParameter * dimVolume READ muteVolume CONSTANT)
  QML_ELEMENT
  QML_UNCREATABLE ("")
public:
  using Fader = structure::tracks::Fader;
  using AudioEngine = engine::device_io::AudioEngine;

  ControlRoom (
    dsp::PortRegistry               &port_registry,
    dsp::ProcessorParameterRegistry &param_registry,
    AudioEngine *                    engine);

  // ========================================================================
  // QML Interface
  // ========================================================================

  bool dimOutput () const { return dim_output_.load (); }
  void setDimOutput (bool dim)
  {
    bool expected = !dim;
    if (dim_output_.compare_exchange_strong (expected, dim))
      {
        Q_EMIT dimOutputChanged (dim);
      }
  }
  Q_SIGNAL void dimOutputChanged (bool dim);

  dsp::ProcessorParameter * muteVolume () const { return mute_volume_.get (); }
  dsp::ProcessorParameter * listenVolume () const
  {
    return listen_volume_.get ();
  }
  dsp::ProcessorParameter * dimVolume () const { return dim_volume_.get (); }

  // ========================================================================

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
    j[kMonitorFaderKey] = *control_room.monitor_fader_;
  }
  friend void from_json (const nlohmann::json &j, ControlRoom &control_room)
  {
    // TODO
    // j.at (kMonitorFaderKey).get_to (control_room.listen_fader_);
  }

  void init_common ();

public:
  /**
   * The volume to set muted channels to when
   * soloing/muting.
   */
  utils::QObjectUniquePtr<dsp::ProcessorParameter> mute_volume_;

  /**
   * The volume to set listened channels to when
   * Listen is enabled on a Channel.
   */
  utils::QObjectUniquePtr<dsp::ProcessorParameter> listen_volume_;

  /**
   * The volume to set other channels to when Listen
   * is enabled on a Channel, or the monitor when
   * dim is enabled.
   */
  utils::QObjectUniquePtr<dsp::ProcessorParameter> dim_volume_;

  /** Dim the output volume. */
  std::atomic_bool dim_output_ = false;

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
