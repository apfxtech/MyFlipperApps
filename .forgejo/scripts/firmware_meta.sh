#!/usr/bin/env bash
set -euo pipefail

FIRMWARE_DIR="${1:?firmware dir is required}"

cd "$FIRMWARE_DIR"

HEAD_SHA="$(git rev-parse HEAD)"
FW_VERSION=""

find_remote_tag_for_head() {
  local refs

  refs="$(git ls-remote --tags origin 2>/dev/null)" || return 1

  printf '%s\n' "$refs" | python3 -c '
import sys

head_sha = sys.argv[1].strip().lower()
matches = []

for raw_line in sys.stdin:
    line = raw_line.strip()
    if not line:
        continue
    parts = line.split()
    if len(parts) != 2:
        continue
    sha, ref = parts
    if sha.strip().lower() != head_sha:
        continue
    if not ref.startswith("refs/tags/"):
        continue
    name = ref[len("refs/tags/"):]
    if name.endswith("^{}"):
        name = name[:-3]
    if name:
        matches.append(name)

if not matches:
    sys.exit(1)

matches.sort()
print(matches[-1])
' "$HEAD_SHA"
}

FW_VERSION="$(find_remote_tag_for_head || true)"

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
