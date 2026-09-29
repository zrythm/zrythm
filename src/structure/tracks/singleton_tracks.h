// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <memory>

#include <QObject>

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
  explicit SingletonTracks (QObject * parent = nullptr);
  ~SingletonTracks () override;

  ChordTrack *     chordTrack () const;
  ModulatorTrack * modulatorTrack () const;
  MasterTrack *    masterTrack () const;
  MarkerTrack *    markerTrack () const;

  void setChordTrack (ChordTrack * track);
  void setModulatorTrack (ModulatorTrack * track);
  void setMasterTrack (MasterTrack * track);
  void setMarkerTrack (MarkerTrack * track);

Q_SIGNALS:
  void chordTrackChanged ();
  void modulatorTrackChanged ();
  void masterTrackChanged ();
  void markerTrackChanged ();

private:
  class Impl;

  std::unique_ptr<Impl> impl_;
};
}
