// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QColor>
#include <QtCanvasPainter/qcanvaspainteritem.h>

namespace zrythm::gui::qquick
{

class ModularGridCanvasRenderer;

class ModularGridCanvasItem : public QCanvasPainterItem
{
  Q_OBJECT
  QML_NAMED_ELEMENT (ModularGridCanvas)
  Q_PROPERTY (double panX READ panX WRITE setPanX NOTIFY panXChanged)
  Q_PROPERTY (double panY READ panY WRITE setPanY NOTIFY panYChanged)
  Q_PROPERTY (double zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
  Q_PROPERTY (
    QColor dotColor READ dotColor WRITE setDotColor NOTIFY dotColorChanged)

public:
  explicit ModularGridCanvasItem (QQuickItem * parent = nullptr);

  QCanvasPainterItemRenderer * createItemRenderer () const override;

  double panX () const { return pan_x_; }
  void   setPanX (double x);
  double panY () const { return pan_y_; }
  void   setPanY (double y);
  double zoom () const { return zoom_; }
  void   setZoom (double zoom);
  QColor dotColor () const { return dot_color_; }
  void   setDotColor (const QColor &color);

Q_SIGNALS:
  void panXChanged ();
  void panYChanged ();
  void zoomChanged ();
  void dotColorChanged ();

private:
  double pan_x_ = 0.0;
  double pan_y_ = 0.0;
  double zoom_ = 1.0;
  QColor dot_color_ = Qt::gray;
};

} // namespace zrythm::gui::qquick
