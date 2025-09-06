<!---
SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
SPDX-License-Identifier: FSFAP
-->

# Zrythm Object Selection System

## Core Architecture

Zrythm uses a unified selection system that handles multiple arranger object types through Qt's [`ItemSelectionModel`](src/gui/qml/components/arranger/Arranger.qml:55). The system bridges C++ models with QML views through several key components.

## Key Components

### Unified Model
[`UnifiedArrangerObjectsModel`](src/gui/backend/unified_arranger_objects_model.h:12) aggregates multiple source models into a single selection context:
- Maps source indices to unified indices with [`mapFromSource()`](src/gui/backend/unified_arranger_objects_model.h:22)
- Supports mixed object types (regions, markers, notes, automation points)

### Selection Tracking
[`SelectionTracker.qml`](src/gui/qml/utils/SelectionTracker.qml) provides per-object selection state:
- Tracks [`isSelected`](src/gui/qml/utils/SelectionTracker.qml:10) property
- Connects to global selection model changes
- Used by all arranger object views

### Selection Operator
[`ArrangerObjectSelectionOperator`](src/actions/arranger_object_selection_operator.h) handles operations on selections:
- [`moveByTicks()`](src/actions/arranger_object_selection_operator.h:47) with validation
- Creates [`MoveArrangerObjectsCommand`](src/commands/move_arranger_objects_command.h) for undo/redo

## QML Integration

### Object Views
Each arranger object view integrates selection through:
```qml
SelectionTracker {
    modelIndex: unifiedObjectsModel.mapFromSource(sourceModel.index(index, 0))
    selectionModel: arrangerSelectionModel
}
```

### Mouse Handling
[`Arranger.qml`](src/gui/qml/components/arranger/Arranger.qml:325) handles movement:
```qml
function moveSelectionsX(ticksDiff) {
    selectionOperator.moveByTicks(ticksDiff);
}
```

### Selection Logic
[`handleObjectSelection()`](src/gui/qml/components/arranger/Arranger.qml:87) manages selection modes:
- Single click: Clear and select
- Ctrl+click: Toggle selection
- Shift+click: Range selection (TODO)

## Movement Implementation

The [`MoveArrangerObjectsCommand`](src/commands/move_arranger_objects_command.h):
- Stores original positions for undo/redo
- Applies tick delta to all selected objects
- Handles mixed object types transparently

## Performance

- Lazy loading with visibility-based activation
- Efficient index mapping through unified model
- Batch processing of selected objects

This system provides consistent selection behavior across all arranger object types with full undo/redo support and Qt-native integration.
