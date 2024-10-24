// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

Control {
    id: control

    property string value
    property string label

    implicitWidth: Math.max(implicitBackgroundWidth + leftInset + rightInset, implicitContentWidth + leftPadding + rightPadding)
    implicitHeight: Math.max(implicitBackgroundHeight + topInset + bottomInset, implicitContentHeight + topPadding + bottomPadding)
    padding: 2

    contentItem: RowLayout {
        id: rowLayout

        spacing: 3
        implicitWidth: valueDisplay.implicitWidth + textDisplay.implicitWidth + spacing
        implicitHeight: Math.max(valueDisplay.implicitHeight, textDisplay.implicitHeight)

        Text {
            id: valueDisplay

            text: control.value
            color: control.palette.text
            font: Style.semiBoldTextFont
             Layout.alignment: Qt.AlignBaseline
        }

        Text {
            id: textDisplay

            text: control.label
            color: control.palette.placeholderText
            font: Style.fadedTextFont
            // anchors.baseline: valueDisplay.baseline
            Layout.alignment: Qt.AlignBaseline
        }

    }

}
