// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
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

  // Function whose parameter dialog is open
  property var _pendingFunction: null

  // Number of rows selected in the open editor pane; kept up to date by
  // connectionsMenuCount and refreshed when the functions menu opens
  property int _selectionCount: 0
  readonly property var activeFunctionDefinitions: {
    if (!root.clipObject)
      return [];
    switch (root.clipObject.type) {
    case ArrangerObject.MidiClip:
      return root.session.editorFunctions.midiFunctions;
    case ArrangerObject.AutomationClip:
      return root.session.editorFunctions.automationFunctions;
    case ArrangerObject.AudioClip:
      return root.session.editorFunctions.audioFunctions;
    default:
      return [];
    }
  }

  // Selection model of the open editor pane (null while the pane loads)
  readonly property var activeSelectionModel: {
    switch (editorSpecializedStack.currentIndex) {
    case 0:
      return midiEditorLoader.item ? midiEditorLoader.item.selectionModel : null;
    case 1:
      return audioEditorLoader.item ? audioEditorLoader.item.selectionModel : null;
    case 2:
      return automationEditorLoader.item ? automationEditorLoader.item.selectionModel : null;
    default:
      return null;
    }
  }
  required property ClipEditor clipEditor
  readonly property Clip clipObject: clipEditor.clipObject
  readonly property Project project: session.project
  required property ProjectSession session
  readonly property Track track: clipEditor.track

  function _applyFunction(definition, options) {
    switch (definition.family) {
    case EditorFunctionDefinition.Midi:
      root.session.arrangerObjectSelectionOperator.applyMidiFunction(root.activeSelectionModel, definition.id, options);
      break;
    case EditorFunctionDefinition.Automation:
      root.session.arrangerObjectSelectionOperator.applyAutomationFunction(root.activeSelectionModel, definition.id);
      break;
    case EditorFunctionDefinition.Audio:
      root.session.arrangerObjectSelectionOperator.applyAudioFunction(root.clipObject, root.clipEditor.audioEditor.selectionStartTick, root.clipEditor.audioEditor.selectionEndTick, definition.id, options);
      break;
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

  function _refreshSelectionCount() {
    root._selectionCount = root.activeSelectionModel !== null ? root.activeSelectionModel.selectedIndexes.length : 0;
  }

  function _rememberParams(definition, options) {
    const settings = GlobalState.application.appSettings;
    let allParams = {};
    try {
      allParams = JSON.parse(settings.editorFunctionLastParams);
    } catch (e) {}
    allParams[definition.id] = options;
    settings.editorFunctionLastParams = JSON.stringify(allParams);
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

  columns: 2
  rows: 2

  onActiveSelectionModelChanged: root._refreshSelectionCount()

  Connections {
    id: connectionsMenuCount

    function onSelectionChanged(selected, deselected) {
      root._refreshSelectionCount();
    }

    target: root.activeSelectionModel
  }

  Loader {
    id: functionDialogLoader

    active: false

    sourceComponent: FunctionDialog {
      functionDefinition: root._pendingFunction
      initialValues: root._lastParamsFor(root._pendingFunction)

      onClosed: functionDialogLoader.active = false
      onOptionsConfirmed: function (options) {
        root._rememberParams(root._pendingFunction, options);
        root._applyFunction(root._pendingFunction, options);
      }
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
      MenuButton {
        text: qsTr("Functions")
        tooltipText: qsTr("Apply a function to the selection")
        visible: root.activeFunctionDefinitions.length > 0

        menu: functionsMenu

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

            onObjectAdded: function (index, object) {
              functionsMenu.insertItem(index, object);
            }
            onObjectRemoved: function (index, object) {
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
