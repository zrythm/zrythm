// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import ZrythmStyle
import ZrythmArrangement
import ZrythmGui
import Zrythm

Arranger {
  id: root

  required property ChordRowListModel chordRowModel
  required property ChordTrack chordTrack
  readonly property ChordRegion region: clipEditor.region
  required property int rowHeight
  readonly property Track track: clipEditor.track

  // Emitted when the user draws on empty space (below all rows) and a chord
  // selector popup is needed to pick the new chord type.
  signal chordCreationRequested(double ticks)

  // Emitted when selected chord objects should change to the target row's chord.
  signal verticalChordChangeRequested(var targetDescriptor)

  function beginObjectCreation(coordinates: point): ChordObject {
    const row = Math.floor(coordinates.y / root.rowHeight);
    const tickPosition = coordinates.x / root.ruler.pxPerTick;
    const localTickPosition = tickPosition - region.position.ticks;

    if (row >= 0 && row < root.chordRowModel.rowCount()) {
      const desc = root.chordRowModel.descriptorAtRow(row);
      const obj = root.objectCreator.addChordObjectFromDescriptor(region, localTickPosition, desc);
      root.currentAction = Arranger.CreatingResizingMovingR;
      root.selectSingleObject(region.chordObjects, region.chordObjects.rowCount() - 1);
      CursorManager.setResizeEndCursor();
      return obj;
    }
    root.chordCreationRequested(localTickPosition);
    return null;
  }

  function getObjectHeight(obj: ChordObject): real {
    return root.rowHeight;
  }

  function getObjectY(obj: ChordObject): real {
    return root.chordRowModel.rowForChordObject(obj) * root.rowHeight;
  }

  function moveSelectionsY(dy: real, prevY: real) {
    const prevRow = Math.floor(prevY / root.rowHeight);
    const currentRow = Math.floor((prevY + dy) / root.rowHeight);
    if (currentRow === prevRow)
      return;
    if (currentRow < 0 || currentRow >= root.chordRowModel.rowCount())
      return; // empty zone: no-op (keeps current data)

    const targetDesc = root.chordRowModel.descriptorAtRow(currentRow);
    root.verticalChordChangeRequested(targetDesc);
  }

  function moveTemporaryObjectsY(dy: real, prevY: real) {
    const prevRow = Math.floor(prevY / root.rowHeight);
    const currentRow = Math.floor((prevY + dy) / root.rowHeight);
    const deltaRows = currentRow - prevRow;
    root.tempQmlArrangerObjects.forEach(qmlObj => {
      qmlObj.y += deltaRows * root.rowHeight;
    });
  }

  // Chord events have no bounds; never treat resize as loop resize.
  function shouldResizeBeLoopResize(object: ArrangerObjectBaseView, fromStart: bool): bool {
    return false;
  }

  editorSettings: clipEditor.chordEditor
  enableYScroll: true
  scrollView.ScrollBar.horizontal.policy: ScrollBar.AsNeeded

  content: Repeater {
    id: chordObjectsRepeater

    anchors.fill: parent
    model: root.region.chordObjects

    delegate: ArrangerObjectLoader {
      id: chordObjectLoader

      readonly property ChordObject chordObject: arrangerObject as ChordObject

      function _recomputeLayout() {
        const row = root.chordRowModel.rowForChordObject(chordObject);
        if (row < 0)
          return; // not yet grouped — keep the last layout until repopulated
        chordObjectLoader.y = row * root.rowHeight;
        chordObjectLoader.width = Math.max((ArrangerObjectHelpers.getObjectEndTicks(chordObject) - chordObject.position.ticks) * root.ruler.pxPerTick, 2);
      }

      arrangerSelectionModel: root.arrangerSelectionModel
      height: root.rowHeight
      model: chordObjectsRepeater.model
      pxPerTick: root.ruler.pxPerTick
      scrollViewWidth: root.scrollViewWidth
      scrollX: root.scrollX
      unifiedObjectsModel: root.unifiedObjectsModel
      useCustomWidth: true

      sourceComponent: Component {
        ChordObjectView {
          id: chordObjectView

          arrangerObject: chordObjectLoader.arrangerObject
          chordTrack: root.chordTrack
          isSelected: chordObjectLoader.selectionTracker.isSelected
          objectCreator: root.objectCreator
          track: root.track
          undoStack: root.undoStack

          onHoveredChanged: {
            root.handleObjectHover(chordObjectView.hovered, chordObjectView);
          }
          onSelectionRequested: function (mouse) {
            root.handleObjectSelection(chordObjectsRepeater.model, chordObjectLoader.index, mouse);
          }
        }
      }

      Component.onCompleted: chordObjectLoader._recomputeLayout()
      onPxPerTickChanged: chordObjectLoader._recomputeLayout()

      Connections {
        function onContentChanged() {
          chordObjectLoader._recomputeLayout();
        }

        target: root.chordRowModel
      }
    }
  }

  // Handle drops from the chord pad panel.
  onCanvasDrop: drop => {
    if (!drop.formats.includes("application/x-zrythm-chord"))
      return;
    try {
      const payload = JSON.parse(drop.getDataAsString("application/x-zrythm-chord"));
      const hasBass = !!payload.hasBass;
      if (!payload || !Number.isFinite(payload.root) || !Number.isFinite(payload.type) || !Number.isFinite(payload.accent) || !Number.isFinite(payload.inversion) || (hasBass && !Number.isFinite(payload.bass)))
        return;
      const tickPosition = drop.x / root.ruler.pxPerTick;
      const localTickPosition = tickPosition - region.position.ticks;
      root.objectCreator.addChordObjectFromFields(region, localTickPosition, payload.root, payload.type, payload.accent, hasBass, hasBass ? payload.bass : MusicalScale.MusicalNote.C, payload.inversion);
      drop.accept(Qt.CopyAction);
    } catch (e) {
      // Malformed MIME data (stale/foreign drag source): ignore silently.
    }
  }
}
