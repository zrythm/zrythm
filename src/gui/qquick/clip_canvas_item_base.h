// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QtCanvasPainter/qcanvaspainteritem.h>

namespace zrythm::gui::qquick
{

/**
 * @brief Base for clip canvas items.
 *
 * During a resize drag the delegate gets wider or narrower, but the content
 * inside should keep its original density. referenceWidth tells the renderer
 * "pretend you are this many pixels wide" regardless of the actual width.
 * referenceX shifts the content left edge (used when resizing from the
 * start). Both default to 0, meaning "use the real width / no offset".
 */
class ClipCanvasItemBase : public QCanvasPainterItem
{
  Q_OBJECT

  Q_PROPERTY (
    double referenceWidth READ referenceWidth WRITE setReferenceWidth NOTIFY
      referenceWidthChanged)
  Q_PROPERTY (
    double referenceX READ referenceX WRITE setReferenceX NOTIFY
      referenceXChanged)
  Q_PROPERTY (
    bool loopPreview READ loopPreview WRITE setLoopPreview NOTIFY
      loopPreviewChanged)

public:
  explicit ClipCanvasItemBase (QQuickItem * parent = nullptr)
      : QCanvasPainterItem (parent)
  {
    // Default fillColor is black — make transparent so the clip background
    // shows through, and enable alpha blending for non-opaque pixels.
    setFillColor (Qt::transparent);
    setAlphaBlending (true);
  }

  double referenceWidth () const { return reference_width_; }
  void   setReferenceWidth (double w)
  {
    if (qFuzzyCompare (reference_width_, w))
      return;
    reference_width_ = w;
    Q_EMIT referenceWidthChanged ();
    update ();
  }

  double referenceX () const { return reference_x_; }
  void   setReferenceX (double x)
  {
    if (qFuzzyCompare (reference_x_, x))
      return;
    reference_x_ = x;
    Q_EMIT referenceXChanged ();
    update ();
  }

  /// Returns referenceWidth if set (> 0), otherwise the actual width().
  double effectiveReferenceWidth () const
  {
    return (reference_width_ > 0) ? reference_width_ : width ();
  }

  /// When true, the renderer wraps content into the loop region even if the
  /// clip is not currently looped. Used during loop-resize drag of a
  /// non-looped clip so the user can preview the looped content.
  bool loopPreview () const { return loop_preview_; }
  void setLoopPreview (bool preview)
  {
    if (loop_preview_ == preview)
      return;
    loop_preview_ = preview;
    Q_EMIT loopPreviewChanged ();
    update ();
  }

Q_SIGNALS:
  void referenceWidthChanged ();
  void referenceXChanged ();
  void loopPreviewChanged ();

private:
  double reference_width_ = 0;
  double reference_x_ = 0;
  bool   loop_preview_ = false;
};

} // namespace zrythm::gui::qquick
