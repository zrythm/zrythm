<!---
SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
SPDX-License-Identifier: FSFAP
-->

Commit Messages
===============

Zrythm's commit messages follow the 72-wrap tradition used by the Linux
kernel and the Git project, kept readable in plain text, in `git log`, and
in email-based review.

Subject line
------------

* Keep the subject within **72 characters**.
* Use the **imperative mood** ("Add", "Fix", "Remove"), as if giving the
  codebase a command — not "Adds" or "Added".
* Format: `<ClassName>: <imperative-summary>` (for example `Tracklist:`,
  `TempoMap:`, `MoveTracksCommand:`). If no single class is central to the
  change, or too many classes are involved, use a general term for what
  changed (for example `cmake:`, `tracks:`, `i18n:`).
* Do not end the subject with a period.

Body
----

* The body is optional, but recommended for anything non-trivial.
* Wrap the body at **72 characters**.
* Use bullet points for significant details. Both the summary and the
  bullet points use the imperative tone.
* Bullets describe **what changed at the level of the named classes,
  modules, or abstractions involved and their observable behaviour**.
  Name the key things a change touches (for example "Add `FooEngine`",
  "Remove the legacy `Bar`") so the commit is scannable and locatable.
  Omit low-level mechanics that are obvious from the diff (specific C++
  patterns, smart-pointer types, refactoring steps).
* Use backticks when referencing code identifiers in the body.
* One logical change per commit.

Trailers
--------

Every commit message ends with one or more trailers (git "trailers", added
with `git commit --trailer "..."` or written by hand):

* `Signed-off-by: Name <email>` — **required on every commit**, certifies
  the [Developer Certificate of Origin](https://developercertificate.org/).
  Added automatically by `git commit -s`. You must use your real name; see
  [CONTRIBUTING.md](../../CONTRIBUTING.md) for the DCO and
  [DCO.txt](../../DCO.txt) for the full text. Only humans certify the DCO —
  see [AI_POLICY.md](AI_POLICY.md).
* `Assisted-by: <model>` — **required on AI-assisted commits**, naming the
  model used (for example `Assisted-by: GLM-5.2`). See
  [AI_POLICY.md](AI_POLICY.md) for the AI-specific rules.
* `Fixes #123` — a bugfix that closes issue #123.
* `Implements #123` — a feature that implements issue #123.
* `GitLab-Work-Item: #123` — relates the commit to a GitLab work item.

Examples
--------

See `git log` for many examples. A typical message looks like:

    TempoMap: anchor base tempo and time signature at tick 0

    - Replace the implicit default tempo/time-signature model with
      explicit `base_bpm_` and `base_time_sig_` members that govern the
      region from tick 0 up to the first inserted event
    - Expose `baseBpm`, `baseTimeSignatureNumerator` and
      `baseTimeSignatureDenominator` as QML properties on `TempoMapWrapper`
    - Add a BPM edit cell in `TransportControls`

    Fixes #1234
    Signed-off-by: Alexandros Theodotou <alex@zrythm.org>
    Assisted-by: GLM-5.2
