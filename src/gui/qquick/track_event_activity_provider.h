// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/midi_activity_provider.h"
#include "dsp/port_observation_manager.h"
#include "structure/tracks/track.h"
#include "utils/qt.h"

#include <QObject>
#include <QPointer>
#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::gui::qquick
{

/**
 * @brief Single event-activity source for a track header.
 *
 * Exposes the track's note events as one eventReceived() signal for
 * indicators. The note source is the channel's post-MIDI-FX stream (see
 * Channel::postMidiFxPort()).
 */
class TrackEventActivityProvider : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY (
    zrythm::structure::tracks::Track * track READ track WRITE setTrack NOTIFY
      trackChanged REQUIRED)
  Q_PROPERTY (
    zrythm::dsp::PortObservationManager * portObservationManager READ
      portObservationManager WRITE setPortObservationManager NOTIFY
        portObservationManagerChanged REQUIRED)
  Q_PROPERTY (
    bool hasEventSource READ hasEventSource NOTIFY hasEventSourceChanged)
  Q_PROPERTY (
    zrythm::dsp::MidiActivityProvider * midiActivityProvider READ
      midiActivityProvider CONSTANT)

public:
  explicit TrackEventActivityProvider (QObject * parent = nullptr);
  ~TrackEventActivityProvider () override;

  // ============================================================================
  // QML Interface
  // ============================================================================

  structure::tracks::Track * track () const;
  void                       setTrack (structure::tracks::Track * track);
  Q_SIGNAL void              trackChanged ();

  dsp::PortObservationManager * portObservationManager () const;
  void setPortObservationManager (dsp::PortObservationManager * manager);
  Q_SIGNAL void portObservationManagerChanged ();

  /**
   * @brief Whether the track has any supported event source.
   *
   * True when a note source exists, independent of whether observation is
   * active (the port observation manager may still be null).
   */
  bool          hasEventSource () const;
  Q_SIGNAL void hasEventSourceChanged ();

  /**
   * @brief The note-event provider, for direct access to held-note state.
   */
  dsp::MidiActivityProvider * midiActivityProvider () const;

  /**
   * @brief Emitted when a batch contains a note-on, note-off, or
   * all-notes-off message.
   */
  Q_SIGNAL void eventReceived ();

  // ============================================================================

private:
  void rebind_sources ();

  structure::tracks::Track *            track_ = nullptr;
  QPointer<dsp::PortObservationManager> manager_;

  utils::QObjectUniquePtr<dsp::MidiActivityProvider> note_provider_;

  bool has_event_source_ = false;
};

} // namespace zrythm::gui::qquick
