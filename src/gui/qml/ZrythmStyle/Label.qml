// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

import QtQuick
import QtQuick.Templates as T
import ZrythmStyle 1.0

T.Label {
  id: control

  color: control.palette.windowText
  font: ZrythmTheme.normalTextFont
  linkColor: control.palette.link
}
