#!/usr/bin/env python3
import argparse
import re
import sys
from datetime import datetime, timezone


def bump_cmakelist(filename, vername):
    with open(filename, "r") as file:
        content = file.read()

    pattern = r"(project\(.+?\s+VERSION\s+)([0-9\.]+)(\))"
    m = re.search(pattern, content, flags=re.MULTILINE)
    if not m:
        raise ValueError("Pattern not found")

    new_content, count = re.subn(
        pattern, f"{m.group(1)}{vername}{m.group(3)}", content, flags=re.MULTILINE
    )
    with open(filename, "w") as file:
        file.write(new_content)
    print(f"{filename}: {count} change(s) made")


def bump_metainfo(filename, vername):
    today_str = datetime.now(timezone.utc).date().isoformat()

    with open(filename, "r") as file:
        content = file.read()

    pattern = r"<releases>\n+(\s*)(<release .+?\/>)\n"
    m = re.search(pattern, content, re.MULTILINE)
    if not m:
        raise ValueError("Pattern not found")

    replacement = (
        f"<releases>\n"
        f'{m.group(1)}<release version="{vername}" date="{today_str}"/>\n'
        f"{m.group(1)}{m.group(2)}\n"
    )

    new_content, count = re.subn(pattern, replacement, content, flags=re.MULTILINE)
    with open(filename, "w") as file:
        file.write(new_content)
    print(f"{filename}: {count} change(s) made, release date: {today_str}")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("vername", help="Version name to bump")
    args = parser.parse_args()

    bump_cmakelist("CMakeLists.txt", args.vername)
    bump_cmakelist("src/CMakeLists.txt", args.vername)
    bump_metainfo("org.fcitx.Fcitx5.Addon.McBopomofo.metainfo.xml.in", args.vername)

    return 0


if __name__ == "__main__":
    sys.exit(main())
