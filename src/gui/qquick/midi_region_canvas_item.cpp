// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/qquick/midi_region_canvas_item.h"
#include "gui/qquick/midi_region_canvas_renderer.h"
#include "structure/arrangement/midi_region.h"

namespace zrythm::gui::qquick
{

MidiRegionCanvasItem::MidiRegionCanvasItem (QQuickItem * parent)
    : QCanvasPainterItem (parent)
{
  setFillColor (Qt::transparent);
  setAlphaBlending (true);
}

QCanvasPainterItemRenderer *
MidiRegionCanvasItem::createItemRenderer () const
{
  return new MidiRegionCanvasRenderer ();
}

void
MidiRegionCanvasItem::setRegion (QObject * region)
{
  if (region_ == region)
    return;
  region_ = region;

  midi_region_ = qobject_cast<structure::arrangement::MidiRegion *> (region);

  for (const auto &connection : region_connections_)
    QObject::disconnect (connection);
  region_connections_.clear ();

  if (midi_region_ != nullptr)
    {
      region_connections_.push_back (
        QObject::connect (
          midi_region_, &structure::arrangement::MidiRegion::contentChanged,
          this, [this] () { update (); }, Qt::ConnectionType::QueuedConnection));

      auto * loop_range = midi_region_->loopRange ();
      if (loop_range != nullptr)
        {
          region_connections_.push_back (
            QObject::connect (
              loop_range,
              &structure::arrangement::ArrangerObjectLoopRange::
                loopableObjectPropertiesChanged,
              this, [this] () { update (); },
              Qt::ConnectionType::QueuedConnection));
        }

      auto * bounds = midi_region_->bounds ();
      if (bounds != nullptr)
        {
          region_connections_.push_back (
            QObject::connect (
              bounds->length (),
              &dsp::AtomicPositionQmlAdapter::positionChanged, this,
              [this] () { update (); }, Qt::ConnectionType::QueuedConnection));
        }
    }

  Q_EMIT regionChanged ();
  update ();
}

void
MidiRegionCanvasItem::setNoteColor (const QColor &color)
{
  if (note_color_ == color)
    return;
  note_color_ = color;
  Q_EMIT noteColorChanged ();
  update ();
}

} // namespace zrythm::gui::qquick
