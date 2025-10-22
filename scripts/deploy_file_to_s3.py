#!/usr/bin/env python3

# SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

"""
Deploys the given file to an S3 bucket.

This script uses boto3 which automatically looks for credentials in:
1. AWS access key ID and secret access key environment variables (AWS_ACCESS_KEY_ID, AWS_SECRET_ACCESS_KEY)
2. AWS session token environment variable (AWS_SESSION_TOKEN) if using temporary credentials
3. Shared credentials file (~/.aws/credentials)
4. IAM role for EC2 instances (if running on EC2)

Example environment variables:
export AWS_ACCESS_KEY_ID=your_access_key
export AWS_SECRET_ACCESS_KEY=your_secret_key
export AWS_DEFAULT_REGION=us-east-1
"""

import argparse
import logging
import os
import sys
from pathlib import Path
from typing import Optional

import boto3
from botocore.exceptions import ClientError, NoCredentialsError

# Set up logging
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s - %(levelname)s - %(message)s",
    datefmt="%Y-%m-%d %H:%M:%S",
)
logger = logging.getLogger(__name__)


class S3Deployer:
    """Handles deployment of artifacts to S3."""

    def __init__(self, bucket_name: str):
        """
        Initialize the S3 deployer.

        Args:
            bucket_name: Name of the S3 bucket
        """
        self.bucket_name = bucket_name
        self.s3_client = boto3.client("s3")

    def upload_file(
        self,
        file_path: str,
        s3_key: Optional[str] = None,
        make_public: bool = False,
    ) -> bool:
        """
        Upload a file to S3.

        Args:
            file_path: Path to the local file to upload
            s3_key: S3 key (path within bucket). If None, uses the filename
            make_public: Whether to make the object publicly readable

        Returns:
            True if upload was successful, False otherwise
        """
        file_path = Path(file_path)
        if not file_path.exists():
            logger.error(f"File not found: {file_path}")
            return False

        if s3_key is None:
            s3_key = file_path.name

        try:
            # Upload the file
            extra_args = {}
            if make_public:
                extra_args["ACL"] = "public-read"

            logger.info(f"Uploading {file_path.name} to S3")
            self.s3_client.upload_file(
                str(file_path),
                self.bucket_name,
                s3_key,
                ExtraArgs=extra_args if extra_args else None,
            )

            logger.info(f"Successfully uploaded to S3")
            return True

        except (ClientError, NoCredentialsError) as e:
            logger.error(f"Failed to upload {file_path.name}: {str(type(e).__name__)}")
            return False

    def upload_file_with_prefix(
        self,
        file_path: str,
        prefix: Optional[str] = None,
    ) -> bool:
        """
        Upload a file with optional prefix in S3 key.

        Args:
            file_path: Path to the file to upload
            prefix: Optional prefix for S3 key path

        Returns:
            True if upload was successful, False otherwise
        """
        file_path = Path(file_path)
        filename = file_path.name

        # Create S3 key with prefix if provided
        if prefix:
            s3_key = f"{prefix}/{filename}"
        else:
            s3_key = filename

        return self.upload_file(file_path, s3_key)


def main():
    """Main entry point."""
    parser = argparse.ArgumentParser(
        description="Deploy installers to S3 bucket",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument(
        "--bucket",
        default=os.environ.get("ZRYTHM_INSTALLERS_S3_BUCKET_NAME", ""),
        help="S3 bucket name",
    )
    parser.add_argument(
        "--prefix",
        default=os.environ.get("CI_COMMIT_TAG", "").lstrip("v"),
        help="Prefix for S3 key path (defaults to CI_COMMIT_TAG without 'v' prefix)",
    )
    parser.add_argument(
        "filepath",
        help="Path to the file to upload",
    )

    args = parser.parse_args()

    # Validate that the file exists
    if not Path(args.filepath).exists():
        logger.error(f"File not found: {args.filepath}")
        sys.exit(1)

    # Initialize deployer
    deployer = S3Deployer(args.bucket)

    # Upload the file
    logger.info(f"Deploying file: {Path(args.filepath).name}")
    result = deployer.upload_file_with_prefix(args.filepath, args.prefix if args.prefix else None)

    if not result:
        logger.error("Upload failed")
        sys.exit(1)

    logger.info("File deployed successfully")


if __name__ == "__main__":
    main()
