// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/qquick/tempo_curve_canvas_item.h"
#include "gui/qquick/tempo_curve_canvas_renderer.h"
#include "structure/arrangement/arranger_object_list_model.h"
#include "structure/arrangement/tempo_object_manager.h"

namespace zrythm::gui::qquick
{

TempoCurveCanvasItem::TempoCurveCanvasItem (QQuickItem * parent)
    : QCanvasPainterItem (parent)
{
  setFillColor (Qt::transparent);
  setAlphaBlending (true);
}

QCanvasPainterItemRenderer *
TempoCurveCanvasItem::createItemRenderer () const
{
  return new TempoCurveCanvasRenderer ();
}

void
TempoCurveCanvasItem::setTempoObjectManager (
  structure::arrangement::TempoObjectManager * manager)
{
  if (manager_ == manager)
    return;

  if (manager_ != nullptr)
    QObject::disconnect (manager_, nullptr, this, nullptr);

  manager_ = manager;

  if (manager_ != nullptr)
    {
      auto * model = manager_->tempoObjects ();
      if (model != nullptr)
        {
          QObject::connect (
            model, &QAbstractListModel::rowsInserted, this,
            &TempoCurveCanvasItem::update, Qt::UniqueConnection);
          QObject::connect (
            model, &QAbstractListModel::rowsRemoved, this,
            &TempoCurveCanvasItem::update, Qt::UniqueConnection);
          QObject::connect (
            model,
            &structure::arrangement::ArrangerObjectListModel::contentChanged,
            this, &TempoCurveCanvasItem::update, Qt::UniqueConnection);
          QObject::connect (
            model, &QAbstractListModel::modelReset, this,
            &TempoCurveCanvasItem::update, Qt::UniqueConnection);
        }
    }

  Q_EMIT tempoObjectManagerChanged ();
  update ();
}

void
TempoCurveCanvasItem::setTempoMap (dsp::TempoMapWrapper * wrapper)
{
  if (tempo_map_ == wrapper)
    return;

  if (tempo_map_ != nullptr)
    QObject::disconnect (tempo_map_, nullptr, this, nullptr);

  tempo_map_ = wrapper;

  if (tempo_map_ != nullptr)
    {
      // Re-render when the base tempo or any inserted event changes.
      QObject::connect (
        tempo_map_, &dsp::TempoMapWrapper::baseBpmChanged, this,
        &TempoCurveCanvasItem::update, Qt::UniqueConnection);
      QObject::connect (
        tempo_map_, &dsp::TempoMapWrapper::tempoEventsChanged, this,
        &TempoCurveCanvasItem::update, Qt::UniqueConnection);
    }

  Q_EMIT tempoMapChanged ();
  update ();
}

double
TempoCurveCanvasItem::baseBpm () const
{
  return tempo_map_ != nullptr ? tempo_map_->baseBpm () : 120.0;
}

void
TempoCurveCanvasItem::setPxPerTick (double px)
{
  if (qFuzzyCompare (px_per_tick_, px))
    return;
  px_per_tick_ = px;
  Q_EMIT pxPerTickChanged ();
  update ();
}

void
TempoCurveCanvasItem::setScrollX (double x)
{
  if (qFuzzyCompare (scroll_x_, x))
    return;
  scroll_x_ = x;
  Q_EMIT scrollXChanged ();
  update ();
}

void
TempoCurveCanvasItem::setScrollXPlusWidth (double w)
{
  if (qFuzzyCompare (scroll_x_plus_width_, w))
    return;
  scroll_x_plus_width_ = w;
  Q_EMIT scrollXPlusWidthChanged ();
  update ();
}

void
TempoCurveCanvasItem::setCurveColor (const QColor &color)
{
  if (curve_color_ == color)
    return;
  curve_color_ = color;
  Q_EMIT curveColorChanged ();
  update ();
}

void
TempoCurveCanvasItem::setSelectionModel (QItemSelectionModel * model)
{
  if (selection_model_ == model)
    return;
  selection_model_ = model;
  Q_EMIT selectionModelChanged ();
  update ();
}

void
TempoCurveCanvasItem::setDragActive (bool active)
{
  if (drag_active_ == active)
    return;
  drag_active_ = active;
  Q_EMIT dragActiveChanged ();
  update ();
}

void
TempoCurveCanvasItem::setDragDeltaPx (double px)
{
  if (qFuzzyCompare (drag_delta_px_, px))
    return;
  drag_delta_px_ = px;
  Q_EMIT dragDeltaPxChanged ();
  update ();
}

} // namespace zrythm::gui::qquick
