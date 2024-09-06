#!/usr/bin/env python3

import os
import sys
import xml.etree.ElementTree as ET
from typing import List

def remove_prefix(text: str, prefix: str) -> str:
    return text[len(prefix):] if text.startswith(prefix) else text

def join_path(parts: List[str]) -> str:
    return os.path.join(*parts)

def scandir(directory: str, condition: callable) -> List[str]:
    return [f for f in os.listdir(directory) if condition(f)]

def main(args: List[str]):
    if len(args) != 5:
        print("Need 4 arguments")
        sys.exit(-1)

    _, resources_dir, resources_build_dir, appdata_file, output_file = args

    with open(output_file, 'w') as f:
        f.write('''<!--
  SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
  SPDX-License-Identifier: LicenseRef-ZrythmLicense
-->
<?xml version='1.0' encoding='UTF-8'?>
<gresources>
  <gresource prefix='/org/gtk/libgtk'>
  </gresource>
  <gresource prefix='/org/zrythm/Zrythm/app'>
''')

        # Print GL shaders
        gl_shaders_dir = "gl/shaders"
        for x in scandir(join_path([resources_dir, gl_shaders_dir]), lambda f: f.endswith('.glsl')):
            f.write(f"    <file>{gl_shaders_dir}/{x}</file>\n")

        # Print UI files
        for x in scandir(join_path([resources_build_dir, "ui"]), lambda f: f.endswith('.ui')):
            f.write(f"    <file preprocess=\"xml-stripblanks\">ui/{x}</file>\n")

        # Add icons
        icon_dirs = ['arena', 'gnome-builder', 'gnome-icon-library', 'fork-awesome', 'font-awesome',
                     'fluentui', 'jam-icons', 'box-icons', 'iconpark', 'iconoir', 'material-design',
                     'untitled-ui', 'css.gg', 'codicons']
        
        for dir in icon_dirs:
            for icon_file in scandir(join_path([resources_dir, "icons", dir]), lambda f: f.endswith(('.svg', '.png'))):
                f.write(f"    <file alias=\"icons/{dir}/{dir}-{icon_file}\">icons/{dir}/{icon_file}</file>\n")

        # Insert standard gtk menus
        f.write(f'''  </gresource>
  <gresource prefix='/org/zrythm/Zrythm'>
    <file preprocess='xml-stripblanks'>gtk/menus.ui</file>
    <file preprocess='xml-stripblanks'>gtk/help-overlay.ui</file>
    <file preprocess='xml-stripblanks'>{os.path.basename(appdata_file)}</file>
  </gresource>
''')

        f.write("</gresources>")

if __name__ == "__main__":
    main(sys.argv)
