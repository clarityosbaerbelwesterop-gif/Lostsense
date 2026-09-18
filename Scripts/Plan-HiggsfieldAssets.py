#!/usr/bin/env python3
"""Emit a reviewed Higgsfield CLI command plan from the Lostsense manifest."""

from __future__ import annotations

import json
import pathlib
import shlex
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "Assets" / "Higgsfield" / "manifest.json"


def quote(value: str) -> str:
    return shlex.quote(value)


def main() -> int:
    data = json.loads(MANIFEST.read_text(encoding="utf-8"))
    print("# Higgsfield CLI production plan")
    print("higgsfield auth status")
    print("higgsfield model list --json")
    for collection in data["collections"]:
        for index, prompt in enumerate(collection["prompts"], start=1):
            output = f'Artifacts/Higgsfield/{collection["id"]}_{index:02d}.json'
            print(
                "higgsfield generate create "
                f'{quote(collection["model"])} '
                f"--prompt {quote(prompt)} "
                f'--aspect_ratio {quote(collection["aspect_ratio"])} '
                f'--resolution {quote(collection["resolution"])} '
                f"--wait --json > {quote(output)}"
            )
    print("higgsfield preset list animation-action --json")
    return 0


if __name__ == "__main__":
    sys.exit(main())
