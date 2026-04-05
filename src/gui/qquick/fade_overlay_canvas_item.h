// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <vector>

#include <QColor>
#include <QPointer>
#include <QtCanvasPainter/qcanvaspainteritem.h>

namespace zrythm::structure::arrangement
{
class AudioRegion;
}

namespace zrythm::gui::qquick
{

class FadeOverlayCanvasRenderer;

/**
 * @brief QML-visible canvas item that renders a single fade overlay.
 *
 * Each instance handles one fade direction (FadeIn or FadeOut).
 * The canvas should be sized to the fade area width only.
 */
class FadeOverlayCanvasItem : public QCanvasPainterItem
{
  Q_OBJECT
  QML_NAMED_ELEMENT (FadeOverlayCanvas)

  Q_PROPERTY (QObject * region READ region WRITE setRegion NOTIFY regionChanged)
  Q_PROPERTY (
    qreal pxPerTick READ pxPerTick WRITE setPxPerTick NOTIFY pxPerTickChanged)
  Q_PROPERTY (
    FadeType fadeType READ fadeType WRITE setFadeType NOTIFY fadeTypeChanged)
  Q_PROPERTY (bool hovered READ hovered WRITE setHovered NOTIFY hoveredChanged)
  Q_PROPERTY (
    QColor overlayColor READ overlayColor WRITE setOverlayColor NOTIFY
      overlayColorChanged)
  Q_PROPERTY (
    QColor curveColor READ curveColor WRITE setCurveColor NOTIFY
      curveColorChanged)

public:
  enum FadeType
  {
    FadeIn = 0,
    FadeOut = 1,
  };
  Q_ENUM (FadeType)

  explicit FadeOverlayCanvasItem (QQuickItem * parent = nullptr);

  QCanvasPainterItemRenderer * createItemRenderer () const override;

  QObject * region () const { return region_; }
  void      setRegion (QObject * region);
  qreal     pxPerTick () const { return px_per_tick_; }
  void      setPxPerTick (qreal px);
  FadeType  fadeType () const { return fade_type_; }
  void      setFadeType (FadeType type);
  bool      hovered () const { return hovered_; }
  void      setHovered (bool hovered);
  QColor    overlayColor () const { return overlay_color_; }
  void      setOverlayColor (const QColor &color);
  QColor    curveColor () const { return curve_color_; }
  void      setCurveColor (const QColor &color);

  structure::arrangement::AudioRegion * audioRegion () const
  {
    return audio_region_;
  }

Q_SIGNALS:
  void regionChanged ();
  void pxPerTickChanged ();
  void fadeTypeChanged ();
  void hoveredChanged ();
  void overlayColorChanged ();
  void curveColorChanged ();

private:
  QObject *                                     region_ = nullptr;
  QPointer<structure::arrangement::AudioRegion> audio_region_;
  qreal                                         px_per_tick_ = 1.0;
  FadeType                                      fade_type_ = FadeIn;
  bool                                          hovered_ = false;
  QColor overlay_color_{ 51, 51, 51, 153 };
  QColor curve_color_{ 255, 255, 255, 200 };

  std::vector<QMetaObject::Connection> region_connections_;
};

} // namespace zrythm::gui::qquick
