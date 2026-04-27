#!/usr/bin/env bash
set -euo pipefail

FIRMWARE_DIR="${1:?firmware dir is required}"
TAGS_API_URL="${TAGS_API_URL:-https://git.aperturefox.ru/api/v1/repos/FlipperZero/unleashed-firmware/tags}"

cd "$FIRMWARE_DIR"

HEAD_SHA="$(git rev-parse HEAD)"
FW_VERSION=""

fetch_tag_from_api() {
  local response
  local -a curl_args
  curl_args=(
    --silent
    --show-error
    --fail
    "$TAGS_API_URL"
  )

  if [ -n "${FORGEJO_TOKEN:-}" ]; then
    curl_args=(
      --silent
      --show-error
      --fail
      --header "Authorization: token ${FORGEJO_TOKEN}"
      "$TAGS_API_URL"
    )
  fi

  response="$(curl "${curl_args[@]}")" || return 1

  printf '%s' "$response" | python3 -c '
import json
import sys

head_sha = sys.argv[1].strip().lower()

try:
    tags = json.load(sys.stdin)
except Exception:
    sys.exit(1)

for tag in tags:
    commit = (tag or {}).get("commit") or {}
    sha = (commit.get("sha") or tag.get("id") or "").strip().lower()
    if sha == head_sha:
        name = (tag.get("name") or "").strip()
        if name:
            print(name)
            sys.exit(0)

sys.exit(1)
' "$HEAD_SHA"
}

FW_VERSION="$(fetch_tag_from_api || true)"

if [ -z "$FW_VERSION" ]; then
  FW_VERSION="$(git tag --points-at HEAD | head -n 1 || true)"
fi

if [ -z "$FW_VERSION" ]; then
  FW_VERSION="$(git describe --tags --abbrev=0 2>/dev/null || true)"
fi

if [ -z "$FW_VERSION" ]; then
  FW_VERSION="${HEAD_SHA:0:7}"
fi

echo "fw_version=$FW_VERSION" >> "$GITHUB_OUTPUT"
