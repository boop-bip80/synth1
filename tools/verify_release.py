#!/usr/bin/env python3
"""Verify the assets attached to a CS-80E GitHub release.

Fetch the assets first, then run the checker:

    gh release download latest --dir downloaded
    python3 tools/verify_release.py --tag latest --dir downloaded

GITHUB_REPOSITORY and GITHUB_SHA are read from the environment in CI; the
repository falls back to boop-bip80/synth1 for local runs.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import pathlib
import re
import subprocess
import sys
import zipfile


SCREENSHOTS = [
    "01-blade-brass.png",
    "02-vangelis-lead.png",
    "03-neon-pads.png",
    "04-preset-library.png",
    "05-brass-stab.png",
    "06-filter-closeup.png",
    "07-vca-sine.png",
    "08-ribbon-keybed.png",
    "09-blade-runner.png",
    "10-channel-two.png",
]

ZIPS = [
    "CS-80E-linux.zip",
    "CS-80E-macos.zip",
    "CS-80E-windows.zip",
    "CS-80E-previews.zip",
]

README_DOWNLOADS = ZIPS + ["latest.json", "SHA256SUMS.txt"]

FORMATS = {
    "linux": ["VST3", "Standalone"],
    "macos": ["VST3", "AU", "Standalone"],
    "windows": ["VST3", "Standalone"],
}


class Checks:
    def __init__(self) -> None:
        self.failures = 0

    def check(self, condition: bool, message: str) -> None:
        print(f"  {'ok  ' if condition else 'FAIL'} {message}")
        if not condition:
            self.failures += 1

    def finish(self) -> None:
        if self.failures:
            print(f"\n{self.failures} release check(s) failed.")
            raise SystemExit(1)
        print("\nAll release checks passed.")


def sha256(path: pathlib.Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def zip_names(path: pathlib.Path) -> set[str]:
    with zipfile.ZipFile(path) as archive:
        return set(archive.namelist())


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tag", required=True, help="release tag, e.g. latest or v5.0.0")
    parser.add_argument("--dir", default="downloaded", help="directory containing the release assets")
    parser.add_argument("--repo", default=os.environ.get("GITHUB_REPOSITORY", "boop-bip80/synth1"))
    parser.add_argument("--commit", default=os.environ.get("GITHUB_SHA", ""))
    args = parser.parse_args()

    root = pathlib.Path(__file__).resolve().parents[1]
    if args.commit:
        try:
            args.commit = subprocess.run(
                ["git", "rev-parse", "--verify", args.commit],
                cwd=root,
                capture_output=True,
                text=True,
                check=True,
            ).stdout.strip()
        except (OSError, subprocess.SubprocessError):
            pass
    downloaded = pathlib.Path(args.dir)
    version = (root / "VERSION").read_text().strip()
    base = f"https://github.com/{args.repo}/releases/download/{args.tag}"
    expected = set(SCREENSHOTS) | set(ZIPS) | {"cs80-demo.wav", "latest.json", "SHA256SUMS.txt"}
    checks = Checks()

    checks.check(downloaded.is_dir(), f"asset directory {downloaded} exists")
    if not downloaded.is_dir():
        checks.finish()
        return

    actual = {path.name for path in downloaded.iterdir() if path.is_file()}
    checks.check(not (expected - actual), f"all {len(expected)} expected assets are attached (missing: {sorted(expected - actual) or 'none'})")
    checks.check(not (actual - expected), f"no stale assets are attached (unexpected: {sorted(actual - expected) or 'none'})")

    for name in SCREENSHOTS:
        local = root / "docs" / "screenshots" / name
        remote = downloaded / name
        checks.check(
            local.is_file() and remote.is_file() and sha256(local) == sha256(remote),
            f"{name} matches docs/screenshots",
        )

    local_audio = root / "docs" / "audio" / "cs80-demo.wav"
    remote_audio = downloaded / "cs80-demo.wav"
    checks.check(
        local_audio.is_file() and remote_audio.is_file() and sha256(local_audio) == sha256(remote_audio),
        "cs80-demo.wav matches docs/audio",
    )

    sums: dict[str, str] = {}
    sums_path = downloaded / "SHA256SUMS.txt"
    if sums_path.is_file():
        for line in sums_path.read_text().splitlines():
            line = line.strip()
            if not line:
                continue
            match = re.match(r"^([0-9a-fA-F]{64})\s+\*?(.+)$", line)
            if match:
                sums[pathlib.Path(match.group(2)).name] = match.group(1).lower()
            else:
                checks.check(False, f"unreadable SHA256SUMS line: {line}")
    checks.check(set(sums) == set(ZIPS), "SHA256SUMS lists exactly the four zip files")
    for name in ZIPS:
        remote = downloaded / name
        checks.check(
            name in sums and remote.is_file() and sums.get(name) == sha256(remote),
            f"{name} matches SHA256SUMS.txt",
        )

    required_members = {
        "CS-80E-linux.zip": ["VST3/CS-80E.vst3/", "Standalone/"],
        "CS-80E-macos.zip": ["VST3/CS-80E.vst3/", "AU/CS-80E.component/", "Standalone/"],
        "CS-80E-windows.zip": ["VST3/CS-80E.vst3/", "Standalone/CS-80E.exe"],
    }
    for name, prefixes in required_members.items():
        archive = downloaded / name
        if not archive.is_file():
            checks.check(False, f"{name} exists")
            continue
        names = zip_names(archive)
        for prefix in prefixes:
            checks.check(any(member.startswith(prefix) for member in names), f"{name} contains {prefix}")

    previews = downloaded / "CS-80E-previews.zip"
    if previews.is_file():
        with zipfile.ZipFile(previews) as archive:
            names = set(archive.namelist())
            for name in SCREENSHOTS:
                member = f"screenshots/{name}"
                checks.check(
                    member in names and hashlib.sha256(archive.read(member)).hexdigest()
                    == sha256(root / "docs" / "screenshots" / name),
                    f"{previews.name} contains current {name}",
                )
            member = "audio/cs80-demo.wav"
            checks.check(
                member in names
                and hashlib.sha256(archive.read(member)).hexdigest() == sha256(local_audio),
                f"{previews.name} contains the current demo WAV",
            )
    else:
        checks.check(False, f"{previews.name} exists")

    manifest_path = downloaded / "latest.json"
    manifest = json.loads(manifest_path.read_text()) if manifest_path.is_file() else {}
    checks.check(manifest.get("version") == version, f"latest.json version is {version}")
    checks.check(manifest.get("commit") == args.commit, f"latest.json commit is {args.commit or '(unset)'}")
    expected_builds = {
        "linux": f"{base}/CS-80E-linux.zip",
        "macos": f"{base}/CS-80E-macos.zip",
        "windows": f"{base}/CS-80E-windows.zip",
        "previews": f"{base}/CS-80E-previews.zip",
        "checksums": f"{base}/SHA256SUMS.txt",
    }
    checks.check(manifest.get("builds") == expected_builds, f"latest.json build URLs point at {args.tag}")
    checks.check(manifest.get("formats") == FORMATS, "latest.json formats are current")
    checks.check(
        manifest.get("screenshots") == [f"{base}/{name}" for name in SCREENSHOTS],
        f"latest.json lists the ten screenshots on {args.tag}",
    )

    try:
        result = subprocess.run(
            ["git", "ls-remote", "--tags", "origin", f"refs/tags/{args.tag}"],
            cwd=root,
            capture_output=True,
            text=True,
            check=True,
        ).stdout
        tag_sha = ""
        for line in result.splitlines():
            parts = line.split()
            if len(parts) < 2:
                continue
            sha, ref = parts[0], parts[1]
            if ref.endswith("^{}"):
                tag_sha = sha
                break
            tag_sha = tag_sha or sha
    except (OSError, subprocess.SubprocessError):
        tag_sha = ""
    checks.check(
        bool(args.commit) and tag_sha == args.commit,
        f"tag {args.tag} points at {args.commit[:7] or '(unset)'} (found {tag_sha[:7] or 'nothing'})",
    )

    if args.tag == "latest":
        readme = (root / "README.md").read_text()
        for name in README_DOWNLOADS:
            url = f"https://github.com/{args.repo}/releases/download/latest/{name}"
            checks.check(url in readme, f"README links {name} on the rolling release")

    checks.finish()


if __name__ == "__main__":
    main()
