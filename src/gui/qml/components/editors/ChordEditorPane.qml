// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.synchronizer
import Zrythm
import ZrythmGui
import ZrythmStyle

GridLayout {
  id: root

  required property ChordClip chordClip
  readonly property ChordTrack chordTrack: root.project.tracklist.singletonTracks.chordTrack ?? null
  required property ClipEditor clipEditor
  readonly property int maxRowHeight: 48

  // Auto-stretch row height derived from the left panel's available height.
  readonly property int minRowHeight: 20
  readonly property Project project: session.project
  readonly property int rowHeight: {
    const count = rowLabels.rowCount;
    if (count === 0)
      return root.maxRowHeight;
    const h = rowLabelsScrollView.height;
    if (h <= 0)
      return root.maxRowHeight;
    const fit = Math.floor(h / (count + 1)); // +1 for trailing "+" row
    return Math.max(root.minRowHeight, Math.min(root.maxRowHeight, fit));
  }
  readonly property ArrangerObjectSelectionOperator selectionOperator: root.session.createArrangerObjectSelectionOperator(arrangerSelectionModel)
  required property ProjectSession session
  readonly property Track track: root.project.tracklist.getTrackForTimelineObject(root.chordClip)

  function _selectedChordObjects(): QVariantList {
    const list = [];
    const idxs = arrangerSelectionModel.selectedIndexes;
    for (let i = 0; i < idxs.length; i++) {
      const srcIdx = unifiedObjectsModel.mapToSource(idxs[i]);
      // mapToSource can return an invalid index if the proxy mapping is stale
      // (e.g. rows removed mid-iteration); skip those to avoid a null-model
      // dereference.
      if (!srcIdx || !srcIdx.valid || !srcIdx.model)
        continue;
      const obj = srcIdx.model.data(srcIdx, ArrangerObjectListModel.ArrangerObjectPtrRole);
      if (obj instanceof ChordObject)
        list.push(obj);
    }
    return list;
  }

  // Helper: edit the descriptor of a list of chord objects (batch, one undo).
  function applyChordToObjects(descriptor, objectList) {
    if (!descriptor || objectList.length === 0)
      return;
    root.session.arrangerObjectCreator.editChordObjectsDescriptor(objectList, descriptor.rootNote, descriptor.chordType, descriptor.chordAccent, descriptor.hasBass, descriptor.hasBass ? descriptor.bassNote : MusicalScale.MusicalNote.C, descriptor.inversion);
  }

  columnSpacing: 0
  columns: 3
  rowSpacing: 0
  rows: 3

  // Cell (0,0): toolbar
  ZrythmToolBar {
    id: editorToolbar
  }

  // Cell (0,1): ruler
  Ruler {
    id: ruler

    Layout.fillWidth: true
    clipObject: root.chordClip
    clipOperator: root.session.clipOperator
    editorSettings: root.clipEditor.chordEditor
    snapGrid: root.session.uiState.snapGridEditor
    tempoMap: root.project.tempoMap
    track: root.track
    transport: root.project.transport
  }

  // Cell (0,2): zoom column (spans all rows)
  ColumnLayout {
    Layout.alignment: Qt.AlignTop
    Layout.rowSpan: 3

    ToolButton {
      icon.source: ResourceManager.getIconUrl("gnome-icon-library", "chat-symbolic.svg")

      ToolTip {
        text: qsTr("Zoom In")
      }
    }
  }

  // Cell (1,0): left panel — chord row labels
  ScrollView {
    id: rowLabelsScrollView

    Layout.fillHeight: true
    Layout.preferredWidth: 120
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
    ScrollBar.vertical.policy: ScrollBar.AlwaysOff
    implicitHeight: chordArranger.implicitHeight

    ChordRowLabels {
      id: rowLabels

      chordRowModel: chordRowModel
      chordTrack: root.chordTrack
      rowHeight: root.rowHeight
      undoStack: root.session.undoStack
      width: parent.width

      onNewChordRequested: {
        createDialogLoader.active = true;
        createDialogLoader.item._createTicks = 0;
        createDialogLoader.item.open();
      }
      onRowEditRequested: function (row) {
        editDialogLoader.active = true;
        editDialogLoader.item._batchRow = row;
        editDialogLoader.item.applyFromDescriptor(chordRowModel.descriptorAtRow(row));
        editDialogLoader.item.open();
      }
    }

    Synchronizer {
      sourceObject: root.clipEditor.chordEditor
      sourceProperty: "y"
      targetObject: rowLabelsScrollView.contentItem
      targetProperty: "contentY"
    }
  }

  // --- Non-visual items (don't take grid cells) ---

  ChordRowListModel {
    id: chordRowModel

    chordClip: root.chordClip
  }

  UnifiedProxyModel {
    id: unifiedObjectsModel
  }

  ItemSelectionModel {
    id: arrangerSelectionModel

    model: unifiedObjectsModel
  }

  // Cell (1,1): chord arranger grid
  ChordArranger {
    id: chordArranger

    Layout.fillHeight: true
    Layout.fillWidth: true
    arrangerContentHeight: rowLabels.height
    arrangerSelectionModel: arrangerSelectionModel
    chordRowModel: chordRowModel
    chordTrack: root.chordTrack
    clipContext: root.clipEditor.clipObject
    clipEditor: root.clipEditor
    objectCreator: root.session.arrangerObjectCreator
    rowHeight: root.rowHeight
    ruler: ruler
    selectionOperator: root.selectionOperator
    snapGrid: root.session.uiState.snapGridEditor
    tempoMap: root.project.tempoMap
    tool: root.session.uiState.tool
    transport: root.project.transport
    undoStack: root.session.undoStack
    unifiedObjectsModel: unifiedObjectsModel

    onChordCreationRequested: function (ticks) {
      createDialogLoader.active = true;
      createDialogLoader.item._createTicks = ticks;
      createDialogLoader.item.open();
    }
    onVerticalChordChangeRequested: function (targetDescriptor) {
      root.applyChordToObjects(targetDescriptor, root._selectedChordObjects());
    }
  }

  // --- Dialogs (0-size Loaders, don't affect layout) ---
  Loader {
    id: createDialogLoader

    active: false

    sourceComponent: ChordSelectorDialog {
      id: createDialog

      property double _createTicks: 0

      chordTrack: root.chordTrack
      playheadTicks: root.project.transport.playhead.ticks

      onAccepted: {
        root.session.arrangerObjectCreator.addChordObjectFromFields(root.chordClip, createDialog._createTicks, createDialog.tempRootNote, createDialog.tempChordType, createDialog.tempChordAccent, createDialog.tempHasBass, createDialog.tempHasBass ? createDialog.tempBassNote : MusicalScale.MusicalNote.C, createDialog.tempInversion);
      }
      onClosed: createDialogLoader.active = false
    }
  }

  Loader {
    id: editDialogLoader

    active: false

    sourceComponent: ChordSelectorDialog {
      id: editDialog

      readonly property ChordDescriptor _applyDesc: ChordDescriptor {
      }
      property int _batchRow: -1

      chordTrack: root.chordTrack
      playheadTicks: root.project.transport.playhead.ticks

      onAccepted: {
        _applyDesc.rootNote = editDialog.tempRootNote;
        _applyDesc.chordType = editDialog.tempChordType;
        _applyDesc.chordAccent = editDialog.tempChordAccent;
        _applyDesc.inversion = editDialog.tempInversion;
        _applyDesc.hasBass = editDialog.tempHasBass;
        if (editDialog.tempHasBass)
          _applyDesc.bassNote = editDialog.tempBassNote;

        if (editDialog._batchRow >= 0) {
          const objs = chordRowModel.chordObjectsAtRow(editDialog._batchRow);
          root.applyChordToObjects(_applyDesc, objs);
        } else {
          root.applyChordToObjects(_applyDesc, root._selectedChordObjects());
        }
      }
      onClosed: editDialogLoader.active = false
    }
  }
}
