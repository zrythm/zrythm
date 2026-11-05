// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import ZrythmStyle
import Zrythm

Rectangle {
  id: root

  property int algorithm
  required property AudioEngine audioEngine
  property int channel
  readonly property real currentPx: meterProcessorItem.toFader(meterProcessorItem.currentAmplitude) * root.height
  readonly property alias meterProcessor: meterProcessorItem
  readonly property real peakPx: meterProcessorItem.toFader(meterProcessorItem.peakAmplitude) * root.height
  required property Port port
  required property PortObservationManager portObservationManager

  clip: true
  color: "transparent"
  layer.enabled: true
  layer.smooth: true
  width: 4

  MeterProcessor {
    id: meterProcessorItem

    algorithm: root.algorithm
    channel: root.channel
    port: root.port
    portObservationManager: root.portObservationManager
    sampleRate: root.audioEngine.sampleRate
  }

  Rectangle {
    id: filledRect

    height: root.currentPx

    gradient: Gradient {
      GradientStop {
        color: "#FF9C11"
        position: 0
      }

      GradientStop {
        color: "#F7FF11"
        position: 0.25
      }

      GradientStop {
        color: "#2BD700"
        position: 1
      }
    }
    Behavior on height {
      animation: ZrythmTheme.propertyAnimation
    }

    anchors {
      bottom: parent.bottom
      left: parent.left
      right: parent.right
    }
  }

  Rectangle {
    id: peakRect

    readonly property bool isOver: meterProcessorItem.peakAmplitude > 1

    color: isOver ? ZrythmTheme.errorColor : ZrythmTheme.backgroundAppendColor
    height: isOver ? 2 : 1.5
    opacity: isOver ? 1 : meterProcessorItem.peakAmplitude
    y: root.height - root.peakPx

    Behavior on y {
      animation: ZrythmTheme.propertyAnimation
    }

    anchors {
      left: parent.left
      right: parent.right
    }
  }
}
