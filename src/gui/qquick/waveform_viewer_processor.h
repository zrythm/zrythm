// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/audio_port.h"
#include "dsp/engine.h"

#include <QObject>
#include <QTimer>
#include <QVector>

namespace zrythm::gui::qquick
{

class WaveformViewerProcessor : public QObject
{
  Q_OBJECT
  Q_PROPERTY (
    zrythm::dsp::AudioEngine * audioEngine READ audioEngine WRITE setAudioEngine
      REQUIRED)
  Q_PROPERTY (
    zrythm::dsp::AudioPort * stereoPort READ stereoPort WRITE setStereoPort
      NOTIFY stereoPortChanged REQUIRED)
  Q_PROPERTY (
    QVector<float> waveformData READ waveformData NOTIFY waveformDataChanged)
  Q_PROPERTY (
    int bufferSize READ bufferSize WRITE setBufferSize NOTIFY bufferSizeChanged)
  Q_PROPERTY (
    int displayPoints READ displayPoints WRITE setDisplayPoints NOTIFY
      displayPointsChanged)
  QML_ELEMENT

public:
  explicit WaveformViewerProcessor (QObject * parent = nullptr);
  ~WaveformViewerProcessor () override;

  // ================================================================
  // QML Interface
  // ================================================================

  dsp::AudioPort * stereoPort () const { return port_obj_; }
  void             setStereoPort (dsp::AudioPort * port_var);

  QVector<float> waveformData () const { return waveform_data_; }

  int  bufferSize () const { return static_cast<int> (buffer_size_); }
  void setBufferSize (int size);

  int  displayPoints () const { return static_cast<int> (display_points_); }
  void setDisplayPoints (int points);

  dsp::AudioEngine * audioEngine () const { return audio_engine_; }
  void setAudioEngine (dsp::AudioEngine * engine) { audio_engine_ = engine; }

Q_SIGNALS:
  void waveformDataChanged ();
  void bufferSizeChanged ();
  void displayPointsChanged ();
  void stereoPortChanged ();

  // ================================================================

private:
  void process_audio ();

  static constexpr size_t DEFAULT_BUFFER_SIZE = 2048;
  static constexpr size_t DEFAULT_DISPLAY_POINTS = 256;

  size_t buffer_size_{};
  size_t display_points_{};

  QPointer<dsp::AudioPort>                                        port_obj_;
  std::optional<dsp::RingBufferOwningPortMixin::RingBufferReader> ring_reader_;

  QVector<float>     waveform_data_;
  std::vector<float> mono_buffer_;
  std::vector<float> left_buffer_;
  std::vector<float> right_buffer_;

  QTimer * update_timer_ = nullptr;

  QPointer<dsp::AudioEngine> audio_engine_;
};
}
