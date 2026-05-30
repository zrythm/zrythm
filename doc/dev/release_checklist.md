<!---
SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
SPDX-License-Identifier: FSFAP
-->

# Release Checklist

## Pre-release

1. Ensure all intended features/fixes are merged to `master`
2. Update `CHANGELOG.md` with a new version section:
   ```
   ## [v2.0.0-alpha.1] - 2026-05-30
   ```
   Use `Added:`, `Changed:`, `Fixed:`, `Removed:` subsections.
3. Verify the changelog section is accurate and complete

## Running the Release

Run the release script:

```bash
bash scripts/release.sh 2.0.0-alpha.1
```

Or do a dry run first:

```bash
bash scripts/release.sh --dry-run 2.0.0-alpha.1
```

The script will:
- Validate the version format and changelog entry
- Update `VERSION.txt`
- Commit `VERSION.txt` and `CHANGELOG.md` with message `Release v2.0.0-alpha.1`
- Create a GPG-signed annotated tag with the changelog as the tag message
- Generate source tarball: `zrythm-2.0.0-alpha.1.tar.xz`
- GPG-sign the tarball: `zrythm-2.0.0-alpha.1.tar.xz.asc`
- Create SHA-256 checksum: `zrythm-2.0.0-alpha.1.tar.xz.sha256sum`

If anything looks wrong, undo with:

```bash
git tag -d v2.0.0-alpha.1
git reset --hard HEAD~1
```

## Publishing

1. Push the commit and tag:

   ```bash
   git push origin master
   git push origin v2.0.0-alpha.1
   ```

2. Upload source tarballs to the website:

   ```bash
   scp zrythm-*.tar.xz zrythm-*.tar.xz.asc zrythm-*.tar.xz.sha256sum <server>:<path>
   ```

3. CI will automatically build, test, and deploy binary installers

## Post-release

1. Verify CI pipeline passes on the tag
2. Verify binary installers are available on the download site
3. Verify source tarballs are available on the website
4. Announce the release (Mastodon, forum, etc.)

## Version Scheme

Tags follow semantic versioning with `v` prefix:

- `v2.0.0-alpha.1` — alpha release
- `v2.0.0-beta.1` — beta release
- `v2.0.0-rc.1` — release candidate
- `v2.0.0` — stable release

## Branches

- Releases are tagged on `master`
- CI runs on `master` (nightly builds) and on tags (releases)
