// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/qquick/chord_clip_canvas_item.h"
#include "gui/qquick/chord_clip_canvas_renderer.h"
#include "structure/arrangement/chord_clip.h"

namespace zrythm::gui::qquick
{

ChordClipCanvasItem::ChordClipCanvasItem (QQuickItem * parent)
    : ClipCanvasItemBase (parent)
{
}

QCanvasPainterItemRenderer *
ChordClipCanvasItem::createItemRenderer () const
{
  return new ChordClipCanvasRenderer ();
}

void
ChordClipCanvasItem::setChordClip (structure::arrangement::ChordClip * clip)
{
  if (chord_clip_ == clip)
    return;

  for (auto &connection : clip_connections_)
    QObject::disconnect (connection);
  clip_connections_.clear ();

  chord_clip_ = clip;

  if (chord_clip_ != nullptr)
    {
      // Re-render when chord content or clip properties change
      if (auto * chord_objects = chord_clip_->chordObjects ())
        {
          clip_connections_.push_back (
            QObject::connect (
              chord_objects,
              &structure::arrangement::ArrangerObjectListModel::contentChanged,
              this, [this] () { update (); }));
        }
      clip_connections_.push_back (
        QObject::connect (
          chord_clip_, &structure::arrangement::Clip::loopablePropertiesChanged,
          this, [this] () { update (); }));
      clip_connections_.push_back (
        QObject::connect (
          chord_clip_, &structure::arrangement::Clip::timelineLengthTicksChanged,
          this, [this] () { update (); }));
      if (auto * length = chord_clip_->length ())
        {
          clip_connections_.push_back (
            QObject::connect (
              length, &dsp::Position::positionChanged, this,
              [this] () { update (); }));
        }
    }

  update ();
  Q_EMIT chordClipChanged ();
}

void
ChordClipCanvasItem::setTextColor (const QColor &color)
{
  if (text_color_ == color)
    return;
  text_color_ = color;
  Q_EMIT textColorChanged ();
  update ();
}

void
ChordClipCanvasItem::setFont (const QFont &font)
{
  if (font_ == font)
    return;
  font_ = font;
  Q_EMIT fontChanged ();
  update ();
}

} // namespace zrythm::gui::qquick
