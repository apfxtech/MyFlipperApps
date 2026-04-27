import os
import re
import subprocess

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

COMMIT_RE = re.compile(r"^([a-z]{3})\(([^)]+)\):\s*(.+)$", re.IGNORECASE)


def run(cmd: str) -> str:
    return subprocess.check_output(cmd, shell=True, text=True).strip()


def get_previous_tag():
    try:
        tags = run("git tag --sort=-creatordate").splitlines()
        for tag in tags:
            tag = tag.strip()
            if tag and tag != CURRENT_TAG:
                return tag
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
    match = COMMIT_RE.match(msg.strip())
    if not match:
        return None

    change_type, scope, text = match.groups()
    change_type = change_type.lower().strip()
    scope = scope.strip()
    text = text.strip()

    label = TYPE_INFO.get(change_type)
    if label:
        return f"{label} in {scope}"

    return f"{text} ({scope})"


def main():
    prev_tag = get_previous_tag()
    commits = get_commits(prev_tag)

    changes = []
    for commit in commits:
        parsed = parse_commit(commit)
        if parsed:
            changes.append(parsed)

    if not changes:
        changes = ["Minor updates"]

    changelog = "Changelog:\n"
    for change in changes:
        changelog += f"- {change}\n"

    with open(os.environ["GITHUB_OUTPUT"], "a", encoding="utf-8") as output:
        output.write("changelog<<EOF\n")
        output.write(changelog)
        output.write("EOF\n")

    print(changelog)


if __name__ == "__main__":
    main()
