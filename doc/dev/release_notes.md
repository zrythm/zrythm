<!---
SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
SPDX-License-Identifier: FSFAP
-->

# Release Notes Guidelines

How to write the two release documents: the `CHANGELOG.md` section
for a release and the announcement post
(`announcement-v<version>.md`).

`CHANGELOG.md` is the curated, complete record of user-visible
changes in a release. The announcement is a shorter, friendlier
version highlighting what matters most; every claim in it comes
from the changelog. See [release_checklist.md](release_checklist.md)
for the overall release process.

## Audience

Both documents are read by users first: assume the reader uses the
application but has never seen the source code, the issue tracker
or the design system. Contributors, packagers and plugin developers
read them too, so keep the system behind each change visible — in
words a user can follow.

## Writing rules

### Use the words users see

Name things as the interface names them: button, dialog, menu item,
track, lane, clip. Replace design vocabulary (chip, pill, token),
implementation vocabulary (command, replay, trim) and specification
vocabulary with plain words wherever one exists.

| ✗ | ✓ |
|---|---|
| Undo history uses a coalescing strategy for batch mutations | Consecutive edits collapse into a single undo step |

### Only describe changes to things that existed in the previous release

"X now does Y" and "X turns/gets Y" require X to have existed in
the previous release. When X is new, describe what it does, not how
it differs.

| ✗ (the popover is new in this release) | ✓ |
|---|---|
| The metronome popover now opens from the transport bar | The transport bar has a metronome popover with volume and count-in settings |

### State exactly what the user sees

Verify each claim against the shipped behavior and name the precise
thing that changes.

| ✗ (it only scrolls during playback) | ✓ |
|---|---|
| The piano roll follows the playhead | The piano roll scrolls to keep the playhead visible during playback |

### Lead with the outcome, keep the system

Open each entry with what the user notices, then describe the
system behind it in plain words. Real system names (lilv, the XDG
desktop portal, the CLAP thread-pool extension) are fine; invented
internal jargon is not. A user who stops after the first clause
still gets the point; a developer who reads to the end recognizes
what changed.

Changes with no user-visible effect stay out of the announcement,
and reach the changelog only when they affect packagers or
scriptability (e.g. a removed command line option).

| ✗ | ✓ |
|---|---|
| Saving serializes the project through an incremental writer that diffs against the last snapshot and writes changed chunks to a journal | Saving is faster: Zrythm now writes only the parts of the project that changed since the last save, instead of the whole project |

### One change per line

Each line covers one change. A change with several facets gets a
single line: headline first, then the facets — the native LV2
hosting entry in [v2.0.0-alpha.5](../../CHANGELOG.md) shows the
shape.

### Prefer the concrete

Name the actual device, menu or theme instead of a category.

| ✗ | ✓ |
|---|---|
| runs on all platforms | runs natively on GNU/Linux, Windows and macOS |
| fixed a crash under some conditions | fixed a crash when connecting an audio device while the project is playing |

## Announcement structure

Follow the previous announcement: title, one-sentence highlights,
screenshot, pre-release warning, grouped "What's new" sections,
notable fixes, downloads, build instructions, issue reporting,
thanks.

- Downloads links point at the release being announced
- Keep the alpha/beta warning while 2.0.0 is in pre-release

## Before publishing

- Read the section aloud: every sentence should survive the
  question "would a user get the point without asking what it
  means?"
- Re-check each claim against the feature as shipped, not against
  the commit message it came from
