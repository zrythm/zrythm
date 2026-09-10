// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/chord_track.h"
#include "structure/tracks/marker_track.h"
#include "structure/tracks/master_track.h"
#include "structure/tracks/modulator_track.h"
#include "structure/tracks/singleton_tracks.h"

#include <QPointer>

namespace zrythm::structure::tracks
{

class SingletonTracks::Impl
{
public:
  QPointer<ChordTrack>     chord_track_;
  QPointer<ModulatorTrack> modulator_track_;
  QPointer<MasterTrack>    master_track_;
  QPointer<MarkerTrack>    marker_track_;
};

SingletonTracks::SingletonTracks (QObject * parent)
    : QObject (parent), impl_ (std::make_unique<Impl> ())
{
}

SingletonTracks::~SingletonTracks () = default;

ChordTrack *
SingletonTracks::chordTrack () const
{
  return impl_->chord_track_;
}

ModulatorTrack *
SingletonTracks::modulatorTrack () const
{
  return impl_->modulator_track_;
}

MasterTrack *
SingletonTracks::masterTrack () const
{
  return impl_->master_track_;
}

MarkerTrack *
SingletonTracks::markerTrack () const
{
  return impl_->marker_track_;
}

void
SingletonTracks::setChordTrack (ChordTrack * track)
{
  if (impl_->chord_track_ != track)
    {
      impl_->chord_track_ = track;
      Q_EMIT chordTrackChanged ();
    }
}

void
SingletonTracks::setModulatorTrack (ModulatorTrack * track)
{
  if (impl_->modulator_track_ != track)
    {
      impl_->modulator_track_ = track;
      Q_EMIT modulatorTrackChanged ();
    }
}

void
SingletonTracks::setMasterTrack (MasterTrack * track)
{
  if (impl_->master_track_ != track)
    {
      impl_->master_track_ = track;
      Q_EMIT masterTrackChanged ();
    }
}

void
SingletonTracks::setMarkerTrack (MarkerTrack * track)
{
  if (impl_->marker_track_ != track)
    {
      impl_->marker_track_ = track;
      Q_EMIT markerTrackChanged ();
    }
}

} // namespace zrythm::structure::tracks
