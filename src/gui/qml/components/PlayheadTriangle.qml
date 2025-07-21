// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Shapes

Shape {
  id: control

  layer.enabled: true
  layer.samples: 8

  ShapePath {
    fillColor: control.palette.text
    strokeColor: control.palette.text

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
