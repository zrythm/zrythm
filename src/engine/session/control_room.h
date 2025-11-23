// SPDX-FileCopyrightText: © 2019-2021, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/fader.h"
#include "structure/tracks/track_fwd.h"

#include <boost/unordered/unordered_flat_map_fwd.hpp>

namespace zrythm::engine::session
{

/**
 * @brief Abstraction to control the signal coming in from Master and going out
 * into the speakers.
 *
 * For example, the control room allows to specify how Listen will work on each
 * Channel and to set overall volume after the Master Channel so you can change
 * the volume without touching the Master Fader.
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
  Q_PROPERTY (zrythm::dsp::Fader * monitorFader READ monitorFader CONSTANT)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  using RealtimeTracks = boost::unordered_flat_map<
    structure::tracks::TrackUuid,
    structure::tracks::TrackPtrVariant>;
  using RealtimeTracksProvider = std::function<const RealtimeTracks &()>;

  ControlRoom (
    RealtimeTracksProvider rt_tracks_provider,
    QObject *              parent = nullptr);

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
  auto * monitorFader () const { return monitor_fader_.get (); }

  // ========================================================================

private:
  /**
   * @brief Self-contained registries (just to satisfy port/fader requirements).
   */
  dsp::PortRegistry               port_registry_;
  dsp::ProcessorParameterRegistry param_registry_{ port_registry_ };

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
   */
  utils::QObjectUniquePtr<dsp::Fader> monitor_fader_;

  RealtimeTracksProvider rt_tracks_provider_;
};

}
