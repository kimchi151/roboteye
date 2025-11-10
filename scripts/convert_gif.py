#!/usr/bin/env python3
"""Toy GIF conversion script.

The real system would likely optimize or transcode the uploaded GIF. To keep this
repository lightweight we simply copy the input GIF to the output path while
adding a short text note in the logs so operators can verify the invocation.
"""

from __future__ import annotations

import shutil
import sys
from pathlib import Path


def main() -> int:
    if len(sys.argv) < 3:
        print("Usage: convert_gif.py <input> <output>")
        return 1

    source = Path(sys.argv[1])
    destination = Path(sys.argv[2])

    if not source.exists():
        print(f"Input GIF {source} does not exist", file=sys.stderr)
        return 2

    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, destination)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
