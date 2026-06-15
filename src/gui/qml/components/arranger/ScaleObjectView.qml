// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import Zrythm
import ZrythmStyle

ArrangerObjectBaseView {
  id: root

  property string scaleName: ""
  readonly property ScaleObject scaleObject: arrangerObject as ScaleObject

  height: textMetrics.height + 2 * ZrythmTheme.buttonPadding
  width: textMetrics.width + 2 * ZrythmTheme.buttonPadding

  Component.onCompleted: {
    if (root.scaleObject?.scale)
      root.scaleName = root.scaleObject.scale.toString();
  }
  onObjectDoubleClicked: scaleDialog.open()

  Connections {
    function onRootKeyChanged() {
      root.scaleName = root.scaleObject.scale.toString();
    }

    function onScaleTypeChanged() {
      root.scaleName = root.scaleObject.scale.toString();
    }

    target: root.scaleObject?.scale ?? null
  }

  Rectangle {
    anchors.fill: parent
    color: root.objectColor
    radius: ZrythmTheme.toolButtonRadius
  }

  Text {
    id: nameText

    color: root.palette.text
    font: root.font
    padding: ZrythmTheme.buttonPadding
    text: root.scaleName
  }

  TextMetrics {
    id: textMetrics

    font: nameText.font
    text: nameText.text
  }

  QObjectPropertyOperator {
    id: scalePropertyOp

    undoStack: root.undoStack
  }

  ScaleSelectorDialog {
    id: scaleDialog

    musicalScale: root.scaleObject.scale

    onScaleSelected: (rootKey, scaleType) => {
      scalePropertyOp.currentObject = root.scaleObject.scale;
      root.undoStack.beginMacro(qsTr("Edit Scale"));
      scalePropertyOp.setValue("rootKey", rootKey);
      scalePropertyOp.setValue("scaleType", scaleType);
      root.undoStack.endMacro();
    }
  }
}
