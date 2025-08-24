// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/tempo_map.h"
#include "utils/qt.h"

#include <QObject>
#include <QtQml/QQmlListProperty>
#include <QtQmlIntegration>

namespace zrythm::dsp
{

class TempoEventWrapper : public QObject
{
  Q_OBJECT
  Q_PROPERTY (qint64 tick READ tick CONSTANT)
  Q_PROPERTY (double bpm READ bpm CONSTANT)
  Q_PROPERTY (CurveType curve READ curve CONSTANT)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  explicit TempoEventWrapper (
    const TempoMap::TempoEvent &event,
    QObject *                   parent = nullptr)
      : QObject (parent), event_ (event)
  {
  }

  enum class CurveType : std::uint_fast8_t
  {
    Constant, ///< Constant tempo
    Linear    ///< Linear tempo ramp
  };
  Q_ENUM (CurveType)

  qint64    tick () const { return event_.tick; }
  double    bpm () const { return event_.bpm; }
  CurveType curve () const { return static_cast<CurveType> (event_.curve); }

private:
  TempoMap::TempoEvent event_;
};

class TimeSignatureEventWrapper : public QObject
{
  Q_OBJECT
  Q_PROPERTY (qint64 tick READ tick CONSTANT)
  Q_PROPERTY (int numerator READ numerator CONSTANT)
  Q_PROPERTY (int denominator READ denominator CONSTANT)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  explicit TimeSignatureEventWrapper (
    const TempoMap::TimeSignatureEvent &event,
    QObject *                           parent = nullptr)
      : QObject (parent), event_ (event)
  {
  }

  qint64 tick () const { return event_.tick; }
  int    numerator () const { return event_.numerator; }
  int    denominator () const { return event_.denominator; }

private:
  TempoMap::TimeSignatureEvent event_;
};

class MusicalPositionWrapper : public QObject
{
  Q_OBJECT
  Q_PROPERTY (int bar READ bar CONSTANT)
  Q_PROPERTY (int beat READ beat CONSTANT)
  Q_PROPERTY (int sixteenth READ sixteenth CONSTANT)
  Q_PROPERTY (int tick READ tick CONSTANT)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  explicit MusicalPositionWrapper (
    const TempoMap::MusicalPosition &pos,
    QObject *                        parent = nullptr)
      : QObject (parent), pos_ (pos)
  {
  }

  int bar () const { return pos_.bar; }
  int beat () const { return pos_.beat; }
  int sixteenth () const { return pos_.sixteenth; }
  int tick () const { return pos_.tick; }

private:
  TempoMap::MusicalPosition pos_;
};

class TempoMapWrapper : public QObject
{
  Q_OBJECT
  Q_PROPERTY (
    QQmlListProperty<TempoEventWrapper> tempoEvents READ tempoEvents NOTIFY
      tempoEventsChanged)
  Q_PROPERTY (
    QQmlListProperty<TimeSignatureEventWrapper> timeSignatureEvents READ
      timeSignatureEvents NOTIFY timeSignatureEventsChanged)
  Q_PROPERTY (
    double sampleRate READ sampleRate WRITE setSampleRate NOTIFY
      sampleRateChanged)
  QML_NAMED_ELEMENT (TempoMap)
  QML_UNCREATABLE ("")

public:
  explicit TempoMapWrapper (TempoMap &tempo_map, QObject * parent = nullptr)
      : QObject (parent), tempo_map_ (tempo_map)
  {
    rebuildWrappers ();
  }

  QQmlListProperty<TempoEventWrapper> tempoEvents ()
  {
    return { this, &tempoEventWrappers_ };
  }

  QQmlListProperty<TimeSignatureEventWrapper> timeSignatureEvents ()
  {
    return { this, &timeSigEventWrappers_ };
  }

  /**
   * @brief Returns the time signature at the given tick.
   */
  Q_INVOKABLE int timeSignatureNumeratorAtTick (int64_t tick) const;
  Q_INVOKABLE int timeSignatureDenominatorAtTick (int64_t tick) const;

  Q_INVOKABLE double tempoAtTick (int64_t tick) const;

  // this should be static but i don't know how to use it from QML then
  Q_INVOKABLE int getPpq () { return TempoMap::get_ppq (); }

  double sampleRate () const { return tempo_map_.get_sample_rate (); }
  void   setSampleRate (double sampleRate)
  {
    if (qFuzzyCompare (sampleRate, tempo_map_.get_sample_rate ()))
      return;
    tempo_map_.set_sample_rate (sampleRate);
    Q_EMIT sampleRateChanged ();
  }

  Q_INVOKABLE void addTempoEvent (qint64 tick, double bpm, int curveType)
  {
    tempo_map_.add_tempo_event (
      tick, bpm, static_cast<TempoMap::CurveType> (curveType));
    Q_EMIT tempoEventsChanged ();
    rebuildTempoWrappers ();
  }

  Q_INVOKABLE void
  addTimeSignatureEvent (qint64 tick, int numerator, int denominator)
  {
    tempo_map_.add_time_signature_event (tick, numerator, denominator);
    Q_EMIT timeSignatureEventsChanged ();
    rebuildTimeSigWrappers ();
  }

  Q_INVOKABLE MusicalPositionWrapper * getMusicalPosition (int64_t tick) const
  {
    return new MusicalPositionWrapper (
      tempo_map_.tick_to_musical_position (tick));
  }

  Q_INVOKABLE QString getMusicalPositionString (int64_t tick) const;

  Q_INVOKABLE int64_t
  getTickFromMusicalPosition (int bar, int beat, int sixteenth, int tick) const
  {
    return tempo_map_.musical_position_to_tick (
      TempoMap::MusicalPosition{
        .bar = bar, .beat = beat, .sixteenth = sixteenth, .tick = tick });
  }

  // additional API when we want to avoid allocations and we only need 1 part

  Q_INVOKABLE int getMusicalPositionBar (int64_t tick) const
  {
    return tempo_map_.tick_to_musical_position (tick).bar;
  }
  Q_INVOKABLE int getMusicalPositionBeat (int64_t tick) const
  {
    return tempo_map_.tick_to_musical_position (tick).beat;
  }
  Q_INVOKABLE int getMusicalPositionSixteenth (int64_t tick) const
  {
    return tempo_map_.tick_to_musical_position (tick).sixteenth;
  }
  Q_INVOKABLE int getMusicalPositionTick (int64_t tick) const
  {
    return tempo_map_.tick_to_musical_position (tick).tick;
  }

  /**
   * @brief To be called when the tempo map has been modified directly.
   */
  void rebuildWrappers ()
  {
    rebuildTempoWrappers ();
    rebuildTimeSigWrappers ();
  }

  /// Read-only access to tempo map
  const TempoMap &get_tempo_map () const { return tempo_map_; }

Q_SIGNALS:
  void tempoEventsChanged ();
  void timeSignatureEventsChanged ();
  void sampleRateChanged ();

private:
  void rebuildTempoWrappers ()
  {
    qDeleteAll (tempoEventWrappers_);
    tempoEventWrappers_.clear ();
    for (const auto &event : tempo_map_.get_tempo_events ())
      {
        tempoEventWrappers_.append (new TempoEventWrapper (event, this));
      }
  }

  void rebuildTimeSigWrappers ()
  {
    qDeleteAll (timeSigEventWrappers_);
    timeSigEventWrappers_.clear ();
    for (const auto &event : tempo_map_.get_time_signature_events ())
      {
        timeSigEventWrappers_.append (
          new TimeSignatureEventWrapper (event, this));
      }
  }

private:
  TempoMap                          &tempo_map_;
  QList<TempoEventWrapper *>         tempoEventWrappers_;
  QList<TimeSignatureEventWrapper *> timeSigEventWrappers_;
};

} // namespace zrythm::dsp

Q_DECLARE_METATYPE (zrythm::dsp::TempoMap::CurveType)
