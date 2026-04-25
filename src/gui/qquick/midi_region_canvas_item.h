// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <vector>

#include <QColor>
#include <QPointer>
#include <QtCanvasPainter/qcanvaspainteritem.h>

namespace zrythm::structure::arrangement
{
class MidiRegion;
}

namespace zrythm::gui::qquick
{

class MidiRegionCanvasRenderer;

class MidiRegionCanvasItem : public QCanvasPainterItem
{
  Q_OBJECT
  QML_NAMED_ELEMENT (MidiRegionCanvas)

  Q_PROPERTY (QObject * region READ region WRITE setRegion NOTIFY regionChanged)
  Q_PROPERTY (
    QColor noteColor READ noteColor WRITE setNoteColor NOTIFY noteColorChanged)

public:
  explicit MidiRegionCanvasItem (QQuickItem * parent = nullptr);

  QCanvasPainterItemRenderer * createItemRenderer () const override;

  QObject * region () const { return region_; }
  void      setRegion (QObject * region);
  QColor    noteColor () const { return note_color_; }
  void      setNoteColor (const QColor &color);

  structure::arrangement::MidiRegion * midiRegion () const
  {
    return midi_region_;
  }

Q_SIGNALS:
  void regionChanged ();
  void noteColorChanged ();

private:
  QObject *                                    region_ = nullptr;
  QPointer<structure::arrangement::MidiRegion> midi_region_;
  QColor                                       note_color_;
  std::vector<QMetaObject::Connection>         region_connections_;
};

} // namespace zrythm::gui::qquick
