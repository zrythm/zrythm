// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/arranger_object.h"
#include "undo/undo_stack.h"
#include "utils/variant_helpers.h"

#include <QItemSelectionModel>
#include <QtQmlIntegration>

namespace zrythm::actions
{
class ArrangerObjectSelectionOperator : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  using SelectedObjectsVector =
    std::vector<structure::arrangement::ArrangerObjectUuidReference>;
  using ArrangerObjectOwnerPtrVariant = to_pointer_variant<wrap_variant_t<
    structure::arrangement::ArrangerObjectVariant,
    structure::arrangement::ArrangerObjectOwner>>;
  using ObjectOwnerProvider = std::function<ArrangerObjectOwnerPtrVariant (
    structure::arrangement::ArrangerObjectPtrVariant)>;

  explicit ArrangerObjectSelectionOperator (
    undo::UndoStack     &undoStack,
    QItemSelectionModel &selectionModel,
    ObjectOwnerProvider  objectOwnerProvider,
    QObject *            parent = nullptr);

  Q_INVOKABLE bool moveByTicks (double tick_delta);

  Q_INVOKABLE bool moveNotesByPitch (int pitch_delta);

  Q_INVOKABLE bool moveAutomationPointsByDelta (double delta);

  Q_INVOKABLE bool deleteObjects ();

private:
  auto extractSelectedObjects () const -> SelectedObjectsVector;

  static bool validateHorizontalMovement (
    const SelectedObjectsVector &objects,
    double                       tick_delta);
  static bool
  validateVerticalMovement (const SelectedObjectsVector &objects, double delta);

  bool process_vertical_move (double delta);

private:
  undo::UndoStack     &undo_stack_;
  QItemSelectionModel &selection_model_;
  ObjectOwnerProvider  object_owner_provider_;
};

} // namespace zrythm::actions
