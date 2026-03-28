#!/usr/bin/env bash
set -euo pipefail

ARTIFACTS_DIR="${1:?artifacts dir is required}"

: "${GH_TOKEN:?GH_TOKEN is required}"
: "${GH_REPO:?GH_REPO is required}"
: "${TAG:?TAG is required}"
: "${GIT_SHA:?GIT_SHA is required}"
: "${FW_VERSION:?FW_VERSION is required}"
: "${API_VERSION:?API_VERSION is required}"

NOTES_FILE="$(mktemp)"

cat > "$NOTES_FILE" <<EOF
Automated FAP build for MyFlipperApps. $GIT_SHA

Build details:
- Applied cdefine flag: ARDULIB_USE_VIEW_PORT
- Firmware source:      flipperdevices/flipperzero-firmware
- Firmware version:     $FW_VERSION
- Firmware API:         $API_VERSION

${CHANGELOG:-}
EOF

if gh release view "$TAG" --repo "$GH_REPO" >/dev/null 2>&1; then
  echo "Release already exists, uploading assets"
  gh release upload "$TAG" "$ARTIFACTS_DIR"/*.fap --repo "$GH_REPO" --clobber
  exit 0
fi

gh release create "$TAG" \
  --repo "$GH_REPO" \
  --title "$TAG" \
  --notes-file "$NOTES_FILE" \
  "$ARTIFACTS_DIR"/*.fap