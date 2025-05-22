<!---
SPDX-FileCopyrightText: © 2019-2023 Alexandros Theodotou <alex@zrythm.org>
SPDX-License-Identifier: FSFAP
-->

Versioning
==========

Zrythm follows the following
[Semantic Versioning (SemVer) 2.0.0](https://semver.org/)
scheme:

```yaml
---
API changes: project file/directory, user directory and settings changes
MAJOR: Manually incremented for breaking API changes
MINOR: Manually incremented for non-breaking API changes
PATCH: Automatically incremented based on number of commits since ${MAJOR}.${MINOR}.0

release tags:
  pre-alpha:
    description: Starts when work on a new major version starts
    identifier: ${unreleased major ver}.0.0-DEV.${MAJOR}.${MINOR}.{PATCH}
    examples: 2.0.0-DEV.0.1.0, 2.0.0-DEV.0.32.1
  alpha:
    description: Starts when a major version under development is somewhat usable
    identifier: ${unreleased major ver}.0.0-alpha.${MAJOR}.${MINOR}.{PATCH}
    examples: 2.0.0-alpha.0.1.0, 2.0.0-alpha.0.32.1
  beta:
    description: Starts when all features expected to break API are implemented
    identifier: ${unreleased major ver}.0.0-beta.${MAJOR}.${MINOR}.{PATCH}
    examples: 2.0.0-beta.0.1.0, 2.0.0-beta.0.32.1
  rc:
    description: Starts when all features are implemented and all known bugs are fixed
    identifier: ${unreleased major ver}.0.0-rc.${manually incremented for each release}
    examples: 2.0.0-rc.0, 2.0.0-rc.32
  v1, 2, 3, etc.:
    description: Starts when all features for the major ver are stable
    identifier: ${MAJOR}.${MINOR}.${PATCH}
    examples: 2.0.0, 2.32.1

nightlies:
  - ${release tag}+r${number of commits since last tag}.g${commit hash}
```
