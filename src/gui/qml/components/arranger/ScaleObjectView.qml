// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import Zrythm
import ZrythmStyle

ArrangerObjectBaseView {
  id: root

  colorRole: ArrangerObjectBaseView.ColorRole.Label

  readonly property real ribbonGap: 4
  readonly property real ribbonInset: 2
  readonly property real ribbonWidth: 4
  property string scaleName: ""
  readonly property ScaleObject scaleObject: arrangerObject as ScaleObject

  height: textMetrics.height + 2 * ZrythmTheme.buttonPadding
  // Ribbon plus a gap, then the name
  width: root.ribbonInset + root.ribbonWidth + root.ribbonGap
    + textMetrics.width + ZrythmTheme.buttonPadding

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
    color: Qt.alpha (root.objectColor, ZrythmTheme.bodyAlpha)
    border.color: root.objectBorderColor
    border.width: 1
    radius: ZrythmTheme.toolButtonRadius
  }

  // Ink ribbon at the span start: the small saturated anchor that
  // makes the quiet span findable (DESIGN.md "Non-clip arranger
  // objects"). Inset so its square corners stay inside the area the
  // body's rounded corners cover
  Rectangle {
    anchors.left: parent.left
    anchors.leftMargin: root.ribbonInset
    anchors.top: parent.top
    anchors.topMargin: root.ribbonInset
    anchors.bottom: parent.bottom
    anchors.bottomMargin: root.ribbonInset
    width: root.ribbonWidth
    topLeftRadius: root.ribbonInset
    bottomLeftRadius: root.ribbonInset
    color: root.objectAccentColor
  }

  Text {
    id: nameText

    anchors.left: parent.left
    anchors.leftMargin: root.ribbonInset + root.ribbonWidth + root.ribbonGap
    anchors.verticalCenter: parent.verticalCenter
    color: root.palette.text
    font: root.font
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
