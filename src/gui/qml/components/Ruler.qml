// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtQuick.Shapes
import Zrythm 1.0
import ZrythmStyle 1.0

Item {
    id: control

    required property var editorSettings
    required property var transport
    required property var tempoMap
    readonly property int rulerHeight: 24
    readonly property int markerSize: 8
    readonly property int playheadTriangleWidth: 12
    readonly property int playheadTriangleHeight: 8
    readonly property real minZoomLevel: 0.04
    readonly property real maxZoomLevel: 1800
    readonly property real defaultPxPerTick: 0.03
    readonly property real pxPerTick: defaultPxPerTick * editorSettings.horizontalZoomLevel
    readonly property int maxBars: 256
    readonly property real ticksPerSixteenth: tempoMap.getPpq() / 4
    readonly property real pxPerSixteenth: ticksPerSixteenth * pxPerTick
    readonly property real pxPerBar: pxPerSixteenth * 16
    readonly property real pxPerBeat: pxPerSixteenth * 4
    readonly property real detailMeasurePxThreshold: 32 // threshold to show/hide more detailed measures
    readonly property real detailMeasureLabelPxThreshold: 64 // threshold to show/hide labels for more detailed measures
    readonly property real barLineOpacity: 0.8
    readonly property real beatLineOpacity: 0.6
    readonly property real sixteenthLineOpacity: 0.4
    readonly property real visibleStartTick: editorSettings.x / pxPerTick
    readonly property real visibleEndTick: visibleStartTick + (parent.width / pxPerTick)
    readonly property int startBar: tempoMap.getMusicalPosition(visibleStartTick).bar
    readonly property int endBar: tempoMap.getMusicalPosition(visibleEndTick).bar + 1 // +1 for padding
    readonly property int visibleBarCount: endBar - startBar + 1

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

                required property int index
                readonly property int bar: startBar + index

                Rectangle {
                    readonly property int barTick: tempoMap.getTickFromMusicalPosition(barItem.bar, 1, 1, 0)

                    width: 2
                    height: 14 // parent.height / 3
                    color: control.palette.text
                    opacity: control.barLineOpacity
                    x: barTick * control.pxPerTick

                    Text {
                        text: barItem.bar
                        color: control.palette.text
                        anchors.left: parent.right
                        anchors.bottom: parent.bottom
                        anchors.leftMargin: 2
                        font.family: Style.smallTextFont.family
                        font.pixelSize: Style.smallTextFont.pixelSize
                        font.weight: Font.Medium
                    }

                }

                Loader {
                    active: control.pxPerBeat > control.detailMeasurePxThreshold
                    visible: active

                    sourceComponent: Repeater {
                        model: tempoMap.timeSignatureAtTick(tempoMap.getTickFromMusicalPosition(barItem.bar, 1, 1, 0)).numerator

                        delegate: Item {
                            id: beatItem

                            required property int index
                            readonly property int beat: index + 1
                            readonly property int beatTick: tempoMap.getTickFromMusicalPosition(barItem.bar, beat, 1, 0)

                            Rectangle {
                                width: 1
                                height: 10
                                color: control.palette.text
                                opacity: control.beatLineOpacity
                                x: beatItem.beatTick * control.pxPerTick
                                visible: beatItem.beat !== 1

                                Text {
                                    text: `${barItem.bar}.${beatItem.beat}`
                                    color: control.palette.text
                                    anchors.left: parent.right
                                    anchors.top: parent.top
                                    anchors.leftMargin: 2
                                    font: Style.xSmallTextFont
                                    visible: control.pxPerBeat > control.detailMeasureLabelPxThreshold
                                }

                            }

                            Loader {
                                active: control.pxPerSixteenth > control.detailMeasurePxThreshold
                                visible: active

                                sourceComponent: Repeater {
                                    model: 16 / tempoMap.timeSignatureAtTick(tempoMap.getTickFromMusicalPosition(barItem.bar, beatItem.beat, 1, 0)).denominator

                                    Rectangle {
                                        id: sixteenthRect

                                        required property int index
                                        readonly property int sixteenth: index + 1
                                        readonly property int sixteenthTick: tempoMap.getTickFromMusicalPosition(barItem.bar, beatItem.beat, sixteenth, 0)

                                        width: 1
                                        height: 8
                                        color: control.palette.text
                                        opacity: control.sixteenthLineOpacity
                                        x: sixteenthTick * control.pxPerTick
                                        visible: sixteenth !== 1

                                        Text {
                                            text: `${barItem.bar}.${beatItem.beat}.${sixteenthRect.sixteenth}`
                                            color: control.palette.text
                                            anchors.left: parent.right
                                            anchors.top: parent.top
                                            anchors.leftMargin: 2
                                            font: Style.xxSmallTextFont
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

        Shape {
            id: playheadShape

            width: control.playheadTriangleWidth
            height: control.playheadTriangleHeight
            x: transport.playhead.ticks * control.pxPerTick - width / 2
            y: control.height - height
            layer.enabled: true
            layer.samples: 4

            ShapePath {
                fillColor: control.palette.text
                strokeColor: control.palette.text

                PathLine {
                    x: 0
                    y: 0
                }

                PathLine {
                    x: playheadShape.width
                    y: 0
                }

                PathLine {
                    x: playheadShape.width / 2
                    y: playheadShape.height
                }

            }

        }

        Item {
            id: loopRange

            readonly property real startX: transport.loopStartPosition.ticks * defaultPxPerTick * editorSettings.horizontalZoomLevel
            readonly property real endX: transport.loopEndPosition.ticks * defaultPxPerTick * editorSettings.horizontalZoomLevel
            readonly property real loopMarkerWidth: 10
            readonly property real loopMarkerHeight: 8

            Shape {
                id: loopStartShape

                antialiasing: true
                visible: transport.loopEnabled
                x: loopRange.startX
                width: loopRange.loopMarkerWidth
                height: loopRange.loopMarkerHeight
                z: 10
                layer.enabled: true
                layer.samples: 4

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
                width: loopRange.loopMarkerWidth
                height: loopRange.loopMarkerHeight
                visible: transport.loopEnabled
                x: loopRange.endX - loopRange.loopMarkerWidth
                z: 10
                layer.enabled: true
                layer.samples: 4

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
                visible: transport.loopEnabled
                x: loopRange.startX
                width: loopRange.endX - loopRange.startX
                height: control.height
            }

        }

    }

    MouseArea {
        property bool dragging: false

        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onPressed: (mouse) => {
            if (mouse.button === Qt.LeftButton) {
                dragging = true;
                transport.playhead.ticks = mouse.x / (defaultPxPerTick * editorSettings.horizontalZoomLevel);
            }
        }
        onReleased: {
            dragging = false;
        }
        onPositionChanged: (mouse) => {
            if (dragging)
                transport.playhead.ticks = mouse.x / (defaultPxPerTick * editorSettings.horizontalZoomLevel);

        }
        onWheel: (wheel) => {
            if (wheel.modifiers & Qt.ControlModifier) {
                const multiplier = wheel.angleDelta.y > 0 ? 1.3 : 1 / 1.3;
                editorSettings.horizontalZoomLevel = Math.min(Math.max(editorSettings.horizontalZoomLevel * multiplier, minZoomLevel), maxZoomLevel);
            }
        }
    }

}
