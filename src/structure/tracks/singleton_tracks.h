// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/tracks/track.h"

namespace zrythm::structure::tracks
{
class ChordTrack;
class ModulatorTrack;
class MasterTrack;
class MarkerTrack;

/**
 * @brief References to tracks that are singletons.
 */
class SingletonTracks : public QObject
{
  Q_OBJECT
  Q_PROPERTY (
    zrythm::structure::tracks::ChordTrack * chordTrack READ chordTrack WRITE
      setChordTrack NOTIFY chordTrackChanged)
  Q_PROPERTY (
    zrythm::structure::tracks::ModulatorTrack * modulatorTrack READ
      modulatorTrack WRITE setModulatorTrack NOTIFY modulatorTrackChanged)
  Q_PROPERTY (
    zrythm::structure::tracks::MasterTrack * masterTrack READ masterTrack WRITE
      setMasterTrack NOTIFY masterTrackChanged)
  Q_PROPERTY (
    zrythm::structure::tracks::MarkerTrack * markerTrack READ markerTrack WRITE
      setMarkerTrack NOTIFY markerTrackChanged)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  SingletonTracks (QObject * parent = nullptr) : QObject (parent) { }

  ChordTrack *     chordTrack () const { return chord_track_; }
  ModulatorTrack * modulatorTrack () const { return modulator_track_; }
  MasterTrack *    masterTrack () const { return master_track_; }
  MarkerTrack *    markerTrack () const { return marker_track_; }

  void setChordTrack (ChordTrack * track)
  {
    if (chord_track_ != track)
      {
        chord_track_ = track;
        Q_EMIT chordTrackChanged ();
      }
  }

  void setModulatorTrack (ModulatorTrack * track)
  {
    if (modulator_track_ != track)
      {
        modulator_track_ = track;
        Q_EMIT modulatorTrackChanged ();
      }
  }

  void setMasterTrack (MasterTrack * track)
  {
    if (master_track_ != track)
      {
        master_track_ = track;
        Q_EMIT masterTrackChanged ();
      }
  }

  void setMarkerTrack (MarkerTrack * track)
  {
    if (marker_track_ != track)
      {
        marker_track_ = track;
        Q_EMIT markerTrackChanged ();
      }
  }

Q_SIGNALS:
  void chordTrackChanged ();
  void modulatorTrackChanged ();
  void masterTrackChanged ();
  void markerTrackChanged ();

private:
  QPointer<ChordTrack>     chord_track_;
  QPointer<ModulatorTrack> modulator_track_;
  QPointer<MasterTrack>    master_track_;
  QPointer<MarkerTrack>    marker_track_;
};
}
