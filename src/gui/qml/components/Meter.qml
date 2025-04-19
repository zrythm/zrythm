// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import Zrythm 1.0
import ZrythmStyle 1.0

Rectangle {
    id: root

    required property var port
    readonly property real currentPx: meterProcessor.toFader(meterProcessor.currentAmplitude) * root.height
    readonly property real peakPx: meterProcessor.toFader(meterProcessor.peakAmplitude) * root.height

    width: 4
    color: "transparent"
    clip: true
    layer.enabled: true
    layer.smooth: true

    MeterProcessor {
        id: meterProcessor

        port: root.port
    }

    Rectangle {
        id: filledRect

        height: root.currentPx

        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }

        gradient: Gradient {
            GradientStop {
                position: 0
                color: "#FF9C11"
            }

            GradientStop {
                position: 0.25
                color: "#F7FF11"
            }

            GradientStop {
                position: 1
                color: "#2BD700"
            }

        }

        Behavior on height {
            animation: Style.propertyAnimation
        }

    }

    Rectangle {
        id: peakRect

        readonly property bool isOver: meterProcessor.peakAmplitude > 1

        height: isOver ? 2 : 1.5
        y: root.height - root.peakPx
        color: isOver ? Style.dangerColor : Style.backgroundAppendColor
        opacity: isOver ? 1 : meterProcessor.peakAmplitude

        anchors {
            left: parent.left
            right: parent.right
        }

        Behavior on y {
            animation: Style.propertyAnimation
        }

    }

}
