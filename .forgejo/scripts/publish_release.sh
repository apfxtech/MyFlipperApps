#!/usr/bin/env bash
set -euo pipefail

ARTIFACTS_DIR="${1:?artifacts dir is required}"

: "${FORGEJO_TOKEN:?FORGEJO_TOKEN is required}"
: "${FORGEJO_REPO:?FORGEJO_REPO is required}"
: "${TAG:?TAG is required}"
: "${GIT_SHA:?GIT_SHA is required}"
: "${FW_VERSION:?FW_VERSION is required}"
: "${API_VERSION:?API_VERSION is required}"

FORGEJO_SERVER_URL="${FORGEJO_SERVER_URL:-https://git.aperturefox.ru}"
API_BASE="${FORGEJO_SERVER_URL%/}/api/v1"
REPO_API="${API_BASE}/repos/${FORGEJO_REPO}"

NOTES_FILE="$(mktemp)"
RELEASE_JSON="$(mktemp)"
RESP_BODY="$(mktemp)"
HTTP_CODE_FILE="$(mktemp)"

cleanup() {
  rm -f "$NOTES_FILE" "$RELEASE_JSON" "$RESP_BODY" "$HTTP_CODE_FILE"
}
trap cleanup EXIT

cat > "$NOTES_FILE" <<EOF
Automated FAP build for MyFlipperApps. $GIT_SHA

Build details:
- Applied cdefine flag: ARDULIB_USE_VIEW_PORT
- Firmware source:      FlipperZero/unleashed-firmware
- Firmware version:     $FW_VERSION
- Firmware API:         $API_VERSION
EOF

api_call() {
  local method="$1"
  local url="$2"
  local data_file="${3:-}"
  local output_file="${4:-$RESP_BODY}"

  if [ -n "$data_file" ]; then
    curl -sS \
      -X "$method" \
      -H "Authorization: token $FORGEJO_TOKEN" \
      -H "Content-Type: application/json" \
      -o "$output_file" \
      -w "%{http_code}" \
      --data @"$data_file" \
      "$url" > "$HTTP_CODE_FILE"
  else
    curl -sS \
      -X "$method" \
      -H "Authorization: token $FORGEJO_TOKEN" \
      -o "$output_file" \
      -w "%{http_code}" \
      "$url" > "$HTTP_CODE_FILE"
  fi

  cat "$HTTP_CODE_FILE"
}

get_release_by_tag() {
  local code
  code="$(api_call GET "${REPO_API}/releases/tags/${TAG}" "" "$RELEASE_JSON" || true)"
  if [ "$code" = "200" ]; then
    return 0
  fi
  return 1
}

create_release() {
  local payload
  payload="$(mktemp)"
  cat > "$payload" <<EOF
{
  "tag_name": ${TAG@Q},
  "target_commitish": ${GIT_SHA@Q},
  "name": ${TAG@Q},
  "body": $(python3 - <<'PY' "$NOTES_FILE"
import json, sys, pathlib
print(json.dumps(pathlib.Path(sys.argv[1]).read_text(encoding="utf-8")))
PY
),
  "draft": false,
  "prerelease": false
}
EOF

  local code
  code="$(api_call POST "${REPO_API}/releases" "$payload" "$RELEASE_JSON")"
  rm -f "$payload"

  if [ "$code" != "201" ]; then
    echo "Failed to create release. HTTP $code" >&2
    cat "$RELEASE_JSON" >&2 || true
    exit 1
  fi
}

extract_release_field() {
  local field="$1"
  python3 - <<'PY' "$RELEASE_JSON" "$field"
import json, sys
with open(sys.argv[1], "r", encoding="utf-8") as f:
    data = json.load(f)
value = data.get(sys.argv[2], "")
if isinstance(value, str):
    print(value)
else:
    print(value if value is not None else "")
PY
}

release_has_asset() {
  local asset_name="$1"
  python3 - <<'PY' "$RELEASE_JSON" "$asset_name"
import json, sys
with open(sys.argv[1], "r", encoding="utf-8") as f:
    data = json.load(f)
for asset in data.get("assets", []):
    if asset.get("name") == sys.argv[2]:
        print(asset.get("id", ""))
        raise SystemExit(0)
raise SystemExit(1)
PY
}

delete_asset() {
  local asset_id="$1"
  local code
  code="$(api_call DELETE "${REPO_API}/releases/assets/${asset_id}" "" "$RESP_BODY")"
  if [ "$code" != "204" ]; then
    echo "Failed to delete existing asset id=$asset_id. HTTP $code" >&2
    cat "$RESP_BODY" >&2 || true
    exit 1
  fi
}

upload_asset() {
  local release_id="$1"
  local file_path="$2"
  local file_name
  file_name="$(basename "$file_path")"

  local existing_asset_id=""
  if existing_asset_id="$(release_has_asset "$file_name" 2>/dev/null)"; then
    echo "Deleting existing asset: $file_name"
    delete_asset "$existing_asset_id"
  fi

  local code
  code="$(
    curl -sS \
      -X POST \
      -H "Authorization: token $FORGEJO_TOKEN" \
      -H "Content-Type: application/octet-stream" \
      --data-binary @"$file_path" \
      -o "$RESP_BODY" \
      -w "%{http_code}" \
      "${REPO_API}/releases/${release_id}/assets?name=$(python3 - <<'PY' "$file_name"
import sys, urllib.parse
print(urllib.parse.quote(sys.argv[1]))
PY
)"
  )"

  if [ "$code" != "201" ]; then
    echo "Failed to upload asset: $file_name. HTTP $code" >&2
    cat "$RESP_BODY" >&2 || true
    exit 1
  fi

  echo "Uploaded: $file_name"
}

if ! compgen -G "${ARTIFACTS_DIR}/*.fap" > /dev/null; then
  echo "No .fap files found in ${ARTIFACTS_DIR}" >&2
  exit 1
fi

if get_release_by_tag; then
  echo "Release for tag ${TAG} already exists"
else
  echo "Creating release for tag ${TAG}"
  create_release
fi

RELEASE_ID="$(extract_release_field id)"
if [ -z "$RELEASE_ID" ]; then
  echo "Failed to determine release id" >&2
  cat "$RELEASE_JSON" >&2 || true
  exit 1
fi

for fap in "${ARTIFACTS_DIR}"/*.fap; do
  upload_asset "$RELEASE_ID" "$fap"
done

echo "Release upload completed successfully"