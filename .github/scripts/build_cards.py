import glob
import os
import shutil
import subprocess
import sys

try:
    import yaml
except ImportError:
    subprocess.run([sys.executable, "-m", "pip", "install", "pyyaml"], check=True)
    import yaml

README_NAMES = ("README.md", "readme.md", "Readme.md", "README", "readme")


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


def build():
    made = 0
    for submodule in sorted(glob.glob("projects/*/*")):
        if not os.path.isdir(submodule):
            continue
        try:
            _, category, project = submodule.split("/")[:3]
            catalog = os.path.join(submodule, ".catalog")
            manifest_path = os.path.join(catalog, "manifest.yml")
            image_path = os.path.join(catalog, "screenshots", "index.png")

            if not os.path.isfile(manifest_path):
                print(f"skip {submodule}: no .catalog/manifest.yml")
                continue

            with open(manifest_path, encoding="utf-8") as f:
                manifest = yaml.safe_load(f) or {}

            origin = manifest["sourcecode"]["location"]["origin"]
            short_description = manifest.get("short_description", "")
            title = first_heading(submodule) or project

            dest = os.path.join("cards", category, project)
            os.makedirs(dest, exist_ok=True)
            with open(os.path.join(dest, "index.md"), "w", encoding="utf-8") as f:
                f.write(f"# title\n{title}\n\n# url\n{origin}\n\n# text\n{short_description}\n")

            if os.path.isfile(image_path):
                shutil.copyfile(image_path, os.path.join(dest, "index.png"))

            made += 1
            print(f"ok {submodule} -> {dest}")
        except Exception as error:
            print(f"skip {submodule}: {error}")

    print(f"generated {made} cards")


if __name__ == "__main__":
    build()
