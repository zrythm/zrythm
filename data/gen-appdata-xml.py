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
    changelog = file_to_string(os.path.join(r"@CMAKE_SOURCE_DIR@", "CHANGELOG.md"))
    changelog_list = changelog.split("## [")[1:]
    releases_list = []
    for release_info in changelog_list:
        lines = release_info.split("\n")
        ver = lines[0].split("]")[0]
        # Skip development and pre-v1 releases
        if ('-' in ver) or ver.startswith("0."):
            continue
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
            f"""<release date="{date_str}" version="{ver}">
                <url>@release_tag_base_url@/v{ver}</url>
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
    <url type="homepage">@homepage_url@</url>
    <url type="bugtracker">@issue_tracker_url@</url>
    <url type="faq">@faq_url@</url>
    <url type="help">@chatroom_url@</url>
    <url type="donation">@donation_url@</url>
    <url type="translate">@translate_url@</url>
    <url type="contact">@contact_url@</url>
    <url type="vcs-browser">@main_repo_url@</url>
    <launchable type="desktop-id">{app_id}.desktop</launchable>
    <screenshots>
        <screenshot type="default">
            <image type="source">@main_screenshot_url@</image>
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
