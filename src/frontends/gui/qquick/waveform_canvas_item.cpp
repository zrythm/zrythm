// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "frontends/gui/qquick/waveform_canvas_item.h"
#include "frontends/gui/qquick/waveform_canvas_renderer.h"

namespace zrythm::frontends::gui::qquick
{

WaveformCanvasItem::WaveformCanvasItem (QQuickItem * parent)
    : ClipCanvasItemBase (parent)
{
  setFillColor (Qt::transparent);
  setAlphaBlending (true);
}

QCanvasPainterItemRenderer *
WaveformCanvasItem::createItemRenderer () const
{
  return new WaveformCanvasRenderer ();
}

void
WaveformCanvasItem::notifyBufferChanged ()
{
  ++buffer_generation_;
  update ();
}

void
WaveformCanvasItem::setWaveformColor (const QColor &color)
{
  if (waveform_color_ == color)
    return;
  waveform_color_ = color;
  Q_EMIT waveformColorChanged ();
  update ();
}

void
WaveformCanvasItem::setOutlineColor (const QColor &color)
{
  if (outline_color_ == color)
    return;
  outline_color_ = color;
  Q_EMIT outlineColorChanged ();
  update ();
}

} // namespace zrythm::frontends::gui::qquick
