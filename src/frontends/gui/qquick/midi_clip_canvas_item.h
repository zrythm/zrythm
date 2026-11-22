// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <vector>

#include "frontends/gui/qquick/clip_canvas_item_base.h"

#include <QColor>
#include <QPointer>

namespace zrythm::structure::arrangement
{
class MidiClip;
}

namespace zrythm::frontends::gui::qquick
{

class MidiClipCanvasRenderer;

class MidiClipCanvasItem : public ClipCanvasItemBase
{
  Q_OBJECT
  QML_NAMED_ELEMENT (MidiClipCanvas)

  Q_PROPERTY (
    zrythm::structure::arrangement::MidiClip * midiClip READ midiClip WRITE
      setMidiClip NOTIFY midiClipChanged)
  Q_PROPERTY (
    QColor noteColor READ noteColor WRITE setNoteColor NOTIFY noteColorChanged)

public:
  explicit MidiClipCanvasItem (QQuickItem * parent = nullptr);

  QCanvasPainterItemRenderer * createItemRenderer () const override;

  structure::arrangement::MidiClip * midiClip () const { return midi_clip_; }
  void   setMidiClip (structure::arrangement::MidiClip * clip);
  QColor noteColor () const { return note_color_; }
  void   setNoteColor (const QColor &color);

Q_SIGNALS:
  void midiClipChanged ();
  void noteColorChanged ();

private:
  QPointer<structure::arrangement::MidiClip> midi_clip_;
  QColor                                     note_color_;
  std::vector<QMetaObject::Connection>       clip_connections_;
};

} // namespace zrythm::frontends::gui::qquick
