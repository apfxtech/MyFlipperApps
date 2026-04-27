#!/usr/bin/env bash
set -euo pipefail

FIRMWARE_DIR="${1:?firmware dir is required}"

if [ -z "${APP_IDS:-}" ]; then
  echo "APP_IDS is empty" >&2
  exit 1
fi

cd "$FIRMWARE_DIR"

: > build_all.log

while IFS= read -r APP_ID; do
  [ -n "$APP_ID" ] || continue

  echo "==============================" | tee -a build_all.log
  echo "Building fap_${APP_ID}" | tee -a build_all.log
  echo "==============================" | tee -a build_all.log

  ./fbt "fap_${APP_ID}" 2>&1 | tee -a build_all.log
done <<< "$APP_IDS"

API_VERSION="$(grep -oE 'API version [0-9]+\.[0-9]+' build_all.log | awk '{print $3}' | tail -n 1 || true)"

if [ -z "$API_VERSION" ]; then
  echo "Failed to detect API version from build log" >&2
  exit 1
fi

echo "api_version=$API_VERSION" >> "$GITHUB_OUTPUT"
