#!/usr/bin/env python3

# SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

"""
Fetches installers from S3 and creates a GitHub release with binaries.

Environment variables:
  GITHUB_TOKEN          - GitHub personal access token (required)
  AWS_ACCESS_KEY_ID     - AWS credentials (or other boto3 auth)
  AWS_SECRET_ACCESS_KEY - AWS secret key
  AWS_DEFAULT_REGION    - AWS region
  ZRYTHM_INSTALLERS_S3_BUCKET_NAME - S3 bucket name

Usage:
  scripts/create_github_release.py v2.0.0-alpha.1
  scripts/create_github_release.py --dry-run v2.0.0-alpha.1
"""

import argparse
import os
import sys
import tempfile
from pathlib import Path

import boto3
from github import Auth, Github

GITHUB_REPO = "zrythm/zrythm"
S3_PREFIXES = [
    "packages/windows",
    "packages/macos-universal",
    "packages/gnu-linux-cpack",
]


def get_changelog(version: str) -> str:
    version_no_v = version.lstrip("v")
    tag = f"v{version_no_v}" if not version.startswith("v") else version
    changelog_path = Path(__file__).resolve().parent.parent / "CHANGELOG.md"
    lines = changelog_path.read_text().splitlines()
    in_section = False
    result = []
    for line in lines:
        if line.startswith("## ["):
            if in_section:
                break
            if tag in line or version_no_v in line:
                in_section = True
            continue
        if in_section:
            result.append(line)
    body = "\n".join(result).strip()
    return body if body else f"Release {tag}"


def fetch_s3_binaries(bucket: str, tag: str, dest: Path):
    version_no_v = tag.lstrip("v")
    s3 = boto3.client("s3")
    downloaded = []
    for prefix in S3_PREFIXES:
        paginator = s3.get_paginator("list_objects_v2")
        for page in paginator.paginate(Bucket=bucket, Prefix=prefix):
            for obj in page.get("Contents", []):
                key = obj["Key"]
                filename = Path(key).name
                if version_no_v not in filename:
                    continue
                if prefix == "packages/gnu-linux-cpack" and not filename.endswith(
                    ".AppImage"
                ):
                    continue
                dest_path = dest / filename
                print(f"  Downloading {filename}")
                s3.download_file(bucket, key, str(dest_path))
                downloaded.append(dest_path)
    return downloaded


def main():
    parser = argparse.ArgumentParser(
        description="Create GitHub release with binaries from S3",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print what would be done without making changes",
    )
    parser.add_argument(
        "version",
        help="Version string (e.g., v2.0.0-alpha.1 or 2.0.0-alpha.1)",
    )
    args = parser.parse_args()

    token = os.environ.get("GITHUB_TOKEN")
    if not token and not args.dry_run:
        print("GITHUB_TOKEN environment variable is required", file=sys.stderr)
        sys.exit(1)

    bucket = os.environ.get("ZRYTHM_INSTALLERS_S3_BUCKET_NAME", "")
    if not bucket and not args.dry_run:
        print(
            "ZRYTHM_INSTALLERS_S3_BUCKET_NAME environment variable is required",
            file=sys.stderr,
        )
        sys.exit(1)

    version = args.version.lstrip("v")
    tag = f"v{version}"
    prerelease = "-" in version

    print(f"Fetching changelog for {tag}")
    body = get_changelog(tag)

    print("Fetching binaries from S3")
    with tempfile.TemporaryDirectory() as tmp:
        if args.dry_run:
            artifacts = []
        else:
            artifacts = fetch_s3_binaries(bucket, tag, Path(tmp))
            if not artifacts:
                print("No binaries found on S3 for this version", file=sys.stderr)
                sys.exit(1)

        if args.dry_run:
            print(f"[dry-run] Would create release {tag} (prerelease={prerelease})")
            print(f"[dry-run] Body:\n{body[:200]}...")
            for a in artifacts:
                print(f"[dry-run] Would upload {a.name}")
            print("Done (dry run)")
            return

        gh = Github(auth=Auth.Token(token))
        repo = gh.get_repo(GITHUB_REPO)

        for release in repo.get_releases():
            if release.tag_name == tag:
                print(f"  Deleting existing release for {tag}")
                release.delete_release()
                break

        print(f"Creating GitHub release for {tag}")
        release = repo.create_git_release(
            tag=tag,
            name=tag,
            message=body,
            prerelease=prerelease,
            draft=False,
        )
        print(f"  Release created: {release.html_url}")

        for artifact in artifacts:
            print(f"  Uploading {artifact.name}")
            release.upload_asset(str(artifact), name=artifact.name, label=artifact.name)
            print(f"    Uploaded {artifact.name}")

    print("Done")


if __name__ == "__main__":
    main()
