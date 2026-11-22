// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Shapes

Shape {
  id: control

  property color color: control.palette.text

  layer.enabled: true
  layer.samples: 8

  ShapePath {
    fillColor: control.color
    strokeColor: control.color

    PathLine {
      x: 0
      y: 0
    }

    PathLine {
      x: control.width
      y: 0
    }

    PathLine {
      x: control.width / 2
      y: control.height
    }
  }
}
