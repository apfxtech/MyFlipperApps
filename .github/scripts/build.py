import glob
import os
import re
import shutil
import subprocess
import sys

try:
    import yaml
except ImportError:
    subprocess.run([sys.executable, "-m", "pip", "install", "pyyaml"], check=True)
    import yaml

ARTIFACTS = "artifacts"
SERVER = os.environ.get("GITHUB_SERVER_URL", "https://github.com")
REPO = os.environ.get("GITHUB_REPOSITORY", "")
TAG = os.environ.get("GITHUB_REF_NAME", "")

README_NAMES = ("README.md", "readme.md", "Readme.md", "README", "readme")
API_RE = re.compile(r"API:\s*([0-9][0-9.]*)")
TARGET_RE = re.compile(r"Target:\s*(\d+)")
FAP_VERSION_RE = re.compile(r"fap_version\s*=\s*([^\n,]+)")


def first_heading(directory):
    for name in README_NAMES:
        path = os.path.join(directory, name)
        if not os.path.isfile(path):
            continue
        with open(path, encoding="utf-8", errors="ignore") as f:
            for line in f:
                stripped = line.strip()
                if stripped.startswith("#"):
                    return stripped.lstrip("#").strip()
    return None


def read_fap_version(appdir):
    fam = os.path.join(appdir, "application.fam")
    if not os.path.isfile(fam):
        return ""
    with open(fam, encoding="utf-8", errors="ignore") as f:
        match = FAP_VERSION_RE.search(f.read())
    if not match:
        return ""
    raw = match.group(1).strip().rstrip(",").strip()
    if raw.startswith("("):
        numbers = re.findall(r"\d+", raw)
        return ".".join(numbers)
    return raw.strip('"').strip("'")


def build_app(appdir):
    result = subprocess.run(["ufbt"], cwd=appdir, capture_output=True, text=True)
    log = result.stdout + result.stderr
    print(log)
    if result.returncode != 0:
        raise RuntimeError(f"ufbt exited with {result.returncode}")
    faps = sorted(glob.glob(os.path.join(appdir, "dist", "*.fap")))
    if not faps:
        raise RuntimeError("no .fap produced")
    api = API_RE.search(log)
    target = TARGET_RE.search(log)
    return faps[0], (api.group(1) if api else ""), (target.group(1) if target else "")


def write_card(category, project, fields, image_path):
    dest = os.path.join("cards", category, project)
    os.makedirs(dest, exist_ok=True)
    blocks = []
    for key in ("title", "url", "text", "version", "api", "download"):
        value = fields.get(key)
        if value:
            blocks.append(f"# {key}\n{value}\n")
    with open(os.path.join(dest, "index.md"), "w", encoding="utf-8") as f:
        f.write("\n".join(blocks))
    if image_path and os.path.isfile(image_path):
        shutil.copyfile(image_path, os.path.join(dest, "index.png"))


def main():
    os.makedirs(ARTIFACTS, exist_ok=True)
    api_used = ""
    target_used = ""
    built = []

    for submodule in sorted(glob.glob("projects/*/*")):
        if not os.path.isdir(submodule):
            continue
        try:
            _, category, project = submodule.split("/")[:3]
            manifest_path = os.path.join(submodule, ".catalog", "manifest.yml")
            image_path = os.path.join(submodule, ".catalog", "screenshots", "index.png")

            if not os.path.isfile(manifest_path):
                print(f"skip {submodule}: no .catalog/manifest.yml")
                continue

            with open(manifest_path, encoding="utf-8") as f:
                manifest = yaml.safe_load(f) or {}

            fields = {
                "title": first_heading(submodule) or project,
                "url": manifest["sourcecode"]["location"]["origin"],
                "text": manifest.get("short_description", ""),
            }

            if os.path.isfile(os.path.join(submodule, "application.fam")):
                try:
                    fap, api, target = build_app(submodule)
                    fap_name = os.path.basename(fap)
                    shutil.copy(fap, os.path.join(ARTIFACTS, fap_name))
                    fields["version"] = read_fap_version(submodule)
                    fields["api"] = api
                    if TAG and REPO:
                        fields["download"] = f"{SERVER}/{REPO}/releases/download/{TAG}/{fap_name}"
                    api_used = api_used or api
                    target_used = target_used or target
                    built.append((fields["title"], fap_name, fields.get("version", ""), api))
                    print(f"built {submodule} -> {fap_name} (api {api})")
                except Exception as build_error:
                    print(f"build skipped for {submodule}: {build_error}")

            write_card(category, project, fields, image_path)
            print(f"card {submodule} -> cards/{category}/{project}")
        except Exception as error:
            print(f"skip {submodule}: {error}")

    notes = []
    if api_used:
        line = f"Built with ufbt dev channel — API {api_used}"
        if target_used:
            line += f", target {target_used}"
        notes.append(line + ".")
        notes.append("")
    if built:
        notes.append("| App | File | Version | API |")
        notes.append("|---|---|---|---|")
        for title, fap_name, version, api in built:
            notes.append(f"| {title} | {fap_name} | {version or '-'} | {api or '-'} |")
    with open("release_notes.md", "w", encoding="utf-8") as f:
        f.write("\n".join(notes) + "\n")

    print(f"built {len(built)} apps, api {api_used or 'unknown'}")


if __name__ == "__main__":
    main()
