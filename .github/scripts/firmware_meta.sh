#!/usr/bin/env bash
set -euo pipefail

FIRMWARE_DIR="${1:?firmware dir is required}"

cd "$FIRMWARE_DIR"

FW_VERSION="$(git tag --points-at HEAD | head -n 1 || true)"

if [ -z "$FW_VERSION" ]; then
  FW_VERSION="$(git describe --tags --abbrev=0 2>/dev/null || true)"
fi

if [ -z "$FW_VERSION" ]; then
  FW_VERSION="$(git rev-parse --short HEAD)"
fi

echo "fw_version=$FW_VERSION" >> "$GITHUB_OUTPUT"
