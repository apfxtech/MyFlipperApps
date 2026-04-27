#!/usr/bin/env bash
set -euo pipefail

FIRMWARE_DIR="${1:?firmware dir is required}"
ARTIFACTS_DIR="${2:?artifacts dir is required}"

if [ -z "${APP_IDS:-}" ]; then
  echo "APP_IDS is empty" >&2
  exit 1
fi

mkdir -p "$ARTIFACTS_DIR"

while IFS= read -r APP_ID; do
  [ -n "$APP_ID" ] || continue

  MATCHES="$(find "$FIRMWARE_DIR/build" -type f -name "${APP_ID}.fap" || true)"
  if [ -z "$MATCHES" ]; then
    echo "FAP not found for $APP_ID" >&2
    exit 1
  fi

  FIRST_MATCH="$(printf '%s\n' "$MATCHES" | head -n 1)"
  cp "$FIRST_MATCH" "$ARTIFACTS_DIR/${APP_ID}.fap"
done <<< "$APP_IDS"

if ! find "$ARTIFACTS_DIR" -maxdepth 1 -type f -name '*.fap' | grep -q .; then
  echo "No FAP artifacts were collected" >&2
  exit 1
fi

ls -l "$ARTIFACTS_DIR"
