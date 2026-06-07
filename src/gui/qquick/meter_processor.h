// SPDX-FileCopyrightText: © 2020, 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <memory>

#include "dsp/port.h"
#include "dsp/port_observation_manager.h"

#include <QObject>
#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::gui::qquick
{

/**
 * @brief A meter processor for a single GUI element.
 *
 * This class is responsible for processing the meter values for a single GUI
 * element, such as a volume meter. It supports various meter algorithms,
 * including digital peak, true peak, RMS, and K-meter.
 *
 * The meter processor is associated with a port, which can be either an
 * AudioPort or a MidiPort. The meter values are updated based on the data
 * from the associated port.
 *
 * The meter processor emits the `valuesChanged` signal whenever the meter
 * values are updated, allowing the GUI to update the display accordingly.
 */
class MeterProcessor : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY (zrythm::dsp::Port * port READ port WRITE setPort REQUIRED)
  Q_PROPERTY (int channel READ channel WRITE setChannel)
  Q_PROPERTY (
    int sampleRate READ sampleRate WRITE setSampleRate NOTIFY sampleRateChanged
      REQUIRED)
  Q_PROPERTY (
    zrythm::dsp::PortObservationManager * portObservationManager READ
      portObservationManager WRITE setPortObservationManager REQUIRED)
  Q_PROPERTY (
    float currentAmplitude READ currentAmplitude NOTIFY currentAmplitudeChanged)
  Q_PROPERTY (float peakAmplitude READ peakAmplitude NOTIFY peakAmplitudeChanged)
  Q_PROPERTY (
    MeterAlgorithm algorithm READ algorithm WRITE setAlgorithm NOTIFY
      algorithmChanged)

public:
  enum class MeterAlgorithm
  {
    /** Use default algorithm for the port. */
    Auto,
    DigitalPeak,
    /** @note True peak is intensive, only use it where needed (mixer). */
    TruePeak,
    RMS,
    K,
  };
  Q_ENUM (MeterAlgorithm)

  enum class AudioValueFormat : std::uint8_t
  {
    /** 0 to 2, amplitude. */
    Amplitude,

    /** dbFS. */
    DBFS,

    /** 0 to 1, suitable for drawing. */
    Fader,
  };

  MeterProcessor (QObject * parent = nullptr);
  ~MeterProcessor () override;

  // ================================================================
  // QML Interface
  // ================================================================

  dsp::Port * port () const;
  void        setPort (dsp::Port * port);

  int  channel () const;
  void setChannel (int channel);

  int           sampleRate () const;
  void          setSampleRate (int rate);
  Q_SIGNAL void sampleRateChanged ();

  dsp::PortObservationManager * portObservationManager () const;
  void setPortObservationManager (dsp::PortObservationManager * manager);

  MeterAlgorithm algorithm () const;
  void           setAlgorithm (MeterAlgorithm algo);
  Q_SIGNAL void  algorithmChanged ();

  float         currentAmplitude () const;
  Q_SIGNAL void currentAmplitudeChanged (float value);

  float         peakAmplitude () const;
  Q_SIGNAL void peakAmplitudeChanged (float value);

  Q_INVOKABLE float toDBFS (float amp) const;
  Q_INVOKABLE float toFader (float amp) const;

private:
  void get_value (AudioValueFormat format, float &val, float &max);
  void try_create_token ();

  struct Impl;
  std::unique_ptr<Impl> impl_;
};
}
