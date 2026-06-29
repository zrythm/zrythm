// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/qquick/midi_clip_canvas_item.h"
#include "gui/qquick/midi_clip_canvas_renderer.h"
#include "structure/arrangement/arranger_object_all.h"

namespace zrythm::gui::qquick
{

MidiClipCanvasItem::MidiClipCanvasItem (QQuickItem * parent)
    : QCanvasPainterItem (parent)
{
  setFillColor (Qt::transparent);
  setAlphaBlending (true);
}

QCanvasPainterItemRenderer *
MidiClipCanvasItem::createItemRenderer () const
{
  return new MidiClipCanvasRenderer ();
}

void
MidiClipCanvasItem::setMidiClip (structure::arrangement::MidiClip * clip)
{
  if (midi_clip_ == clip)
    return;
  midi_clip_ = clip;

  for (const auto &connection : clip_connections_)
    QObject::disconnect (connection);
  clip_connections_.clear ();

  if (midi_clip_ != nullptr)
    {
      clip_connections_.push_back (
        QObject::connect (
          midi_clip_, &structure::arrangement::MidiClip::contentChanged, this,
          [this] () { update (); }, Qt::ConnectionType::QueuedConnection));

      clip_connections_.push_back (
        QObject::connect (
          midi_clip_, &structure::arrangement::Clip::loopablePropertiesChanged,
          this, [this] () { update (); }, Qt::ConnectionType::QueuedConnection));

      clip_connections_.push_back (
        QObject::connect (
          midi_clip_->length (), &dsp::Position::positionChanged, this,
          [this] () { update (); }, Qt::ConnectionType::QueuedConnection));
    }

  Q_EMIT midiClipChanged ();
  update ();
}

void
MidiClipCanvasItem::setNoteColor (const QColor &color)
{
  if (note_color_ == color)
    return;
  note_color_ = color;
  Q_EMIT noteColorChanged ();
  update ();
}

} // namespace zrythm::gui::qquick
