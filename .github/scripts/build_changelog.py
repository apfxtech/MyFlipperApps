import os
import subprocess
import re

CURRENT_TAG = os.environ.get("GITHUB_REF_NAME", "").strip()

TYPE_INFO = {
    "fix": "Bug fix",
    "add": "Add feature",
    "ref": "Refactor",
    "tst": "Test",
    "opt": "Performance improvement",
    "sec": "Security fix",
    "rmv": "Remove",
    "dep": "Dependency update",
    "doc": "Docs update",
    "bld": "Build update",
    "cii": "CI/CD update",
    "cfg": "Config update",
    "cln": "Cleanup",
}

COMMIT_RE = re.compile(r'^([a-z]{3})\(([^)]+)\):\s*(.+)$', re.IGNORECASE)

def run(cmd):
    return subprocess.check_output(cmd, shell=True, text=True).strip()

def get_previous_tag():
    try:
        tags = run("git tag --sort=-creatordate").splitlines()
        for t in tags:
            t = t.strip()
            if t and t != CURRENT_TAG:
                return t
    except Exception:
        pass
    return None

def get_commits(prev_tag):
    try:
        if prev_tag:
            log = run(f'git log {prev_tag}..HEAD --pretty=format:"%s"')
        else:
            log = run('git log --pretty=format:"%s"')
        return [line.strip() for line in log.splitlines() if line.strip()]
    except Exception:
        return []

def parse_commit(msg):
    m = COMMIT_RE.match(msg.strip())
    if not m:
        return None

    typ, scope, text = m.groups()
    typ = typ.lower().strip()
    scope = scope.strip()
    text = text.strip()

    label = TYPE_INFO.get(typ)
    if label:
        return f"{label} in {scope}"

    return f"{text} ({scope})"

def main():
    prev_tag = get_previous_tag()
    commits = get_commits(prev_tag)

    changes = []
    for c in commits:
        parsed = parse_commit(c)
        if parsed:
            changes.append(parsed)

    if not changes:
        changes = ["Minor updates"]

    out = "Changelog:\n"
    for ch in changes:
        out += f"- {ch}\n"

    github_output = os.environ["GITHUB_OUTPUT"]
    with open(github_output, "a", encoding="utf-8") as f:
        f.write("changelog<<EOF\n")
        f.write(out)
        f.write("EOF\n")

    print(out)

if __name__ == "__main__":
    main()