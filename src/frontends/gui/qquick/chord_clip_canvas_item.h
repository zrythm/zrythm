// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "frontends/gui/qquick/clip_canvas_item_base.h"

#include <QColor>
#include <QFont>
#include <QPointer>

namespace zrythm::structure::arrangement
{
class ChordClip;
}

namespace zrythm::frontends::gui::qquick
{

class ChordClipCanvasRenderer;

/**
 * @brief Canvas item for rendering chord clips.
 *
 * Replaces the old ChordClipSegmenter + QML Repeater approach.
 * Inherits referenceWidth/referenceX/loopPreview from ClipCanvasItemBase
 * so drag-preview works the same as audio/MIDI/automation clips.
 */
class ChordClipCanvasItem : public ClipCanvasItemBase
{
  Q_OBJECT
  QML_NAMED_ELEMENT (ChordClipCanvas)

  Q_PROPERTY (
    zrythm::structure::arrangement::ChordClip * chordClip READ chordClip WRITE
      setChordClip NOTIFY chordClipChanged)
  Q_PROPERTY (
    QColor textColor READ textColor WRITE setTextColor NOTIFY textColorChanged)
  Q_PROPERTY (QFont font READ font WRITE setFont NOTIFY fontChanged)

public:
  explicit ChordClipCanvasItem (QQuickItem * parent = nullptr);

  QCanvasPainterItemRenderer * createItemRenderer () const override;

  structure::arrangement::ChordClip * chordClip () const { return chord_clip_; }
  void setChordClip (structure::arrangement::ChordClip * clip);

  QColor textColor () const { return text_color_; }
  void   setTextColor (const QColor &color);

  QFont font () const { return font_; }
  void  setFont (const QFont &font);

Q_SIGNALS:
  void chordClipChanged ();
  void textColorChanged ();
  void fontChanged ();

private:
  QPointer<structure::arrangement::ChordClip> chord_clip_;
  QColor                                      text_color_;
  QFont                                       font_;
  std::vector<QMetaObject::Connection>        clip_connections_;
};

} // namespace zrythm::frontends::gui::qquick
