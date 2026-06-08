// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <memory>

#include "dsp/port_observation_manager.h"

#include <QObject>
#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::dsp
{

/**
 * @brief Tracks note activity and general MIDI activity on an observed port.
 *
 * Observes a MidiPort via the PortObservationManager infrastructure. A 60fps
 * timer drains MIDI events from the observation cache and updates two kinds of
 * state exposed to QML:
 *
 * 1. **Note activity** — per-pitch tracking using a reference counter that
 *    correctly handles overlapping note-ons on the same pitch (from different
 *    channels or repeated presses). A pitch is considered active until all
 *    matching note-offs have been received. Query via isNoteActive().
 *
 * 2. **General activity** — a boolean that is true for one update cycle when
 *    any MIDI events (notes, CC, program change, etc.) are received, and false
 *    when the cache is empty. Useful for activity indicators.
 *
 * Typical QML usage:
 * @code
 * MidiActivityProvider {
 *     port: track.trackProcessorMidiOut
 *     portObservationManager: session.project.portObservationManager
 * }
 * @endcode
 *
 * @par Thread safety
 * All public methods run on the Qt event loop (main thread). The RT thread
 * writes to ring buffers via PortObserver; the observation manager's drain
 * timer copies data into per-requester caches on the UI thread. No locks or
 * atomics are needed in this class.
 */
class MidiActivityProvider : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY (
    zrythm::dsp::Port * port READ port WRITE setPort NOTIFY portChanged)
  Q_PROPERTY (
    zrythm::dsp::PortObservationManager * portObservationManager READ
      portObservationManager WRITE setPortObservationManager NOTIFY
        portObservationManagerChanged)
  Q_PROPERTY (bool hasActivity READ hasActivity NOTIFY activityChanged)

public:
  explicit MidiActivityProvider (QObject * parent = nullptr);
  ~MidiActivityProvider () override;

  dsp::Port *   port () const;
  void          setPort (dsp::Port * port);
  Q_SIGNAL void portChanged ();

  dsp::PortObservationManager * portObservationManager () const;
  void setPortObservationManager (dsp::PortObservationManager * manager);
  Q_SIGNAL void portObservationManagerChanged ();

  /**
   * @brief True when any MIDI events were received in the last update cycle.
   */
  bool          hasActivity () const;
  Q_SIGNAL void activityChanged ();

  /**
   * @brief Returns true if @p note (0-127) is currently held.
   *
   * Channels are aggregated — a note is active if any channel has it held.
   * Overlapping note-ons are reference-counted; the note only becomes inactive
   * when all note-offs have been received.
   */
  Q_INVOKABLE bool isNoteActive (int note) const;

  /**
   * @brief Emitted when the set of active notes changes.
   */
  Q_SIGNAL void activeNotesChanged ();

  /**
   * @brief Drains pending events from the observation cache and updates state.
   *
   * Called automatically by the internal 60fps timer. Can also be called
   * manually (e.g., in tests).
   */
  void update ();

private:
  void reset_state ();
  void try_create_token ();
  void process_midi_events ();

  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}
