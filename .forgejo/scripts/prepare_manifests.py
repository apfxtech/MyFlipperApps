import os
import re
import ast
import shutil

REPO_ROOT = "repo"
SOURCE_APPS_ROOT = os.path.join(REPO_ROOT, "apps")
TARGET_APPS_ROOT = os.path.join("firmware", "applications_user")
DEFINE_NAME = "ARDULIB_USE_VIEW_PORT"

def split_comment(line: str):
    in_str = False
    quote = ""
    escape = False
    for i, ch in enumerate(line):
        if escape:
            escape = False
            continue
        if ch == "\\":
            escape = True
            continue
        if in_str:
            if ch == quote:
                in_str = False
            continue
        if ch in ("'", '"'):
            in_str = True
            quote = ch
            continue
        if ch == "#":
            return line[:i], line[i:]
    return line, ""

def patch_manifest_text(text: str):
    lines = text.splitlines(True)

    cdef_idx = None
    app_idx = None
    app_indent = ""

    for i, line in enumerate(lines):
        code, _comment = split_comment(line)
        if app_idx is None and re.search(r"\bApp\s*\(", code):
            app_idx = i
            app_indent = re.match(r"[ \t]*", line).group(0)

        if re.match(r"^[ \t]*cdefines\s*=", code):
            cdef_idx = i
            break

    patched_line = None

    if cdef_idx is not None:
        original_line = lines[cdef_idx]
        code, comment = split_comment(original_line)

        m = re.match(r"^([ \t]*)cdefines\s*=\s*(.+?)(\s*,\s*)?$", code.rstrip("\r\n"))
        if not m:
            raise SystemExit(f"Failed to parse cdefines line: {original_line!r}")

        indent = m.group(1)
        raw_value = m.group(2).strip()

        try:
            parsed = ast.literal_eval(raw_value)
            if not isinstance(parsed, list):
                parsed = []
        except Exception:
            parsed = []

        parsed = [str(x) for x in parsed]
        if DEFINE_NAME not in parsed:
            parsed.append(DEFINE_NAME)

        patched_line = f"{indent}cdefines={repr(parsed)},"
        newline = "\n"
        if original_line.endswith("\r\n"):
            newline = "\r\n"

        lines[cdef_idx] = patched_line + (f" {comment.strip()}" if comment.strip() else "") + newline
    else:
        if app_idx is None:
            raise SystemExit("Cannot find App(")

        entry_indent = app_indent + "    "
        patched_line = f'{entry_indent}cdefines=["{DEFINE_NAME}"],'

        newline = "\n"
        if lines[app_idx].endswith("\r\n"):
            newline = "\r\n"

        insert_at = app_idx + 1
        lines.insert(insert_at, patched_line + newline)

    new_text = "".join(lines)

    cdefine_count = 0
    for line in new_text.splitlines():
        code, _comment = split_comment(line)
        if re.match(r"^[ \t]*cdefines\s*=", code):
            cdefine_count += 1

    if cdefine_count != 1:
        raise SystemExit(f"Expected exactly one cdefines entry after patch, got {cdefine_count}")

    return new_text, patched_line

def main():
    if not os.path.isdir(SOURCE_APPS_ROOT):
        raise SystemExit(f"Source apps directory not found: {SOURCE_APPS_ROOT}")

    fam_dirs = []
    for root, _dirs, files in os.walk(SOURCE_APPS_ROOT):
        if "application.fam" in files:
            fam_dirs.append(root)

    if not fam_dirs:
        raise SystemExit(f"No application.fam found under {SOURCE_APPS_ROOT}")

    appid_re = re.compile(r'^\s*appid\s*=\s*"([^"]+)"', re.MULTILINE)

    app_entries = []
    seen_appids = set()
    used_target_dirs = set()

    for src_dir in sorted(fam_dirs):
        fam_path = os.path.join(src_dir, "application.fam")

        with open(fam_path, "r", encoding="utf-8") as f:
            text = f.read()

        appid_match = appid_re.search(text)
        if not appid_match:
            raise SystemExit(f"No appid found in {fam_path}")

        app_id = appid_match.group(1).strip()
        if not app_id:
            raise SystemExit(f"Empty appid in {fam_path}")

        if app_id in seen_appids:
            raise SystemExit(f"Duplicate appid detected: {app_id}")
        seen_appids.add(app_id)

        rel_src_dir = os.path.relpath(src_dir, SOURCE_APPS_ROOT).replace("\\", "/")
        base_target_name = rel_src_dir.replace("/", "__")
        target_name = base_target_name
        suffix = 2
        while target_name in used_target_dirs:
            target_name = f"{base_target_name}__{suffix}"
            suffix += 1
        used_target_dirs.add(target_name)

        dst_dir = os.path.join(TARGET_APPS_ROOT, target_name)

        print(f"COPY: {rel_src_dir} -> applications_user/{target_name}")
        shutil.copytree(src_dir, dst_dir, dirs_exist_ok=False)

        staged_fam_path = os.path.join(dst_dir, "application.fam")
        with open(staged_fam_path, "r", encoding="utf-8") as f:
            staged_text = f.read()

        patched_text, patched_line = patch_manifest_text(staged_text)

        with open(staged_fam_path, "w", encoding="utf-8") as f:
            f.write(patched_text)

        rel_staged_fam = os.path.relpath(staged_fam_path, "firmware").replace("\\", "/")
        print(f"PATCH: {rel_staged_fam}")
        print(patched_line + "\n")

        app_entries.append((app_id, target_name))

    github_output = os.environ["GITHUB_OUTPUT"]
    with open(github_output, "a", encoding="utf-8") as out:
        out.write("app_ids<<EOF\n")
        for app_id, _ in app_entries:
            out.write(app_id + "\n")
        out.write("EOF\n")

if __name__ == "__main__":
    main()