// SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
#version 130

smooth in vec4 vertexColor;

out vec4 outputColor;

void main() {
  outputColor = vertexColor;
}
