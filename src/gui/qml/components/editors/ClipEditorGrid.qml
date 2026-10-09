// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt.labs.synchronizer
import Zrythm
import ZrythmGui

GridLayout {
  id: root

  required property ClipEditor clipEditor
  readonly property Project project: session.project
  readonly property var clipObject: clipEditor.clipObject
  required property ProjectSession session
  readonly property var track: clipEditor.track

  columns: 2
  rows: 2

  readonly property var activeFunctionDefinitions: {
    if (!root.clipObject)
      return [];
    switch (root.clipObject.type) {
    case ArrangerObject.MidiClip:
      return root.session.editorFunctions.midiFunctions;
    default:
      return [];
    }
  }

  // Selection model of the open editor pane (null while the pane loads)
  readonly property var activeSelectionModel: {
    switch (editorSpecializedStack.currentIndex) {
    case 0:
      return midiEditorLoader.item ? midiEditorLoader.item.selectionModel : null;
    default:
      return null;
    }
  }

  // Number of rows selected in the open editor pane; kept up to date by
  // connectionsMenuCount and refreshed when the functions menu opens
  property int _selectionCount: 0

  // Function whose parameter dialog is open
  property var _pendingFunction: null

  function _refreshSelectionCount() {
    root._selectionCount = root.activeSelectionModel !== null ? root.activeSelectionModel.selectedIndexes.length : 0;
  }

  function _applyFunction(definition, options) {
    switch (definition.family) {
    case EditorFunctionDefinition.Midi:
      root.session.arrangerObjectSelectionOperator.applyMidiFunction(root.activeSelectionModel, definition.id, options);
      break;
      break;
    }
  }

  function _triggerFunction(definition) {
    if (definition.hasParameters) {
      root._pendingFunction = definition;
      functionDialogLoader.active = true;
      if (functionDialogLoader.item)
        functionDialogLoader.item.open();
    } else {
      root._applyFunction(definition, {});
    }
  }

  function _lastParamsFor(definition) {
    if (!definition)
      return {};
    try {
      const allParams = JSON.parse(GlobalState.application.appSettings.editorFunctionLastParams);
      return allParams[definition.id] !== undefined ? allParams[definition.id] : {};
    } catch (e) {
      return {};
    }
  }

  function _rememberParams(definition, options) {
    const settings = GlobalState.application.appSettings;
    let allParams = {};
    try {
      allParams = JSON.parse(settings.editorFunctionLastParams);
    } catch (e) {
    }
    allParams[definition.id] = options;
    settings.editorFunctionLastParams = JSON.stringify(allParams);
  }

  onActiveSelectionModelChanged: root._refreshSelectionCount()

  Connections {
    id: connectionsMenuCount

    target: root.activeSelectionModel

    function onSelectionChanged(selected, deselected) {
      root._refreshSelectionCount();
    }
  }

  Loader {
    id: functionDialogLoader

    active: false

    sourceComponent: FunctionDialog {
      functionDefinition: root._pendingFunction
      initialValues: root._lastParamsFor(root._pendingFunction)

      onOptionsConfirmed: function(options) {
        root._rememberParams(root._pendingFunction, options);
        root._applyFunction(root._pendingFunction, options);
      }
      onClosed: functionDialogLoader.active = false
    }

    // Hosts a popup dialog; invisible so it takes no grid cell (layouts
    // skip invisible children)
    visible: false
  }

  RowLayout {
    id: selectedRegionInfo

    Layout.fillHeight: true
    Layout.rowSpan: 2

    Rectangle {
      Layout.fillHeight: true
      Layout.preferredWidth: 5
      color: root.clipObject.color.useColor ? root.clipObject.color.color : root.track.color
    }

    RotatedLabel {
      id: rotatedText

      Layout.fillHeight: true
      font: ZrythmTheme.normalTextFont
      text: root.clipObject.name.name
    }
  }

  ZrythmToolBar {
    id: editorToolbar

    Layout.fillWidth: true

    leftItems: [
      SnapGridButton {
        snapGrid: root.session.uiState.snapGridEditor
      },
      RowLayout {
        visible: root.clipObject.type === ArrangerObject.MidiClip

        Label {
          Layout.alignment: Qt.AlignVCenter
          text: qsTr("Highlight")
        }

        ComboBox {
          model: [
            {
              text: qsTr("No Highlight"),
              value: MidiEditor.None
            },
            {
              text: qsTr("Chord"),
              value: MidiEditor.Chord
            },
            {
              text: qsTr("Scale"),
              value: MidiEditor.Scale
            },
            {
              text: qsTr("Scale + Chord"),
              value: MidiEditor.Both
            }
          ]
          textRole: "text"
          valueRole: "value"

          Synchronizer on currentValue {
            sourceObject: GlobalState.application.appSettings
            sourceProperty: "pianoRollHighlight"
          }

          ToolTip {
            text: qsTr("Piano Roll Highlighting")
          }
        }
      },

      ToolButton {
        text: qsTr("Functions")
        visible: root.activeFunctionDefinitions.length > 0

        onClicked: functionsMenu.popup()

        Menu {
          id: functionsMenu

          onAboutToShow: root._refreshSelectionCount()

          Instantiator {
            model: root.activeFunctionDefinitions

            delegate: MenuItem {
              required property var modelData

              enabled: root._selectionCount >= modelData.minimumSelectionCount
              text: modelData.label

              onTriggered: root._triggerFunction(modelData)
            }

            onObjectAdded: function(index, object) {
              functionsMenu.insertItem(index, object);
            }
            onObjectRemoved: function(index, object) {
              functionsMenu.removeItem(object);
            }
          }
        }
      }
    ]
    rightItems: [
      ToolButton {
        text: "right"
      }
    ]
  }

  StackLayout {
    id: editorSpecializedStack

    currentIndex: {
      switch (root.clipObject.type) {
      case ArrangerObject.MidiClip:
        return 0;
      case ArrangerObject.AudioClip:
        return 1;
      case ArrangerObject.ChordClip:
        return 3;
      case ArrangerObject.AutomationClip:
        return 2;
      default:
        return 0;
      }
    }

    Loader {
      id: midiEditorLoader

      active: editorSpecializedStack.currentIndex === 0

      sourceComponent: MidiEditorPane {
        clipEditor: root.clipEditor
        midiClip: root.clipObject as MidiClip
        midiEditor: root.clipEditor.midiEditor
        session: root.session
        unifiedObjectsModel: root.session.editorArrangerObjects
      }
    }

    Loader {
      id: audioEditorLoader

      active: editorSpecializedStack.currentIndex === 1

      sourceComponent: AudioEditorPane {
        audioClip: root.clipObject as AudioClip
        clipEditor: root.clipEditor
        session: root.session
      }
    }

    Loader {
      id: automationEditorLoader

      active: editorSpecializedStack.currentIndex === 2

      sourceComponent: AutomationEditorPane {
        automationClip: root.clipObject as AutomationClip
        automationEditor: root.clipEditor.automationEditor
        clipEditor: root.clipEditor
        session: root.session
        unifiedObjectsModel: root.session.editorArrangerObjects
      }
    }

    Loader {
      id: chordEditorLoader

      active: editorSpecializedStack.currentIndex === 3

      sourceComponent: ChordEditorPane {
        chordClip: root.clipObject as ChordClip
        clipEditor: root.clipEditor
        session: root.session
        unifiedObjectsModel: root.session.editorArrangerObjects
      }
    }
  }
}
