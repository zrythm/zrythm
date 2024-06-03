#/usr/bin/env python3
# SPDX-FileCopyrightText: © 2022-2024 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

import os
import sys

def file_to_string(path):
    with open(path, 'r') as f:
        return f.read()

def get_list_for_changelog_group(changelog_nfo, title):
    last_title = ""
    result = []
    for line in changelog_nfo.split("\n"):
        if line.startswith("###"):
            last_title = line[4:]
        elif last_title == title and line.strip():
            result.append(f"<li>{line[2:]}</li>")
    return result

def get_releases():
    changelog = file_to_string(os.path.join("@MESON_SOURCE_ROOT@", "CHANGELOG.md"))
    changelog_list = changelog.split("## [")[1:]
    releases_list = []
    for release_info in changelog_list:
        lines = release_info.split("\n")
        ver = lines[0].split("]")[0]
        date_str = lines[0].split("] - ")[1].strip()
        changelog_nfo = "\n".join(lines[1:])
        description = []
        for line in changelog_nfo.split("\n"):
            if line.startswith("###"):
                title = line[4:]
                description.append(f"<p>{title}:</p>")
                description.append("<ul>")
                description.extend(get_list_for_changelog_group(changelog_nfo, title))
                description.append("</ul>")
        releases_list.append(
            f"""<release date="{date_str}" version="{ver}" type="development">
                <url>@RELEASE_TAG_BASE_URL@/v{ver}</url>
                <description>
                    {"".join(description)}
                </description>
            </release>"""
        )
    return "\n".join(releases_list[:4])

def main(output_file, app_id):
    with open(output_file, "w") as f:
        f.write(f"""<?xml version="1.0" encoding="UTF-8"?>
<!-- Copyright 2022-2024 Alexandros Theodotou -->
<component type="desktop-application">
    <id>org.zrythm.Zrythm</id>
    <metadata_license>CC0-1.0</metadata_license>
    <project_license>AGPL-3.0-or-later</project_license>
    <name>Zrythm</name>
    <developer_name>The Zrythm project</developer_name>
    <summary>Digital audio workstation</summary>
    <description>
        <p>Zrythm is a digital audio workstation tailored for both professionals and beginners, offering an intuitive interface and robust functionality.</p>
        <p>Key features include:</p>
        <ul>
            <li>Streamlined editing workflows</li>
            <li>Flexible tools for creative expression</li>
            <li>Limitless automation capabilities</li>
            <li>Powerful mixing features</li>
            <li>Chord assistance for musical composition</li>
            <li>Support for various plugin and file formats</li>
        </ul>
    </description>
    <categories>
        <category>AudioVideo</category>
        <category>Audio</category>
    </categories>
    <url type="homepage">@HOMEPAGE_URL@</url>
    <url type="bugtracker">@BUG_REPORT_URL@</url>
    <url type="faq">@FAQ_URL@</url>
    <url type="help">@CHATROOM_URL@</url>
    <url type="donation">@DONATION_URL@</url>
    <url type="translate">@TRANSLATE_URL@</url>
    <url type="contact">@CONTACT_URL@</url>
    <url type="vcs-browser">@MAIN_REPO_URL@</url>
    <launchable type="desktop-id">{app_id}.desktop</launchable>
    <screenshots>
        <screenshot type="default">
            <image type="source">@MAIN_SCREENSHOT_URL@</image>
            <caption>Main window with plugins</caption>
        </screenshot>
    </screenshots>
    <update_contact>alex_at_zrythm.org</update_contact>
    <keywords>
        <keyword>Zrythm</keyword>
        <keyword>DAW</keyword>
    </keywords>
    <kudos>
        <kudo>HiDpiIcon</kudo>
        <kudo>ModernToolkit</kudo>
    </kudos>
    <project_group>Zrythm</project_group>
    <translation type="gettext">zrythm</translation>
    <provides>
        <binary>zrythm_launch</binary>
    </provides>
    <branding>
        <color type="primary" scheme_preference="light">#FF8B73</color>
        <color type="primary" scheme_preference="dark">#B12408</color>
    </branding>
    <releases>
        {get_releases()}
    </releases>
    <content_rating type="oars-1.1" />
</component>
""")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Need 2 arguments")
        sys.exit(-1)
    main(sys.argv[1], sys.argv[2])
