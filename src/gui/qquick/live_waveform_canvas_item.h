// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <optional>

#include "dsp/audio_port.h"
#include "dsp/port_observation_manager.h"
#include "gui/qquick/waveform_canvas_item.h"

#include <QPointer>
#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::gui::qquick
{

class LiveWaveformCanvasItem : public WaveformCanvasItem
{
  Q_OBJECT
  QML_NAMED_ELEMENT (LiveWaveformViewer)
  Q_PROPERTY (
    zrythm::dsp::AudioPort * stereoPort READ stereoPort WRITE setStereoPort
      NOTIFY stereoPortChanged REQUIRED)
  Q_PROPERTY (
    zrythm::dsp::PortObservationManager * portObservationManager READ
      portObservationManager WRITE setPortObservationManager REQUIRED)
  Q_PROPERTY (
    int bufferSize READ bufferSize WRITE setBufferSize NOTIFY bufferSizeChanged)

public:
  explicit LiveWaveformCanvasItem (QQuickItem * parent = nullptr);
  ~LiveWaveformCanvasItem () override;

  zrythm::dsp::AudioPort * stereoPort () const;
  void                     setStereoPort (zrythm::dsp::AudioPort * port);

  zrythm::dsp::PortObservationManager * portObservationManager () const;
  void
  setPortObservationManager (zrythm::dsp::PortObservationManager * manager);

  int  bufferSize () const;
  void setBufferSize (int size);

Q_SIGNALS:
  void stereoPortChanged ();
  void bufferSizeChanged ();

private:
  void process_audio ();
  void try_create_token ();

  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}
