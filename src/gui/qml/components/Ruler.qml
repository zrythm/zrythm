// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Shapes
import ZrythmStyle
import ZrythmDsp
import ZrythmGui

Control {
  id: control

  readonly property real barLineOpacity: 0.8
  readonly property real beatLineOpacity: 0.6
  readonly property real defaultPxPerTick: 0.03
  readonly property real detailMeasureLabelPxThreshold: 64 // threshold to show/hide labels for more detailed measures
  readonly property real detailMeasurePxThreshold: 32 // threshold to show/hide more detailed measures
  required property EditorSettings editorSettings
  readonly property int endBar: tempoMap.getMusicalPosition(visibleEndTick).bar + 1 // +1 for padding
  readonly property int markerSize: 8
  readonly property int maxBars: 256
  readonly property real maxZoomLevel: 1800
  readonly property real minZoomLevel: 0.04
  readonly property real pxPerBar: pxPerSixteenth * 16
  readonly property real pxPerBeat: pxPerSixteenth * 4
  readonly property real pxPerSixteenth: ticksPerSixteenth * pxPerTick
  readonly property real pxPerTick: defaultPxPerTick * editorSettings.horizontalZoomLevel
  readonly property int rulerHeight: 24
  readonly property real sixteenthLineOpacity: 0.4
  readonly property int startBar: tempoMap.getMusicalPosition(visibleStartTick).bar
  required property TempoMap tempoMap
  readonly property real ticksPerSixteenth: tempoMap.getPpq() / 4
  required property Transport transport
  readonly property int visibleBarCount: endBar - startBar + 1
  readonly property real visibleEndTick: visibleStartTick + (parent.width / pxPerTick)
  readonly property real visibleStartTick: editorSettings.x / pxPerTick

  clip: true
  height: rulerHeight
  width: maxBars * pxPerBar

  // Grid lines and time markers
  Item {
    id: timeGrid

    height: parent.height

    // Generate bars, beats, sixteenths based on zoom level
    Repeater {
      model: control.visibleBarCount

      delegate: Item {
        id: barItem

        readonly property int bar: control.startBar + index
        required property int index

        Rectangle {
          readonly property int barTick: control.tempoMap.getTickFromMusicalPosition(barItem.bar, 1, 1, 0)

          color: control.palette.text
          height: 14 // parent.height / 3
          opacity: control.barLineOpacity
          width: 2
          x: barTick * control.pxPerTick

          Text {
            anchors.bottom: parent.bottom
            anchors.left: parent.right
            anchors.leftMargin: 2
            color: control.palette.text
            font.family: Style.smallTextFont.family
            font.pixelSize: Style.smallTextFont.pixelSize
            font.weight: Font.Medium
            text: barItem.bar
          }
        }

        Loader {
          active: control.pxPerBeat > control.detailMeasurePxThreshold
          visible: active

          sourceComponent: Repeater {
            model: control.tempoMap.timeSignatureAtTick(control.tempoMap.getTickFromMusicalPosition(barItem.bar, 1, 1, 0)).numerator

            delegate: Item {
              id: beatItem

              readonly property int beat: index + 1
              readonly property int beatTick: control.tempoMap.getTickFromMusicalPosition(barItem.bar, beat, 1, 0)
              required property int index

              Rectangle {
                color: control.palette.text
                height: 10
                opacity: control.beatLineOpacity
                visible: beatItem.beat !== 1
                width: 1
                x: beatItem.beatTick * control.pxPerTick

                Text {
                  anchors.left: parent.right
                  anchors.leftMargin: 2
                  anchors.top: parent.top
                  color: control.palette.text
                  font: Style.xSmallTextFont
                  text: `${barItem.bar}.${beatItem.beat}`
                  visible: control.pxPerBeat > control.detailMeasureLabelPxThreshold
                }
              }

              Loader {
                active: control.pxPerSixteenth > control.detailMeasurePxThreshold
                visible: active

                sourceComponent: Repeater {
                  model: 16 / control.tempoMap.timeSignatureAtTick(control.tempoMap.getTickFromMusicalPosition(barItem.bar, beatItem.beat, 1, 0)).denominator

                  Rectangle {
                    id: sixteenthRect

                    required property int index
                    readonly property int sixteenth: index + 1
                    readonly property int sixteenthTick: control.tempoMap.getTickFromMusicalPosition(barItem.bar, beatItem.beat, sixteenth, 0)

                    color: control.palette.text
                    height: 8
                    opacity: control.sixteenthLineOpacity
                    visible: sixteenth !== 1
                    width: 1
                    x: sixteenthTick * control.pxPerTick

                    Text {
                      anchors.left: parent.right
                      anchors.leftMargin: 2
                      anchors.top: parent.top
                      color: control.palette.text
                      font: Style.xxSmallTextFont
                      text: `${barItem.bar}.${beatItem.beat}.
${sixteenthRect.sixteenth}`
                      visible: control.pxPerSixteenth > control.detailMeasureLabelPxThreshold
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  Item {
    id: markers

    height: parent.height

    PlayheadTriangle {
      id: playheadShape

      height: 8
      width: 12
      x: control.transport.playhead.ticks * control.pxPerTick - width / 2
      y: control.height - height
    }

    Item {
      id: loopRange

      readonly property real endX: control.transport.loopEndPosition.ticks * defaultPxPerTick * editorSettings.horizontalZoomLevel
      readonly property real loopMarkerHeight: 8
      readonly property real loopMarkerWidth: 10
      readonly property real startX: control.transport.loopStartPosition.ticks * defaultPxPerTick * editorSettings.horizontalZoomLevel

      Shape {
        id: loopStartShape

        antialiasing: true
        height: loopRange.loopMarkerHeight
        layer.enabled: true
        layer.samples: 4
        visible: control.transport.loopEnabled
        width: loopRange.loopMarkerWidth
        x: loopRange.startX
        z: 10

        ShapePath {
          fillColor: control.palette.accent
          strokeColor: control.palette.accent

          PathLine {
            x: 0
            y: 0
          }

          PathLine {
            x: loopStartShape.width
            y: 0
          }

          PathLine {
            x: 0
            y: loopStartShape.height
          }
        }
      }

      Shape {
        id: loopEndShape

        antialiasing: true
        height: loopRange.loopMarkerHeight
        layer.enabled: true
        layer.samples: 4
        visible: control.transport.loopEnabled
        width: loopRange.loopMarkerWidth
        x: loopRange.endX - loopRange.loopMarkerWidth
        z: 10

        ShapePath {
          fillColor: control.palette.accent
          strokeColor: control.palette.accent

          PathLine {
            x: 0
            y: 0
          }

          PathLine {
            x: loopEndShape.width
            y: 0
          }

          PathLine {
            x: loopEndShape.width
            y: loopEndShape.height
          }
        }
      }

      Rectangle {
        id: loopRangeRect

        color: Qt.alpha(control.palette.accent, 0.1)
        height: control.height
        visible: control.transport.loopEnabled
        width: loopRange.endX - loopRange.startX
        x: loopRange.startX
      }
    }
  }

  MouseArea {
    property bool dragging: false

    acceptedButtons: Qt.LeftButton | Qt.RightButton
    anchors.fill: parent

    onPositionChanged: mouse => {
      if (dragging)
        control.transport.playhead.ticks = mouse.x / (control.defaultPxPerTick * control.editorSettings.horizontalZoomLevel);
    }
    onPressed: mouse => {
      if (mouse.button === Qt.LeftButton) {
        dragging = true;
        control.transport.playhead.ticks = mouse.x / (control.defaultPxPerTick * editorSettings.horizontalZoomLevel);
      }
    }
    onReleased: {
      dragging = false;
    }
    onWheel: wheel => {
      if (wheel.modifiers & Qt.ControlModifier) {
        const multiplier = wheel.angleDelta.y > 0 ? 1.3 : 1 / 1.3;
        control.editorSettings.horizontalZoomLevel = Math.min(Math.max(control.editorSettings.horizontalZoomLevel * multiplier, control.minZoomLevel), control.maxZoomLevel);
      }
    }
  }
}
