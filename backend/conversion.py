from __future__ import annotations

import subprocess
from pathlib import Path
from typing import Sequence


class ConversionError(RuntimeError):
    """Raised when the conversion script fails."""


class GifConverter:
    """Coordinates calling the external GIF conversion script."""

    def __init__(self, script_path: Path) -> None:
        self.script_path = script_path

    def convert(self, source: Path, destination: Path, extra_args: Sequence[str] | None = None) -> None:
        destination.parent.mkdir(parents=True, exist_ok=True)
        args = ["python", str(self.script_path), str(source), str(destination)]
        if extra_args:
            args.extend(extra_args)
        try:
            subprocess.run(args, check=True, capture_output=True)
        except subprocess.CalledProcessError as exc:  # pragma: no cover - defensive
            raise ConversionError(exc.stderr.decode() or "GIF conversion failed") from exc
