#!/usr/bin/env bash
# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense
#
# Release script for Zrythm.
# Usage: scripts/release.sh [--dry-run] <version>
# Example: scripts/release.sh 2.0.0-alpha.1
#          scripts/release.sh --dry-run 2.0.0-alpha.1
#
# Prerequisites:
#   - CHANGELOG.md must have a ## [v?<version>] section
#   - Working tree must be clean
#   - GPG key available for tag signing
#
# This script:
#   1. Validates the version format and changelog entry
#   2. Updates VERSION.txt
#   3. Commits the version bump
#   4. Creates a GPG-signed annotated tag with changelog excerpt
#   5. Generates source tarball with GPG signature and SHA-256 checksum
#
# The commit, tag, and tarballs are created locally. Push manually when ready.
# CI handles binary builds and installer deployment on tag push.

set -euo pipefail

DRY_RUN=0
if [ "${1:-}" = "--dry-run" ]; then
    DRY_RUN=1
    shift
fi
VERSION="${1:?Usage: $0 [--dry-run] <version> (e.g., 2.0.0-alpha.1)}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

# Strip leading 'v' if provided
VERSION="${VERSION#v}"
TAG="v${VERSION}"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

info()  { echo -e "${GREEN}[INFO]${NC} $*"; }
warn()  { echo -e "${YELLOW}[WARN]${NC} $*"; }
error() { echo -e "${RED}[ERROR]${NC} $*" >&2; exit 1; }

cd "${PROJECT_DIR}"

# ── Validate version format ──────────────────────────────────────────────────

if ! echo "${VERSION}" | grep -qE '^[0-9]+\.[0-9]+\.[0-9]+(-[a-zA-Z0-9.]+)?$'; then
    error "Invalid version format: '${VERSION}'. Expected semver (e.g., 2.0.0-alpha.1)"
fi

info "Preparing release ${TAG}$([ "${DRY_RUN}" -eq 1 ] && echo ' (dry run)')"

# ── Validate changelog entry ─────────────────────────────────────────────────

if ! grep -qE "## \[(${TAG}|${VERSION})\]" CHANGELOG.md; then
    error "CHANGELOG.md does not contain a section for [${TAG}] or [${VERSION}]. Add it first."
fi

# ── Validate working tree ────────────────────────────────────────────────────

if ! git diff --quiet VERSION.txt || ! git diff --cached --quiet VERSION.txt; then
    error "VERSION.txt has uncommitted changes. Commit or stash them first."
fi

# ── Check we're on master ────────────────────────────────────────────────────

BRANCH=$(git rev-parse --abbrev-ref HEAD)
if [ "${BRANCH}" != "master" ]; then
    if [ -t 0 ]; then
        warn "Not on master (on ${BRANCH}). Continue anyway? [y/N]"
        read -r reply
        [ "${reply}" = "y" ] || error "Aborted."
    else
        error "Not on master (on ${BRANCH}). Re-run on master."
    fi
fi

# ── Check tag doesn't already exist ──────────────────────────────────────────

if git tag -l "${TAG}" | grep -q .; then
    error "Tag ${TAG} already exists."
fi

# ── Update VERSION.txt ───────────────────────────────────────────────────────

if [ "${DRY_RUN}" -eq 1 ]; then
    info "[dry-run] Would update VERSION.txt to ${TAG}"
else
    info "Updating VERSION.txt to ${TAG}"
    echo "${TAG}" > VERSION.txt
fi

# ── Commit version bump ──────────────────────────────────────────────────────

if [ "${DRY_RUN}" -eq 1 ]; then
    info "[dry-run] Would commit VERSION.txt and CHANGELOG.md with message 'Release ${TAG}'"
else
    git add VERSION.txt CHANGELOG.md
    git commit -s -m "Release ${TAG}"
fi

# ── Extract changelog for tag message ────────────────────────────────────────

CHANGELOG=$(awk "/^## \[(v?${VERSION}|${TAG})\]/ {found=1; next} /^## \[/ {found=0} found" CHANGELOG.md | sed -e '/./,$!d' -e :a -e '/^\n*$/{$d;N;ba}' | sed 's/^### \(Added\|Changed\|Fixed\|Removed\)/\1:/')

if [ -z "${CHANGELOG}" ]; then
    warn "Changelog section for ${TAG} appears empty. Using generic tag message."
    TAG_MESSAGE="Release ${TAG}"
else
    TAG_MESSAGE="Release ${TAG}

${CHANGELOG}"
fi

# ── Create signed tag ────────────────────────────────────────────────────────

if [ "${DRY_RUN}" -eq 1 ]; then
    info "[dry-run] Would create signed tag ${TAG} with message:"
    echo "${TAG_MESSAGE}"
else
    info "Creating signed tag ${TAG}"
    git tag -s -a "${TAG}" -m "${TAG_MESSAGE}"
fi

# ── Generate source tarball with signatures ──────────────────────────────────

TARBALL_NAME="zrythm-${VERSION}"
TARBALL="${TARBALL_NAME}.tar.xz"
TARBALL_ASC="${TARBALL}.asc"
TARBALL_SHA256SUM="${TARBALL}.sha256sum"

if [ "${DRY_RUN}" -eq 1 ]; then
    info "[dry-run] Would generate ${TARBALL}, ${TARBALL_ASC}, ${TARBALL_SHA256SUM}"
else
    info "Generating source tarball ${TARBALL}"
    git archive --prefix="${TARBALL_NAME}/" --format=tar "${TAG}" | xz -9 > "${TARBALL}"

    info "Creating GPG detached signature ${TARBALL_ASC}"
    gpg --armor --detach-sign --output "${TARBALL_ASC}" "${TARBALL}"

    info "Creating SHA-256 checksum ${TARBALL_SHA256SUM}"
    sha256sum "${TARBALL}" > "${TARBALL_SHA256SUM}"
fi

# ── Done ─────────────────────────────────────────────────────────────────────

echo ""
if [ "${DRY_RUN}" -eq 1 ]; then
    info "Dry run complete. No changes were made."
else
    info "Release ${TAG} prepared successfully."
    echo ""
    info "Files created:"
    echo "  ${TARBALL}"
    echo "  ${TARBALL_ASC}"
    echo "  ${TARBALL_SHA256SUM}"
    echo ""
    info "To publish:"
    echo "  git push origin master"
    echo "  git push origin ${TAG}"
    echo "  scp ${TARBALL} ${TARBALL_ASC} ${TARBALL_SHA256SUM} <server>:<path>"
fi
echo ""
info "CI will build, test, and deploy binary installers on tag push."
