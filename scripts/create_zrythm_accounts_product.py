#!/usr/bin/env python3

# SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
# SPDX-License-Identifier: LicenseRef-ZrythmLicense

import argparse
import json
import os
import sys
import time
from datetime import datetime
from typing import Optional

import requests


class ZrythmProductCreator:
    """Modern Python client for creating Zrythm products via API."""

    def __init__(self, api_token: str, base_url: Optional[str] = None):
        """
        Initialize the product creator.

        Args:
            api_token: Authentication token for the API
            base_url: Base URL for the products API endpoint (defaults to ZRYTHM_ACCOUNTS_PRODUCTS_API_ENDPOINT env var)
        """
        self.api_token = api_token
        self.base_url = base_url or os.getenv("ZRYTHM_ACCOUNTS_PRODUCTS_API_ENDPOINT")
        self.headers = {
            "Accept": "application/json",
            "Content-Type": "application/json",
            "Authorization": f"Token {api_token}"
        }
        self.session = requests.Session()
        self.session.headers.update(self.headers)

    def get_product(self, suffix: str) -> Optional[dict]:
        """
        Get a product from the API.

        Args:
            suffix: URL suffix for the product endpoint

        Returns:
            Product data as dict, or None if request failed
        """
        try:
            time.sleep(2)  # Rate limiting
            response = self.session.get(f"{self.base_url}{suffix}")
            response.raise_for_status()
            return response.json()
        except requests.RequestException as e:
            print(f"Error getting product: {e}", file=sys.stderr)
            return None

    def create_product(self, product_data: dict) -> Optional[dict]:
        """
        Create a new product via the API.

        Args:
            product_data: Dictionary containing product information

        Returns:
            Created product data as dict, or None if creation failed
        """
        try:
            time.sleep(2)  # Rate limiting
            print(f"POST {json.dumps(product_data)}", file=sys.stderr)

            response = self.session.post(
                self.base_url,
                json=product_data
            )
            response.raise_for_status()

            # Log response to files as well
            with open("out.log", "w") as f:
                f.write(response.text)

            return response.json()
        except requests.RequestException as e:
            print(f"Error creating product: {e}", file=sys.stderr)
            # Log error to err.log as well
            with open("err.log", "w") as f:
                f.write(str(e))
            return None

    def create_zrythm_product(self, version: str, is_tag: bool = False) -> Optional[dict]:
        """
        Create a Zrythm product with appropriate type and description.

        Args:
            version: Package version (e.g., "1.0.0")
            is_tag: Whether this is a release tag (vs nightly build)

        Returns:
            Created product data as dict, or None if creation failed
        """
        if is_tag:
            product_type = "Single"
            summary = "Single version"
            description = "No description"
        else:
            product_type = "Nightly"
            summary = "Nightly build"
            description = f"Nightly build at {datetime.now().isoformat()}."

        product_data = {
            "type": product_type,
            "summary": summary,
            "description": description,
            "version": version,
            "image_url": "https://www.zrythm.org/static/icons/zrythm/z_frame_8.png",
            "price_jpy": "2200"
        }

        return self.create_product(product_data)


def parse_arguments():
    """Parse command line arguments."""
    parser = argparse.ArgumentParser(
        description="Create a Zrythm product via the accounts API",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s --version 1.0.0 --tag
  %(prog)s --version 1.0.0-beta.1 --no-tag
        """
    )

    parser.add_argument(
        "--version",
        required=True,
        help="Package version (e.g., '1.0.0')"
    )

    parser.add_argument(
        "--tag",
        action="store_true",
        help="Create a release tag product (default is nightly build)"
    )

    parser.add_argument(
        "--api-token",
        default=os.getenv("ZRYTHM_ACCOUNTS_TOKEN"),
        help="API token for authentication (defaults to ZRYTHM_ACCOUNTS_TOKEN env var)"
    )

    parser.add_argument(
        "--api-endpoint",
        default=os.getenv("ZRYTHM_ACCOUNTS_PRODUCTS_API_ENDPOINT"),
        help="API endpoint URL (defaults to ZRYTHM_ACCOUNTS_PRODUCTS_API_ENDPOINT env var)"
    )

    return parser.parse_args()


def main():
    """Main function to create a Zrythm product."""
    # Parse command line arguments
    args = parse_arguments()

    # Validate API token
    if not args.api_token:
        print("Error: API token not provided. Set ZRYTHM_ACCOUNTS_TOKEN environment variable or use --api-token", file=sys.stderr)
        sys.exit(1)

    # Initialize the product creator
    creator = ZrythmProductCreator(args.api_token, args.api_endpoint)

    print(f"bundle: {args.version}")

    # Create the product
    result = creator.create_zrythm_product(args.version, args.tag)

    if result:
        print(f"Successfully created product: {json.dumps(result, indent=2)}")
    else:
        print("Failed to create product", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
