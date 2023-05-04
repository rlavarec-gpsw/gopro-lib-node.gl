import subprocess
import sys
from datetime import datetime
from pathlib import Path

__doc__ = """
Release process:

    1. on a clean git state, run this script and input the demanded libnodegl version
    2. check the last commit and tag
    3. git push && git push --tags

"""


def _run(*args: str):
    return subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE)


def main():
    # Ensure the running path is the repo root dir
    if not Path("CHANGELOG.md").is_file():
        print("This script must be executed from the project root directory")
        sys.exit(1)

    # Check if git index is clean
    if _run("git", "diff-index", "--quiet", "HEAD").returncode != 0:
        print("Git index is not clean")
        sys.exit(1)

    today = datetime.now().date().isoformat()
    cur_year = datetime.now().year

    # Get previous project version
    with open("VERSION") as f:
        prv_prj_year, prv_prj_digit = map(int, f.read().strip().split("."))

    # Retrieve new project version
    new_digit = 0 if prv_prj_year != cur_year else prv_prj_digit + 1
    prv_prj_ver = f"{prv_prj_year}.{prv_prj_digit}"
    new_prj_ver = f"{cur_year}.{new_digit}"
    print(f"New project version: {prv_prj_ver} → {new_prj_ver}")

    # Get libnodegl/VERSION
    with open("libnodegl/VERSION") as f:
        cur_lib_ver = f.read().strip()

    new_lib_ver = input(f"Enter new libnodegl version [current: {cur_lib_ver}]: ")

    # Update VERSION
    with open("VERSION", "w") as f:
        f.write(new_prj_ver)

    # Update libnodegl/VERSION
    with open("libnodegl/VERSION", "w") as f:
        f.write(new_lib_ver)

    # Get CHANGELOG.md
    with open("CHANGELOG.md") as f:
        content = f.read()

    # Update CHANGELOG.md
    with open("CHANGELOG.md", "w") as f:
        f.write(
            content.replace(
                "## [Unreleased]", f"## [Unreleased]\n\n## [{new_prj_ver}] [libnodegl {new_lib_ver}] - {today}"
            )
        )

    # Commit the changes and create a tag
    _run("git", "add", "VERSION", "libnodegl/VERSION", "CHANGELOG.md")
    _run("git", "commit", "-m", f"Release {new_prj_ver}")
    _run("git", "tag", f"v{new_prj_ver}")


if __name__ == "__main__":
    main()
