// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

Control {
    id: control

    property alias value: valueDisplay.text
    property string label
    property real minValueWidth: 0
    property real minValueHeight: 0

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)
    padding: 2
    opacity: Style.getOpacity(control.enabled, control.Window.active)

    contentItem: RowLayout {
        id: rowLayout

        spacing: 3
        implicitWidth: valueDisplay.implicitWidth + textDisplay.implicitWidth + spacing
        implicitHeight: Math.max(valueDisplay.implicitHeight, textDisplay.implicitHeight)

        Text {
            id: valueDisplay

            color: control.palette.text
            font: Style.semiBoldTextFont
            Layout.alignment: Qt.AlignBaseline
            Layout.minimumWidth: control.minValueWidth
            Layout.minimumHeight: control.minValueHeight
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignCenter
            textFormat: Text.PlainText

            Behavior on text {
                animation: Style.propertyAnimation
            }

        }

        Text {
            id: textDisplay

            text: control.label
            color: control.palette.placeholderText
            font: Style.fadedTextFont
            Layout.alignment: Qt.AlignBaseline
        }

    }

}
