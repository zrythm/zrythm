// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/tempo_map.h"

#include <QObject>
#include <QtQml/QQmlListProperty>

namespace zrythm::dsp
{

class TempoEventWrapper : public QObject
{
  Q_OBJECT
  Q_PROPERTY (qint64 tick READ tick CONSTANT)
  Q_PROPERTY (double bpm READ bpm CONSTANT)
  Q_PROPERTY (TempoMap::CurveType curve READ curve CONSTANT)

public:
  explicit TempoEventWrapper (
    const TempoMap::TempoEvent &event,
    QObject *                   parent = nullptr)
      : QObject (parent), event_ (event)
  {
  }

  qint64              tick () const { return event_.tick; }
  double              bpm () const { return event_.bpm; }
  TempoMap::CurveType curve () const { return event_.curve; }

private:
  TempoMap::TempoEvent event_;
};

class TimeSignatureEventWrapper : public QObject
{
  Q_OBJECT
  Q_PROPERTY (qint64 tick READ tick CONSTANT)
  Q_PROPERTY (int numerator READ numerator CONSTANT)
  Q_PROPERTY (int denominator READ denominator CONSTANT)

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
  QML_ELEMENT

public:
  explicit TempoMapWrapper (TempoMap &tempo_map, QObject * parent = nullptr)
      : QObject (parent), tempo_map_ (tempo_map)
  {
    rebuildWrappers ();
  }

  QQmlListProperty<TempoEventWrapper> tempoEvents ()
  {
    return QQmlListProperty<TempoEventWrapper> (this, &tempoEventWrappers_);
  }

  QQmlListProperty<TimeSignatureEventWrapper> timeSignatureEvents ()
  {
    return QQmlListProperty<TimeSignatureEventWrapper> (
      this, &timeSigEventWrappers_);
  }

  double sampleRate () const { return tempo_map_.getSampleRate (); }
  void   setSampleRate (double sampleRate)
  {
    if (qFuzzyCompare (sampleRate, tempo_map_.getSampleRate ()))
      return;
    tempo_map_.setSampleRate (sampleRate);
    Q_EMIT sampleRateChanged ();
  }

  Q_INVOKABLE void addTempoEvent (int tick, double bpm, int curveType)
  {
    tempo_map_.addEvent (
      tick, bpm, static_cast<TempoMap::CurveType> (curveType));
    Q_EMIT tempoEventsChanged ();
    rebuildTempoWrappers ();
  }

  Q_INVOKABLE void
  addTimeSignatureEvent (int tick, int numerator, int denominator)
  {
    tempo_map_.addTimeSignatureEvent (tick, numerator, denominator);
    Q_EMIT timeSignatureEventsChanged ();
    rebuildTimeSigWrappers ();
  }

  /**
   * @brief To be called when the tempo map has been modified directly.
   */
  void rebuildWrappers ()
  {
    rebuildTempoWrappers ();
    rebuildTimeSigWrappers ();
  }

Q_SIGNALS:
  void tempoEventsChanged ();
  void timeSignatureEventsChanged ();
  void sampleRateChanged ();

private:
  void rebuildTempoWrappers ()
  {
    qDeleteAll (tempoEventWrappers_);
    tempoEventWrappers_.clear ();
    for (const auto &event : tempo_map_.getEvents ())
      {
        tempoEventWrappers_.append (new TempoEventWrapper (event, this));
      }
  }

  void rebuildTimeSigWrappers ()
  {
    qDeleteAll (timeSigEventWrappers_);
    timeSigEventWrappers_.clear ();
    for (const auto &event : tempo_map_.getTimeSignatureEvents ())
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
